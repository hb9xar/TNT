/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Procedures for boxlist utility (boxlist.c)
   created: Mark Wahl DL4YBG 94/10/03
   updated: Mark Wahl DL4YBG 96/05/27
*/

#include "tnt.h"
#include "boxlist.h"


extern void win_textout_len(char *str,int len,struct window *win,int conv);
extern void win_charout_cntl(char ch,struct window *win);
extern void win_attrib(char ch,struct window *win);
extern int get_line_noconv(char *str,struct window *win);
#ifndef DPBOXT
#ifdef USE_IFACE
extern void check_bid(int channel,char *bid);
#endif
#endif
extern void cmd_display(int flag,int channel,char *buffer,int cr);
#ifdef USE_IFACE
extern void mb_input(char *str,int len);
extern void mb_nextline();
extern void mb_charout(char ch);
extern void rem_mb_display_buf(char *buffer,int len);
#endif
#ifndef DPBOXT
extern void dat_input(int channel,char *str,int len);
extern void pre_nextline(int channel);
extern void pre_charout(int channel,char ch);
extern void insert_cr_rx(int channel);
extern void rem_data_display_buf(int channel,char *buffer,int len);
#endif
extern void beep();
extern void real_screen_hold(struct real_layout *layout,int part);
extern void statlin_update();
extern void open_logfile(int type,int flag,int channel,
                         int len,int mode,char *str);
extern void close_file(int par1,int par2,int channel,
                       int len,int mode,char *str);
extern int open_window(int lines,struct window *win,int bot);
extern int close_window(struct window *win);


extern int act_mode;
extern int act_channel;
extern int tnc_channels;
extern char download_dir[];
extern int LINES,COLS;
extern char ok_text[];
extern struct window statlin;
extern int txecho_flag;
extern char att_monitor;
extern char att_normal;
extern char att_special;
#ifdef DPBOXT
extern struct rx_file mb_file;
extern struct real_layout mb_layout[LAYOUTPARTS];
#else
extern struct rx_file *rx_file;
#ifdef USE_IFACE
extern struct rx_file mb_file;
#endif
extern struct real_layout (*layout)[LAYOUTPARTS];
#ifdef USE_IFACE
extern struct real_layout mb_layout[LAYOUTPARTS];
#endif
#endif

int blist_add_plus;
#ifdef DPBOXT
struct boxlist_file mbboxlist_file;
static struct window mbboxlistwin;
struct real_layout mbboxlist_layout[LAYOUTPARTS];
int bl_mode;
extern struct rx_file mb_file;
#else
struct boxlist_file *boxlist_file;
static struct window *boxlistwin;
struct real_layout (*boxlist_layout)[LAYOUTPARTS];

#ifdef USE_IFACE
struct boxlist_file mbboxlist_file;
static struct window mbboxlistwin;
struct real_layout mbboxlist_layout[LAYOUTPARTS];
int bl_mode;
extern struct rx_file mb_file;
extern int box_active_flag;
extern int box_busy_flag;
#endif
#endif

static struct tmpname_entry *tmpname_root;


/* display line on boxlist-screen */
static void blist_textout_len(buf,len,mode,channel)
char *buf;
int len;
int mode;
int channel;
{
#ifdef DPBOXT
  win_textout_len(buf,len,&mbboxlistwin,1);
#else
#ifdef USE_IFACE
  if (mode == M_MAILBOX) {
    win_textout_len(buf,len,&mbboxlistwin,1);
  }
  else {
    win_textout_len(buf,len,&boxlistwin[channel],1);
  }
#else
  win_textout_len(buf,len,&boxlistwin[channel],1);
#endif
#endif
}

/* character to boxlist screen */
void blist_charout_cntl(channel,ch)
int channel;
char ch;
{
#ifdef DPBOXT
  win_charout_cntl(ch,&mbboxlistwin);
#else
#ifdef USE_IFACE
  if (bl_mode == M_MAILBOX) {
    win_charout_cntl(ch,&mbboxlistwin);
  }
  else {
    win_charout_cntl(ch,&boxlistwin[channel]);
  }
#else
  win_charout_cntl(ch,&boxlistwin[channel]);
#endif
#endif
}

