/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Remote client for tnt (tntc.c)
   created: Mark Wahl DL4YBG 95/08/17
   updated: Mark Wahl DL4YBG 96/10/27

   08.01.00 hb9xar	struct fd_set changed to fd_set (is a typedef'd struct)
   25.04.03 hb9xar	TERM handler

*/

#include "tnt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <signal.h>

#define INIT_FILE "tntc.ini"
#define DEF_TNTC_DIR ""
#define DEF_FRONTEND_SOCKET "unix:tntsock"

int sockfd=-1;

static struct termios okbd_termios;
static struct termios nkbd_termios;

static union {
  struct sockaddr sa;
  struct sockaddr_in si;
  struct sockaddr_un su;
} addr;

static char tntc_initfile[80];
static char tntc_dir[80];
static char frontend_socket[80];
static char frontend_socket2[80];


static void getrowcols(rows, cols)
int *rows;
int *cols;
{
#ifdef TIOCGWINSZ
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) < 0) {
		*rows = 0;
		*cols = 0;
	} else {
		*rows = ws.ws_row;
		*cols = ws.ws_col;
	}	
#else
#  ifdef TIOCGSIZE
	struct ttysize ws;

	if (ioctl(0, TIOCGSIZE, &ws) < 0) {
		*rows = 0;
		*cols = 0;
	} else {
		*rows = ws.ts_lines;
		*cols = ws.ts_cols;
	}
#  else 
	char *p, *getenv();

	if (p = getenv("LINES"))
		*rows = atoi(p);
	else
		*rows = 0;
	if (p = getenv("COLUMNS"))
		*cols = atoi(p);
	else
		*cols = 0;
#  endif
#endif	
}

void init_console()
{
  tcgetattr(0,&okbd_termios);
  nkbd_termios = okbd_termios;
  nkbd_termios.c_cc[VTIME] = 0;
  nkbd_termios.c_cc[VMIN] = 1;
  nkbd_termios.c_cc[VSTART] = -1;
  nkbd_termios.c_cc[VSTOP] = -1;
  nkbd_termios.c_iflag = 0;
  nkbd_termios.c_iflag |= (IGNBRK|ICRNL);
  nkbd_termios.c_oflag = 0;
  nkbd_termios.c_lflag = 0;
  nkbd_termios.c_cflag |= (CS8|CREAD|CLOCAL);
#ifdef HAS_CRTSCTS
  nkbd_termios.c_cflag &= ~(CSTOPB|PARENB|PARODD|CRTSCTS|HUPCL);
#else
  nkbd_termios.c_cflag &= ~(CSTOPB|PARENB|PARODD|HUPCL);
#endif
  tcsetattr(0,TCSADRAIN,&nkbd_termios);
}

void exit_console()
{
  tcsetattr(0,TCSADRAIN,&okbd_termios);
}

void sigalarm()
{
}

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
    if (*serv_name != '/') strcpy(addr.su.sun_path,tntc_dir);
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

static int analyse_value(str1,str2)
char *str1;
char *str2;
{
  int tmp;

  if (strcmp(str1,"tntc_dir") == 0) {
    strcpy(tntc_dir,str2);
    tmp = strlen(tntc_dir);
    if (tntc_dir[tmp-1] != '/') {
      tntc_dir[tmp] = '/';
      tntc_dir[tmp+1] = '\0';
    }
    return(0);
  }
  else if (strcmp(str1,"frontend_socket") == 0) {
    strcpy(frontend_socket,str2);
    return(0);
  }
  return(1);
}

int read_init_file(argc,argv)
int argc;
char *argv[];
{
  FILE *init_file_fp;
  int file_end;
  int file_corrupt;
  char line[82];
  char str1[82];
  char str2[82];
  char tmp_str[80];
  int rslt;
  int warning;
  int wrong_usage;
  char *str_ptr;
  int scanned;

  strcpy(tntc_dir,DEF_TNTC_DIR);
  strcpy(frontend_socket,DEF_FRONTEND_SOCKET);
  
  strcpy(tntc_initfile,INIT_FILE);
  frontend_socket2[0] = '\0';
  wrong_usage = 0;
  scanned = 1;
  while ((scanned < argc) && (!wrong_usage)) {
    if (strcmp(argv[scanned],"-i") == 0) {
      scanned++;
      if (scanned < argc) {
        strcpy(tntc_initfile,argv[scanned]);
      }
      else wrong_usage = 1;
    }
    else if (strcmp(argv[scanned],"-s") == 0) {
      scanned++;
      if (scanned < argc) {
        strcpy(frontend_socket2,argv[scanned]);
      }
      else wrong_usage = 1;
    }
    else {
      wrong_usage = 1;
    }
    scanned++;
  }
  if (wrong_usage) {
    printf("Usage : tntc [-i <init-file>] [-s <tnt-socket>]\n");
    return(1);
  }
  
  warning = 0;
  if (!(init_file_fp = fopen(tntc_initfile,"r"))) {
    str_ptr = getenv("HOME");
    if (str_ptr != NULL) {
      strcpy(tmp_str,str_ptr);
      strcat(tmp_str,"/");
      strcat(tmp_str,tntc_initfile);
      if (!(init_file_fp = fopen(tmp_str,"r"))) {
        warning = 1;
      }
    }
    else warning = 1;
  }
  if (warning) {
    printf("ERROR: %s not found\n\n",tntc_initfile);
    return(1);
  }
  file_end = 0;
  file_corrupt = 0;
  while (!file_end) {
    if (fgets(line,82,init_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_end = 1;
        file_corrupt = 1;
      }
      else {
        if (line[0] != '#') { /* ignore comment-lines */
          rslt = sscanf(line,"%s %s",str1,str2);
          switch (rslt) {
          case EOF: /* ignore blank lines */
            break;
          case 2:
            if (analyse_value(str1,str2)) {
              file_end = 1;
              file_corrupt = 1;
            }
            break;
          default:
            file_end = 1;
            file_corrupt = 1;
            break;
          }
        }
      }
    }
  }
  fclose(init_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    printf("ERROR: %s is in wrong format, wrong line:\n%s\n\n",
           tntc_initfile,line);
    return(1);
  }
  if (frontend_socket2[0] != '\0') {
    strcpy(frontend_socket,frontend_socket2);
  }
  return(0);
}

