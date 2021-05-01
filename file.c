/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for file i/o (file.c)
   created: Mark Wahl DL4YBG 93/08/14
   updated: Mark Wahl DL4YBG 97/01/21
*/

#include "tnt.h"
#ifndef DPBOXT
#include "xmon.h"
#endif

#include <errno.h>
extern int errno;

extern void statlin_update();
extern void cmd_display(int flag,int channel,char *buffer,int cr);
#ifndef DPBOXT
extern void rem_data_display(int channel,char *buffer);
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
#endif
extern int conv_rx_to_local(char code,char *newcode1,char *newcode2);
extern int conv_local_to_tx(char code,char *newcode1,char *newcode2);
#ifndef DPBOXT
extern int calc_crc(char *buf,int n,unsigned int crc);
extern void rem_stat_display(int channel,char *buffer);
#ifdef USE_IFACE
extern int boxcut_active(int channel);
extern void abinfile_to_boxcut(int channel,char *file,int erase,int error);
#endif
extern void cmd_input(int channel,int mode,char *str,int len,int cscript);
extern int senddata_allowed(int channel);
extern void rem_data_display_len(int channel,char *buffer);
#ifdef USE_SOCKET
extern void out_socket(int channel,char *str);
#endif
extern void yapp_tx_flush(int channel);
extern void yapp_rx_flush(int channel);
extern int yapp_download(int init, int abort, yapptype *yapp,
                         char *buffp, int blen);
extern int yapp_upload(int init, int abort, yapptype *yapp, char *buffp,
                       int blen);
extern long sfsize(char *name);
#endif


#ifdef DPBOXT
extern int act_mode;
extern char tnt_dir[];
extern char download_dir[];
extern int append_flag;

struct rx_file mb_file;

#else
extern int tnc_channels;
extern int act_channel;
extern int act_mode;
extern char rem_cls_str[];
extern char rem_brk_str[];
extern char rem_wri_str[];
extern char rem_wyapp_str[];
extern char tnt_upfile[];
extern char tnt_downfile[];
extern char tnt_dir[];
extern char remote_dir[];
extern char upload_dir[];
extern char download_dir[];
extern int tnc_command;
extern int append_flag;
extern char remote_user[];
extern char rem_newlin_str[];
extern int xmon_screen;
extern char xmon_fewpar_txt[];
extern char xmon_illscr_txt[];
extern int file_paclen;

int script_channel;
char abin_dir[80];

struct tx_file *tx_file;
struct rx_file *rx_file;
struct tx_file tx_file_com;
static struct rx_file rx_file_com;
static struct rx_file rx_file_mon;
struct rx_file rx_file_xmon[XMON_SCREENS];
#ifdef USE_IFACE
struct rx_file mb_file;
#endif
#endif

static char line[82];
char file_open_text[] = "File already open";
char no_perm_read_text[] = "No permission to read file";
char ok_text[] = "Ok";

static void execute_line();

#define MAXFILL 1785

static void cut_filename(str,filename)
char *str;
char *filename;
{
  int len;
  char tmpstr[20];
  int i;
  
  if ((len = strlen(filename)) > 13) {
    for (i=0;i<13;i++) {
      tmpstr[12-i] = filename[len-1-i];
    }
    tmpstr[13] = ' ';
  }
  else {
    for (i=0;i<14;i++) {
      if (i < len) {
        tmpstr[i] = filename[i];
      }
      else if (i == len) {
        tmpstr[i] = ' ';
      }
      else {
        tmpstr[i] = '*';
      }
    }
  }
  tmpstr[14] = '\0';
  strcat(str,tmpstr);
}

void show_txfile(channel,str)
int channel;
char *str;
{
#ifdef DPBOXT
  strcat(str,"******************");
#else
  if ((tx_file[channel].type == -1) || (act_mode != M_CONNECT)) {
    strcat(str,"******************");
  }
  else {
    switch (tx_file[channel].type) {
    case TX_NORM:
      strcat(str," TN ");
      break;
    case TX_BIN:
      strcat(str," TB ");
      break;
    case TX_ABIN:
      strcat(str," TA ");
      break;
    case TX_ABINQ:
      strcat(str," TQ ");
      break;
    case TX_PLAIN:
      strcat(str," TP ");
      break;
    case TX_SCRIPT:
      strcat(str," TS ");
      break;
    case TX_YAPP:
      strcat(str," TY ");
      break;
    }
    cut_filename(str,tx_file[channel].name);
  }
#endif
}

void show_rxfile(channel,str)
int channel;
char *str;
{
#ifdef DPBOXT
  if ((act_mode == M_MAILBOX) && (mb_file.type != -1)) {
    switch (mb_file.type) {
    case RX_NORM:
      strcat(str," RN ");
      break;
    case RX_BIN:
      strcat(str," RB ");
      break;
    case RX_PLAIN:
      strcat(str," RP ");
      break;
    }
    cut_filename(str,mb_file.name);
    return;
  }
#else
#ifdef USE_IFACE
  if ((act_mode == M_MAILBOX) && (mb_file.type != -1)) {
    switch (mb_file.type) {
    case RX_NORM:
      strcat(str," RN ");
      break;
    case RX_BIN:
      strcat(str," RB ");
      break;
    case RX_PLAIN:
      strcat(str," RP ");
      break;
    }
    cut_filename(str,mb_file.name);
    return;
  }
#endif
  if ((act_mode == M_MONITOR) && (rx_file_mon.type != -1)) {
    switch (rx_file_mon.type) {
    case RX_NORM:
      strcat(str," RN ");
      break;
    case RX_BIN:
      strcat(str," RB ");
      break;
    case RX_PLAIN:
      strcat(str," RP ");
      break;
    }
    cut_filename(str,rx_file_mon.name);
    return;
  }
  if ((act_mode == M_EXTMON) && (rx_file_xmon[xmon_screen].type != -1)) {
    switch (rx_file_xmon[xmon_screen].type) {
    case RX_NORM:
      strcat(str," RN ");
      break;
    case RX_BIN:
      strcat(str," RB ");
      break;
    case RX_PLAIN:
      strcat(str," RP ");
      break;
    }
    cut_filename(str,rx_file_xmon[xmon_screen].name);
    return;
  }
  if ((rx_file[channel].type == -1) || (act_mode != M_CONNECT)) {
    strcat(str,"******************");
  }
  else {
    switch (rx_file[channel].type) {
    case RX_NORM:
      strcat(str," RN ");
      break;
    case RX_BIN:
      strcat(str," RB ");
      break;
    case RX_ABIN:
      strcat(str," RA ");
      break;
    case RX_ABIN_Q:
      strcat(str," RQ ");
      break;
    case RX_ABIN_E:
      strcat(str," RE ");
      break;
    case RX_PLAIN:
      strcat(str," RP ");
      break;
    case RX_YAPP:
      strcat(str," RY ");
      break;
    case RX_AUTO7P:
      strcat(str," A7 ");
      break;
    }
    cut_filename(str,rx_file[channel].name);
  }
#endif
}