/* mark selected line in boxlist screen */
static void blist_mark_line(channel,str,len)
int channel;
char *str;
int len;
{
#ifdef DPBOXT
  win_attrib(att_monitor,&mbboxlistwin);
  win_textout_len(str,len,&mbboxlistwin,1);
  win_attrib(att_normal,&mbboxlistwin);
  win_charout_cntl(C_STLINE,&mbboxlistwin);
  win_charout_cntl(C_CUUP,&mbboxlistwin);
#else
#ifdef USE_IFACE
  if (bl_mode == M_MAILBOX) {
    win_attrib(att_monitor,&mbboxlistwin);
    win_textout_len(str,len,&mbboxlistwin,1);
    win_attrib(att_normal,&mbboxlistwin);
    win_charout_cntl(C_STLINE,&mbboxlistwin);
    win_charout_cntl(C_CUUP,&mbboxlistwin);
  }
  else {
    win_attrib(att_monitor,&boxlistwin[channel]);
    win_textout_len(str,len,&boxlistwin[channel],1);
    win_attrib(att_normal,&boxlistwin[channel]);
    win_charout_cntl(C_STLINE,&boxlistwin[channel]);
    win_charout_cntl(C_CUUP,&boxlistwin[channel]);
  }
#else
  win_attrib(att_monitor,&boxlistwin[channel]);
  win_textout_len(str,len,&boxlistwin[channel],1);
  win_attrib(att_normal,&boxlistwin[channel]);
  win_charout_cntl(C_STLINE,&boxlistwin[channel]);
  win_charout_cntl(C_CUUP,&boxlistwin[channel]);
#endif
#endif
}

#ifndef DPBOXT
#ifdef USE_IFACE
/* mark selected line as present in box */
static void blist_have_line(channel,str,len)
int channel;
char *str;
int len;
{
  win_attrib(att_special,&boxlistwin[channel]);
  win_textout_len(str,len,&boxlistwin[channel],1);
  win_attrib(att_normal,&boxlistwin[channel]);
  win_charout_cntl(C_STLINE,&boxlistwin[channel]);
  win_charout_cntl(C_CUUP,&boxlistwin[channel]);
}

/* check if line is present in box */
static int check_bid_in_line(channel)
int channel;
{
  char str[MAXCOLS+1];
  int len;
  char call[MAXCOLS+1];
  char file[MAXCOLS+1];
  char date[MAXCOLS+1];
  char bid[MAXCOLS+1];
  char mbx[MAXCOLS+1];
  char title[MAXCOLS+1];
  int check_nr;
  int bytes;
  int lt;
  int i;
  int res;
  char rubrik[MAXCOLS+1];
  int number;
  int line_ok;

  win_charout_cntl(C_EOLINE,&boxlistwin[channel]);
  len = get_line_noconv(str,&boxlistwin[channel]);
  str[len] = '\0';
  strcpy(boxlist_file[channel].str,str);
  boxlist_file[channel].strlen = len;
  win_charout_cntl(C_STLINE,&boxlistwin[channel]);

/* DIEBOX checklist with BID

   85 DH3FBI > KENWOOD..423 055514DB0GV  DL   851   5 LF & VLF Empfang mit TS-5
   
*/  

  res = sscanf(str,"%d %s > %s %s %s %d %d %s",
               &check_nr, call, file, bid, mbx, &bytes, &lt, title);
  line_ok = (res == 8);
  if (!line_ok) {
    /* special scan if mbx is empty */
    res = sscanf(str,"%d %s > %s %s %d %d %s",
               &check_nr, call, file, bid, &bytes, &lt, title);
    if (res == 7) {
      mbx[0] = '\0';
      line_ok = 1;
    }
  }

/* RUN C mit option D=CRD$@L

DG0XC  DIGI......17 28.04.95 2845DB0BALWE DL      1 DB0BRO-1 wieder ok.

*/

  if (!line_ok) {
    res = sscanf(str,"%s %s %s %s %s %d %s",
                 call, file, date, bid, mbx, &bytes, title);
    line_ok = (res == 7);
    if (!line_ok) {
      /* special scan if mbx is empty */
      res = sscanf(str,"%s %s %s %s %d %s",
                   call, file, date, bid, &bytes, title);
      if (res == 6) {
        mbx[0] = '\0';
        line_ok = 1;
      }
    }
  }

  if (line_ok) {
    for (i=0;i<strlen(file);i++) {
      if (file[i] == '.') file[i] = ' ';
    }
    res = sscanf(file,"%s %d",rubrik,&number);
    if (res != 2) {
      strncpy(rubrik,file,8);
      rubrik[8] = '\0';
      sscanf(file+8,"%d",&number);
    }
    /* check if really bid and not date */
    if (!((bid[2] == '.') && (bid[5] == '.'))) {
      check_bid(channel,bid);
      return(1);
    }
  }
  return(0);
}

