/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for initialization (init.c)
   created: Mark Wahl DL4YBG 94/01/11
   updated: Mark Wahl DL4YBG 97/01/20
*/

#include "tnt.h"
#include "init.h"

extern void cmd_display(int flag,int channel,char *buffer,int cr);

#ifdef DPBOXT
extern int scr_divide;
extern int blist_add_plus;
extern char tnt_help_file[];
extern char tnt_dir[];
extern int auto_newline;
extern int supp_hicntl;
extern char box_socket[];
static char box_socket2[80];
extern int tnt_daemon;
int use_select;

char tnt_initfile[80];

/* variables filled from init-file */
char attc_normal;
char attc_statline;
char attc_monitor;
char attc_cstatline;
char attc_controlchar;
char attc_remote;
char attc_special;
char attm_normal;
char attm_statline;
char attm_monitor;
char attm_cstatline;
char attm_controlchar;
char attm_remote;
char attm_special;
int input_linelen;
int lines_mbinput;
int lines_mboutput;
int mbscr_divide;
int color;
int color_save;
int termcap;
int termcap_save;
char download_dir[80];
char func_key_file[80];
char macrotext_dir[80];
extern int insertmode;

#else

extern int scr_divide;
extern int blist_add_plus;
extern char rem_info_file[];
extern char rem_help_file[];
extern char news_file_name[];
extern char name_file_name[];
extern char tnt_help_file[];
extern char tnt_dir[];
extern char remote_dir[];
extern char ctext_dir[];
extern char abin_dir[];
extern char tnt_logbookfile[];
extern int pty_timeout;
extern int auto_newline;
extern int supp_hicntl;
#ifdef GEN_NEW_USER
extern int unix_new_user;
extern char unix_user_dir[];
extern int unix_first_uid;
extern int unix_user_gid;
#endif
#ifdef USE_IFACE
extern char box_socket[];
extern int tnt_box_ssid;
extern char tnt_box_call[];
extern char node_socket[];
extern int tnt_node_ssid;
extern char tnt_node_call[];
#endif
extern char tnt_pwfile[];
extern char tnt_sysfile[];
extern char tnt_noremfile[];
extern char tnt_flchkfile[];
extern char tnt_notownfile[];
extern char tnt_autostartfile[];
extern char tnt_extremotefile[];
extern int tnt_daemon;
int use_select;

char tnt_initfile[80];
char tnt_logfile[80];

/* variables filled from init-file */
char device[80];
int soft_tnc;
unsigned int speed;
int speedflag;
int tnc_channels;
int r_channels;
int file_paclen;
int tnt_comp;
char tnt_upfile[80];
char tnt_downfile[80];
char attc_normal;
char attc_statline;
char attc_monitor;
char attc_cstatline;
char attc_controlchar;
char attc_remote;
char attc_special;
char attm_normal;
char attm_statline;
char attm_monitor;
char attm_cstatline;
char attm_controlchar;
char attm_remote;
char attm_special;
int lines_command;
int lines_monitor;
int lines_input;
int lines_output;
int lines_moncon;
int lines_r_input;
int lines_r_output;
int input_linelen;
#ifdef USE_IFACE
int lines_mbinput;
int lines_mboutput;
int mbscr_divide;
#endif
extern int lines_xmon;
extern int lines_xmon_pre;
extern int xmon_scr_divide;
extern int num_heardentries;
int color;
int color_save;
int termcap;
int termcap_save;
char proc_file[80];
char tnt_cookiefile[80];
char tnt_lockfile[80];
char tnt_ctextfile[80];
char remote_user[80];
char resy_log_file[80];
#ifdef BCAST
char bcast_log_file[80];
/*&&&& Broadcast path defineable  HB9XAR, 19.7.98 */
char bcviapath[128]="\0";
#endif
char upload_dir[80];
char download_dir[80];
char tnt_7plus_dir[80];
char yapp_dir[80];
char run_dir[80];
#ifdef USE_SOCKET
char sock_passfile[80];
#endif
char func_key_file[80];
char macrotext_dir[80];
#ifdef USE_IFACE
char newmaildir[80];
char autobox_dir[80];
char tnt_boxender[80];
char f6fbb_box[80];
#endif
char route_file_name[80];
char frontend_socket[80];
extern int altstat;
extern int disc_on_start;
#ifdef BCAST
extern char tnt_bctempdir[];
extern char tnt_bcsavedir[];
extern char tnt_bcnewmaildir[];
#endif
#ifdef USE_SOCKET
extern int fixed_wait;
extern int amount_wait;
#endif
extern int insertmode;