#ifndef DPBOXT
void close_txfile(channel,report)
int channel;
int report;
{
  if (tx_file[channel].type != -1) {
    close(tx_file[channel].fd);
    if (tx_file[channel].type == TX_YAPP) {
      yapp_tx_flush(channel);
      free(tx_file[channel].yapp);
    }
    else {
      if (tx_file[channel].mode == M_REMOTE_TEMP)
        unlink(tx_file[channel].name);
    }
    tx_file[channel].yapp = NULL;
    tx_file[channel].fd = -1;
    tx_file[channel].type = -1;
  }
  statlin_update();
}

/* if during autobin receive an error occurs,
   the file will be renamed and dropped in abin_dir */
static void abin_error(channel)
int channel;
{
  char tmpname[100];
  
  if ((rx_file[channel].type == RX_ABIN) ||
      (rx_file[channel].type == RX_ABIN_Q)) {
    strcpy(tmpname,abin_dir);
    strcat(tmpname,"abinXXXXXX");
    mkstemp(tmpname);
    rename(rx_file[channel].name,tmpname);
    strcpy(rx_file[channel].name,tmpname);
  }
}

void close_rxfile2(channel,report,rename)
int channel;
int report;
int rename;
{
  if (rx_file[channel].type != -1) {
    close(rx_file[channel].fd);
    if (rx_file[channel].type == RX_YAPP) {
      yapp_rx_flush(channel);
      free(rx_file[channel].yapp);
    }
    if (rename) {
      abin_error(channel);
    }
    rx_file[channel].yapp = NULL;
    rx_file[channel].fd = -1;
    rx_file[channel].type = -1;
  }
  statlin_update();
}
#endif

#ifdef USE_IFACE
void close_mbfile()
{
  if (mb_file.type != -1) {
    close(mb_file.fd);
    mb_file.type = -1;
  }
  statlin_update();
}
#endif

#ifndef DPBOXT
static void close_rxfile_mon()
{
  if (rx_file_mon.type != -1) {
    close(rx_file_mon.fd);
    rx_file_mon.type = -1;
  }
  statlin_update();
}

static void close_rxfile_xmon(screen)
int screen;
{
  if (rx_file_xmon[screen].type != -1) {
    close(rx_file_xmon[screen].fd);
    rx_file_xmon[screen].type = -1;
  }
  statlin_update();
}

void close_rxfile(channel,report)
int channel;
int report;
{
  switch (rx_file[channel].type) {
  case RX_NORM:
    /* LOG files are not closed if disconnect */
    return;
  default:
    close_rxfile2(channel,report,1);
    break;
  }
}
#endif

/* set all file-flags to file not open */
void init_file()
{
#ifdef DPBOXT
  append_flag = 1;  
  mb_file.type = -1;
  mb_file.name[0] = '\0';
#else
  int i;

  append_flag = 1;  
  for (i = 0; i < tnc_channels; i++) {
    tx_file[i].type = -1;
    tx_file[i].fd = -1;
    tx_file[i].yapp = NULL;

    rx_file[i].type = -1;
    rx_file[i].fd = -1;
    rx_file[i].yapp = NULL;
    rx_file[i].name[0] = '\0';
  }
  tx_file_com.type = -1;
  rx_file_com.type = -1;
  rx_file_mon.type = -1;
  for (i = 0; i < XMON_SCREENS; i++) {
    rx_file_xmon[i].type = -1;
  }
#ifdef USE_IFACE
  mb_file.type = -1;
  mb_file.name[0] = '\0';
#endif
#endif
}

/* close all open files */
void exit_file()
{
#ifdef DPBOXT
  if (mb_file.type != -1) {
    close(mb_file.fd);
  }
#else
  int i;
  
  for (i = 0; i < tnc_channels; i++) {
    close_txfile(i,0);
    close_rxfile2(i,0,1);
  }
  if (tx_file_com.type != -1) {
    fclose(tx_file_com.fp);
  }
  if (rx_file_com.type != -1) {
    close(rx_file_com.fd);
  }
  if (rx_file_mon.type != -1) {
    close(rx_file_mon.fd);
  }
  for (i = 0; i < XMON_SCREENS; i++) {
    close_rxfile_xmon(i);
  }
#ifdef USE_IFACE
  if (mb_file.type != -1) {
    close(mb_file.fd);
  }
#endif
#endif
}

#ifndef DPBOXT
void drop_priv(mode,channel,uid,gid)
int mode;
int channel;
int *uid;
int *gid;
{
  struct passwd *pstp;
  int nuid,ngid;
  
  if (mode != M_REMOTE) return;
  *uid = geteuid();
  *gid = getegid();
  pstp = getpwuid(*uid);
  if (pstp->pw_uid == 0) { /* uid=0 ? (root) */
    pstp = getpwnam(remote_user);
    if (pstp == NULL) {
      cmd_display(M_CONNECT,channel,
         "Warning: 'remote_user' not existing, using root permissions!",1);
      *uid = -1;
      return;
    }
    nuid = pstp->pw_uid;
    ngid = pstp->pw_gid;
    if (setegid(ngid) == -1) {
      cmd_display(M_CONNECT,channel,"Can't change to remote gid",1);
    }
    if (seteuid(nuid) == -1) {
      cmd_display(M_CONNECT,channel,"Can't change to remote uid",1);
    }
  }
  else {
    *uid = -1;
  }
}

void rest_priv(mode,channel,uid,gid)
int mode;
int channel;
int uid;
int gid;
{
  if (mode != M_REMOTE) return;
  if (uid == -1) return;
  if (seteuid(uid) == -1) {
    cmd_display(M_CONNECT,channel,"Can't change back uid",1);
  }
  if (setegid(gid) == -1) {
    cmd_display(M_CONNECT,channel,"Can't change back gid",1);
  }
}
#endif