void next_bid(result,channel)
int result;
int channel;
{
  struct boxlist_file *blfile;

  blfile = &boxlist_file[channel];
  if (result) {
    blist_have_line(channel,blfile->str,blfile->strlen);
  }
  blfile->curline++;
  win_charout_cntl(C_CUDWN,&boxlistwin[channel]);
  while (blfile->curline < blfile->lines) {
    if (check_bid_in_line(channel))
      return;
    blfile->curline++;
    win_charout_cntl(C_CUDWN,&boxlistwin[channel]);
  }
  cmd_display(blfile->mode,channel,ok_text,1);
  blfile->curline = -1;
}
#endif
#endif

/* send generated line */
static void blist_send_line(channel,str,len,end)
int channel;
char *str;
int len;
int end;
{
  int i;
#ifdef DPBOXT
  if (end) mb_input(str,len);
  for (i=0;i<len;i++) {
    if (str[i] == '\r') {
      mb_nextline();
    }
    else {
      mb_charout(str[i]);
    }
  }
  if (end && txecho_flag) {
    rem_mb_display_buf(str,len);
  }
#else  
#ifdef USE_IFACE
  if (bl_mode == M_MAILBOX) {
    if (end) mb_input(str,len);
    for (i=0;i<len;i++) {
      if (str[i] == '\r') {
        mb_nextline();
      }
      else {
        mb_charout(str[i]);
      }
    }
    if (end && txecho_flag) {
      rem_mb_display_buf(str,len);
    }
  }
  else {
    if (end) dat_input(act_channel,str,len);
    for (i=0;i<len;i++) {
      if (str[i] == '\r') {
        pre_nextline(channel);
      }
      else {
        pre_charout(channel,str[i]);
      }
    }
    if (end && txecho_flag) {
      insert_cr_rx(channel);
      rem_data_display_buf(channel,str,len);
    }
  }
#else
  if (end) dat_input(act_channel,str,len);
  for (i=0;i<len;i++) {
    if (str[i] == '\r') {
      pre_nextline(channel);
    }
    else {
      pre_charout(channel,str[i]);
    }
  }
  if (end && txecho_flag) {
    insert_cr_rx(channel);
    rem_data_display_buf(channel,str,len);
  }
#endif
#endif
}