void terminate(int sig) {
  char *buffer="\01\31\33c:quit\n";
  write(sockfd,buffer,strlen(buffer));
}

int main(int argc, char *argv[])
{
  char buffer[257];
  fd_set fdmask;
  struct timeval timevalue;
  int nrread;
  int max_fd;
  int count;
  int escflag;
  int active;
//  int sockfd;
  int i;
  int servlen;
  struct sockaddr *saddr;
  int LINES;
  int COLS;
  char *term;
  char frontend_para[80];

  if ((term = getenv("TERM")) == NULL) {
    printf("ERROR: environment variable TERM not set\n");
    return(1);
  }
  getrowcols(&LINES,&COLS);
  
  if (read_init_file(argc,argv))
    exit(1);

  signal(SIGHUP, terminate);	/* terminate on loss of terminal */
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGTERM, terminate);	/* allow to terminate properly */
  signal(SIGINT, terminate);	/* allow to terminate properly */

  saddr = build_sockaddr(frontend_socket,&servlen);
  if (!saddr) {
    printf("ERROR: invalid socket definition\n");
    exit(1);
  }
  /* open socket */
  if ((sockfd = socket(saddr->sa_family,SOCK_STREAM,0)) < 0) {
    printf("ERROR: cannot open socket\n");
    exit(1);
  }
  signal(SIGALRM,sigalarm);
  alarm(2);
  /* connect other program */
  if (connect(sockfd, saddr, servlen) < 0) {
    close(sockfd);
    printf("ERROR: cannot connect to program\n");
    signal(SIGALRM,SIG_IGN);
    exit(1);
  }
  signal(SIGALRM,SIG_IGN);

  /* fcntl(sockfd,F_SETFL,O_NONBLOCK); */

  init_console();
  sprintf(frontend_para,"%s %d %d\n",term,LINES,COLS);
  write(sockfd,frontend_para,strlen(frontend_para));
  active = 1;
  escflag = 0;
  
  while (active) {
    FD_ZERO(&fdmask);
    FD_SET(0,&fdmask);
    FD_SET(sockfd,&fdmask);
    max_fd = sockfd + 1;
    timevalue.tv_usec = 0;
    timevalue.tv_sec = RESY_TIME;
    count = select(max_fd,&fdmask,
                  (fd_set *) 0,(fd_set *) 0,&timevalue);
    if (count == -1) {
      continue;
    }
    if (FD_ISSET(0,&fdmask)) {
      nrread = read(0,buffer,256);
      /* EOF?? - terminal end got killed */
      if (nrread == 0) {
         active=0;
	 terminate(SIGTERM);
      }
      if (nrread > 0) {
        if (write(sockfd,buffer,nrread) != nrread) {
          active = 0;
        }
      }
    }
    if (FD_ISSET(sockfd,&fdmask)) {
      nrread = read(sockfd,buffer,256);
      i = 0;
      while ((i < nrread) && active) {
        switch (escflag) {
        case 0:
          if (buffer[i] == 0x1B) {
            escflag++;
          }
          break;
        case 1:
          if (buffer[i] == 'A') {
            escflag++;
          }
          else {
            escflag = 0;
          }
          break;
        case 2:
          if (buffer[i] == 'A' ) {
            active = 0;
            if (i > 2) {
              nrread = i - 2;
            }
            else {
              nrread = 0;
            }
          }
          else {
            escflag = 0;
          }
          break;
        }
        i++;
      }
      if (nrread > 0) 
        write(1,buffer,nrread);
    }
  }
  
  close(sockfd);
  exit_console();
  exit(0);
}