void open_logfile(type,flag,channel,len,mode,str)
int type;
int flag;
int channel;
int len;
int mode;
char *str;
{
  char tmpstr[MAXCOLS+1];
  char ans_str[200];
  char file_str[MAXCOLS+1];
  int res;
  int flag1;
  int file_flags;
  int uid,gid;
  int pmode;
#ifndef DPBOXT
  yapptype *yapp;
#endif

  if ((len == 0) && (type != RX_YAPP)) {
    cmd_display(mode,channel,"Illegal filename",1);
    return;
  }
#ifdef USE_IFACE
  if (mode == M_MAILBOX) {
    if (mb_file.type != -1) {
      cmd_display(mode,channel,file_open_text,1);
      return;
    }
    if(strchr(str,'/') != NULL) {
      strcpy(mb_file.name,str);
    }
    else {
      strcpy(mb_file.name,download_dir);
      strcat(mb_file.name,str);
    }
    if ((append_flag) && (type == RX_NORM))
      file_flags = O_RDWR|O_CREAT|O_APPEND;
    else
      file_flags = O_RDWR|O_CREAT|O_EXCL;
    pmode = (mode == M_REMOTE) ? PMODE_REMOTE : PMODE;
    mb_file.fd = open(mb_file.name,file_flags,pmode);
    if (mb_file.fd == -1) {
      if (errno == EEXIST)
        cmd_display(mode,channel,"File already exists",1);
      else if (errno == EACCES)
        cmd_display(mode,channel,"No permission to create file",1);
      else
        cmd_display(mode,channel,"Cannot create file",1);
      return;
    }
    mb_file.type = type;
    mb_file.flag = flag;
    mb_file.mode = mode;
    cmd_display(mode,channel,ok_text,1);
    statlin_update();
    return;
  }
#endif
#ifndef DPBOXT
  if (rx_file[channel].type != -1) {
    cmd_display(mode,channel,file_open_text,1);
    return;
  }
  res = sscanf(str,"%s %s",file_str,tmpstr);
  if ((res <= 0) && (type != RX_YAPP)) {
    cmd_display(mode,channel,"Illegal filename",1);
    return;
  }
  if (res > 0) {
    if (mode == M_REMOTE) {
      if (strchr(file_str,'/') != NULL) {
        cmd_display(mode,channel,"Cannot create file",1);
        return;
      }
      strcpy(rx_file[channel].name,remote_dir);
      strcat(rx_file[channel].name,file_str);
    }
    else {
      if(strchr(file_str,'/') != NULL) {
        strcpy(rx_file[channel].name,file_str);
      }
      else {
        strcpy(rx_file[channel].name,download_dir);
        strcat(rx_file[channel].name,file_str);
      }
    }
  }
  else {
    rx_file[channel].name[0] = '\0';
  }
  if (type != RX_YAPP) {
    if ((append_flag) && (type == RX_NORM))
      file_flags = O_RDWR|O_CREAT|O_APPEND;
    else
      file_flags = O_RDWR|O_CREAT|O_EXCL;
    pmode = (mode == M_REMOTE) ? PMODE_REMOTE : PMODE;
    drop_priv(mode,channel,&uid,&gid);
    rx_file[channel].fd = open(rx_file[channel].name,file_flags,pmode);
    rest_priv(mode,channel,uid,gid);
    if (rx_file[channel].fd == -1) {
      if (type != RX_AUTO7P) {
        if (errno == EEXIST)
          cmd_display(mode,channel,"File already exists",1);
        else if (errno == EACCES)
          cmd_display(mode,channel,"No permission to create file",1);
        else
          cmd_display(mode,channel,"Cannot create file",1);
      }
      return;
    }
  }
  rx_file[channel].type = type;
  rx_file[channel].flag = flag;
  rx_file[channel].mode = mode;

  switch (type) {
  case RX_NORM:
  case RX_BIN:
  case RX_PLAIN:
    if (mode == M_REMOTE) {
      cmd_display(mode,channel,rem_wri_str,1);
    }
    else {
      cmd_display(mode,channel,ok_text,1);
    }
    break;
  case RX_AUTO7P:
    break;
  case RX_ABIN:
    if (res == 1)
      cmd_display(mode,channel,"Ready for autoBIN-receive",1);
    else {
      if (strcmp(tmpstr,"`") != 0) {
        sprintf(ans_str,"//RPRG %s\015",tmpstr);
        flag1 = 0;
        rem_data_display(channel,ans_str);
        queue_cmd_data(channel,X_DATA,strlen(ans_str),flag1,ans_str);
      }
    }
    rx_file[channel].wait_bin = 1;
    break;
  case RX_ABIN_Q:
  case RX_ABIN_E:
    rx_file[channel].wait_bin = 1;
    break;
  case RX_YAPP:
    if ((yapp = (yapptype *)malloc(sizeof(yapptype))) == NULL) {
      cmd_display(mode,channel,"Cannot create file (YAPP-malloc)",1);
      rx_file[channel].type = -1;
      rx_file[channel].fd = -1;
      rx_file[channel].yapp = NULL;
      return;
    }
    rx_file[channel].yapp = yapp;
    yapp->state = YAPPSTATE_R;
    yapp->init = 1;
    yapp->yappc = 0;
    yapp->total = 0;
    yapp->filelength = 0;
    yapp->delete = 0;
    yapp->write = 0;
    yapp->maxfill = MAXFILL;
    yapp->seekpos = 0;
    yapp->channel = channel;
    yapp->outlen = 0;
    yapp->outbufptr = 0;
    yapp->buflen = 0;
    yapp->progress = 0;
    yapp->startval = 0;
    yapp->curval = 0;
    yapp->fdate = 0;
    yapp->ftime = 0;
    yapp->sendbuflen = 0;
    yapp->bufupdate = time(NULL);
    if (mode == M_REMOTE) {
      cmd_display(mode,channel,rem_wyapp_str,1);
    }
    else {
      cmd_display(mode,channel,ok_text,1);
    }
    break;
  }
  statlin_update();
#endif
}

void close_file(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
#ifndef DPBOXT
  int error;
  char abort_str[] = "#ABORT#\r";
  int flag;
#endif

#ifdef USE_IFACE
  if (mode == M_MAILBOX) {
    if (par1) mode = M_CMDSCRIPT;
    if (mb_file.type == -1) {
      cmd_display(mode,channel,"No file open",1);
      return;
    }
    close_mbfile();
    cmd_display(mode,channel,ok_text,1);
    return;
  }
#endif
#ifndef DPBOXT
  if (par1) mode = M_CMDSCRIPT;
  error = 0;
  if (rx_file[channel].type == -1) {
    cmd_display(mode,channel,"No file open",1);
    return;
  }
  if (rx_file[channel].type == RX_ABIN) {
    flag = 0;
    rem_data_display(channel,"\r");
    rem_data_display(channel,abort_str);
    queue_cmd_data(channel,X_DATA,strlen(abort_str),flag,abort_str);
  }
  if (rx_file[channel].type == RX_YAPP) {
    yapp_download(0,1,rx_file[channel].yapp,NULL,0);
  }
  close_rxfile2(channel,0,1);
  if (mode != M_REMOTE)
    cmd_display(mode,channel,ok_text,1);
  else
    cmd_display(mode,channel,rem_cls_str,1);
#endif
}