/* analyse boxlist line */
void blist_analyse_line(channel,func)
int channel;
int func;
{
  char str[MAXCOLS+1];
  int len;
  char call[MAXCOLS+1];
  char file[MAXCOLS+1];
  char date[MAXCOLS+1];
  char bid[MAXCOLS+1];
  char time[MAXCOLS+1];
  char mbx[MAXCOLS+1];
  char title[MAXCOLS+1];
  int check_nr;
  int bytes;
  int lt;
  int i;
  int res;
  char rubrik[MAXCOLS+1];
  int number;
  int end;

#ifdef DPBOXT
  win_charout_cntl(C_EOLINE,&mbboxlistwin);
  len = get_line_noconv(str,&mbboxlistwin);
  str[len] = '\0';
  win_charout_cntl(C_STLINE,&mbboxlistwin);
#else
#ifdef USE_IFACE
  if (bl_mode == M_MAILBOX) {
    win_charout_cntl(C_EOLINE,&mbboxlistwin);
    len = get_line_noconv(str,&mbboxlistwin);
    str[len] = '\0';
    win_charout_cntl(C_STLINE,&mbboxlistwin);
  }
  else {
    win_charout_cntl(C_EOLINE,&boxlistwin[channel]);
    len = get_line_noconv(str,&boxlistwin[channel]);
    str[len] = '\0';
    win_charout_cntl(C_STLINE,&boxlistwin[channel]);
  }
#else  
  win_charout_cntl(C_EOLINE,&boxlistwin[channel]);
  len = get_line_noconv(str,&boxlistwin[channel]);
  str[len] = '\0';
  win_charout_cntl(C_STLINE,&boxlistwin[channel]);
#endif
#endif

/* DIEBOX checklist 

    7 DL4BCU > TERMINE...16 24.09.94 DL      2214   5 2m Mobilfuchsjagd I05 08.

*/  

  res = sscanf(str,"%d %s > %s %s %s %d %d %s",
               &check_nr, call, file, date, mbx, &bytes, &lt, title);
  if (res != 8) {
    /* special scan if mbx is empty */
    res = sscanf(str,"%d %s > %s %s %d %d %s",
               &check_nr, call, file, date, &bytes, &lt, title);
    if (res == 7) {
      mbx[0] = '\0';
      res++;
    }
  }

/* RUN C mit option D=CRD$@L

DG0XC  DIGI......17 28.04.95 2845DB0BALWE DL      1 DB0BRO-1 wieder ok.

*/
  if (res != 8) {
    res = sscanf(str,"%s %s %s %s %s %d %s",
                 call, file, date, bid, mbx, &bytes, title);
    if (res != 7) {
      /* special scan if mbx is empty */
      res = sscanf(str,"%s %s %s %s %d %s",
                   call, file, date, bid, &bytes, title);
      if (res == 6) {
        mbx[0] = '\0';
        res = 8;
      }
    }
    else {
      res = 8;
    }
    if (res == 8) {
      /* if call contains only numbers, it is a DIEBOX-list */
      res = 0;
      for (i=0;i<strlen(call);i++) {
        if (isalpha(call[i])) res = 8;
      }
    }
  }

  if (res == 8) {
    for (i=0;i<strlen(file);i++) {
      if (file[i] == '.') file[i] = ' ';
    }
    res = sscanf(file,"%s %d",rubrik,&number);
    if (res != 2) {
      strncpy(rubrik,file,8);
      rubrik[8] = '\0';
      sscanf(file+8,"%d",&number);
    }
    blist_mark_line(channel,str,len);
    end = 1;
    switch (func) {
    case FUNC_READ:
      if (blist_add_plus)
        sprintf(str,"R %s %d +\r",rubrik,number);
      else
        sprintf(str,"R %s %d\r",rubrik,number);
      break;
    case FUNC_LIST:
      sprintf(str,"L %s %d\r",rubrik,number);
      break;
    case FUNC_ERASE:
      sprintf(str,"E %s %d\r",rubrik,number);
      break;
    case FUNC_KILL:
      sprintf(str,"K %s %d\r",rubrik,number);
      break;
    case FUNC_TRANSFER:
      sprintf(str,"TRA %s %d ",rubrik,number);
      end = 0;
      break;
    case FUNC_LIFETIME:
      sprintf(str,"LT %s %d 0\r",rubrik,number);
      break;
    }
    len = strlen(str);
    blist_send_line(channel,str,len,end);
    return;
  }

/* DIEBOX list 

 263 DL1ZAX 02.11.94 18:03   6763  DL-RUNDSPRUCH NR. 39/94

*/  

  res = sscanf(str,"%d %s %s %s %d %s",
               &number, call, date, time, &bytes, title);
  if (res == 6) {
    blist_mark_line(channel,str,len);
    end = 1;
    switch (func) {
    case FUNC_READ:
      if (blist_add_plus)
        sprintf(str,"R %d +\r",number);
      else
        sprintf(str,"R %d\r",number);
      break;
    case FUNC_LIST:
      sprintf(str,"L %d\r",number);
      break;
    case FUNC_ERASE:
      sprintf(str,"E %d\r",number);
      break;
    case FUNC_KILL:
      sprintf(str,"K %d\r",number);
      break;
    case FUNC_TRANSFER:
      sprintf(str,"TRA %d ",number);
      end = 0;
      break;
    case FUNC_LIFETIME:
      sprintf(str,"LT %d 0\r",number);
      break;
    }
    len = strlen(str);
    blist_send_line(channel,str,len,end);
    return;
  }
  
/* all other not yet implemented */

  beep();
  return;
}

