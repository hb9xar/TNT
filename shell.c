/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for shell-login (shell.c)
   created: Mark Wahl DL4YBG 94/01/22
   updated: Mark Wahl DL4YBG 97/01/21

   08.01.00 hb9xar	struct fd_set changed to fd_set (is a typedef'd struct)

*/

#include "tnt.h"
#include "shell.h"
#include <pwd.h>
#include <utmp.h>

#ifdef USE_SOCKET
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#endif


extern void cmd_display(int flag,int channel,char *buffer,int cr);
#ifdef USE_IFACE
extern int iface_active(int channel);
#endif
extern void rem_dir(int par1,int par2,int channel,int len,int mode,char *str);
#ifdef USE_SOCKET
extern int write_socket(ACTIVE_SOCKET *active_socket,int len,
                        char *str,int noconv);
#endif
extern void cmd_input(int channel,int mode,char *str,int len,int cscript);
extern void rem_data_display_buf(int channel,char *buffer,int len);
extern void dat_input(int channel,char *str,int len);
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void close_xconnect_2(int channel,int success,int noaction);
extern int find_free_channel();
extern void update_owncall(int channel,char *call);
extern void cmd_xconnect(int par1,int par2,int channel,
                         int len,int mode,char *str);
extern int senddata_allowed(int channel);
extern void strip_call_log(char *call,int channel);


#ifndef UTMP_FILE
#define UTMP_FILE       "/etc/utmp"
#endif

#ifndef WTMP_FILE
#define WTMP_FILE       "/var/adm/wtmp"
#endif

void close_shell();
#ifdef USE_SOCKET
static void close_asocket();
#endif

extern int tnc_channels;
struct shell_stat *sh_stat;
extern struct channel_stat *ch_stat;
extern char remote_user[];
extern char run_dir[];
extern char tnt_dir[];
extern int ptyecho_flag;
int pty_timeout;
extern char rem_tnt_str[];
extern char rem_inv_str[];
extern char rem_newlin_str[];
extern char ok_text[];
extern int is_root;
extern int file_paclen;
extern int use_select;

static char rem_not_conn_txt[] = "Only while connected";
static char only_root_txt[] = "Only available if TNT is started as root";
static char rem_only_root_txt[] = "Not available";

#ifdef USE_SOCKET
static struct sockaddr_in cli_addr;
static int clilen;
struct sock_data *sd;
extern char sock_passfile[];
extern void sigalarm();

static union {
  struct sockaddr sa;
  struct sockaddr_in si;
  struct sockaddr_un su;
} addr;

static LISTEN_SOCKET *listen_socket_root;
static LISTEN_SOCKET *listen_socket_last;

static ACTIVE_SOCKET *active_socket_root;
static ACTIVE_SOCKET *active_socket_last;

#define SOCK_TIMEOUT 120

#endif

#ifdef GEN_NEW_USER
int unix_new_user;
char unix_user_dir[80];
int unix_first_uid;
int unix_user_gid;
#endif

#ifdef USE_SOCKET
char sh_active_txt[] = "Shell, redirection or socket already active";
#else
char sh_active_txt[] = "Shell or redirection already active";
#endif
#ifdef USE_IFACE
extern char iface_active_txt[];
#endif


static long secclock()
{
  struct timeval tv;
  struct timezone tz;
  
  gettimeofday(&tv,&tz);
  return(tv.tv_sec);
}

static int find_pty(numptr, slave)
int *numptr;
char *slave;
{

  char master[80];
  int fd;
  int num;

  for (num = 0; num < NUMPTY; num++) {
    pty_name(master, MASTERPREFIX, num);
    if (use_select)
      fd = open(master, O_RDWR, 0600);
    else
      fd = open(master, O_RDWR | O_NONBLOCK, 0600);
    if (fd >= 0) {
      *numptr = num;
      pty_name(slave, SLAVEPREFIX, num);
      return fd;
    }
  }
  return (-1);
}

static void restore_pty(id)
const char *id;
{
  char filename[80];

  sprintf(filename, "%s%s", MASTERPREFIX, id);
  chown(filename, 0, 0);
  chmod(filename, 0666);
  sprintf(filename, "%s%s", SLAVEPREFIX, id);
  chown(filename, 0, 0);
  chmod(filename, 0666);
}

void fixutmpfile()
{

#ifdef USER_PROCESS

  struct utmp *up;

  while ((up = getutent()))
    if (up->ut_type == USER_PROCESS && kill(up->ut_pid, 0)) {
      restore_pty(up->ut_id);
      up->ut_user[0] = 0;
      up->ut_type = DEAD_PROCESS;
#ifdef HAS_UTEXIT
      up->ut_exit.e_termination = 0;
      up->ut_exit.e_exit = 0;
#endif
      up->ut_time = secclock();
      pututline(up);
    }
  endutent();

#endif
}

static int callvalid(call)
const char *call;
{
  int d, l;

  l = strlen(call);
  if (l < 3 || l > 6) return 0;
  if (isdigit(uchar(call[0])) && isdigit(uchar(call[1]))) return 0;
  if (!(isdigit(uchar(call[1])) || isdigit(uchar(call[2])))) return 0;
  if (!isalpha(uchar(call[l-1]))) return 0;
  d = 0;
  for (; *call; call++) {
    if (!isalnum(uchar(*call))) return 0;
    if (isdigit(uchar(*call))) d++;
  }
  if (d < 1 || d > 2) return 0;
  return 1;
}

static char *find_user_name(name)
const char *name;
{

  char *cp;
  static char username[128];

  for (; ; ) {
    while (*name && !isalnum(uchar(*name))) name++;
    for (cp = username; isalnum(uchar(*name)); *cp++ = tolower(uchar(*name++))) ;
    *cp = 0;
    if (!*username) return remote_user;
    if (callvalid(username)) return username;
  }
}

struct passwd *getpasswdentry(name, create)
const char *name;
int create;
{
#ifdef GEN_NEW_USER
  FILE *fp;
  char bitmap[MAXUID+1];
  char homedir[80];
  char homedirparent[80];
  int fd;
  int uid;
#endif
  struct passwd *pw;

  /* Search existing passwd entry */

  if ((pw = getpwnam(name))) return pw;
#ifdef GEN_NEW_USER
  if (!create) return 0;

  /* Find free user id */

  if ((fd = open(PWLOCKFILE, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0) return 0;
  close(fd);
  memset(bitmap, 0, sizeof(bitmap));
  while ((pw = getpwent())) {
    if (!strcmp(name, pw->pw_name)) break;
    if (pw->pw_uid <= MAXUID) bitmap[pw->pw_uid] = 1;
  }
  endpwent();
  if (pw) {
    unlink(PWLOCKFILE);
    return pw;
  }
  for (uid = unix_first_uid; uid <= MAXUID && bitmap[uid]; uid++) ;
  if (uid > MAXUID) {
    unlink(PWLOCKFILE);
    return 0;
  }

  /* Add user to passwd file */

  sprintf(homedirparent, "%s%.3s...", unix_user_dir, name);
  sprintf(homedir, "%s/%s", homedirparent, name);
  if (!(fp = fopen(PASSWDFILE, "a"))) {
    unlink(PWLOCKFILE);
    return 0;
  }
#ifdef PWD_NOT_EMPTY
  fprintf(fp, "%s:,./:%d:%d::%s:\n", name, uid, unix_user_gid, homedir);
#else
  fprintf(fp, "%s::%d:%d::%s:\n", name, uid, unix_user_gid, homedir);
#endif
  fclose(fp);
  pw = getpwuid(uid);
  unlink(PWLOCKFILE);

  /* Create home directory */

  mkdir(homedirparent, 0755);
  mkdir(homedir, 0755);
  chown(homedir, uid, unix_user_gid);
  return pw;
#else
  else return 0;
#endif
}

/* open a shell on the current channel */
void cmd_shell(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{

  char *env = 0;
  char slave[80];
  int i;
  struct passwd *pw;
  struct termios termios;
  struct utmp utmpbuf;
  char envcall[30];

  if (!is_root) {
    if (mode != M_REMOTE)
      cmd_display(mode,channel,only_root_txt,1);
    else
      cmd_display(mode,channel,rem_only_root_txt,1);
    return;
  }
#ifdef USE_IFACE
  if (iface_active(channel)) {
    cmd_display(mode,channel,iface_active_txt,1);
    return;
  }
#endif
  if (ch_stat[channel].conn_state != CS_CONN) {
    cmd_display(mode,channel,rem_not_conn_txt,1);
    return;
  }
  if (sh_stat[channel].active) {
    cmd_display(mode,channel,sh_active_txt,1);
    return;
  }
  if ((sh_stat[channel].pty = find_pty(&sh_stat[channel].num, slave)) < 0) {
    cmd_display(mode,channel,"Can't open pseudo terminal",1);
    return;
  }
  sh_stat[channel].active = A_SHELL;
  sh_stat[channel].buflen = 0;
  sh_stat[channel].mode = mode;
  sh_stat[channel].direct_started = (par2 == 1);
  for (i=0;i < len;i++) {
    str[i] = tolower(str[i]);
  }
  sh_stat[channel].lfcrconv = ((par1 & 1) == 0);
  strcpy(sh_stat[channel].id, slave + strlen(slave) - 2);
  if (!(sh_stat[channel].pid = fork())) {
    if ((par1 & 2) == 0) {
      pw = getpasswdentry(find_user_name(ch_stat[channel].call),
                          unix_new_user);
      /*if (!pw || pw->pw_passwd[0]) {*/
      if (!pw) {
        pw = getpasswdentry(remote_user, 0);
        /*if (!pw || pw->pw_passwd[0]) exit(1);*/
        if (!pw) exit(1);
      }
    }
    for (i = 0; i < FD_SETSIZE; i++) close(i);
    setsid();
    open(slave, O_RDWR, 0666);
    dup(0);
    dup(0);
    chmod(slave, 0622);
    if (sh_stat[channel].lfcrconv) {
      memset((char *) &termios, 0, sizeof(termios));
      termios.c_iflag = ICRNL | IXOFF;
      termios.c_oflag = OPOST | TAB3 | ONLRET;
      termios.c_oflag &= ~(OCRNL | ONLCR | ONOCR);
      termios.c_cflag = CS8 | CREAD | CLOCAL;
      termios.c_lflag = ISIG | ICANON;
      termios.c_cc[VINTR]  = 127;
      termios.c_cc[VQUIT]  =  28;
      termios.c_cc[VERASE] =   8;
      termios.c_cc[VKILL]  =  24;
      termios.c_cc[VEOF]   =   4;
      cfsetispeed(&termios, B1200);
      cfsetospeed(&termios, B1200);
      /*setenv("TERM","network",1);*/
    }
    else {
      tcgetattr(0,&termios);
    }
    tcsetattr(0, TCSANOW, &termios);
    if (is_root) {
#ifdef LOGIN_PROCESS
      memset(&utmpbuf, 0, sizeof(utmpbuf));
      strcpy(utmpbuf.ut_user, "LOGIN");
      strcpy(utmpbuf.ut_id, sh_stat[channel].id);
      strcpy(utmpbuf.ut_line, slave + 5);
      utmpbuf.ut_pid = getpid();
      utmpbuf.ut_type = LOGIN_PROCESS;
      utmpbuf.ut_time = secclock();
#ifdef HAS_UTHOST 
      strncpy(utmpbuf.ut_host,"AX25", sizeof(utmpbuf.ut_host));
#endif
      pututline(&utmpbuf);
      endutent();
#endif
    }
    if ((par1 & 2) == 0) {
      strcpy(envcall,"CALLSIGN=");
      strcat(envcall,find_user_name(ch_stat[channel].call));
      execle("/bin/login", "login", pw->pw_name, envcall, (char *) 0, &env);
      /*setenv("CALLSIGN",find_user_name(ch_stat[channel].call),1);
      execl("/bin/su","su","-",pw->pw_name,NULL);
      execl("/bin/login", "login", pw->pw_name, (char *) 0);*/
    }
    else {
      execle("/bin/sh","-login",(char *) 0, &env);
      /*execl("/bin/su","su","-",NULL);*/
    }
    exit(1);
  }
  cmd_display(mode,channel,ok_text,1);
  return;
}

/* redirect the current channel */
void cmd_redir(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{

  char ans_str[80];

#ifdef USE_IFACE
  if (iface_active(channel)) {
    cmd_display(mode,channel,iface_active_txt,1);
    return;
  }
#endif
  if (sh_stat[channel].active) {
    cmd_display(mode,channel,sh_active_txt,1);
    return;
  }
  if (use_select)
    sh_stat[channel].pty = open(str,O_RDWR,0600);
  else
    sh_stat[channel].pty = open(str,O_RDWR|O_NONBLOCK,0600);
  if (sh_stat[channel].pty < 0) {
    sprintf(ans_str,"Can't redirect to %s",str);
    cmd_display(mode,channel,ans_str,1);
    return;
  }
  sh_stat[channel].active = A_REDIR;
  sh_stat[channel].pid = 0;
  sh_stat[channel].lfcrconv = 0;
  sh_stat[channel].buflen = 0;
  sh_stat[channel].mode = mode;
  sh_stat[channel].direct_started = 0;
  sprintf(ans_str,"Redirect to %s",str);
  cmd_display(mode,channel,ans_str,1);
}

/* run a program on the current channel */
void cmd_run(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{

  char *envp[4];
  char envcall[30];
  char logcall[30];
  char envcssid[30];
  char tmpcall[10];
  char slave[80];
  int i;
  struct passwd *pw;
  struct termios termios;
  char command[256];
  char *ptr;
  char *ptr2;
  char prgstr[256];
  int fd;

#ifdef USE_IFACE
  if (iface_active(channel)) {
    cmd_display(mode,channel,iface_active_txt,1);
    return;
  }
#endif
  if (ch_stat[channel].conn_state != CS_CONN) {
    cmd_display(mode,channel,rem_not_conn_txt,1);
    return;
  }
  if (sh_stat[channel].active) {
    cmd_display(mode,channel,sh_active_txt,1);
    return;
  }
  if (len == 0) {
    rem_dir(1,1,channel,0,mode,"");
    return;
  }
  
  for (i=0;i < len;i++) {
    str[i] = tolower(str[i]);
  }

  ptr = str;
  while ((*ptr == ' ') && (*ptr != '\0')) ptr++;
  ptr2 = prgstr;
  while ((*ptr != ' ') && (*ptr != '\0')) {
    *ptr2 = *ptr;
    ptr++;
    ptr2++;
  }
  *ptr2 = '\0';
  strcpy(command,run_dir);
  strcat(command,prgstr);
  fd = open(command,O_RDONLY);
  if (fd == -1) {
    if (mode == M_REMOTE)
      cmd_display(mode,channel,rem_inv_str,1);
    else
      cmd_display(mode,channel,"program not found",1);
    return;
  }
  close(fd);
  
  if ((sh_stat[channel].pty = find_pty(&sh_stat[channel].num, slave)) < 0) {
    cmd_display(mode,channel,"Can't open pseudo terminal",1);
    return;
  }
  sh_stat[channel].active = A_RUN;
  sh_stat[channel].mode = mode;
  sh_stat[channel].buflen = 0;
  sh_stat[channel].lfcrconv = (par1 == 0);
  sh_stat[channel].direct_started = (par2 == 1);
  strcpy(sh_stat[channel].id, slave + strlen(slave) - 2);
  if (!(sh_stat[channel].pid = fork())) {
    for (i = 0; i < FD_SETSIZE; i++) close(i);
    setsid();
    if (is_root) {
      pw = getpwnam(remote_user);
      if (pw == NULL) exit(1);
      if (setgid(pw->pw_gid) == -1) exit(1);
      if (setuid(pw->pw_uid) == -1) exit(1);
    }
    open(slave, O_RDWR, 0666);
    dup(0);
    dup(0);
    chmod(slave, 0622);
    if (sh_stat[channel].lfcrconv) {
      memset((char *) &termios, 0, sizeof(termios));
      termios.c_iflag = ICRNL | IXOFF;
      termios.c_oflag = OPOST | TAB3 | ONLRET;
      termios.c_oflag &= ~(OCRNL | ONLCR | ONOCR);
      termios.c_cflag = CS8 | CREAD | CLOCAL;
      termios.c_lflag = ISIG | ICANON;
      termios.c_cc[VINTR]  = 127;
      termios.c_cc[VQUIT]  =  28;
      termios.c_cc[VERASE] =   8;
      termios.c_cc[VKILL]  =  24;
      termios.c_cc[VEOF]   =   4;
      cfsetispeed(&termios, B1200);
      cfsetospeed(&termios, B1200);
    }
    else {
      tcgetattr(0,&termios);
    }
    tcsetattr(0, TCSANOW, &termios);
    /* execute command using shell */
    if (is_root)
      chdir(pw->pw_dir);
    strcpy(command,run_dir);
    strcat(command,str);
    strcpy(envcall,"CALLSIGN=");
    strcat(envcall,find_user_name(ch_stat[channel].call));
    strcpy(logcall,"LOGNAME=");
    strcat(logcall,find_user_name(ch_stat[channel].call));
    strcpy(envcssid,"CALLSSID=");
    strip_call_log(tmpcall,channel);
    strcat(envcssid,tmpcall);
    envp[0] = envcall;
    envp[1] = logcall;
    envp[2] = envcssid;
    envp[3] = NULL;
    execle("/bin/sh","sh","-c",command, (char *) 0, envp);
    exit(1);
  }
  cmd_display(mode,channel,ok_text,1);
  return;
}

#ifdef USE_SOCKET

/* from buildsaddr.c */
struct sockaddr *build_sockaddr(const char *name, int *addrlen)
{

  char *host_name;
  char *serv_name;
  char buf[1024];

  memset((char *) &addr, 0, sizeof(addr));
  *addrlen = 0;

  host_name = strcpy(buf, name);
  serv_name = strchr(buf, ':');
  if (!serv_name) return 0;
  *serv_name++ = 0;
  if (!*host_name || !*serv_name) return 0;

  if (!strcmp(host_name, "local") || !strcmp(host_name, "unix")) {
    addr.su.sun_family = AF_UNIX;
    *addr.su.sun_path = 0;
    if (*serv_name != '/') strcpy(addr.su.sun_path,tnt_dir);
    strcat(addr.su.sun_path, serv_name);
    *addrlen = sizeof(struct sockaddr_un);
    return &addr.sa;
  }

  addr.si.sin_family = AF_INET;

  if (!strcmp(host_name, "*")) {
    addr.si.sin_addr.s_addr = INADDR_ANY;
  } else if (!strcmp(host_name, "loopback")) {
    addr.si.sin_addr.s_addr = inet_addr("127.0.0.1");
  } else if ((addr.si.sin_addr.s_addr = inet_addr(host_name)) == -1L) {
    struct hostent *hp = gethostbyname(host_name);
    endhostent();
    if (!hp) return 0;
    addr.si.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;
  }

  if (isdigit(*serv_name & 0xff)) {
    addr.si.sin_port = htons(atoi(serv_name));
  } else {
    struct servent *sp = getservbyname(serv_name, (char *) 0);
    endservent();
    if (!sp) return 0;
    addr.si.sin_port = sp->s_port;
  }

  *addrlen = sizeof(struct sockaddr_in);
  return &addr.sa;
}

static void strtolower(char *str)
{
  int i;
  
  for (i=0;i<strlen(str);i++) str[i] = tolower(str[i]);
}

static void strtoupper(char *str)
{
  int i;
  
  for (i=0;i<strlen(str);i++) str[i] = toupper(str[i]);
}

/* this procedure converts all EOL-sequences CR,CR/LF,LF to a single CR */ 
static int conv_tocr(str,len)
char *str;
int len;
{
  int i,j;
  int lastcr;
  char ch;
  
  j = 0;
  lastcr = 0;
  for (i = 0;i < len;i++) {
    ch = str[i];
    if (ch == '\n') {
      if (!lastcr) {
        str[j] = '\r';
        j++;
      }
      lastcr = 0;
    }
    else if (ch == '\r') {
      lastcr = 1;
      str[j] = ch;
      j++;
    }
    else {
      lastcr = 0;
      str[j] = ch;
      j++;
    }
  }
  str[j] = '\0';
  return(j);
}

/* this procedure converts all EOL-sequences CR,CR/LF,LF to a single LF */ 
static int conv_tolf(str,len)
char *str;
int len;
{
  int i,j;
  int lastcr;
  char ch;
  
  j = 0;
  lastcr = 0;
  for (i = 0;i < len;i++) {
    ch = str[i];
    if (ch == '\n') {
      if (!lastcr) {
        str[j] = '\n';
        j++;
      }
      lastcr = 0;
    }
    else if (ch == '\r') {
      lastcr = 1;
      str[j] = '\n';
      j++;
    }
    else {
      lastcr = 0;
      str[j] = ch;
      j++;
    }
  }
  str[j] = '\0';
  return(j);
}

void cmd_sockconn(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct sockaddr *saddr;
  int saddrlen;
  int sockfd;

  saddr = build_sockaddr(str,&saddrlen);
  if (!saddr) {
    cmd_display(mode,channel,"Invalid parameters",1);
    return;
  }
  if ((sockfd = socket(saddr->sa_family, SOCK_STREAM, 0)) < 0) {
    cmd_display(mode,channel,"Can't open stream socket",1);
    return;
  }
  signal(SIGALRM,sigalarm);
  alarm(2);
  if (connect(sockfd, saddr, saddrlen) < 0) {
    close(sockfd);
    signal(SIGALRM,SIG_IGN);
    cmd_display(mode,channel,"Can't connect to socket",1);
    return;
  }
  signal(SIGALRM,SIG_IGN);
  if (!use_select) {
    fcntl(sockfd,F_SETFL,O_NONBLOCK);
  }
  sh_stat[channel].active = A_SOCKCONN;
  sh_stat[channel].pty = sockfd;
  sh_stat[channel].pid = 0;
  sh_stat[channel].mode = mode;
  sh_stat[channel].lfcrconv = (par1 == 0);
  sh_stat[channel].buflen = 0;
  sh_stat[channel].direct_started = (par2 == 1);
  cmd_display(mode,channel,ok_text,1);
}

void cmd_socket(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct sockaddr *saddr;
  int saddrlen;
  int sockfd;
  int arg;
  LISTEN_SOCKET *listen_socket;
  int found;
  char hs[300];
  int res;
  int error;
  char sockmode[160];
  char hostname[160];
  char owncall[160];
  int type;
  int ax25special;
  
  if (!len) {
    found = 0;
    listen_socket = listen_socket_root;
    while (listen_socket != NULL) {
      if (!found) {
        cmd_display(mode,channel,"Sockettype   Mode/Mycall Socketaddress",1);
        cmd_display(mode,channel,"----------------------------------------"
        "---------------------------------------",1);
      }
      found = 1;
      switch (listen_socket->type) {
      case STYP_AXSERV:
        strcpy(hs,"AX25-Server  ");
        if (listen_socket->ax25special) {
          strcat(hs,"Special     ");
        }
        else {
          strcat(hs,"Normal      ");
        }
        break;
      case STYP_NETCMD:
        strcpy(hs,"Net-Command  ");
        sprintf(owncall,"%-9.9s   ",listen_socket->mycall);
        strcat(hs,owncall);
        break;
      }
      strcat(hs,listen_socket->socketname);
      cmd_display(mode,channel,hs,1);
      listen_socket = listen_socket->next;
    }
    if (!found) {
      cmd_display(mode,channel,"No sockets listening",1);
    }
    return;
  }
  error = 0;
  res = sscanf(str,"%s %s %s",sockmode,hostname,owncall);
  switch (res) {
  case 2:
    strtolower(sockmode);
    if (strncmp(sockmode,"axserv",strlen(sockmode)) == 0) {
      type = STYP_AXSERV;
      ax25special = 0;
    }
    else if (strncmp(sockmode,"axspec",strlen(sockmode)) == 0) {
      type = STYP_AXSERV;
      ax25special = 1;
    }
    else error = 1;
    break;
  case 3:
    strtolower(sockmode);
    if (strncmp(sockmode,"netcmd",strlen(sockmode)) == 0) {
      if (strlen(owncall) > 9) {
        error = 1;
      }
      else {
        strtoupper(owncall);
        type = STYP_NETCMD;
        ax25special = 0;
      }
    }
    else error = 1;
    break;
  default:
    error = 1;
    break;
  }

  if (!error) {  
    saddr = build_sockaddr(hostname,&saddrlen);
    if (!saddr) error = 1;
  }
  if (error) {
    cmd_display(mode,channel,"Invalid parameters",1);
    return;
  }
  listen_socket = (LISTEN_SOCKET *)malloc(sizeof(LISTEN_SOCKET));
  if (listen_socket == NULL) {
    cmd_display(mode,channel,"Cannot alloc memory",1);
    return;
  }
  sockfd = socket(saddr->sa_family,SOCK_STREAM,0);
  if (sockfd < 0) {
    cmd_display(mode,channel,"Can't open stream socket",1);
    free(listen_socket);
    return;
  }
  switch (saddr->sa_family) {
    case AF_UNIX:
      unlink(saddr->sa_data);
      break;
    case AF_INET:
      arg = 1;
      setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *) &arg,sizeof(arg));
      break;
  }
  if (!use_select) {
    fcntl(sockfd,F_SETFL,O_NONBLOCK);
  }
  if (bind(sockfd,saddr,saddrlen) < 0) {
    cmd_display(mode,channel,"Can't bind socket",1);
    close(sockfd);
    free(listen_socket);
    return;
  }
  if (listen(sockfd,5) < 0) {
    cmd_display(mode,channel,"Can't listen to socket",1);
    close(sockfd);
    free(listen_socket);
    return;
  }
  listen_socket->sockfd = sockfd;
  listen_socket->type = type;
  listen_socket->ax25special = ax25special;
  if (type == STYP_NETCMD) {
    strcpy(listen_socket->mycall,owncall);
  }
  listen_socket->next = NULL;
  strcpy(listen_socket->socketname,hostname);
  if (listen_socket_root == NULL) {
    listen_socket->prev = NULL;
    listen_socket_root = listen_socket;
  }
  else {
    listen_socket->prev = listen_socket_last;
    listen_socket_last->next = listen_socket;
  }
  listen_socket_last = listen_socket;
  cmd_display(mode,channel,ok_text,1);
}
#endif

static int check_buffer_cmd(channel,buffer,len)
int channel;
char *buffer;
int len;
{
#ifdef USE_SOCKET
  ACTIVE_SOCKET *active_socket;
  char tmpstr[300];
  int i;
  
  if (sh_stat[channel].active == A_SOCKET) {
    if (sh_stat[channel].active_socket->type == STYP_AXSERV) {
      active_socket = sh_stat[channel].active_socket;
      if (len > 1) {
        if (buffer[0] == ':') {
          if (len >= 4) {
            /* check for escape-string from converse mode to command mode */
            if (!strncmp(buffer,":cmd",4)) {
              active_socket->login_stat = LS_ACTCMD;
              sprintf(tmpstr,"*Ok*\n\nCmd> ");
              write_socket(active_socket,strlen(tmpstr),tmpstr,0);
              return(1);
            }
          }
          if ((len > 2) && (active_socket->level == 99)) {
            /* here special level for mail delivery, level=99 allows any ext.
               command via '::command' (to open logfiles or uploads) */
            if (buffer[1] == ':') {
              strncpy(tmpstr,&buffer[2],sizeof(tmpstr));
              i = strlen(tmpstr) - 1;
              /* strip off LF or CR at end of input */
              while (tmpstr[i] == '\n' || tmpstr[i] == '\r') {
                tmpstr[i] = 0;
                i--;
              }
              cmd_input(channel,M_CMDSCRIPT,tmpstr,strlen(tmpstr),0);
              sprintf(tmpstr,"\n*Ok*> ");
              write_socket(active_socket,strlen(tmpstr),tmpstr,0);
              return(1);
            }
          }
        }
      }
    }
  }
#endif
  return(0);
}

void dat_input_pty(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  if (check_buffer_cmd(channel,buffer,len)) return;
  if (ptyecho_flag) rem_data_display_buf(channel,buffer,len);
  dat_input(channel,buffer,len);
}

static int send_immediate(channel)
int channel;
{
  char ch;
#ifdef USE_SOCKET
  if (sh_stat[channel].active == A_SOCKET) {
    if (sh_stat[channel].active_socket->type == STYP_NETCMD) return(1);
    if ((sh_stat[channel].active_socket->type == STYP_AXSERV) &&
        (sh_stat[channel].active_socket->ax25special)) {
      ch = sh_stat[channel].buffer[sh_stat[channel].buflen-1];
      if ((ch == '\r') || (ch == '\n')) return(1);
    }
  }
#endif
  return(0);
}

void pty_send_data(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  int tmp_len;
  
  if (len > file_paclen) return; /* sanity check */

  if ((sh_stat[channel].buflen + len) <= file_paclen) {
    memcpy((sh_stat[channel].buffer + sh_stat[channel].buflen),
           buffer,len);
    sh_stat[channel].buflen += len;       
    sh_stat[channel].bufupdate = time(NULL);
  }
  else {
    tmp_len = file_paclen - sh_stat[channel].buflen;
    memcpy((sh_stat[channel].buffer + sh_stat[channel].buflen),
           buffer,tmp_len);
    sh_stat[channel].buflen += tmp_len;       
    dat_input_pty(channel,sh_stat[channel].buffer,
                          sh_stat[channel].buflen);
    sh_stat[channel].buflen = 0;
    memcpy(sh_stat[channel].buffer,buffer+tmp_len,len-tmp_len);
    sh_stat[channel].buflen = len-tmp_len;
    sh_stat[channel].bufupdate = time(NULL);
  }

  if (sh_stat[channel].buflen > 0) {
    if ((sh_stat[channel].buflen == file_paclen) || 
        send_immediate(channel)) {
      dat_input_pty(channel,sh_stat[channel].buffer,
                            sh_stat[channel].buflen);
      sh_stat[channel].buflen = 0;
    }
  }
}

void pty_check_timeout(channel)
int channel;
{
  if (!sh_stat[channel].buflen) return;
  if ((time(NULL) - sh_stat[channel].bufupdate) >= pty_timeout) {
    dat_input_pty(channel,sh_stat[channel].buffer,sh_stat[channel].buflen);
    sh_stat[channel].buflen = 0;
  }
}

static int read_data_pty(channel)
int channel;
{
  char buffer[PACKETSIZE];
  int len;
  int fc;

  if (sh_stat[channel].active == A_SOCKET) return(0);
  if (use_select)
    fcntl(sh_stat[channel].pty,F_SETFL,O_NONBLOCK);
  len = read(sh_stat[channel].pty,buffer,file_paclen);
  if (use_select) {
    fc = fcntl(sh_stat[channel].pty,F_GETFL,NULL);
    fcntl(sh_stat[channel].pty,F_SETFL,fc & ~O_NONBLOCK);
  }
  if (len > 0) {
    if (sh_stat[channel].lfcrconv) {
      len = conv_tocr(buffer,len);
    }
    pty_send_data(channel,buffer,len);
    return(1);
  }
  else {
    if (len == 0) {
      if (sh_stat[channel].active == A_SOCKCONN) {
        close_shell(channel,1,0);
      }
    }
    return(0);
  }
}

#ifdef USE_SOCKET
static int add_new_char(active_socket,bufpos,actlen,error)
ACTIVE_SOCKET *active_socket;
char **bufpos;
int *actlen;
int *error;
{
  char newch;

  newch = **bufpos;
  (*bufpos)++;
  (*actlen)--;  
  if ((newch == '\r') || (newch == '\n')) {
    if (*actlen) {
      if ((newch == '\r') && ((**bufpos) == '\n')) {
        (*bufpos)++;
        (*actlen)--;
      }
    }
    active_socket->input_string[active_socket->len_input_string] = '\0';
    return(1);
  }
  else {
    active_socket->input_string[active_socket->len_input_string] = newch;
    active_socket->len_input_string++;
    if (active_socket->len_input_string > 255) {
      *actlen = 0;
      *error = 1;
    }
  }
  return(0);
}

void socksetup_to_conn(channel)
int channel;
{
  ACTIVE_SOCKET *active_socket;
  char tmp[100];
  int newlen;
  
  active_socket = active_socket_root;
  while (active_socket != NULL) {
    if (active_socket->channel == channel) {
      if (active_socket->type == STYP_NETCMD) {
        active_socket->login_stat = NC_CONNECT;
        if (active_socket->len_next_input) {
          if (active_socket->lfcrconv) {
            newlen = conv_tocr(active_socket->next_input,
                                  active_socket->len_next_input);
            pty_send_data(active_socket->channel,
                          active_socket->next_input,
                          newlen);
          }
          else {
            pty_send_data(active_socket->channel,
                          active_socket->next_input,
                          active_socket->len_next_input);
          }
        }
      }
      else {
        active_socket->login_stat = LS_ACTDAT;
        sprintf(tmp,"* Ok, back to cmd-mode with ':cmd' *\n");
        write_socket(active_socket,strlen(tmp),tmp,0);
      }
      return;
    }
    active_socket = active_socket->next;
  }
}

void socksetup_timeout(channel)
int channel;
{
  ACTIVE_SOCKET *active_socket;
  char tmp[100];
  
  active_socket = active_socket_root;
  while (active_socket != NULL) {
    if (active_socket->channel == channel) {
      if (active_socket->type == STYP_NETCMD) {
        active_socket->login_stat = NC_COMMAND;
        if (active_socket->channel != -1) {
          sh_stat[active_socket->channel].active_socket = NULL;
          sh_stat[active_socket->channel].active = 0;
          active_socket->channel = -1;
        }
        close_asocket(active_socket);
      }
      else {
        active_socket->login_stat = LS_NOTACT;
        if (active_socket->channel != -1) {
          sh_stat[active_socket->channel].active_socket = NULL;
          sh_stat[active_socket->channel].active = 0;
          active_socket->channel = -1;
        }
        sprintf(tmp,"* connect not successful *\n\nCmd> ");
        write_socket(active_socket,strlen(tmp),tmp,0);
      }
      return;
    }
    active_socket = active_socket->next;
  }
}

static void abort_socksetup(active_socket,mess)
ACTIVE_SOCKET *active_socket;
int mess;
{
  char tmp[100];

  queue_cmd_data(active_socket->channel,X_COMM,1,M_CMDSCRIPT,"D");
  close_xconnect_2(active_socket->channel,0,1);  
  if (active_socket->type == STYP_NETCMD) {
    active_socket->login_stat = NC_COMMAND;
    if (active_socket->channel != -1) {
      sh_stat[active_socket->channel].active_socket = NULL;
      sh_stat[active_socket->channel].active = 0;
      active_socket->channel = -1;
    }
  }
  else {
    active_socket->login_stat = LS_NOTACT;
    if (active_socket->channel != -1) {
      sh_stat[active_socket->channel].active_socket = NULL;
      sh_stat[active_socket->channel].active = 0;
      active_socket->channel = -1;
    }
    if (mess) {
      sprintf(tmp,"* connection aborted *\n\nCmd> ");
      write_socket(active_socket,strlen(tmp),tmp,0);
    }
  }
}

static void ana_server_command(active_socket,bufpos,len)
ACTIVE_SOCKET *active_socket;
char *bufpos;
int len;
{
  char buffer[257];
  char tmp[300];
  int i;
  int con_channel;
  int error;
  char callsign[10];
  char *ptr;
  
  if ((len == 0) || (len > 256)) return;
  error = 0;
  memcpy(buffer,bufpos,len);
  buffer[len] = '\0';
  if ((ptr = strchr(buffer,'\r')) != NULL) *ptr = '\0';
  if ((ptr = strchr(buffer,'\n')) != NULL) *ptr = '\0';

  switch (toupper(buffer[0])) {
  case 'C':
    /* connect command */
    if ((active_socket->login_stat == LS_SETUP) ||
        (active_socket->login_stat == LS_ACTCMD)) {
      sprintf(tmp,"* connection already active *\n\nCmd> ");
      write_socket(active_socket,strlen(tmp),tmp,0);
    }
    else {
      for (i=0;i<strlen(buffer);i++)
        if (buffer[i]==' ') break;
      if (i<strlen(buffer) || (strcmp(active_socket->connect,"none"))) {
        con_channel = find_free_channel();
        if (con_channel == -1) {
          sprintf(tmp,"* no free channel left *\n\nCmd> ");
          write_socket(active_socket,strlen(tmp),tmp,0);
        }
        else {
          strcpy(callsign,active_socket->user_id);
          strtoupper(callsign);
          sprintf(tmp,"* Ok, link setup *\n");
          write_socket(active_socket,strlen(tmp),tmp,0);
          strcpy(tmp,"I");
          strcat(tmp,callsign);
          queue_cmd_data(con_channel,X_COMM,strlen(tmp),M_CMDSCRIPT,tmp);
          update_owncall(con_channel,callsign);
          if (strcmp(active_socket->connect,"none")) /* autoconnect */
            sprintf(tmp,"%s %d",active_socket->connect,SOCK_TIMEOUT);
          else
            sprintf(tmp,"%s %d",&buffer[i+1],SOCK_TIMEOUT);
          cmd_xconnect(3,0,con_channel,strlen(tmp),M_CMDSCRIPT,tmp);
          active_socket->login_stat = LS_SETUP;
          sh_stat[con_channel].active = A_SOCKET;
          sh_stat[con_channel].active_socket = active_socket;
          sh_stat[con_channel].pid = 0;
          active_socket->channel = con_channel;
        }
      }
      else {
        sprintf(tmp,"* no callsign ? *\n\nCmd> ");
        write_socket(active_socket,strlen(tmp),tmp,0);
      }
    }
    break;
  case 'D':
    /* disconnect command */
    switch (active_socket->login_stat) {
    case LS_SETUP:
      abort_socksetup(active_socket,1);
      break;
    case LS_ACTCMD:
      queue_cmd_data(active_socket->channel,X_COMM,1,M_CMDSCRIPT,"D");
      break;
    default:
      sprintf(tmp,"* no connection active *\n\nCmd> ");
      write_socket(active_socket,strlen(tmp),tmp,0);
      break;
    }
    break;
  case 'E':
  case 'Q':
    /* exit command (quit) */
    switch (active_socket->login_stat) {
    case LS_SETUP:
      abort_socksetup(active_socket,0);
      active_socket->login_stat = LS_NOTACT;
      if (active_socket->channel != -1) {
        sh_stat[active_socket->channel].active_socket = NULL;
        sh_stat[active_socket->channel].active = 0;
        active_socket->channel = -1;
      }
      break;
    case LS_ACTCMD:
      queue_cmd_data(active_socket->channel,X_COMM,1,M_CMDSCRIPT,"D");
      active_socket->login_stat = LS_NOTACT;
      if (active_socket->channel != -1) {
        sh_stat[active_socket->channel].active_socket = NULL;
        sh_stat[active_socket->channel].active = 0;
        active_socket->channel = -1;
      }
      break;
    }
    sprintf(tmp,"*Ok*\n\n");
    write_socket(active_socket,strlen(tmp),tmp,0);
    close_asocket(active_socket);
    break;
  case ':':
    /* extended commands, just for users level=9 ! */
    if (active_socket->level >= 9) {
      strncpy(tmp,&buffer[1],sizeof(tmp));
      i=strlen(tmp)-1;
      while(tmp[i]=='\n' || tmp[i]=='\r') {
        tmp[i]=0; i--;
      }
      if ((active_socket->login_stat == LS_NOTACT) ||
          (active_socket->login_stat == LS_SETUP)) {
        sprintf(tmp,"\n* only while connected *\n\nCmd> ");
        write_socket(active_socket,strlen(tmp),tmp,0);
      }
      else {
        cmd_input(active_socket->channel,M_SOCKET,tmp,strlen(tmp),0);
      }
    }
    else {
      sprintf(tmp,"\n* permission denied *\n\nCmd> ");
      write_socket(active_socket,strlen(tmp),tmp,0);
    }
    break;
  case '\r':
    switch (active_socket->login_stat) {
    case LS_SETUP:
      abort_socksetup(active_socket);
      break;
    case LS_ACTCMD:
      sprintf(tmp,"* Back to converse mode *\n");
      write_socket(active_socket,strlen(tmp),tmp,0);
      active_socket->login_stat = LS_ACTDAT;
      break;
    default:
      error = 1;
      break;
    }
    break;
  default:
    if (active_socket->login_stat == LS_SETUP) {
      abort_socksetup(active_socket);
    }
    else {
      error = 1;
    }
    break;
  }
  if (error) {
    /* do a little missionary work, help the user, hi */
    sprintf(tmp,"\nInvalid command.\n\n");
    strcat(tmp,"- (c)onnect <callsign>\n");
    strcat(tmp,"- (d)isconnect\n");
    strcat(tmp,"- (e)xit\n");
    strcat(tmp,"\n  To change from converse-mode to command-mode\n");
    strcat(tmp,"  enter ':cmd' while connection established.\n\nCmd> ");
    write_socket(active_socket,strlen(tmp),tmp,0);
  }
}

static void handle_socket_rxdata(active_socket,buffer,len)
ACTIVE_SOCKET *active_socket;
char *buffer;
int len;
{
  int actlen;
  char *bufpos;
  int newlen;
  int error;
  int res;
  char tmpstr[300];
  FILE *fp;
  char *pos;
  char command[256];
  char protocol[256];
  char address[256];
  char owncall[256];
  int con_channel;

  error = 0;
  actlen = len;
  bufpos = buffer;
  while ((actlen) && (!error)) {
    switch (active_socket->type) {
    case STYP_AXSERV:
      switch (active_socket->login_stat) {
      case LS_LOGIN:
        if (add_new_char(active_socket,&bufpos,&actlen,&error)) {
          strcpy(tmpstr,sock_passfile);
          fp = fopen(tmpstr,"r");
          if (fp == NULL) {
            sprintf(tmpstr,"\n\n* Can't open password file *\n\n");
            write_socket(active_socket,strlen(tmpstr),tmpstr,0);
            error = 1;
          }
          else {
            error = 1;
            do {
              fgets(tmpstr,99,fp);
              tmpstr[strlen(tmpstr)-1] = 0;
              if (tmpstr[0] == '#' || tmpstr[0] == ' ' ||
                  tmpstr[0] == 9 || tmpstr[0] == LF) continue;
              res = sscanf(tmpstr,"%s%s%s%d",active_socket->user_id,
                                             active_socket->user_pw,
	                                     active_socket->connect,
	                                     &(active_socket->level));
	      if (res != 4) continue;
              if (!strncmp(active_socket->user_id,active_socket->input_string,
                           active_socket->len_input_string)) {
                if (strcmp(active_socket->user_pw,"")) {
                  error = 0;
	          break;
	        }
	      }
            } while(!feof(fp));
            fclose(fp);

            if (!error) {
              pos = strchr(active_socket->user_id,'-');
              if (pos != NULL) *pos = '\0';
              sprintf(tmpstr,"password: ");
              write_socket(active_socket,strlen(tmpstr),tmpstr,0);
              active_socket->login_stat = LS_PASSWD;
            }
          }
          active_socket->len_input_string = 0;
        }
        break;
      case LS_PASSWD:
        if (add_new_char(active_socket,&bufpos,&actlen,&error)) {
          if (!strcmp(active_socket->user_pw,active_socket->input_string) &&
               active_socket->level != 0) {
            sprintf(tmpstr,"\nlogin acknowledged.\n\nCmd> ");
            write_socket(active_socket,strlen(tmpstr),tmpstr,0);
            active_socket->login_stat = LS_NOTACT;
          }
          else {
            /* an intruder, terminating telnet */
            sprintf(tmpstr,"\nlogin failed, permission denied.\n\n");
            write_socket(active_socket,strlen(tmpstr),tmpstr,0);
            error = 1;
          }
          active_socket->len_input_string = 0;
        }
        break;
      case LS_NOTACT:
      case LS_ACTCMD:
        ana_server_command(active_socket,bufpos,actlen);
        actlen = 0;
        break;
      case LS_SETUP:
        ana_server_command(active_socket,bufpos,actlen);
        actlen = 0;
        break;
      case LS_ACTDAT:
        newlen = conv_tocr(bufpos,actlen);
        actlen = 0;
        pty_send_data(active_socket->channel,bufpos,newlen);
        break;
      }
      break;
    case STYP_NETCMD:
      switch (active_socket->login_stat) {
      case NC_COMMAND:
        if (add_new_char(active_socket,&bufpos,&actlen,&error)) {
          active_socket->len_input_string = 0;
          res = sscanf(active_socket->input_string,"%s %s %s %s",
                       command,protocol,address,owncall);
          if (res == 1) {
            strtolower(command);
            if (strncmp(command,"ascii",strlen(command)) == 0) {
              active_socket->lfcrconv = 1;
            }
            else if (strncmp(command,"binary",strlen(command)) == 0) {
              active_socket->lfcrconv = 0;
            }
            else error = 1;
          }
          else if ((res >= 3) && (res <= 4)) {
            strtolower(command);
            if (strncmp(command,"connect",strlen(command)) != 0) error = 1;
            if (!error) {
              strtolower(protocol);
              if (strcmp(protocol,"ax25") != 0) error = 1;
            }
            if (!error) {
              strtoupper(address);
              if (strlen(address) > 9) error = 1;
            }
            if (!error && (res == 4)) {
              strtoupper(owncall);
              if (strlen(owncall) > 9) error = 1;
              else strcpy(active_socket->mycall,owncall);
            }
            if (!error) {
              con_channel = find_free_channel();
              if (con_channel == -1) {
                error = 1;
              }
              else {
                strcpy(tmpstr,"I");
                strcat(tmpstr,active_socket->mycall);
                queue_cmd_data(con_channel,X_COMM,strlen(tmpstr),
                               M_CMDSCRIPT,tmpstr);
                update_owncall(con_channel,active_socket->mycall);
                sprintf(tmpstr,"%s %d",address,SOCK_TIMEOUT);
                cmd_xconnect(3,0,con_channel,strlen(tmpstr),M_CMDSCRIPT,tmpstr);
                active_socket->login_stat = NC_SETUP;
                sh_stat[con_channel].active = A_SOCKET;
                sh_stat[con_channel].active_socket = active_socket;
                sh_stat[con_channel].pid = 0;
                active_socket->channel = con_channel;
                if (actlen > 0) {
                  strncpy(active_socket->next_input,bufpos,actlen);
                  active_socket->next_input[actlen] = '\0';
                }
                active_socket->len_next_input = actlen;
                actlen = 0;
              }
            }
          }
        }
        break;
      case NC_SETUP:
        /* not possible (normally...) */
        actlen = 0;
        break;
      case NC_CONNECT:
        if (active_socket->lfcrconv) {
          newlen = conv_tocr(bufpos,actlen);
          actlen = 0;
          pty_send_data(active_socket->channel,bufpos,newlen);
        }
        else {
          pty_send_data(active_socket->channel,bufpos,actlen);
          actlen = 0;
        }
        break;
      }
      break;
    }
  }
  if (error) {
    close_asocket(active_socket);
  }
}

static int read_data_socket(active_socket)
ACTIVE_SOCKET *active_socket;
{
  char buffer[PACKETSIZE];
  int len;
  int fc;

  if (active_socket == NULL) return(0);
  if (use_select)
    fcntl(active_socket->sockfd,F_SETFL,O_NONBLOCK);
  len = read(active_socket->sockfd,buffer,file_paclen);
  if (use_select) {
    fc = fcntl(active_socket->sockfd,F_GETFL,NULL);
    fcntl(active_socket->sockfd,F_SETFL,fc & ~O_NONBLOCK);
  }
  if (len > 0) {
    handle_socket_rxdata(active_socket,buffer,len);
    return(1);
  }
  else {
    if (len == 0) {
      close_asocket(active_socket);
    }
    return(0);
  }
}
#endif

void close_shell2(channel,report,disc,errmode)
int channel;
int report;
int disc;
int errmode;
{
  char slave[80];
  int fdut = -1;
  struct utmp utmpbuf;
  char ans_str[80];
  int no_report;
  int active;
  int mode;
#ifdef USE_SOCKET
  ACTIVE_SOCKET *active_socket;
#endif

  if (!sh_stat[channel].active) {
    if (report) {
      strcpy(ans_str,"Nothing to close");
      cmd_display(errmode,channel,ans_str,1);
    }
    return;
  }
  
  mode = sh_stat[channel].mode;
  active = sh_stat[channel].active;

  if (sh_stat[channel].active == A_SOCKET) {
#ifdef USE_SOCKET
    active_socket = sh_stat[channel].active_socket;
    switch (active_socket->type) {
    case STYP_AXSERV:
      sh_stat[channel].active_socket = NULL;
      active_socket->channel = -1;
      if (active_socket->login_stat == LS_ACTDAT) {
        strcpy(ans_str,"\nCmd> ");
        write_socket(active_socket,strlen(ans_str),ans_str,0);
      }
      active_socket->login_stat = LS_NOTACT;
      break;
    case STYP_NETCMD:
      sh_stat[channel].active_socket = NULL;
      close_asocket(active_socket);
      break;
    }
#endif
  }
  else {
    if (sh_stat[channel].active != A_SOCKCONN) {
      /* read all remaining data from pty */
      if (use_select)
        fcntl(sh_stat[channel].pty,F_SETFL,O_NONBLOCK);
      while (read_data_pty(channel) == 1);
    }
    if (sh_stat[channel].buflen) {
      if (ptyecho_flag)
        rem_data_display_buf(channel,sh_stat[channel].buffer,
                                     sh_stat[channel].buflen);
      dat_input(channel,sh_stat[channel].buffer,sh_stat[channel].buflen);
    }
    if (sh_stat[channel].pty > 0) {
      if ((sh_stat[channel].active == A_SHELL) ||
          (sh_stat[channel].active == A_RUN) ||
          (sh_stat[channel].active == A_REDIR))
        ioctl(sh_stat[channel].pty, TCFLSH, 2);
      close(sh_stat[channel].pty);
      if ((sh_stat[channel].active == A_SHELL) ||
          (sh_stat[channel].active == A_RUN))
        restore_pty(sh_stat[channel].id);
    }
    if (((sh_stat[channel].active == A_SHELL) ||
         (sh_stat[channel].active == A_RUN)) && 
        (sh_stat[channel].pid > 0)) {
      kill(-sh_stat[channel].pid, SIGHUP);
      pty_name(slave, SLAVEPREFIX, sh_stat[channel].num);
      if ((fdut = open(UTMP_FILE, O_RDWR, 0644)) >= 0) {
        while (read(fdut, &utmpbuf, sizeof(utmpbuf)) == sizeof(utmpbuf)) {
	  if (!strncmp(utmpbuf.ut_line, slave + 5, sizeof(utmpbuf.ut_line))) {
	    utmpbuf.ut_name[0] = 0;
#ifdef HAS_UTHOST	  
	    utmpbuf.ut_host[0] = 0;
#endif
	    utmpbuf.ut_time = secclock();
#ifdef DEAD_PROCESS
	    utmpbuf.ut_type = DEAD_PROCESS;
#ifdef HAS_UTEXIT
	    utmpbuf.ut_exit.e_termination = 0;
	    utmpbuf.ut_exit.e_exit = 0;
#endif
#endif
	    lseek(fdut, -sizeof(utmpbuf), SEEK_CUR);
	    write(fdut, &utmpbuf, sizeof(utmpbuf));
	    close(fdut);
	    if ((fdut = open(WTMP_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644)) >= 0)
	      write(fdut, &utmpbuf, sizeof(utmpbuf));
	    break;
	  }
        }	
      }
      if (fdut >= 0) close(fdut);
    }
  }
  if (report) {
    no_report = 0;
    switch (active) {
    case A_SHELL:
      if (sh_stat[channel].direct_started) {
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
        no_report = 1;
      }
      else
        strcpy(ans_str,"Shell closed");
      break;
    case A_RUN:
      if (sh_stat[channel].direct_started) {
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
        no_report = 1;
      }
      else
        strcpy(ans_str,"Program finished");
      break;
    case A_REDIR:
      strcpy(ans_str,"Redirection closed");
      break;
    case A_SOCKET:
      strcpy(ans_str,"Socket connection to server closed");
      break;
    case A_SOCKCONN:
      if (sh_stat[channel].direct_started) {
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
        no_report = 1;
      }
      else
        strcpy(ans_str,"Socket connection closed");
      break;
    }
    if (!no_report)
      cmd_display(mode,channel,ans_str,1);
  }
  if (disc && !report) {
    queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
  }
  sh_stat[channel].active = 0;
}

void close_shell(channel,report,disc)
int channel;
int report;
int disc;
{
  close_shell2(channel,report,disc,M_COMMAND);
}

#ifdef USE_SOCKET
/* activate connection on socket */
static int open_asocket(listen_socket,sockfd)
LISTEN_SOCKET *listen_socket;
int sockfd;
{
  ACTIVE_SOCKET *active_socket;
  char tmp[300];
  
  active_socket = (ACTIVE_SOCKET *)malloc(sizeof(ACTIVE_SOCKET));
  if (active_socket == NULL) {
    close(sockfd);
    return(1);
  }
  active_socket->sockfd = sockfd;
  active_socket->type = listen_socket->type;
  active_socket->ax25special = listen_socket->ax25special;
  if (listen_socket->type == STYP_NETCMD) {
    strcpy(active_socket->mycall,listen_socket->mycall);
  }
  active_socket->listen_socket = listen_socket;
  active_socket->channel = -1;
  active_socket->next = NULL;
  if (active_socket_root == NULL) {
    active_socket_root = active_socket;
    active_socket->prev = NULL;
  }
  else {
    active_socket->prev = active_socket_last;
    active_socket_last->next = active_socket;
  }
  active_socket_last = active_socket;
  switch (active_socket->type) {
  case STYP_AXSERV:
    active_socket->login_stat = LS_LOGIN;
    active_socket->lfcrconv = 1;
    active_socket->len_input_string = 0;
    sprintf(tmp,"\nWelcome to %s remote AX25-server.\n",
            ch_stat[0].curcall);
    strcat(tmp,"\nYou have to login to establish access permission\n");
    strcat(tmp,"to remote ham-radio AX25-server via ip-network.\n\n");
    strcat(tmp,"login: ");
    write_socket(active_socket,strlen(tmp),tmp,0);
    break;
  case STYP_NETCMD:
    active_socket->login_stat = NC_COMMAND;
    active_socket->lfcrconv = 1;
    active_socket->len_input_string = 0;
    break;
  }
  return(0);
}

/* deactivate connection on socket */
static void close_asocket(active_socket)
ACTIVE_SOCKET *active_socket;
{
  ACTIVE_SOCKET *active_socket_wrk;
  
  if (active_socket == NULL) return;
  active_socket_wrk = active_socket_root;
  while (active_socket_wrk != NULL) {
    if (active_socket_wrk == active_socket) break;
    active_socket_wrk = active_socket_wrk->next;
  }
  if (active_socket_wrk == NULL) return;
  switch (active_socket->type) {
  case STYP_AXSERV:
    switch (active_socket->login_stat) {
    case LS_LOGIN:
    case LS_PASSWD:
    case LS_NOTACT:
      /* nothing to do */
      break;
    case LS_SETUP:
      abort_socksetup(active_socket,0);
      active_socket->login_stat = LS_NOTACT;
      if (active_socket->channel != -1) {
        sh_stat[active_socket->channel].active_socket = NULL;
        sh_stat[active_socket->channel].active = 0;
        active_socket->channel = -1;
      }
      break;
    case LS_ACTCMD:
    case LS_ACTDAT:
      queue_cmd_data(active_socket->channel,X_COMM,1,M_CMDSCRIPT,"D");
      active_socket->login_stat = LS_NOTACT;
      if (active_socket->channel != -1) {
        sh_stat[active_socket->channel].active_socket = NULL;
        sh_stat[active_socket->channel].active = 0;
        active_socket->channel = -1;
      }
      break;
    }
    break;
  case STYP_NETCMD:
    switch (active_socket->login_stat) {
    case NC_COMMAND:
      /* nothing to do */
      break;
    case NC_SETUP:
      abort_socksetup(active_socket,0);
      active_socket->login_stat = NC_COMMAND;
      if (active_socket->channel != -1) {
        sh_stat[active_socket->channel].active_socket = NULL;
        sh_stat[active_socket->channel].active = 0;
        active_socket->channel = -1;
      }
      break;
    case NC_CONNECT:
      queue_cmd_data(active_socket->channel,X_COMM,1,M_CMDSCRIPT,"D");
      active_socket->login_stat = NC_COMMAND;
      if (active_socket->channel != -1) {
        sh_stat[active_socket->channel].active_socket = NULL;
        sh_stat[active_socket->channel].active = 0;
        active_socket->channel = -1;
      }
      break;
    }
    break;
  }
  if (active_socket->prev == NULL) {
    active_socket_root = active_socket->next;
  }
  else {
    active_socket->prev->next = active_socket->next;
  }
  if (active_socket->next == NULL) {
    active_socket_last = active_socket->prev;
  }
  else {
    active_socket->next->prev = active_socket->prev;
  }
  close(active_socket->sockfd);
  free(active_socket);
}

static void close_socket(listen_socket)
LISTEN_SOCKET *listen_socket;
{
  ACTIVE_SOCKET *active_socket;
  ACTIVE_SOCKET *next_active_socket;
  
  if (listen_socket == NULL) return;
  
  /* go through active sockets and close sockets using the listen socket */
  active_socket = active_socket_root;
  while (active_socket != NULL) {
    next_active_socket = active_socket->next;
    if (active_socket->listen_socket == listen_socket) {
      close_asocket(active_socket);
    }
    active_socket = next_active_socket;
  }

  /* now dequeue listen-socket and close it */  
  if (listen_socket->prev == NULL) {
    listen_socket_root = listen_socket->next;
  }
  else {
    listen_socket->prev->next = listen_socket->next;
  }
  if (listen_socket->next == NULL) {
    listen_socket_last = listen_socket->prev;
  }
  else {
    listen_socket->next->prev = listen_socket->prev;
  }
  close(listen_socket->sockfd);
  free(listen_socket);
}

/* close a socket and all connections using it */
void cmd_endsock(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int found;
  LISTEN_SOCKET *listen_socket;
  char ans_str[256];
  
  /* search for socket in listen-socket-queue */
  found = 0;
  listen_socket = listen_socket_root;
  while ((listen_socket != NULL) && (!found)) {
    if (strcmp(str,listen_socket->socketname) == 0) {
      found = 1;
    }
    else {
      listen_socket = listen_socket->next;
    }
  }
  if (!found) {
    strcpy(ans_str,"Socket not found");
    cmd_display(mode,channel,ans_str,1);
    return;
  }
  close_socket(listen_socket);
  cmd_display(mode,channel,"Socket closed",1);
}
#endif

/* close a shell or redir on the current channel */
void cmd_endshell(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  close_shell2(channel,1,0,mode);
}

#ifdef USE_SOCKET
int write_socket(active_socket,len,str,noconv)
ACTIVE_SOCKET *active_socket;
int len;
char *str;
int noconv;
{
  char buffer[300];
  int newlen;

  if (active_socket == NULL) return(0);
  if (active_socket->type == STYP_NETCMD) {
    if (active_socket->login_stat != NC_CONNECT) return(0);
  }
  memcpy(buffer,str,len);
  newlen = len;
  if (!noconv && active_socket->lfcrconv) {
    newlen = conv_tolf(buffer,len);
  }
  if (write(active_socket->sockfd,buffer,newlen) < newlen) {
    close_asocket(active_socket);
    return(0);
  }
  return(1);
}

void connstr_to_socket(channel,str)
int channel;
char *str;
{

  if (!sh_stat[channel].active) return;
  if (sh_stat[channel].active == A_SOCKET) {
    if (sh_stat[channel].active_socket->type == STYP_AXSERV) {
      if (sh_stat[channel].active_socket->login_stat == LS_SETUP) { 
        write_socket(sh_stat[channel].active_socket,strlen(str),str,0);
      }
    }
  }
}
#endif

int write_pty(channel,len,str)
int channel;
int len;
char *str;
{
  char buffer[300];
  int newlen;

  if (!sh_stat[channel].active) return(0);
#ifdef USE_SOCKET
  if (sh_stat[channel].active == A_SOCKET) {
    return(write_socket(sh_stat[channel].active_socket,len,str,0));
  }
#endif
  memcpy(buffer,str,len);
  newlen = len;
  if (sh_stat[channel].lfcrconv) {
    newlen = conv_tolf(buffer,len);
  }
  if (write(sh_stat[channel].pty,buffer,newlen) < newlen) {
    close_shell(channel,1,0);
    return(0);
  }
  return(1);
}

#ifdef USE_SOCKET
static int actsock_data_allowed(active_socket)
ACTIVE_SOCKET *active_socket;
{
  switch (active_socket->type) {
  case STYP_AXSERV:
    if (active_socket->login_stat == LS_ACTDAT) {
      if (active_socket->channel != -1) {
        return(senddata_allowed(active_socket->channel));
      }
    }
    return(1);
  case STYP_NETCMD:
    switch (active_socket->login_stat) {
    case NC_COMMAND:
      return(1);
    case NC_SETUP:
      return(0);
    case NC_CONNECT:
      if (active_socket->channel != -1) {
        return(senddata_allowed(active_socket->channel));
      }
      return(1);
    }
    break;
  }
  return(1);
}
#endif

void shell_fdset(max_fd,fdmask)
int *max_fd;
fd_set *fdmask;
{
  int channel;
  int fd;
#ifdef USE_SOCKET
  LISTEN_SOCKET *listen_socket;
  ACTIVE_SOCKET *active_socket;
#endif
  
  for (channel=0;channel<tnc_channels;channel++) {
    if ((sh_stat[channel].active) && senddata_allowed(channel)) {
      if (sh_stat[channel].active != A_SOCKET) {
        fd = sh_stat[channel].pty;
        FD_SET(fd,fdmask);
        if (fd > ((*max_fd) - 1)) {
          *max_fd = fd + 1;
        }
      }
    }
  }
#ifdef USE_SOCKET
  listen_socket = listen_socket_root;
  while (listen_socket != NULL) {
    fd = listen_socket->sockfd;
    FD_SET(fd,fdmask);
    if (fd > ((*max_fd) - 1)) {
      *max_fd = fd + 1;
    }
    listen_socket = listen_socket->next;
  }
  active_socket = active_socket_root;
  while (active_socket != NULL) {
    if (actsock_data_allowed(active_socket)) {
      fd = active_socket->sockfd;
      FD_SET(fd,fdmask);
      if (fd > ((*max_fd) - 1)) {
        *max_fd = fd + 1;
      }
    }
    active_socket = active_socket->next;
  }
#endif
}

int shell_receive(fdmask)
fd_set *fdmask;
{
  int channel;
  int result;
  int nodata;
  int status;
  pid_t pid;
#ifdef USE_SOCKET
  LISTEN_SOCKET *listen_socket;
  LISTEN_SOCKET *next_listen_socket;
  ACTIVE_SOCKET *active_socket;
  ACTIVE_SOCKET *next_active_socket;
  int sockfd;
#endif  
  
  result = 1;
  /* now handle data */
  for (channel=0;channel<tnc_channels;channel++) {
    if (sh_stat[channel].active) {
      pty_check_timeout(channel);
    }
    if (use_select) {
      if (sh_stat[channel].active &&
          (sh_stat[channel].active != A_SOCKET)) {
        if (FD_ISSET(sh_stat[channel].pty,fdmask)) {
          read_data_pty(channel);
          result = 0;
        }
      }
    }
    else {
      nodata = 0;
      while (sh_stat[channel].active &&
             (sh_stat[channel].active != A_SOCKET) &&
             senddata_allowed(channel) && !nodata) {
        if (read_data_pty(channel)) {
          result = 0;
        }
        else {
          nodata = 1;
        }
      }
    }
  }

#ifdef USE_SOCKET
  /* go through active sockets */
  active_socket = active_socket_root;
  while (active_socket != NULL) {
    next_active_socket = active_socket->next;
    if (use_select) {
      if (FD_ISSET(active_socket->sockfd,fdmask)) {
        read_data_socket(active_socket);
        result = 0;
      }
    }
    else {
      nodata = 0;
      while ((!nodata) && (actsock_data_allowed(active_socket))) {
        if (read_data_socket(active_socket)) {
          result = 0;
        }
        else {
          nodata = 1;
        }
      }
    }
    active_socket = next_active_socket;
  }

  /* go through sockets waiting for connection */
  listen_socket = listen_socket_root;
  while (listen_socket != NULL) {
    next_listen_socket = listen_socket->next;
    if (use_select) {
      if (FD_ISSET(listen_socket->sockfd,fdmask)) {
        if ((sockfd = accept(listen_socket->sockfd,
            (struct sockaddr *) &cli_addr, &clilen)) >= 0) {
          if (!open_asocket(listen_socket,sockfd)) result = 0;
        }
      }
    }
    else {
      if ((sockfd = accept(listen_socket->sockfd,
          (struct sockaddr *) &cli_addr, &clilen)) >= 0) {
        fcntl(sockfd,F_SETFL,O_NONBLOCK);
        if (!open_asocket(listen_socket,sockfd)) result = 0;
      }
    }
    listen_socket = next_listen_socket;
  }
#endif
  
  /* clean up all remaining zombies */
  while ((pid = waitpid(-1,&status,WNOHANG)) > 0) {
    for (channel=0;channel<tnc_channels;channel++) {
      if (sh_stat[channel].active) {
        if (sh_stat[channel].pid == pid) {
          close_shell(channel,1,0);
        }
      }
    }
  }
  return (result);
}

void init_shell()
{
  int i;
  
  for (i=0;i<tnc_channels;i++) {
    sh_stat[i].active = 0;
    sh_stat[i].pid = 0;
    sh_stat[i].active_socket = NULL;
  }
#ifdef USE_SOCKET
  listen_socket_root = NULL;
  listen_socket_last = NULL;
  active_socket_root = NULL;
  active_socket_last = NULL;
#endif
  if (is_root) fixutmpfile();
}

void exit_shell()
{
  int i;
#ifdef USE_SOCKET
  LISTEN_SOCKET *listen_socket;
  LISTEN_SOCKET *next_listen_socket;
  ACTIVE_SOCKET *active_socket;
  ACTIVE_SOCKET *next_active_socket;
#endif
  
  for (i=0;i<tnc_channels;i++) {
    close_shell(i,0,1);
  }
#ifdef USE_SOCKET
  active_socket = active_socket_root;
  while (active_socket != NULL) {
    next_active_socket = active_socket->next;
    close_asocket(active_socket);
    active_socket = next_active_socket;
  }
  
  listen_socket = listen_socket_root;
  while (listen_socket != NULL) {
    next_listen_socket = listen_socket->next;
    close_socket(listen_socket);
    listen_socket = next_listen_socket;
  }
#endif
}

#ifdef USE_SOCKET
void sock_status_out(channel,str)
int channel;
char *str;
{
  ACTIVE_SOCKET *active_socket;
  char tmpstring[20];
  
  if (sh_stat[channel].active != A_SOCKET) return;
  if (sh_stat[channel].active_socket->type == STYP_AXSERV) {
    active_socket = sh_stat[channel].active_socket;
    if (write_socket(active_socket,strlen(str),str,0)) {
      if ((active_socket->login_stat == LS_ACTCMD) ||
          (active_socket->login_stat == LS_SETUP)) {
        strcpy(tmpstring,"\nCmd> ");
        write_socket(active_socket,strlen(tmpstring),tmpstring,0);
      }
    }
  }
}
#endif

#ifdef USE_SOCKET

void out_socket(channel,str)
int channel;
char *str;
{
  char tmpstr[300];
  ACTIVE_SOCKET *active_socket;
  
  if (sh_stat[channel].active != A_SOCKET) return;
  if (sh_stat[channel].active_socket->type == STYP_AXSERV) {
    active_socket = sh_stat[channel].active_socket;
    
    strcpy(tmpstr,"\n");
    strcat(tmpstr,rem_tnt_str);
    strcat(tmpstr,str);
    write_socket(active_socket,strlen(tmpstr),tmpstr,0);
  }
}

/* answer to tnt-command */
void out_cmd_socket(channel,buffer)
int channel;
char *buffer;
{
  char ans_str[256];
  int ans_len;
  ACTIVE_SOCKET *active_socket;
  
  if (sh_stat[channel].active != A_SOCKET) return;
  if (sh_stat[channel].active_socket->type == STYP_AXSERV) {
    active_socket = sh_stat[channel].active_socket;
    if (active_socket->login_stat >= LS_NOTACT) {
      strcpy(ans_str,rem_tnt_str);
      strcat(ans_str,buffer);
      strcat(ans_str,rem_newlin_str);
      if ((active_socket->login_stat == LS_NOTACT) ||
          (active_socket->login_stat == LS_ACTCMD))
        strcat(ans_str,"\nCmd> ");
      ans_len = strlen(ans_str);
      write_socket(active_socket,ans_len,ans_str,0);
    }
  }
}
#endif

int shell_active(channel)
int channel;
{
  if (sh_stat[channel].active) return(1);
  return(0);
}

void free_shell()
{
  free(sh_stat);
}

int alloc_shell()
{
  sh_stat = (struct shell_stat *)
    malloc(tnc_channels * sizeof(struct shell_stat));
  return(sh_stat == NULL);
}