#endif /* DPBOXT */

static int analyse_value(str1,str2)
char *str1;
char *str2;
{
  int tmp;
  
#ifndef DPBOXT
  if (strcmp(str1,"use_select") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    use_select = tmp;
    return(0);
  }
  else if (strcmp(str1,"device") == 0) {
    strcpy(device,str2);
    return(0);
  }
  else if (strcmp(str1,"soft_tnc") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    soft_tnc = tmp;
    return(0);
  }
#ifdef USE_SOCKET
  else if (strcmp(str1,"fixed_wait") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    fixed_wait = tmp;
    return(0);
  }
   else if (strcmp(str1,"amount_wait") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    amount_wait = tmp;
    return(0);
  }
#endif
  else if (strcmp(str1,"speed") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    speedflag = 0;
    switch (tmp) {
    case 4800:
      speed = B4800;
      return(0);
    case 9600:
      speed = B9600;
      return(0);
    case 19200:
      speed = B19200;
      return(0);
    case 38400:
      speed = B38400;
      return(0);
#ifdef USE_HIBAUD
    case 57600:
      speed = B38400;
      speedflag = ASYNC_SPD_HI;
      return(0);
    case 115200:
      speed = B38400;
      speedflag = ASYNC_SPD_VHI;
      return(0);
#endif
    default:
      return(1);
    }
  }
  else if (strcmp(str1,"tnc_channels") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    tnc_channels = tmp + 1; /* include unproto channel */
    return(0);
  }
  else if (strcmp(str1,"r_channels") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    r_channels = tmp;
    return(0);
  }
  else if (strcmp(str1,"file_paclen") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    file_paclen = tmp;
    if ((file_paclen > 256) || (file_paclen < 20)) return(1);
    if (tnt_comp && (file_paclen == 256)) return(1);
    return(0);
  }
  else if (strcmp(str1,"tnt_comp") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    tnt_comp = tmp;
    if (tnt_comp && (file_paclen == 256)) return(1);
    return(0);
  }
  else if (strcmp(str1,"attc_normal") == 0) {
#else
  if (strcmp(str1,"attc_normal") == 0) {
#endif
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_normal = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attc_statline") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_statline = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attc_monitor") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_monitor = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attc_cstatline") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_cstatline = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attc_controlchar") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_controlchar = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attc_remote") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_remote = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attc_special") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attc_special = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_normal") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_normal = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_statline") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_statline = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_monitor") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_monitor = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_cstatline") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_cstatline = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_controlchar") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_controlchar = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_remote") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_remote = (char) tmp;
    return(0);
  }
  else if (strcmp(str1,"attm_special") == 0) {
    if (sscanf(str2,"%x",&tmp) != 1) return(1);
    attm_special = (char) tmp;
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"lines_command") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_command = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_monitor") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_monitor = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_input") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_input = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_output") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_output = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_moncon") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_moncon = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_r_input") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_r_input = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_r_output") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_r_output = tmp;
    return(0);
  }
  else if (strcmp(str1,"scr_divide") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    scr_divide = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_xmon") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_xmon = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_xmon_pre") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_xmon_pre = tmp;
    return(0);
  }
  else if (strcmp(str1,"xmon_scr_divide") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    xmon_scr_divide = tmp;
    return(0);
  }
  else if (strcmp(str1,"num_heardentries") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    num_heardentries = tmp;
    return(0);
  }
#endif
#ifdef USE_IFACE
  else if (strcmp(str1,"lines_mbinput") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_mbinput = tmp;
    return(0);
  }
  else if (strcmp(str1,"lines_mboutput") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    lines_mboutput = tmp;
    return(0);
  }
  else if (strcmp(str1,"mbscr_divide") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    mbscr_divide = tmp;
    return(0);
  }