/* select boxlist screen */
void sel_boxlist()
{
  if (act_mode == M_BOXLIST) {
#ifdef DPBOXT
    real_screen_hold(mb_layout,0);
    act_mode = M_MAILBOX;
    statlin_update();
#else
#ifdef USE_IFACE
    if (bl_mode == M_MAILBOX) {
      real_screen_hold(mb_layout,0);
      act_mode = M_MAILBOX;
      statlin_update();
    }
    else {
      real_screen_hold(layout[act_channel],0);
      act_mode = M_CONNECT;
      statlin_update();
    }
#else
    real_screen_hold(layout[act_channel],0);
    act_mode = M_CONNECT;
    statlin_update();
#endif
#endif
  }
  else {
#ifdef DPBOXT
    if (mbboxlist_file.type == -1) return;
    real_screen_hold(mbboxlist_layout,2);
    bl_mode = M_MAILBOX;
#else
    if (act_mode == M_CONNECT) {
      if (boxlist_file[act_channel].type == -1) return;
#ifdef USE_IFACE
      if (boxlist_file[act_channel].curline != -1) return;
#endif
      real_screen_hold(boxlist_layout[act_channel],2);
#ifdef USE_IFACE
      bl_mode = M_CONNECT;
#endif
    }
#ifdef USE_IFACE  
    else if (act_mode == M_MAILBOX) {
      if (mbboxlist_file.type == -1) return;
      real_screen_hold(mbboxlist_layout,2);
      bl_mode = M_MAILBOX;
    }
#endif
    else {
      return;
    }
#endif /* DPBOXT */
    act_mode = M_BOXLIST;
    statlin_update();
  }
}

/* open a temporary file for boxlist */
void cmd_logblist(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char *tmpname;
  struct tmpname_entry *newentry;
  struct tmpname_entry *listentry;
  
  tmpname = strdup("/tmp/tntbliXXXXXX");
  mkstemp(tmpname);
  /* do entry in tempfile list */
  newentry = (struct tmpname_entry *)malloc(sizeof(struct tmpname_entry));
  newentry->name = tmpname;
  newentry->next = NULL;
  if (tmpname_root == NULL) {
    tmpname_root = newentry;
  }
  else {
    listentry = tmpname_root;
    while (listentry->next != NULL) {
      listentry = listentry->next;
    }
    listentry->next = newentry;    
  }
  
  open_logfile(RX_NORM,RX_RCV,channel,strlen(tmpname),mode,tmpname);
}

#define BUFSIZE 512