#ifndef DPBOXT
void open_monfile(type,flag,channel,len,mode,str)
int type;
int flag;
int channel;
int len;
int mode;
char *str;
{
  int file_flags;

  if (rx_file_mon.type != -1) {
    cmd_display(mode,channel,file_open_text,1);
    return;
  }
#ifdef HAS_INDEX
  if(index(str,'/') != NULL) {
#else
  if(strchr(str,'/') != NULL) {
#endif
    strcpy(rx_file_mon.name,str);
  }
  else {
    strcpy(rx_file_mon.name,download_dir);
    strcat(rx_file_mon.name,str);
  }
  if ((append_flag) && (type == RX_NORM))
    file_flags = O_RDWR|O_CREAT|O_APPEND;
  else
    file_flags = O_RDWR|O_CREAT|O_EXCL;
  rx_file_mon.fd = open(rx_file_mon.name,file_flags,PMODE);
  if (rx_file_mon.fd == -1) {
    if (errno == EEXIST)
      cmd_display(mode,channel,"File already exists",1);
    else if (errno == EACCES)
      cmd_display(mode,channel,"No permission to create file",1);
    else
      cmd_display(mode,channel,"Cannot create file",1);
    return;
  }
  rx_file_mon.type = type;
  rx_file_mon.flag = flag;
  rx_file_mon.mode = mode;
  cmd_display(mode,channel,ok_text,1);
  statlin_update();
}

void close_monfile(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if (rx_file_mon.type == -1) {
    cmd_display(mode,channel,"No file open",1);
    return;
  }
  close_rxfile_mon();
  cmd_display(mode,channel,ok_text,1);
}

void open_xmonfile(type,flag,channel,len,mode,str)
int type;
int flag;
int channel;
int len;
int mode;
char *str;
{
  int file_flags;
  int res;
  int screen;
  char file_str[MAXCOLS+1];

  if (mode == M_EXTMON) {
    screen = xmon_screen;
    strcpy(file_str,str);
  }
  else {
    res = sscanf(str,"%d %s",&screen,file_str);
    if (res != 2) {
      cmd_display(mode,channel,xmon_fewpar_txt,1);
      return;
    }
    if ((screen < 0) || (screen >= XMON_SCREENS)) {
      cmd_display(mode,channel,xmon_illscr_txt,1);
      return;
    }
  }
  if (rx_file_xmon[screen].type != -1) {
    cmd_display(mode,channel,file_open_text,1);
    return;
  }
#ifdef HAS_INDEX
  if(index(file_str,'/') != NULL) {
#else
  if(strchr(file_str,'/') != NULL) {
#endif
    strcpy(rx_file_xmon[screen].name,file_str);
  }
  else {
    strcpy(rx_file_xmon[screen].name,download_dir);
    strcat(rx_file_xmon[screen].name,file_str);
  }
  if ((append_flag) && (type == RX_NORM))
    file_flags = O_RDWR|O_CREAT|O_APPEND;
  else
    file_flags = O_RDWR|O_CREAT|O_EXCL;
  rx_file_xmon[screen].fd = open(rx_file_xmon[screen].name,file_flags,PMODE);
  if (rx_file_xmon[screen].fd == -1) {
    if (errno == EEXIST)
      cmd_display(mode,channel,"File already exists",1);
    else if (errno == EACCES)
      cmd_display(mode,channel,"No permission to create file",1);
    else
      cmd_display(mode,channel,"Cannot create file",1);
    return;
  }
  rx_file_xmon[screen].type = type;
  rx_file_xmon[screen].flag = flag;
  rx_file_xmon[screen].mode = mode;
  cmd_display(mode,channel,ok_text,1);
  statlin_update();
}

void close_xmonfile(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int screen;
  int res;
  
  if (mode == M_EXTMON) {
    screen = xmon_screen;
  }
  else {
    res = sscanf(str,"%d",&screen);
    if (res != 1) {
      cmd_display(mode,channel,xmon_fewpar_txt,1);
      return;
    }
    if ((screen < 0) || (screen >= XMON_SCREENS)) {
      cmd_display(mode,channel,xmon_illscr_txt,1);
      return;
    }
  }
  if (rx_file_xmon[screen].type == -1) {
    cmd_display(mode,channel,"No file open",1);
    return;
  }
  close_rxfile_xmon(screen);
  cmd_display(mode,channel,ok_text,1);
}
#endif

void conv_crtolf(str,len)
char *str;
int len;
{
  int i;
  for (i = 0; i < len; i++) {
    if (str[i] == CR) str[i] = LF;
  }
}

void conv_lftocr(str,len)
char *str;
int len;
{
  int i;
  for (i = 0; i < len; i++) {
    if (str[i] == LF) str[i] = CR;
  }
}

static int conv_noctrl(str,len,newstr)
char *str;
int len;
char *newstr;
{
  int i;
  int j;
  char newch1,newch2;
  int cnt;
  
  i = 0;
  j = 0;
  while (i < len) {
    if ((str[i] >= SPACE) && (str[i] != DEL) && (str[i] != ESC2)) {
      if (str[i] >= 0x80) {
        cnt = conv_rx_to_local(str[i],&newch1,&newch2);
        if (cnt) {
          newstr[j] = newch1;
          if (cnt == 2) {
            j++;
            newstr[j] = newch2;
          }
        }
        else newstr[j] = str[i];  
      }
      else newstr[j] = str[i];
    }
    else {
      switch (str[i]) {
        case CR:
          newstr[j] = LF;
          break;
        case TAB:
          newstr[j] = TAB;
          break;
        case DEL:
          newstr[j] = '^';
          j++;
          newstr[j] = '0';
          break;
        case ESC2:
          newstr[j] = '^';
          j++;
          newstr[j] = '1';
          break;
        default:
          newstr[j] = '^';
          j++;
          newstr[j] = str[i] | 0x40;
          break;
      }
    }
    i++;
    j++;
  }
  return(j);
}

static int txconv_noctrl(str,len,newstr)
char *str;
int len;
char *newstr;
{
  int i;
  int j;
  char newch1,newch2;
  int cnt;
  
  i = 0;
  j = 0;
  while (i < len) {
    if ((str[i] >= SPACE) && (str[i] != DEL) && (str[i] != ESC2)) {
      if (str[i] >= 0x80) {
        cnt = conv_local_to_tx(str[i],&newch1,&newch2);
        if (cnt) {
          newstr[j] = newch1;
          if (cnt == 2) {
            j++;
            newstr[j] = newch2;
          }
        }
        else newstr[j] = str[i];  
      }
      else newstr[j] = str[i];
    }
    else {
      switch (str[i]) {
        case LF:
          newstr[j] = CR;
          break;
        case TAB:
          newstr[j] = TAB;
          break;
        case DEL:
          newstr[j] = '^';
          j++;
          newstr[j] = '0';
          break;
        case ESC2:
          newstr[j] = '^';
          j++;
          newstr[j] = '1';
          break;
        default:
          newstr[j] = '^';
          j++;
          newstr[j] = str[i] | 0x40;
          break;
      }
    }
    i++;
    j++;
  }
  return(j);
}

#ifndef DPBOXT
void write_file(channel,len,flag,str)
int channel;
int len;
int flag;
char *str;
{
  int newlen;
  char newstr[560];
  
  /* check if file open */
  if (rx_file[channel].type == -1) return;
  /* check if data shall be logged */
  if ((rx_file[channel].flag & flag) == 0) return; 
  switch (rx_file[channel].type) {
    case RX_NORM:
      newlen = conv_noctrl(str,len,newstr);
      if (write(rx_file[channel].fd,newstr,newlen) < newlen) {
        close_rxfile2(channel,0,0);
      }
      break;
    case RX_BIN:
      if (write(rx_file[channel].fd,str,len) < len) {
        close_rxfile2(channel,0,0);
      }
      break;
    case RX_ABIN:
    case RX_ABIN_Q:
    case RX_ABIN_E:
      break; /* ignore, handled in write_file_abin */
    case RX_YAPP:
      break; /* ignore, handled in write_file_yapp */
    case RX_AUTO7P:
    case RX_PLAIN:
      memcpy(newstr,str,len);
      conv_crtolf(newstr,len);
      if (write(rx_file[channel].fd,newstr,len) < len) {
        close_rxfile2(channel,0,0);
      }
      break;
  }
}

void write_file_abin(channel,len,flag,str)
int channel;
int *len;
int flag;
char *str;
{
  char ans_str[257];
  int tmp;
  int end_abin;
  int time_trans;
  int baud_trans;
  int abin_result;
  int type;
  time_t curtime;
  
  /* check if data shall be logged */
  if ((rx_file[channel].flag & flag) == 0) return; 
  switch (rx_file[channel].type) {
    case RX_NORM:
    case RX_BIN:
    case RX_PLAIN:
    case RX_AUTO7P:
      break; /* ignore, handled in write_file */
    case RX_YAPP:
      break; /* ignore, handled in write_file_yapp */
    case RX_ABIN:
    case RX_ABIN_Q:
    case RX_ABIN_E:
      end_abin = 0;
      if ((tmp = rx_file[channel].len - rx_file[channel].len_tmp) <= *len) {
        *len = tmp;
        end_abin = 1;
      }
      rx_file[channel].len_tmp += *len;
      rx_file[channel].crc_tmp = calc_crc(str,*len,rx_file[channel].crc_tmp);
      curtime = time(NULL);
      time_trans = (int) (curtime -
                          rx_file[channel].start_time);
      if (time_trans < 1) time_trans = 1;
      baud_trans = rx_file[channel].len_tmp * 8 / time_trans;
      sprintf(ans_str,"RX: %d/%d, %d Baud",rx_file[channel].len_tmp,
                                  rx_file[channel].len, baud_trans);
      rem_stat_display(channel,ans_str);
      if (write(rx_file[channel].fd,str,*len) < *len) {
        close_rxfile2(channel,0,1);
      }
      else if (end_abin) {
        rem_data_display(channel,"\015");
        rx_file[channel].end_time = time(NULL);
        time_trans = (int) (rx_file[channel].end_time -
                            rx_file[channel].start_time);
        if (time_trans < 1) time_trans = 1;
        baud_trans = rx_file[channel].len * 8 / time_trans;
        abin_result = (rx_file[channel].crc_tmp != rx_file[channel].crc);
        /* no error, if no checksum received */
        if (rx_file[channel].crc == 0) abin_result = 0;
        type = rx_file[channel].type;
        close_rxfile2(channel,0,abin_result);
        if (abin_result) { /* transfer not successful */
          if (type == RX_ABIN) {
            sprintf(ans_str,
              "AutoBIN-receive not successful !\r"
              "       Checksum: %d, Received Checksum: %d\r"
              "       Total time: %d seconds, Transfer rate: %d Baud",
              rx_file[channel].crc_tmp,rx_file[channel].crc,
              time_trans,baud_trans);
          }
          else {
            sprintf(ans_str,
              "AutoBIN-receive not successful !\r"
              "Checksum: %d, Received Checksum: %d\r"
              "Total time: %d seconds, Transfer rate: %d Baud",
              rx_file[channel].crc_tmp,rx_file[channel].crc,
              time_trans,baud_trans);
          }
          if (type == RX_ABIN_E)
            unlink(rx_file[channel].name);
        }
        else { /* transfer successful */
          if (type == RX_ABIN) {
            sprintf(ans_str,
              "AutoBIN-receive finished\r"
              "       Checksum: %d, Received Checksum: %d\r"
              "       Total time: %d seconds, Transfer rate: %d Baud",
              rx_file[channel].crc_tmp,rx_file[channel].crc,
              time_trans,baud_trans);
          }
          else {
            sprintf(ans_str,
              "AutoBIN-receive finished\r"
              "Filename: %s\r"
              "Checksum: %d, Received Checksum: %d\r"
              "Total time: %d seconds, Transfer rate: %d Baud",
              rx_file[channel].name,
              rx_file[channel].crc_tmp,rx_file[channel].crc,
              time_trans,baud_trans);
          }
        }
#ifdef USE_IFACE
        if (boxcut_active(channel)) {
          abinfile_to_boxcut(channel,rx_file[channel].name,
                             (type == RX_ABIN_E),abin_result);
        }
#endif
        if (type == RX_ABIN) {
          cmd_display(M_REMOTE,channel,ans_str,1);
        }
        else {
          strcat(ans_str,rem_newlin_str);
          rem_data_display(channel,ans_str);
        }
      }
      break;
  }
}

void write_file_yapp(channel,str,len)
int channel;
char *str;
int len;
{
  yapptype *yapp;
  int i;
  char text[256];
  char *ptr;
  int len2;
  
  yapp = rx_file[channel].yapp;
  ptr = str;
  len2 = len;
  if (yapp->init) {
    if ((memchr(ptr,0x15,len) != NULL) || (memchr(ptr,0x18,len) != NULL)) {
      strcpy(text,"RcdABORT");
      strcat(text,rem_newlin_str);
      rem_data_display(channel,text);
      close_rxfile2(channel,0,0);
      return;
    }
    for (i=0;i<len-1;i++) {
      if ((str[i] == 0x05) && (str[i+1] == 0x01)) {
        yapp->init = 0;
        ptr = &str[i];
        len2 = len - i;
      }
    }
  }
  if (!yapp->init) {
    if (yapp_download(0,0,rx_file[channel].yapp,ptr,len2)) {
      return;
    }
    close_rxfile2(channel,0,0);
  }
}
#endif

#ifdef USE_IFACE
void write_mbfile(len,flag,str)
int len;
int flag;
char *str;
{
  int newlen;
  char newstr[560];
  
  /* check if file open */
  if (mb_file.type == -1) return;
  /* check if data shall be logged */
  if ((mb_file.flag & flag) == 0) return; 
  switch (mb_file.type) {
    case RX_NORM:
      newlen = conv_noctrl(str,len,newstr);
      if (write(mb_file.fd,newstr,newlen) < newlen) {
        close_mbfile();
      }
      break;
    case RX_BIN:
      if (write(mb_file.fd,str,len) < len) {
        close_mbfile();
      }
      break;
    case RX_PLAIN:
      memcpy(newstr,str,len);
      conv_crtolf(newstr,len);
      if (write(mb_file.fd,newstr,len) < len) {
        close_mbfile();
      }
      break;
  }
}
#endif

#ifndef DPBOXT
void write_monfile(len,str,append_cr)
int len;
char *str;
int append_cr;
{
  int newlen;
  char newstr[560];
  
  /* check if file open */
  if (rx_file_mon.type == -1) return;
  /* check if data shall be logged */
  switch (rx_file_mon.type) {
    case RX_NORM:
      newlen = conv_noctrl(str,len,newstr);
      if (write(rx_file_mon.fd,newstr,newlen) < newlen) {
        close_rxfile_mon();
      }
      break;
    case RX_BIN:
      if (write(rx_file_mon.fd,str,len) < len) {
        close_rxfile_mon();
      }
      break;
    case RX_PLAIN:
      memcpy(newstr,str,len);
      conv_crtolf(newstr,len);
      if (write(rx_file_mon.fd,newstr,len) < len) {
        close_rxfile_mon();
      }
      break;
  }
  if (!append_cr || (rx_file_mon.type == RX_BIN) || 
     (rx_file_mon.type == -1)) return;
  if (write(rx_file_mon.fd,"\n",1) < 1) {
    close_rxfile_mon();
  }
}

void write_xmonfile(len,screen,str)
int len;
int screen;
char *str;
{
  int newlen;
  char newstr[560];
  
  /* check if file open */
  if (rx_file_xmon[screen].type == -1) return;
  /* check if data shall be logged */
  switch (rx_file_xmon[screen].type) {
    case RX_NORM:
      newlen = conv_noctrl(str,len,newstr);
      if (write(rx_file_xmon[screen].fd,newstr,newlen) < newlen) {
        close_rxfile_mon();
      }
      break;
    case RX_BIN:
      if (write(rx_file_xmon[screen].fd,str,len) < len) {
        close_rxfile_mon();
      }
      break;
    case RX_PLAIN:
      memcpy(newstr,str,len);
      conv_crtolf(newstr,len);
      if (write(rx_file_xmon[screen].fd,newstr,len) < len) {
        close_rxfile_mon();
      }
      break;
  }
}

void open_upfile(state)
int *state;
{
  if (tx_file_com.type != -1) {
    cmd_display(M_COMMAND,0,file_open_text,1);
    *state = S_NORMAL;
    return;
  }
  strcpy(tx_file_com.name,tnt_upfile);
  if ((tx_file_com.fp = fopen(tx_file_com.name,"r"))) {
    script_channel = act_channel;
    tx_file_com.type = TX_SCRIPT;
    tx_file_com.flag = M_CMDSCRIPT;
    do {
      execute_line(state,1);
    } while ((tx_file_com.type != -1) && (!tnc_command));
  }
  else {
    cmd_display(M_COMMAND,0,"ERROR: ",0);
    cmd_display(M_COMMAND,0,tx_file_com.name,0);
    cmd_display(M_COMMAND,0," not found",1);
    *state = S_NORMAL;
  }
  statlin_update();
}

void open_dwnfile(state)
int *state;
{
  if (tx_file_com.type != -1) {
    cmd_display(M_COMMAND,0,file_open_text,1);
    *state = S_END;
    return;
  }
  strcpy(tx_file_com.name,tnt_downfile);
  if ((tx_file_com.fp = fopen(tx_file_com.name,"r"))) {
    script_channel = act_channel;
    tx_file_com.type = TX_SCRIPT;
    tx_file_com.flag = M_CMDSCRIPT;
    do {
      execute_line(state,1);
    } while ((tx_file_com.type != -1) && (!tnc_command));
  }
  else {
    cmd_display(M_COMMAND,0,"ERROR: ",0);
    cmd_display(M_COMMAND,0,tx_file_com.name,0);
    cmd_display(M_COMMAND,0," not found",1);
    *state = S_END;
  }
  statlin_update();
}

void open_comscript(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int dummy;

  if (tx_file_com.type != -1) {
    cmd_display(M_COMMAND,0,file_open_text,1);
    return;
  }
  strcpy(tx_file_com.name,tnt_dir);  
  strcat(tx_file_com.name,str);
  if ((tx_file_com.fp = fopen(tx_file_com.name,"r"))) {
    script_channel = act_channel;
    tx_file_com.type = TX_SCRIPT;
    tx_file_com.flag = act_mode;
    cmd_display(act_mode,act_channel,ok_text,1);
    do {
      execute_line(&dummy,0);
    } while ((tx_file_com.type != -1) && (!tnc_command));
  }
  else {
    cmd_display(act_mode,act_channel,"ERROR: ",0);
    cmd_display(act_mode,act_channel,tx_file_com.name,0);
    cmd_display(act_mode,act_channel," not found",1);
  }
  statlin_update();
}

void next_command(state)
int *state;
{
  if (tx_file_com.type == TX_SCRIPT) {
    do {
      execute_line(state,1);
    } while ((tx_file_com.type != -1) && (!tnc_command));
  }
}

static void execute_line(state,statevalid)
int *state;
int statevalid;
{
  int len;
  
  if (fgets(line,82,tx_file_com.fp) == NULL) {
    fclose(tx_file_com.fp);
    statlin_update();
    tx_file_com.type = -1;
    if (statevalid) {
      if (*state == S_TNT_UP) {
        *state = S_NORMAL;
      }
      else if (*state == S_TNT_DWN) {
        *state = S_END;
      }
    }
    return;
  }
  if (line[0] == '#') {
    tnc_command = 0;
    return;
  }
  len = strlen(line);
  if (line[len-1] == '\n') line[len-1] = '\0';
  cmd_display(tx_file_com.flag,act_channel,line,1);
  cmd_input(script_channel,tx_file_com.flag,line,strlen(line),1);
}

#define BUFSIZE 512

void open_sendfile(type,par2,channel,len,mode,str)
int type;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int file_len;
  unsigned int crc;
  int end;
  int len2;
  char buf[BUFSIZE];
  char ans_str[80];
  int flag1;
  int i;
  char ch;
  int uid,gid;
  int dirlen;
  char tmpstr[MAXCOLS+1];
  char file_str[MAXCOLS+1];
  int res;
  char filename[80];
  char *slashptr;
  int size;
  yapptype *yapp;
  
  flag1 = 0;
  if (tx_file[channel].type != -1) {
    cmd_display(mode,channel,file_open_text,1);
    return;
  }
  res = sscanf(str,"%s %s",file_str,tmpstr);
  if ((res <= 0) || (len == 0)) {
    cmd_display(mode,channel,"Illegal filename",1);
    return;
  }
  if ((mode == M_REMOTE) && (par2 == 0)) {
    if (strchr(file_str,'/') != NULL) {
      cmd_display(mode,channel,no_perm_read_text,1);
      return;
    }
    strcpy(tx_file[channel].name,remote_dir);
    dirlen = strlen(remote_dir);
    strcat(tx_file[channel].name,file_str);
  }
  else {
    if(strchr(file_str,'/') != NULL) {
      dirlen = 0;
      strcpy(tx_file[channel].name,file_str);
    }
    else {
      strcpy(tx_file[channel].name,upload_dir);
      dirlen = strlen(upload_dir);
      strcat(tx_file[channel].name,file_str);
    }
  }
  drop_priv(mode,channel,&uid,&gid);
  tx_file[channel].fd = open(tx_file[channel].name,O_RDONLY); 
  rest_priv(mode,channel,uid,gid);
  if (tx_file[channel].fd == -1) {
    /* change all chars to lower case */
    for (i = dirlen; i < strlen(tx_file[channel].name); i++) {
      ch = *(tx_file[channel].name+i);
      if ((ch > 0x40) && (ch < 0x5b)) {
        ch |= 0x20;
        *(tx_file[channel].name+i) = ch;
      }
    }
    /* now try again */
    if (errno != EACCES) {
      drop_priv(mode,channel,&uid,&gid);
      tx_file[channel].fd = open(tx_file[channel].name,O_RDONLY); 
      rest_priv(mode,channel,uid,gid);
    }
    if (tx_file[channel].fd == -1) {
      /* file does not exist */
      if (type == TX_ABIN) {
        strcpy(ans_str,"#ABORT#\015");
        rem_data_display(channel,ans_str);
        queue_cmd_data(channel,X_DATA,strlen(ans_str),flag1,ans_str);
      }
      if (errno == EACCES)
        cmd_display(mode,channel,no_perm_read_text,1);
      else
        cmd_display(mode,channel,"File not existing",1);
      return;
    }
  }
  tx_file[channel].type = type;
  tx_file[channel].mode = mode;
  
  if ((mode != M_REMOTE) && (mode != M_REMOTE_TEMP))
    cmd_display(mode,channel,ok_text,1);

  switch (type) {
  case TX_NORM:
  case TX_BIN:
  case TX_PLAIN:
  case TX_SCRIPT:
    break;
  case TX_ABIN:
  case TX_ABINQ:
    if (res == 2) {
      if (strcmp(tmpstr,"`") != 0) {
        sprintf(ans_str,"//WPRG %s\015",tmpstr);
        rem_data_display(channel,ans_str);
        queue_cmd_data(channel,X_DATA,strlen(ans_str),flag1,ans_str);
      }
    }
    file_len = 0;
    crc = 0;
    end = 1;
    while (end) {
      len2 = read(tx_file[channel].fd,buf,BUFSIZE);
      file_len += len2;
      crc = calc_crc(buf,len2,crc);
      if (len2 < BUFSIZE) end = 0;
    }
    /* seek to file begin */
    lseek(tx_file[channel].fd,0,SEEK_SET);
    tx_file[channel].len = file_len;
    tx_file[channel].crc = crc;
    tx_file[channel].len_tmp = 0;
    tx_file[channel].wait_ok = 1;
    if (type == TX_ABINQ) { /* no wait if quiet sending */
      tx_file[channel].wait_ok = 0;
      tx_file[channel].start_time = time(NULL);
    }
    slashptr = strrchr(tx_file[channel].name,'/');
    if (slashptr == NULL)
      slashptr = tx_file[channel].name;
    else
      slashptr++;
    for (i=0;i<strlen(slashptr);i++)
      filename[i] = toupper(slashptr[i]);
    filename[strlen(slashptr)] = '\0';
    sprintf(ans_str,"#BIN#%d#|%d#$1EDEADF0#%s\015",file_len,crc,filename);
    rem_data_display(channel,ans_str);
    queue_cmd_data(channel,X_DATA,strlen(ans_str),flag1,ans_str);
    break;
  case TX_YAPP:
    size = (int)sfsize(tx_file[channel].name);
    if ((yapp = (yapptype *)malloc(sizeof(yapptype))) == NULL) {
      cmd_display(mode,channel,"Cannot open file (YAPP-malloc)",1);
      tx_file[channel].type = -1;
      tx_file[channel].fd = -1;
      tx_file[channel].yapp = NULL;
      return;
    }
    tx_file[channel].yapp = yapp;
    yapp->state = 0;
    yapp->init = 0;
    yapp->yappc = 0;
    yapp->total = 0;
    yapp->filelength = size;
    yapp->delete = 0;
    yapp->write = 0;
    yapp->maxfill = MAXFILL;
    yapp->seekpos = 0;
    yapp->channel = channel;
    yapp->outlen = 0;
    yapp->outbufptr = 0;
    yapp->buflen = 0;
    yapp->progress = 0;
    yapp->startval = 0;
    yapp->curval = 0;
    yapp->fdate = 0;
    yapp->ftime = 0;
    yapp->sendbuflen = 0;
    yapp->bufupdate = time(NULL);
    
    yapp_upload(1,0,yapp,NULL,0);

    break;
  }
  statlin_update();
}

void break_send(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if (tx_file[channel].type == -1) {
    cmd_display(mode,channel,"Sending no file",1);
  }
  else {
    if (tx_file[channel].type == TX_YAPP) {
      yapp_upload(0,1,tx_file[channel].yapp,NULL,0);
    }
    close_txfile(channel,0);
    if (mode != M_REMOTE)
      cmd_display(mode,channel,ok_text,1);
    else
      cmd_display(mode,channel,rem_brk_str,1);
  }
}

void next_sendframe(channel)
int channel;
{
  char buf[256];
  char oldbuf[256];
  char buf2[257];
  int buflen;
  int flag;
  char ans_str[200];
  int size;
  int time_trans;
  int baud_trans;
  
  if (tx_file[channel].type == -1) return; /* no file open */
  if ((tx_file[channel].type == TX_ABIN) && (tx_file[channel].wait_ok))
    return;
  if (tx_file[channel].type == TX_YAPP) {
    while (senddata_allowed(channel) && (tx_file[channel].type != -1)) {
      if (yapp_upload(0,0,tx_file[channel].yapp,NULL,0)) {
        return;
      }
      close_txfile(channel,0);
    }
    return;
  }
  while (senddata_allowed(channel) && (tx_file[channel].type != -1)) {
    if (tx_file[channel].type != TX_NORM) size = file_paclen;
    else size = file_paclen/2;
    buflen = read(tx_file[channel].fd,buf,size);
    if (buflen > 0) {
      switch (tx_file[channel].type) {
      case TX_NORM:
        memcpy(oldbuf,buf,buflen);
        buflen = txconv_noctrl(oldbuf,buflen,buf);
        buf2[0] = buflen - 1;
        memcpy(&buf2[1],buf,buflen);
        rem_data_display_len(channel,buf2);
        break;
      case TX_BIN:
        break;
      case TX_ABIN:
      case TX_ABINQ:
        tx_file[channel].len_tmp += buflen;
        time_trans = (int) (time(NULL) -
                            tx_file[channel].start_time);
        if (time_trans < 1) time_trans = 1;
        baud_trans = tx_file[channel].len_tmp * 8 / time_trans;
        sprintf(ans_str,"TX: %d/%d, %d Baud",tx_file[channel].len_tmp,
                                    tx_file[channel].len, baud_trans);
        rem_stat_display(channel,ans_str);
        break;
      case TX_PLAIN:
        conv_lftocr(buf,buflen);
        buf2[0] = buflen - 1;
        memcpy(&buf2[1],buf,buflen);
        rem_data_display_len(channel,buf2);
        break;
      }
      flag = 0;
      queue_cmd_data(channel,X_DATA,buflen,flag,buf);
    }
    if (buflen < size) {
      if ((tx_file[channel].type == TX_ABIN) ||
         (tx_file[channel].type == TX_ABINQ)) {
        rem_data_display(channel,"\015");
        tx_file[channel].end_time = time(NULL);
        time_trans = (int) (tx_file[channel].end_time -
                            tx_file[channel].start_time);
        if (time_trans < 1) time_trans = 1;
        baud_trans = tx_file[channel].len * 8 / time_trans;
        if (tx_file[channel].type == TX_ABIN) {
          sprintf(ans_str,
            "AutoBIN-send finished\r"
            "       Total time: %d seconds, Transfer rate: %d Baud",
            time_trans,baud_trans);
          cmd_display(M_REMOTE,channel,ans_str,1);
        }
        else {
          sprintf(ans_str,
            "AutoBIN-send finished\r"
            "Total time: %d seconds, Transfer rate: %d Baud",
            time_trans,baud_trans);
          strcat(ans_str,rem_newlin_str);
          rem_data_display(channel,ans_str);
        }
      }
      close_txfile(channel,0);
#ifdef USE_SOCKET
      out_socket(channel,"Sending of file finished\n");
#endif      
    }
  }
}

void ana_response_yapp(int channel,char *buffer,int len)
{
  if (yapp_upload(0,0,tx_file[channel].yapp,buffer,len)) {
    return;
  }
  close_txfile(channel,0);
}


int act_abin_rx(channel)
int channel;
{
  return(((rx_file[channel].type == RX_ABIN) ||
          (rx_file[channel].type == RX_ABIN_Q) ||
          (rx_file[channel].type == RX_ABIN_E)) &&
         !rx_file[channel].wait_bin);
}

int wait_abin_rx(channel)
int channel;
{
  return(((rx_file[channel].type == RX_ABIN) ||
          (rx_file[channel].type == RX_ABIN_Q) ||
          (rx_file[channel].type == RX_ABIN_E)) &&
         rx_file[channel].wait_bin);
}

int act_yapp_rx(channel)
int channel;
{
  return(rx_file[channel].type == RX_YAPP);
}

int act_yapp_tx(channel)
int channel;
{
  return(tx_file[channel].type == TX_YAPP);
}

void insert_cr_tx(channel)
int channel;
{
  if ((tx_file[channel].type == TX_ABIN) && (!tx_file[channel].wait_ok)) {
    rem_data_display(channel,"\r");
  }
}

void insert_cr_rx(channel)
int channel;
{
  if ((rx_file[channel].type == RX_ABIN) || 
      (rx_file[channel].type == RX_ABIN_Q) ||
      (rx_file[channel].type == RX_ABIN_E)) {
    rem_data_display(channel,"\r");
  }
}

void free_file()
{
  free(tx_file);
  free(rx_file);
}

int alloc_file()
{
  tx_file = (struct tx_file *)malloc(tnc_channels * sizeof(struct tx_file));
  rx_file = (struct rx_file *)malloc(tnc_channels * sizeof(struct rx_file));
  return((tx_file == NULL) ||
         (rx_file == NULL ));
}
#endif