#endif
  else if (strcmp(str1,"input_linelen") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    input_linelen = tmp;
    return(0);
  }
  else if (strcmp(str1,"insertmode") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    insertmode = tmp;
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"tnt_upfile") == 0) {
    strcpy(tnt_upfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_downfile") == 0) {
    strcpy(tnt_downfile,str2);
    return(0);
  }
  else if (strcmp(str1,"proc_file") == 0) {
    strcpy(proc_file,str2);
    return(0);
  }
  else if (strcmp(str1,"rem_info_file") == 0) {
    strcpy(rem_info_file,str2);
    return(0);
  }
  else if (strcmp(str1,"rem_help_file") == 0) {
    strcpy(rem_help_file,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_cookiefile") == 0) {
    strcpy(tnt_cookiefile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_lockfile") == 0) {
    strcpy(tnt_lockfile,str2);
    return(0);
  }
#endif
  else if (strcmp(str1,"tnt_help_file") == 0) {
    strcpy(tnt_help_file,str2);
    return(0);
  }
  else if (strcmp(str1,"termcap") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    termcap = tmp;
    return(0);
  }
  else if (strcmp(str1,"color") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    color = tmp;
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"remote_user") == 0) {
    strcpy(remote_user,str2);
    return(0);
  }
  else if (strcmp(str1,"news_file_name") == 0) {
    strcpy(news_file_name,str2);
    return(0);
  }
  else if (strcmp(str1,"name_file_name") == 0) {
    strcpy(name_file_name,str2);
    return(0);
  }
  else if (strcmp(str1,"route_file_name") == 0) {
    strcpy(route_file_name,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_ctextfile") == 0) {
    strcpy(tnt_ctextfile,str2);
    return(0);
  }
#endif
  else if (strcmp(str1,"tnt_dir") == 0) {
    strcpy(tnt_dir,str2);
    tmp = strlen(tnt_dir);
    if (tnt_dir[tmp-1] != '/') {
      tnt_dir[tmp] = '/';
      tnt_dir[tmp+1] = '\0';
    }
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"remote_dir") == 0) {
    strcpy(remote_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"ctext_dir") == 0) {
    strcpy(ctext_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"abin_dir") == 0) {
    strcpy(abin_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_logbookfile") == 0) {
    strcpy(tnt_logbookfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_pwfile") == 0) {
    strcpy(tnt_pwfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_sysfile") == 0) {
    strcpy(tnt_sysfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_noremfile") == 0) {
    strcpy(tnt_noremfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_flchkfile") == 0) {
    strcpy(tnt_flchkfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_notownfile") == 0) {
    strcpy(tnt_notownfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_autostartfile") == 0) {
    strcpy(tnt_autostartfile,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_extremotefile") == 0) {
    strcpy(tnt_extremotefile,str2);
    return(0);
  }
#endif
  else if (strcmp(str1,"blist_add_plus") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    blist_add_plus = tmp;
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"pty_timeout") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    pty_timeout = tmp;
    return(0);
  }
#endif
  else if (strcmp(str1,"auto_newline") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    auto_newline = tmp;
    return(0);
  }
  else if (strcmp(str1,"supp_hicntl") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    supp_hicntl = tmp;
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"resy_log_file") == 0) {
    strcpy(resy_log_file,str2);
    return(0);
  }
#ifdef BCAST
  else if (strcmp(str1,"bcast_log_file") == 0) {
    strcpy(bcast_log_file,str2);
    return(0);
  }
/*&&&& Broadcast path defineable  HB9XAR, 19.7.98 */
  else if (strcmp(str1,"bcviapath") == 0) {
    strcpy(bcviapath,str2);
    return(0);
  }
#endif
  else if (strcmp(str1,"run_dir") == 0) {
    strcpy(run_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"upload_dir") == 0) {
    strcpy(upload_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_7plus_dir") == 0) {
    strcpy(tnt_7plus_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"yapp_dir") == 0) {
    strcpy(yapp_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"altstat") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    altstat = tmp;
    return(0);
  }
  else if (strcmp(str1,"disc_on_start") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    disc_on_start = tmp;
    return(0);
  }
#endif
  else if (strcmp(str1,"download_dir") == 0) {
    strcpy(download_dir,str2);
    return(0);
  }
#ifndef DPBOXT
#ifdef USE_SOCKET  
  else if (strcmp(str1,"sock_passfile") == 0) {
    strcpy(sock_passfile,str2);
    return(0);
  }
#endif
#ifdef GEN_NEW_USER
  else if (strcmp(str1,"unix_new_user") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    unix_new_user = tmp;
    return(0);
  }
  else if (strcmp(str1,"unix_user_dir") == 0) {
    strcpy(unix_user_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"unix_first_uid") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    unix_first_uid = tmp;
    return(0);
  }
  else if (strcmp(str1,"unix_user_gid") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    unix_user_gid = tmp;
    return(0);
  }
#endif
#endif /* DPBOXT */
  else if (strcmp(str1,"func_key_file") == 0) {
    strcpy(func_key_file,str2);
    return(0);
  }
  else if (strcmp(str1,"macrotext_dir") == 0) {
    strcpy(macrotext_dir,str2);
    return(0);
  }
#ifdef USE_IFACE
  else if (strcmp(str1,"box_socket") == 0) {
    strcpy(box_socket,str2);
    return(0);
  }
#ifndef DPBOXT
  else if (strcmp(str1,"node_socket") == 0) {
    strcpy(node_socket,str2);
    return(0);
  }
  else if (strcmp(str1,"frontend_socket") == 0) {
    strcpy(frontend_socket,str2);
    return(0);
  }
  else if (strcmp(str1,"newmaildir") == 0) {
    strcpy(newmaildir,str2);
    return(0);
  }
  else if (strcmp(str1,"autobox_dir") == 0) {
    strcpy(autobox_dir,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_boxender") == 0) {
    strcpy(tnt_boxender,str2);
    return(0);
  }
  else if (strcmp(str1,"f6fbb_box") == 0) {
    strcpy(f6fbb_box,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_box_ssid") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    tnt_box_ssid = tmp;
    return(0);
  }
  else if (strcmp(str1,"tnt_box_call") == 0) {
    if (strlen(str2) > 9) return(1);
    strcpy(tnt_box_call,str2);
    for (tmp=0;tmp<strlen(tnt_box_call);tmp++)
      tnt_box_call[tmp] = toupper(tnt_box_call[tmp]);
    return(0);
  }
  else if (strcmp(str1,"tnt_node_ssid") == 0) {
    if (sscanf(str2,"%d",&tmp) != 1) return(1);
    tnt_node_ssid = tmp;
    return(0);
  }
  else if (strcmp(str1,"tnt_node_call") == 0) {
    if (strlen(str2) > 9) return(1);
    strcpy(tnt_node_call,str2);
    for (tmp=0;tmp<strlen(tnt_node_call);tmp++)
      tnt_node_call[tmp] = toupper(tnt_node_call[tmp]);
    return(0);
  }
#ifdef BCAST
  else if (strcmp(str1,"tnt_bctempdir") == 0) {
    strcpy(tnt_bctempdir,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_bcsavedir") == 0) {
    strcpy(tnt_bcsavedir,str2);
    return(0);
  }
  else if (strcmp(str1,"tnt_bcnewmaildir") == 0) {
    strcpy(tnt_bcnewmaildir,str2);
    return(0);
  }
#endif /* BCAST */
#endif /* !DPBOXT */
#endif
  else {
    return(1);
  }
}

#ifndef DPBOXT
int init_proc()
{
  FILE *fp;
  char file[160];
  pid_t pid;

  strcpy(file,proc_file);
  fp = fopen(file,"w+");
  if (fp == NULL) {
    printf("ERROR: Can't create process file\n\n");
    return(1);
  }
  pid = getpid();
  fprintf(fp,"%d",pid);
  fclose(fp);
  return(0);
}

void exit_proc()
{
  char file[160];

  strcpy(file,proc_file);
  unlink(file);
}
#endif

static void add_tntdir(char *str)
{
  char temp[80];
  
  if (str[0] == '\0')
    return;
  if (str[0] != '/') {
    strcpy(temp,tnt_dir);
    strcat(temp,str);
    strcpy(str,temp);
  }
}

static int check_dir(char *str,char *name)
{
  DIR *ptr;
  
  ptr = opendir(str);
  if (ptr == NULL) {
    printf("ERROR: %s is defined as %s, but directory is not existing\n",
           name,str);
    return(1);
  }
  closedir(ptr);
  return(0);
}

static int add_tntdir_slash(char *str,char *name)
{
  char temp[80];
  int len;
  
  if (str[0] == '\0') {
    strcpy(str,tnt_dir);
    return(check_dir(str,name));
  }
  
  if (str[0] != '/') {
    strcpy(temp,tnt_dir);
    strcat(temp,str);
    strcpy(str,temp);
  }
  len = strlen(str);
  if (str[len-1] != '/') {
    str[len] = '/';
    str[len+1] = '\0';
  }
  return(check_dir(str,name));
}

static int update_filenames()
{
  int res;
  
  res = 0;
#ifndef DPBOXT
  add_tntdir(tnt_upfile);
  add_tntdir(tnt_downfile);
  add_tntdir(proc_file);
  add_tntdir(rem_info_file);
  add_tntdir(rem_help_file);
  add_tntdir(tnt_cookiefile);
  add_tntdir(tnt_lockfile);
#endif
  add_tntdir(tnt_help_file);
#ifndef DPBOXT
  add_tntdir(news_file_name);
  add_tntdir(name_file_name);
  add_tntdir(route_file_name);
  add_tntdir(tnt_ctextfile);
  res += add_tntdir_slash(remote_dir,"remote_dir");
  res += add_tntdir_slash(ctext_dir,"ctext_dir");
  res += add_tntdir_slash(abin_dir,"abin_dir");
  add_tntdir(tnt_logbookfile);
  add_tntdir(tnt_pwfile);
  add_tntdir(tnt_sysfile);
  add_tntdir(tnt_noremfile);
  add_tntdir(tnt_flchkfile);
  add_tntdir(tnt_notownfile);
  add_tntdir(tnt_autostartfile);
  add_tntdir(tnt_extremotefile);
  add_tntdir(resy_log_file);
  add_tntdir(bcast_log_file);
  res += add_tntdir_slash(run_dir,"run_dir");
  res += add_tntdir_slash(upload_dir,"upload_dir");
  res += add_tntdir_slash(tnt_7plus_dir,"tnt_7plus_dir");
  res += add_tntdir_slash(yapp_dir,"yapp_dir");
#endif
  res += add_tntdir_slash(download_dir,"download_dir");
#ifndef DPBOXT
  add_tntdir(sock_passfile);
  res += add_tntdir_slash(unix_user_dir,"unix_user_dir");
#endif
  add_tntdir(func_key_file);
  res += add_tntdir_slash(macrotext_dir,"macrotext_dir");
  add_tntdir(box_socket);
#ifndef DPBOXT
  add_tntdir(node_socket);
/*  add_tntdir(frontend_socket); */
  res += add_tntdir_slash(newmaildir,"newmaildir");
  add_tntdir(autobox_dir);
  add_tntdir(tnt_boxender);
  add_tntdir(f6fbb_box);
  res += add_tntdir_slash(tnt_bctempdir,"tnt_bctempdir");
  res += add_tntdir_slash(tnt_bcsavedir,"tnt_bcsavedir");
  res += add_tntdir_slash(tnt_bcnewmaildir,"tnt_bcnewmaildir");
#endif
  return(res > 0);
}

void list_filenames(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_display(mode,channel,"List of all configured filenames:",1);
  cmd_display(mode,channel,tnt_dir,1);
#ifndef DPBOXT
  cmd_display(mode,channel,tnt_upfile,1);
  cmd_display(mode,channel,tnt_downfile,1);
  cmd_display(mode,channel,proc_file,1);
  cmd_display(mode,channel,rem_info_file,1);
  cmd_display(mode,channel,rem_help_file,1);
  cmd_display(mode,channel,tnt_cookiefile,1);
  cmd_display(mode,channel,tnt_lockfile,1);
#endif
  cmd_display(mode,channel,tnt_help_file,1);
#ifndef DPBOXT
  cmd_display(mode,channel,news_file_name,1);
  cmd_display(mode,channel,name_file_name,1);
  cmd_display(mode,channel,route_file_name,1);
  cmd_display(mode,channel,tnt_ctextfile,1);
  cmd_display(mode,channel,remote_dir,1);
  cmd_display(mode,channel,ctext_dir,1);
  cmd_display(mode,channel,abin_dir,1);
  cmd_display(mode,channel,tnt_logbookfile,1);
  cmd_display(mode,channel,tnt_pwfile,1);
  cmd_display(mode,channel,tnt_sysfile,1);
  cmd_display(mode,channel,tnt_noremfile,1);
  cmd_display(mode,channel,tnt_flchkfile,1);
  cmd_display(mode,channel,tnt_notownfile,1);
  cmd_display(mode,channel,tnt_autostartfile,1);
  cmd_display(mode,channel,tnt_extremotefile,1);
  cmd_display(mode,channel,resy_log_file,1);
  cmd_display(mode,channel,bcast_log_file,1);
  cmd_display(mode,channel,run_dir,1);
  cmd_display(mode,channel,upload_dir,1);
  cmd_display(mode,channel,tnt_7plus_dir,1);
  cmd_display(mode,channel,yapp_dir,1);
#endif
  cmd_display(mode,channel,download_dir,1);
#ifndef DPBOXT
  cmd_display(mode,channel,sock_passfile,1);
  cmd_display(mode,channel,unix_user_dir,1);
#endif
  cmd_display(mode,channel,func_key_file,1);
  cmd_display(mode,channel,macrotext_dir,1);
  cmd_display(mode,channel,box_socket,1);
#ifndef DPBOXT
  cmd_display(mode,channel,node_socket,1);
  cmd_display(mode,channel,frontend_socket,1);
  cmd_display(mode,channel,newmaildir,1);
  cmd_display(mode,channel,autobox_dir,1);
  cmd_display(mode,channel,tnt_boxender,1);
  cmd_display(mode,channel,f6fbb_box,1);
  cmd_display(mode,channel,tnt_bctempdir,1);
  cmd_display(mode,channel,tnt_bcsavedir,1);
  cmd_display(mode,channel,tnt_bcnewmaildir,1);
  cmd_display(mode,channel,tnt_logfile,1);
#endif
}

int read_init_file(argc,argv,unlock)
int argc;
char *argv[];
int *unlock;
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

  use_select = DEF_USE_SELECT;
#ifndef DPBOXT
  strcpy(device,DEF_DEVICE);
  soft_tnc = 0;
#ifdef USE_SOCKET
  fixed_wait = DEF_FIXED_WAIT;
  amount_wait = DEF_AMOUNT_WAIT;
#endif
  speed = DEF_SPEED;
  speedflag = DEF_SPEEDFLAG;
  tnc_channels = DEF_TNC_CHANNELS + 1; /* include unproto channel */
  r_channels = DEF_R_CHANNELS;
  file_paclen = DEF_FILE_PACLEN;
  tnt_comp = DEF_TNT_COMP;
#endif
  attc_normal = DEF_ATTC_NORMAL;
  attc_statline = DEF_ATTC_STATLINE;
  attc_monitor = DEF_ATTC_MONITOR;
  attc_cstatline = DEF_ATTC_CSTATLINE;
  attc_controlchar = DEF_ATTC_CONTROLCHAR;
  attc_remote = DEF_ATTC_REMOTE;
  attc_special = DEF_ATTC_SPECIAL;
  attm_normal = DEF_ATTM_NORMAL;
  attm_statline = DEF_ATTM_STATLINE;
  attm_monitor = DEF_ATTM_MONITOR;
  attm_cstatline = DEF_ATTM_CSTATLINE;
  attm_controlchar = DEF_ATTM_CONTROLCHAR;
  attm_remote = DEF_ATTM_REMOTE;
  attm_special = DEF_ATTM_SPECIAL;
#ifndef DPBOXT
  lines_command = DEF_LINES_COMMAND;
  lines_monitor = DEF_LINES_MONITOR;
  lines_input = DEF_LINES_INPUT;
  lines_output = DEF_LINES_OUTPUT;
  lines_moncon = DEF_LINES_MONCON;
  lines_r_input = DEF_LINES_R_INPUT;
  lines_r_output = DEF_LINES_R_OUTPUT;
  scr_divide = DEF_SCR_DIVIDE;
#endif
  blist_add_plus = DEF_BLIST_ADD_PLUS;
#ifndef DPBOXT
  strcpy(tnt_upfile,UP_FILE);
  strcpy(tnt_downfile,DWN_FILE);
  strcpy(proc_file,DEF_PROC_FILE);
  strcpy(rem_info_file,DEF_INFO_FILE);
  strcpy(rem_help_file,DEF_HELP_FILE);
#endif
  strcpy(tnt_help_file,DEF_TNT_HELP_FILE);
#ifndef DPBOXT
  strcpy(tnt_cookiefile,COOKIE_FILE);
  strcpy(tnt_lockfile,DEF_LOCK_FILE);
#endif
  color = DEF_COLOR;
  termcap = DEF_TERMCAP;
#ifndef DPBOXT
  strcpy(remote_user,DEF_REMOTE_USER);
  strcpy(news_file_name,DEF_NEWS_FILE_NAME);
  strcpy(name_file_name,DEF_NAME_FILE_NAME);
  strcpy(route_file_name,DEF_ROUTE_FILE_NAME);
  strcpy(tnt_ctextfile,DEF_TNT_CTEXTFILE);
#endif
  strcpy(tnt_dir,DEF_TNT_DIR);
#ifndef DPBOXT
  strcpy(remote_dir,DEF_REMOTE_DIR);
  strcpy(ctext_dir,DEF_CTEXT_DIR);
  strcpy(abin_dir,DEF_ABIN_DIR);
  strcpy(tnt_logbookfile,DEF_TNT_LOGBOOKFILE);
  strcpy(tnt_pwfile,DEF_TNT_PWFILE);
  strcpy(tnt_sysfile,DEF_TNT_SYSFILE);
  strcpy(tnt_noremfile,DEF_TNT_NOREMFILE);
  strcpy(tnt_flchkfile,DEF_TNT_FLCHKFILE);
  strcpy(tnt_notownfile,DEF_TNT_NOTOWNFILE);
  strcpy(tnt_autostartfile,DEF_TNT_AUTOSTARTFILE);
  strcpy(tnt_extremotefile,DEF_TNT_EXTREMOTEFILE);
  pty_timeout = DEF_PTY_TIMEOUT;
#ifdef BCAST
  strcpy(tnt_bctempdir,DEF_TNT_BCTEMPDIR);
  strcpy(tnt_bcsavedir,DEF_TNT_BCSAVEDIR);
  strcpy(tnt_bcnewmaildir,DEF_TNT_BCNEWMAILDIR);
#endif
#endif
  auto_newline = DEF_AUTO_NEWLINE;
  supp_hicntl = DEF_SUPP_HICNTL;
#ifndef DPBOXT
  strcpy(resy_log_file,DEF_RESY_LOG_FILE);
#ifdef BCAST
  strcpy(bcast_log_file,DEF_BCAST_LOG_FILE);
#endif
  strcpy(run_dir,DEF_RUN_DIR);
  strcpy(upload_dir,DEF_UPLOAD_DIR);
  strcpy(tnt_7plus_dir,DEF_TNT_7PLUS_DIR);
  strcpy(yapp_dir,DEF_YAPP_DIR);
  altstat = DEF_ALTSTAT;
  disc_on_start = DEF_DISC_ON_START;
#endif
  strcpy(download_dir,DEF_DOWNLOAD_DIR);
#ifndef DPBOXT
#ifdef USE_SOCKET
  strcpy(sock_passfile,DEF_SOCKPASS_FILE);
#endif
#ifdef GEN_NEW_USER
  unix_new_user = DEF_UNIX_NEW_USER;
  strcpy(unix_user_dir,DEF_UNIX_USER_DIR);
  unix_first_uid = DEF_UNIX_FIRST_UID;
  unix_user_gid = DEF_UNIX_USER_GID;
#endif
#endif /* DPBOXT */
  strcpy(func_key_file,DEF_FUNC_KEY_FILE);
  strcpy(macrotext_dir,DEF_MACROTEXT_DIR);
#ifdef USE_IFACE
  strcpy(box_socket,DEF_BOX_SOCKET);
#ifndef DPBOXT
  strcpy(newmaildir,DEF_NEWMAILDIR);
  strcpy(autobox_dir,DEF_AUTOBOX_DIR);
  strcpy(tnt_boxender,DEF_TNT_BOXENDER);
  strcpy(f6fbb_box,DEF_F6FBB_BOX);
  lines_mbinput = DEF_LINES_MBINPUT;
  lines_mboutput = DEF_LINES_MBOUTPUT;
  tnt_box_ssid = DEF_TNT_BOX_SSID;
  strcpy(tnt_box_call,DEF_TNT_BOX_CALL);
  strcpy(node_socket,DEF_NODE_SOCKET);
  tnt_node_ssid = DEF_TNT_NODE_SSID;
  strcpy(tnt_node_call,DEF_TNT_NODE_CALL);
  strcpy(frontend_socket,DEF_FRONTEND_SOCKET);
#endif
  mbscr_divide = DEF_MBSCR_DIVIDE;
#endif
  input_linelen = DEF_INPUT_LINELEN;
  insertmode = DEF_INSERTMODE;
#ifndef DPBOXT
  lines_xmon = DEF_LINES_XMON;
  lines_xmon_pre = DEF_LINES_XMON_PRE;
  xmon_scr_divide = DEF_XMON_SCR_DIVIDE;
  num_heardentries = DEF_NUM_HEARDENTRIES;
#endif
  
  strcpy(tnt_initfile,INIT_FILE);
#ifdef DPBOXT
  box_socket2[0] = '\0';
#else
  strcpy(tnt_logfile,"");
#endif
  wrong_usage = 0;
  scanned = 1;
  while ((scanned < argc) && (!wrong_usage)) {
    if (strcmp(argv[scanned],"-i") == 0) {
      scanned++;
      if (scanned < argc) {
        strcpy(tnt_initfile,argv[scanned]);
      }
      else wrong_usage = 1;
    }
#ifdef DPBOXT
    else if (strcmp(argv[scanned],"-s") == 0) {
      scanned++;
      if (scanned < argc) {
        strcpy(box_socket2,argv[scanned]);
      }
      else wrong_usage = 1;
    }
#else
    else if (strcmp(argv[scanned],"-l") == 0) {
      scanned++;
      if (scanned < argc) {
        strcpy(tnt_logfile,argv[scanned]);
      }
      else wrong_usage = 1;
    }
    else if (strcmp(argv[scanned],"-u") == 0) {
      *unlock = 1;
    }
    else if (strcmp(argv[scanned],"-d") == 0) {
      tnt_daemon = 1;
    }
#endif
    else {
      wrong_usage = 1;
    }
    scanned++;
  }
  if (wrong_usage) {
#ifdef DPBOXT
    printf("Usage : dpboxt [-i <init-file>] [-s <box-socket>]\n");
#else
    printf("Usage : tnt [-i <init-file>] [-l <log-file>] [-u] [-d]\n");
#endif
    return(1);
  }
  
  warning = 0;
  if (!(init_file_fp = fopen(tnt_initfile,"r"))) {
    str_ptr = getenv("HOME");
    if (str_ptr != NULL) {
      strcpy(tmp_str,str_ptr);
      strcat(tmp_str,"/");
      strcat(tmp_str,tnt_initfile);
      if (!(init_file_fp = fopen(tmp_str,"r"))) {
        warning = 1;
      }
    }
    else warning = 1;
  }
  if (warning) {
    printf("ERROR: %s not found\n\n",tnt_initfile);
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
           tnt_initfile,line);
    return(1);
  }
  color_save = color;
  termcap_save = termcap;
  if (check_dir(tnt_dir,"tnt_dir")) return(1);
  if (update_filenames()) return(1);
#ifdef DPBOXT
  if (box_socket2[0] != '\0') {
    strcpy(box_socket,box_socket2);
    add_tntdir(box_socket);
  }
#endif
  return(0);
}