/* start a boxlist screen with specified file */
void cmd_blist(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int file_len;
  int end;
  int len2;
  char buf[BUFSIZE];
  int i;
  int lines;
  struct boxlist_file *blfile;
  char *linebuf;
  int linelen;

#ifdef DPBOXT
  blfile = &mbboxlist_file;
#else
#ifdef USE_IFACE  
  if (mode == M_MAILBOX) {
    blfile = &mbboxlist_file;
  }
  else {
    blfile = &boxlist_file[channel];
  }
#else
  blfile = &boxlist_file[channel];
#endif
#endif
  
  if (blfile->type != -1) {
    cmd_display(mode,channel,"Boxlist already active",1);
    return;
  }
  if (len == 0) {
    close_file(1,0,channel,0,mode,NULL);
#ifdef DPBOXT
  if ((mode == M_MAILBOX) && (mb_file.name[0] != '\0')) {
    strcpy(blfile->name,mb_file.name);
  }
#else
#ifdef USE_IFACE
    if ((mode == M_MAILBOX) && (mb_file.name[0] != '\0')) {
      strcpy(blfile->name,mb_file.name);
    }
    else if ((mode != M_MAILBOX) && (rx_file[channel].name[0] != '\0')) {
      strcpy(blfile->name,rx_file[channel].name);
    }
#else
    if (rx_file[channel].name[0] != '\0') {
      strcpy(blfile->name,rx_file[channel].name);
    }
#endif
#endif
    else {
      cmd_display(mode,channel,"No boxlist file stored",1);
      return;
    }
  }
  else {
    if(strchr(str,'/') != NULL) {
      strcpy(blfile->name,str);
    }
    else {
      strcpy(blfile->name,download_dir);
      strcat(blfile->name,str);
    }
  }
  blfile->fd = open(blfile->name,O_RDONLY); 
  if (blfile->fd == -1) {
    /* file does not exist */
    cmd_display(mode,channel,"File not existing",1);
    return;
  }
  
  blfile->type = BL_NORMAL;
  /* counting number of LFs in file to get number of lines */
  file_len = 0;
  lines = 0;
  end = 1;
  while (end) {
    len2 = read(blfile->fd,buf,BUFSIZE);
    file_len += len2;
    for (i=0;i<len2;i++) {
      if (buf[i] == '\n') lines++;
    }
    if (len2 < BUFSIZE) end = 0;
  }
  blfile->len = file_len;
  lines += 2; /* just to be sure... */
  blfile->lines = lines;

#ifdef DPBOXT
  open_window(lines,&mbboxlistwin,1);
  mbboxlist_layout[0].win = &mbboxlistwin;
  mbboxlist_layout[0].first_real_line = 0;
  mbboxlist_layout[0].win_num_lines = LINES-1;
  mbboxlist_layout[0].pagesize = LINES-4;
  mbboxlist_layout[1].win = &statlin;
  mbboxlist_layout[1].first_real_line = LINES-1;
  mbboxlist_layout[1].win_num_lines = 1;
  mbboxlist_layout[1].pagesize = 0;
#else
#ifdef USE_IFACE
  if (mode == M_MAILBOX) {
    open_window(lines,&mbboxlistwin,1);
    mbboxlist_layout[0].win = &mbboxlistwin;
    mbboxlist_layout[0].first_real_line = 0;
    mbboxlist_layout[0].win_num_lines = LINES-1;
    mbboxlist_layout[0].pagesize = LINES-4;
    mbboxlist_layout[1].win = &statlin;
    mbboxlist_layout[1].first_real_line = LINES-1;
    mbboxlist_layout[1].win_num_lines = 1;
    mbboxlist_layout[1].pagesize = 0;
  }
  else {
    open_window(lines,&boxlistwin[channel],1);
    boxlist_layout[channel][0].win = &boxlistwin[channel];
    boxlist_layout[channel][0].first_real_line = 0;
    boxlist_layout[channel][0].win_num_lines = LINES-1;
    boxlist_layout[channel][0].pagesize = LINES-4;
    boxlist_layout[channel][1].win = &statlin;
    boxlist_layout[channel][1].first_real_line = LINES-1;
    boxlist_layout[channel][1].win_num_lines = 1;
    boxlist_layout[channel][1].pagesize = 0;
  }
#else
  open_window(lines,&boxlistwin[channel],1);
  boxlist_layout[channel][0].win = &boxlistwin[channel];
  boxlist_layout[channel][0].first_real_line = 0;
  boxlist_layout[channel][0].win_num_lines = LINES-1;
  boxlist_layout[channel][0].pagesize = LINES-4;
  boxlist_layout[channel][1].win = &statlin;
  boxlist_layout[channel][1].first_real_line = LINES-1;
  boxlist_layout[channel][1].win_num_lines = 1;
  boxlist_layout[channel][1].pagesize = 0;
#endif
#endif
  
  /* seek to file begin */
  lseek(blfile->fd,0,SEEK_SET);
  linelen = 0;
  end = 1;
  linebuf = (char *)malloc(COLS+2);
  while (end) {
    len2 = read(blfile->fd,buf,BUFSIZE);
    for (i=0;i<len2;i++) {
      if (buf[i] == '\n') {
        linebuf[linelen] = '\r';
        linelen++;
        blist_textout_len(linebuf,linelen,mode,channel);
        linelen = 0;
      }
      else {
        if (linelen < (COLS-1)) {
          linebuf[linelen] = buf[i];
          linelen++;
        }
      }
    }
    if (len2 < BUFSIZE) end = 0;
  }
  if (linelen) {
    linebuf[linelen] = '\r';
    linelen++;
    blist_textout_len(linebuf,linelen,mode,channel);
    linelen = 0;
  }
  free(linebuf);
  close(blfile->fd);
#ifdef DPBOXT
  cmd_display(mode,channel,ok_text,1);
#else
#ifdef USE_IFACE  
  if (mode == M_MAILBOX) {
    cmd_display(mode,channel,ok_text,1);
  }
  else { 
    if ((box_active_flag) && (!box_busy_flag)) {
      /* mark all mails in box */
      cmd_display(mode,channel,"Scanning DPBox",1);
      blfile->curline = 0;
      blfile->mode = mode;
      win_charout_cntl(C_CUTOP,&boxlistwin[channel]);
      while (blfile->curline < blfile->lines) {
        if (check_bid_in_line(channel))
          return;
        blfile->curline++;
        win_charout_cntl(C_CUDWN,&boxlistwin[channel]);
      }
    }
    cmd_display(mode,channel,ok_text,1);
    blfile->curline = -1;
  }
#else
  cmd_display(mode,channel,ok_text,1);
#endif
#endif
}

/* finish boxlist */
void cmd_xblist(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct boxlist_file *blfile;

#ifdef DPBOXT
  blfile = &mbboxlist_file;
#else
#ifdef USE_IFACE  
  if (mode == M_MAILBOX) {
    blfile = &mbboxlist_file;
  }
  else {
    blfile = &boxlist_file[channel];
  }
#else
  blfile = &boxlist_file[channel];
#endif
#endif
  
  if (blfile->type == -1) {
    cmd_display(mode,channel,"No boxlist active",1);
    return;
  }

#ifdef DPBOXT
  close_window(&mbboxlistwin);
#else
#ifdef USE_IFACE
  if (mode == M_MAILBOX) {
    close_window(&mbboxlistwin);
  }
  else {
    close_window(&boxlistwin[channel]);
  }
#else
  close_window(&boxlistwin[channel]);
#endif
#endif

  blfile->type = -1;  
  cmd_display(mode,channel,ok_text,1);
}

/* init of boxlist */
void init_blist()
{
  int i;
  int j;


  tmpname_root = NULL;
  
#ifdef DPBOXT
  mbboxlist_file.type = -1;
  for (j = 0; j < 5; j++) {
    mbboxlist_layout[j].win = NULL;
    mbboxlist_layout[j].first_real_line = 0;
    mbboxlist_layout[j].win_num_lines = 0;
    mbboxlist_layout[j].pagesize = 0;
  }
  bl_mode = M_MAILBOX;
#else
  for (i=0;i<tnc_channels;i++) {
    boxlist_file[i].type = -1;
    for (j = 0; j < 5; j++) {
      boxlist_layout[i][j].win = NULL;
      boxlist_layout[i][j].first_real_line = 0;
      boxlist_layout[i][j].win_num_lines = 0;
      boxlist_layout[i][j].pagesize = 0;
    }
  }
#ifdef USE_IFACE
  mbboxlist_file.type = -1;
  for (j = 0; j < 5; j++) {
    mbboxlist_layout[j].win = NULL;
    mbboxlist_layout[j].first_real_line = 0;
    mbboxlist_layout[j].win_num_lines = 0;
    mbboxlist_layout[j].pagesize = 0;
  }
  bl_mode = M_CONNECT;
#endif
#endif  
}

void reinit_blist()
{
  int i;
  int j;

#ifdef DPBOXT
  if (mbboxlist_file.type != -1) {
    for (j = 0; j < LAYOUTPARTS; j++) {
      mbboxlist_layout[j].win = NULL;
      mbboxlist_layout[j].first_real_line = 0;
      mbboxlist_layout[j].win_num_lines = 0;
      mbboxlist_layout[j].pagesize = 0;
    }
    mbboxlist_layout[0].win = &mbboxlistwin;
    mbboxlist_layout[0].first_real_line = 0;
    mbboxlist_layout[0].win_num_lines = LINES-1;
    mbboxlist_layout[0].pagesize = LINES-4;
    mbboxlist_layout[1].win = &statlin;
    mbboxlist_layout[1].first_real_line = LINES-1;
    mbboxlist_layout[1].win_num_lines = 1;
    mbboxlist_layout[1].pagesize = 0;
  }
#else
  for (i=0;i<tnc_channels;i++) {
    if (boxlist_file[i].type != -1) {
      for (j = 0; j < LAYOUTPARTS; j++) {
        boxlist_layout[i][j].win = NULL;
        boxlist_layout[i][j].first_real_line = 0;
        boxlist_layout[i][j].win_num_lines = 0;
        boxlist_layout[i][j].pagesize = 0;
      }
      boxlist_layout[i][0].win = &boxlistwin[i];
      boxlist_layout[i][0].first_real_line = 0;
      boxlist_layout[i][0].win_num_lines = LINES-1;
      boxlist_layout[i][0].pagesize = LINES-4;
      boxlist_layout[i][1].win = &statlin;
      boxlist_layout[i][1].first_real_line = LINES-1;
      boxlist_layout[i][1].win_num_lines = 1;
      boxlist_layout[i][1].pagesize = 0;
    }
  }
#ifdef USE_IFACE
  if (mbboxlist_file.type != -1) {
    for (j = 0; j < LAYOUTPARTS; j++) {
      mbboxlist_layout[j].win = NULL;
      mbboxlist_layout[j].first_real_line = 0;
      mbboxlist_layout[j].win_num_lines = 0;
      mbboxlist_layout[j].pagesize = 0;
    }
    mbboxlist_layout[0].win = &mbboxlistwin;
    mbboxlist_layout[0].first_real_line = 0;
    mbboxlist_layout[0].win_num_lines = LINES-1;
    mbboxlist_layout[0].pagesize = LINES-4;
    mbboxlist_layout[1].win = &statlin;
    mbboxlist_layout[1].first_real_line = LINES-1;
    mbboxlist_layout[1].win_num_lines = 1;
    mbboxlist_layout[1].pagesize = 0;
  }
#endif
#endif  
}

/* exit of boxlist */
void exit_blist()
{
  int i;
  struct tmpname_entry *listentry;
  struct tmpname_entry *oldentry;
  
  if (tmpname_root != NULL) {
    listentry = tmpname_root;
    do {
      oldentry = listentry;
      listentry = oldentry->next;
      unlink(oldentry->name);
      free(oldentry->name);
      free(oldentry);
    } while (listentry != NULL);
  }
#ifdef DPBOXT
  if (mbboxlist_file.type != -1) {
    close_window(&mbboxlistwin);
    mbboxlist_file.type = -1;
  }
#else
  for (i=0;i<tnc_channels;i++) {
    if (boxlist_file[i].type != -1) {
      close_window(&boxlistwin[i]);
      boxlist_file[i].type = -1;
    }
  }
#ifdef USE_IFACE
  if (mbboxlist_file.type != -1) {
    close_window(&mbboxlistwin);
    mbboxlist_file.type = -1;
  }
#endif
#endif  
}

#ifndef DPBOXT
void free_boxlist()
{
  free(boxlist_file);
  free(boxlistwin);
  free(boxlist_layout);
}

int alloc_boxlist()
{
  boxlist_file = (struct boxlist_file *)
    malloc(tnc_channels * sizeof(struct boxlist_file));
  boxlistwin = (struct window *)
    malloc(tnc_channels * sizeof(struct window));
  boxlist_layout = (struct real_layout (*) [LAYOUTPARTS])
    malloc(tnc_channels * sizeof(struct real_layout [LAYOUTPARTS]));
  return((boxlist_file == NULL) ||
         (boxlistwin == NULL) ||
         (boxlist_layout == NULL));
}
#endif

