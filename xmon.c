/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Procedures for extended monitor (xmon.c)
   created: Mark Wahl DL4YBG 94/07/17
   updated: Mark Wahl DL4YBG 96/12/01

   25.03.04 hb9xar	include <time.h> for struct tm

*/

#include "tnt.h"
#include "xmon.h"

#include <time.h>


extern int open_window(int lines,struct window *win,int bot);
extern void win_attrib(char ch,struct window *win);
extern int close_window(struct window *win);
extern void win_stringout(char *str,struct window *win, int conv);
extern void real_screen(struct real_layout *layout);
extern void statlin_update();
extern void clear_window(struct window *win);
extern void win_textout(char *str,struct window *win,int conv);
extern void write_xmonfile(int len,int screen,char *str);
extern int get_line(char *str,struct window *win);
extern void win_charout(char ch,struct window *win,int conv);
extern void win_charout_cntl(char ch,struct window *win);
extern void beep();
#ifdef USE_IFACE
extern void check_mbeacon_head(int srcsum,int destsum,
                               char *srccall,char *destcall);
extern void cancel_monbox(int xmon_ch,int monbox_ch);
#endif
extern int decstathuf(char *src,char *dest);
#ifdef USE_IFACE
extern void write_monbox(int monbox_ch,char *buffer);
extern int check_monbox(char *buffer);
#endif
extern void cmd_display(int flag,int channel,char *buffer,int cr);
extern void moni_display2(char *buffer,int attrib);
extern void moni_display_len2(char *buffer,int len);
extern void get_qrg_from_chanstr(char *qrg,char *chanstr);

extern char att_normal;
extern char att_monitor;
extern char att_cstatline;
extern int LINES,COLS;
extern struct window statlin;
extern int act_mode;
extern char ok_text[];
extern int input_linelen;
int xmon_screen;
int xmon_flag;
int mheard_flag;

#ifdef USE_IFACE
extern int scanmbea_flag;
extern int scanmbea_valid;
extern int scanmbea_start;
#endif

#ifdef USE_IFACE
extern int accept_uireq_flag;
#endif

/* variables containing info of frameheader */
char srccall[10];
char destcall[10];
char vialist[100];
char un_calls[10][10];
int un_callcount;
int un_heardfrom;
char un_qrg[20];
int pid;
int frame_ui;
int frame_i;
int head_valid;
#ifdef BCAST
extern int decbcast_flag;
#endif

static struct heardlist *heardlistroot;
static struct heardlist *heardlisttail;
static int heardlistcount;
static struct monheader monheader_info;
static struct xmon_stat xm_stat[MAX_XMON];
static struct window xmonwin[XMON_SCREENS];
static struct window xmonprewin[XMON_SCREENS];
static struct window xmonstatwin[XMON_SCREENS];
struct real_layout xmon_layout[XMON_SCREENS][LAYOUTPARTS];
static struct window heardwin;
struct real_layout heard_layout[LAYOUTPARTS];

static char xmon_xmondis_txt[] = "XMON disabled";
char xmon_fewpar_txt[] = "Too few parameters";
char xmon_illscr_txt[] = "Illegal screen";
static char xmon_scruse_txt[] = "Screen already in use";
static char xmon_illcall_txt[] = "Callsign(s) illegal";
static char xmon_noxmon_txt[] = "No XMON channel left";
static char xmon_close_txt[] = "Screen closed";
static char xmon_notact_txt[] = "Screen not active";
static char xmon_conact_txt[] = "Connection is already monitored";
static char xmon_nofree_txt[] = "No free screen available";
static char xmon_disp1_txt[] = "Screen ";
static char xmon_disp2_txt[] = " is used for monitoring";
static char xmon_ill_txt[] = "only allowed in extmon screen";

int lines_xmon;
int lines_xmon_pre;
int xmon_scr_divide;
int num_heardentries;

void init_xmon()
{
  int i;
  int j;
  
#ifdef USE_IFACE
  scanmbea_flag = 0;
  scanmbea_valid = 0;
  scanmbea_start = 0;
#endif
  head_valid = 0;
  monheader_info.header_valid = 0;

  if (((LINES/xmon_scr_divide-1) < 2) ||
      (xmon_scr_divide < 2) || (xmon_scr_divide > 40)) xmon_scr_divide = 3;  
  xmon_flag = 1;
  for (i=0;i<MAX_XMON;i++) {
    xm_stat[i].active = 0;
  }
  for (i=0;i<XMON_SCREENS;i++) {
    for (j=0;j<LAYOUTPARTS;j++) {
      xmon_layout[i][j].win = NULL;
      xmon_layout[i][j].first_real_line = 0;
      xmon_layout[i][j].win_num_lines = 0;
      xmon_layout[i][j].pagesize = 0;
    }
  }
  for (i=0;i<XMON_SCREENS;i++) {
    open_window(lines_xmon_pre,&xmonprewin[i],1);
    open_window(1,&xmonstatwin[i],1);
    open_window(lines_xmon,&xmonwin[i],1);
    win_attrib(att_cstatline,&xmonstatwin[i]);
    xmon_layout[i][0].win = &xmonprewin[i];
    xmon_layout[i][0].first_real_line = 0;
    xmon_layout[i][0].win_num_lines = LINES/xmon_scr_divide-1;
    xmon_layout[i][0].pagesize = LINES/xmon_scr_divide-2;
    xmon_layout[i][1].win = &xmonstatwin[i];
    xmon_layout[i][1].first_real_line = LINES/xmon_scr_divide-1;
    xmon_layout[i][1].win_num_lines = 1;
    xmon_layout[i][1].pagesize = 0;
    xmon_layout[i][2].win = &xmonwin[i];
    xmon_layout[i][2].first_real_line = LINES/xmon_scr_divide;
    xmon_layout[i][2].win_num_lines = LINES-LINES/xmon_scr_divide-1;
    xmon_layout[i][2].pagesize = LINES-LINES/xmon_scr_divide-4;
    xmon_layout[i][3].win = &statlin;
    xmon_layout[i][3].first_real_line = LINES - 1;
    xmon_layout[i][3].win_num_lines = 1;
    xmon_layout[i][3].pagesize = 0;
  }
  xmon_screen = 0;
}

void reinit_xmon()
{
  int i;
  int j;

  if (((LINES/xmon_scr_divide-1) < 2) ||
      (xmon_scr_divide < 2) || (xmon_scr_divide > 40)) xmon_scr_divide = 3;  
  for (i=0;i<XMON_SCREENS;i++) {
    for (j=0;j<LAYOUTPARTS;j++) {
      xmon_layout[i][j].win = NULL;
      xmon_layout[i][j].first_real_line = 0;
      xmon_layout[i][j].win_num_lines = 0;
      xmon_layout[i][j].pagesize = 0;
    }
  }
  for (i=0;i<XMON_SCREENS;i++) {
    xmon_layout[i][0].win = &xmonprewin[i];
    xmon_layout[i][0].first_real_line = 0;
    xmon_layout[i][0].win_num_lines = LINES/xmon_scr_divide-1;
    xmon_layout[i][0].pagesize = LINES/xmon_scr_divide-2;
    xmon_layout[i][1].win = &xmonstatwin[i];
    xmon_layout[i][1].first_real_line = LINES/xmon_scr_divide-1;
    xmon_layout[i][1].win_num_lines = 1;
    xmon_layout[i][1].pagesize = 0;
    xmon_layout[i][2].win = &xmonwin[i];
    xmon_layout[i][2].first_real_line = LINES/xmon_scr_divide;
    xmon_layout[i][2].win_num_lines = LINES-LINES/xmon_scr_divide-1;
    xmon_layout[i][2].pagesize = LINES-LINES/xmon_scr_divide-4;
    xmon_layout[i][3].win = &statlin;
    xmon_layout[i][3].first_real_line = LINES - 1;
    xmon_layout[i][3].win_num_lines = 1;
    xmon_layout[i][3].pagesize = 0;
  }
}

void exit_xmon()
{
  int i;
  
  for (i=0;i<XMON_SCREENS;i++) {
    close_window(&xmonwin[i]);
    close_window(&xmonprewin[i]);
    close_window(&xmonstatwin[i]);
  }
}

void init_mheard()
{
  int j;
  
  mheard_flag = 1;
  heardlistroot = NULL;
  heardlisttail = NULL;
  heardlistcount = 0;
  for (j=0;j<LAYOUTPARTS;j++) {
    heard_layout[j].win = NULL;
    heard_layout[j].first_real_line = 0;
    heard_layout[j].win_num_lines = 0;
    heard_layout[j].pagesize = 0;
  }
  open_window(num_heardentries+3,&heardwin,1);
  heard_layout[0].win = &heardwin;
  heard_layout[0].first_real_line = 0;
  heard_layout[0].win_num_lines = LINES-1;
  heard_layout[0].pagesize = LINES-4;
  heard_layout[1].win = &statlin;
  heard_layout[1].first_real_line = LINES - 1;
  heard_layout[1].win_num_lines = 1;
  heard_layout[1].pagesize = 0;
}

void reinit_mheard()
{
  int j;
  
  for (j=0;j<LAYOUTPARTS;j++) {
    heard_layout[j].win = NULL;
    heard_layout[j].first_real_line = 0;
    heard_layout[j].win_num_lines = 0;
    heard_layout[j].pagesize = 0;
  }
  heard_layout[0].win = &heardwin;
  heard_layout[0].first_real_line = 0;
  heard_layout[0].win_num_lines = LINES-1;
  heard_layout[0].pagesize = LINES-4;
  heard_layout[1].win = &statlin;
  heard_layout[1].first_real_line = LINES - 1;
  heard_layout[1].win_num_lines = 1;
  heard_layout[1].pagesize = 0;
}

void exit_mheard()
{
  struct heardlist *current;
  struct heardlist *next;

  if (heardlistroot != NULL) {
    current = heardlistroot;
    do {
      next = current->next;
      free(current->srccall);
      free(current->destcall);
      free(current);
      current = next;
    } while (current != NULL);
  }
  close_window(&heardwin);
}

static void xmonstat_display(screen)
int screen;
{
  char *stat_str;
  int i;
  int stat_len;

  stat_str = (char *)malloc(COLS+2);
  strcpy(stat_str,"\015*******************");
  for (i=0;i<MAX_XMON;i++) {
    if ((xm_stat[i].active) && (xm_stat[i].screen == screen)) {
      strcat(stat_str," ");
      strcat(stat_str,xm_stat[i].srccall);
      strcat(stat_str,"->");
      strcat(stat_str,xm_stat[i].destcall);
      strcat(stat_str," ***");
    }
  }
  if ((stat_len = strlen(stat_str)) < (COLS + 1)) {
    strncat(stat_str,
      "********************************************************************"
      "****************************************"
      "****************************************",
      COLS + 1 - stat_len);
  }
  win_stringout(stat_str,&xmonstatwin[screen],0);
  free(stat_str);
}

void sel_xmon(xmon_channel)
int xmon_channel;
{
  if ((xmon_channel == xmon_screen) && (act_mode == M_EXTMON)) return;
  real_screen(xmon_layout[xmon_channel]);
  act_mode = M_EXTMON;
  xmon_screen = xmon_channel;
  xmonstat_display(xmon_screen);
  statlin_update();
}

void sel_heardlist()
{
  struct heardlist *current;
  struct tm first_time;
  struct tm last_time;
  char heardstring[81];
  
  if (!mheard_flag) return;
  clear_window(&heardwin);
  real_screen(heard_layout);
    /* "   DL4YBG-15 > DB0BLO-10   94/08/08 12:00   94/08/07 12:00" */
  sprintf(heardstring,
       "     From    >    To         Last Heard       First Heard    \r");
      win_stringout(heardstring,&heardwin,0);
  sprintf(heardstring,
       "-------------------------------------------------------------\r");
      win_stringout(heardstring,&heardwin,0);
  if (heardlistroot != NULL) {
    current = heardlistroot;
    do {
      first_time = *localtime(&current->first_heard);
      last_time = *localtime(&current->last_heard);
      sprintf(heardstring,
              "   %-9.9s > %-9.9s"
              "   %2.2u/%2.2u/%2.2u %2.2u:%2.2u"
              "   %2.2u/%2.2u/%2.2u %2.2u:%2.2u\r",
              current->srccall,current->destcall,
              last_time.tm_year,last_time.tm_mon+1,last_time.tm_mday,
              last_time.tm_hour,last_time.tm_min,
              first_time.tm_year,first_time.tm_mon+1,first_time.tm_mday,
              first_time.tm_hour,first_time.tm_min);
      win_stringout(heardstring,&heardwin,0);
      current = current->next;
    } while (current != NULL);
  }
  act_mode = M_HEARD;
  statlin_update();
}

static void xmon_display_len(buffer,xmon_ch)
char *buffer;
int xmon_ch;
{
  int screen;
  
  screen = xm_stat[xmon_ch].screen;
  win_attrib(xm_stat[xmon_ch].attribute,&xmonwin[screen]);
  win_textout(buffer,&xmonwin[screen],0);
  win_attrib(att_normal,&xmonwin[screen]);
  write_xmonfile((*buffer)+1,screen,buffer+1);
}

int xm_newline(screen,str)
int screen;
char *str;
{
  int len;
  char tmpstr[MAXCOLS+1];
  
  len = get_line(tmpstr,&xmonprewin[screen]);
  win_charout(CR,&xmonprewin[screen],0);
  if (tmpstr[0] == ':') {
    strcpy(str,tmpstr+1);
    len--;
  }
  else
    strcpy(str,tmpstr);
  return(len);
}

void xm_charout(screen,ch)
int screen;
char ch;
{
  if (xmonprewin[screen].column < input_linelen-1) {
    win_charout_cntl(ch,&xmonprewin[screen]);
  }
  else {
    beep();
  }
}

void xm_charout_nobnd(screen,ch)
int screen;
char ch;
{
  win_charout_cntl(ch,&xmonprewin[screen]);
}

void heard_charout_cntl(ch)
char ch;
{
  win_charout_cntl(ch,&heardwin);
}

void xm_pre_display(screen,buffer,cr)
int screen;
char *buffer;
int cr;
{
  win_stringout(buffer,&xmonprewin[screen],0);
  if (cr) win_charout(CR,&xmonprewin[screen],0);
}

/* add new entry to heardlist */
static void add_mhlist(srcsum,destsum,srccall,destcall)
int srcsum;
int destsum;
char *srccall;
char *destcall;
{
  struct heardlist *current;
  struct heardlist *next;
  struct heardlist *previous;
  
  /* search if entry already existing */
  if ((current = heardlistroot) != NULL) {
    do {
      if ((srcsum == current->srcsum) && (destsum == current->destsum)) {
        if ((strcmp(srccall,current->srccall) == 0) &&
            (strcmp(destcall,current->destcall) == 0)) {
          /* qso found, update and exit */
          current->last_heard = time(NULL);
          
          previous = current->previous;
          next = current->next;
          
          /* last or only one entry, no sort needed, return */
          if (next == NULL)
            return;
            
          /* not last entry, put at end of list */          
          if (previous != NULL)
            previous->next = next;
          else
            heardlistroot = next;
          next->previous = previous;
          current->next = NULL;
          current->previous = heardlisttail;
          heardlisttail->next = current;
          heardlisttail = current;

          return;
        }
      }
      current = current->next;
    } while (current != NULL);
  }
  /* not found, must be created */
  if ((heardlistcount >= num_heardentries) && (heardlistroot != NULL)) {
    /* oldest entry must be deleted first */
    next = heardlistroot->next;
    free(heardlistroot->srccall);
    free(heardlistroot->destcall);
    free(heardlistroot);
    heardlistroot = next;
    next->previous = NULL;
    heardlistcount--;
  }
  current = (struct heardlist *)malloc(sizeof(struct heardlist));
  if (current == NULL) return;
  current->srcsum = srcsum;
  current->destsum = destsum;
  current->srccall = strdup(srccall);
  current->destcall = strdup(destcall);
  current->first_heard = time(NULL);
  current->last_heard = time(NULL);
  current->next = NULL;
  if (heardlistroot == NULL) {
    current->previous = NULL;
    heardlistroot = current;
    heardlisttail = current;
    heardlistcount++;
  }
  else {
    current->previous = heardlisttail;
    heardlisttail->next = current;
    heardlisttail = current;
    heardlistcount++;
  }
}

int analyse_mon_header(buffer)
char *buffer;
{
  char *ptr;
  char viacall[10][8];
  char i_nbr_ch;
  int i_nbr;
  char fm_str[] = "fm ";
  char to_str[] = "to ";
  char via_str[] = "via ";
  char ctl_str[] = "ctl ";
  char pid_str[] = "pid ";
  int srcsum;
  int destsum;
  int i;
  int j;
  int heardfrom;
  int end;
  int xmon_ch;
  int fini_flag;
  int mh_only;
  char frametype[10];
  char xmon_error2[50];
  char errbuf[51];
  char tmp_str[10];
  char chanstr[3];
  

#ifdef USE_IFACE
  scanmbea_valid = 0;
#endif  
  head_valid = 0;
  monheader_info.header_valid = 0;
  frame_ui = 0;
  frame_i = 0;
#ifdef BCAST
#ifdef USE_IFACE
  /* return, if broadcast-rx, extended monitor, heardlist,
     mailbeacon and UI-request not active */
  if ((!decbcast_flag) && (!xmon_flag) && 
      (!mheard_flag) && (!scanmbea_flag) &&
      (!accept_uireq_flag)) return(-1);
#else
  /* return, if broadcast-rx, extended monitor and heardlist not active */
  if ((!decbcast_flag) && (!xmon_flag) && (!mheard_flag)) return(-1);
#endif
#else
#ifdef USE_IFACE
  /* return, if extended monitor, heardlist, mailbeacon and
     UI-request not active */
  if ((!xmon_flag) && (!mheard_flag) &&
      (!scanmbea_flag) && (!accept_uireq_flag)) return(-1);
#else
  /* return, if extended monitor and heardlist not active */
  if ((!xmon_flag) && (!mheard_flag)) return(-1);
#endif
#endif
  fini_flag = 0;
  mh_only = 0;
  
  /* set QRG */
  chanstr[0] = '\0';
  if (buffer[1] == ':') {
    chanstr[0] = buffer[0];
    chanstr[1] = buffer[1];
    chanstr[2] = '\0';
  }
  get_qrg_from_chanstr(un_qrg,chanstr);

  /* fetch source callsign */
  srcsum = 0;
  if ((ptr = strstr(buffer,fm_str)) == NULL) {
    return(-1);
  }
  ptr += strlen(fm_str);
  i = 0;
  while ((i < 9) && (*ptr != ' ')) {
    if (*ptr == '\0') return(-1);
    srccall[i] = *ptr;
    srcsum += *ptr;
    ptr++;
    i++;
  }
  srccall[i] = '\0';
  strcpy(un_calls[0],srccall);

  /* fetch destination call sign */
  destsum = 0;
  if ((ptr = strstr(buffer,to_str)) == NULL) {
    return(-1);
  }
  ptr += strlen(to_str);
  i = 0;
  while ((i < 9) && (*ptr != ' ')) {
    if (*ptr == '\0') return(-1);
    destcall[i] = *ptr;
    destsum += *ptr;
    ptr++;
    i++;
  }
  destcall[i] = '\0';
  strcpy(un_calls[1],destcall);
  
  un_heardfrom = 0;
  un_callcount = 2;
  un_calls[2][0] = '\0';

  /* fetch digipeater list */
  vialist[0] = '\0';
  if ((ptr = strstr(buffer,via_str)) != NULL) {
    ptr += strlen(via_str);
    end = 0;
    j = 0;
    heardfrom = 0;
    while ((j < 8) && (!end)) {
      i = 0;
      while ((i < 9) && (*ptr != ' ') && (*ptr != '*')) {
        if (*ptr == '\0') return(-1);
        viacall[j][i] = *ptr;
        ptr++;
        i++;
      }
      viacall[j][i] = '\0';

      strcpy(un_calls[j+2],viacall[j]);

      switch (*ptr) {
      case ' ':
        j++;
        ptr++;
        if (strncmp(ptr,ctl_str,strlen(ctl_str)) == 0) end = 1;
        break;
      case '*':
        j++;
        heardfrom = j;
        ptr++;
        if (*ptr != ' ') return (-1);
        ptr++;
        if (strncmp(ptr,ctl_str,strlen(ctl_str)) == 0) end = 1;
        break;
      default:
        return(-1);
        break;
      }
    }

    un_callcount = 2 + j;

    if (heardfrom > 0) {
      un_heardfrom = 2 + heardfrom;
      strcpy(vialist,viacall[heardfrom-1]);
      if (heardfrom > 1) {
        for (i=heardfrom-1;i>0;i--) {
          strcat(vialist," ");
          strcat(vialist,viacall[i-1]);
        }
      }
    }
  }

  /* find ctl */
  if ((ptr = strstr(buffer,ctl_str)) == NULL) {
    return(-1);
  }
  ptr += strlen(ctl_str);
  switch (*ptr) {
  case 'I':
    frame_i = 1;
    ptr++;
    /* fetch number of i-frame */
    if (*ptr == '\0') return(-1);
    ptr++;
    i_nbr_ch = *ptr;
    if ((i_nbr_ch < '0') || (i_nbr_ch > '7')) return(-1);
    i_nbr = i_nbr_ch - '0';
    break;
  case 'S':
  case 'D':
    fini_flag = 1;
    sscanf(ptr,"%s",frametype);
    break;
  case 'U':
    switch (*(ptr+1)) {
    case 'I':
      frame_ui = 1;
      mh_only = 1;
      break;
    case 'A':
      fini_flag = 1;
      sscanf(ptr,"%s",frametype);
      break;
    }
    break;
  default:
    mh_only = 1;
    break;
  }
  
  pid = 0;
  if (frame_ui || frame_i) {
    /* find pid */
    if ((ptr = strstr(buffer,pid_str)) != NULL) {
      ptr += strlen(pid_str);
      strncpy(tmp_str,ptr,2);
      tmp_str[0] = tolower(tmp_str[0]);
      tmp_str[1] = tolower(tmp_str[1]);
      tmp_str[2] = '\0';
      sscanf(tmp_str,"%x",&pid);
    }
  }
  
  head_valid = 1;

#ifdef USE_IFACE
  if (frame_ui) check_mbeacon_head(srcsum,destsum,srccall,destcall);
#endif
  if (mheard_flag) add_mhlist(srcsum,destsum,srccall,destcall);
  if (mh_only) return(-1);
  if (!xmon_flag) return(-1);
  /* look if connection should be monitored */
  for (xmon_ch=0;xmon_ch<MAX_XMON;xmon_ch++) {
    if (xm_stat[xmon_ch].active) {
      if ((xm_stat[xmon_ch].srcsum == srcsum) && 
          (xm_stat[xmon_ch].destsum == destsum)) {
        if ((strcmp(xm_stat[xmon_ch].srccall,srccall) == 0) &&
            (strcmp(xm_stat[xmon_ch].destcall,destcall) == 0)) {
          break;
        }
      }
    }
  }
  if (xmon_ch == MAX_XMON) {
    if (!frame_i) return(-1);
    /* no xmon active, packet can be analysed */
    if (pid == 0xCC) {
      /* no analysis of tcpip-frames */
      return(-1);
    }
    monheader_info.header_valid = 1;
    strcpy(monheader_info.srccall,srccall);
    strcpy(monheader_info.destcall,destcall);
    monheader_info.srcsum = srcsum;
    monheader_info.destsum = destsum;
    monheader_info.last_i_nbr = i_nbr;
    return(-1);
  }
  /* check if DISC,SABM,UA received */
  if (fini_flag) {
    if (xm_stat[xmon_ch].monbox) {
#ifdef USE_IFACE
      /* cancel boxmonitor */
      cancel_monbox(xmon_ch,xm_stat[xmon_ch].monbox_channel);
#endif
    }
    else {
      /* inform about frame and restart */
      xm_stat[xmon_ch].next_i_nbr = -1;
      sprintf(xmon_error2,"\r%s %s\r",frametype,"received");
      errbuf[0] = (char)(strlen(xmon_error2) - 1);
      memcpy(errbuf+1,xmon_error2,strlen(xmon_error2));
      xmon_display_len(errbuf,xmon_ch);
    }
    return(-1);
  }
  /* check if it is first packet monitored */
  if (xm_stat[xmon_ch].next_i_nbr == -1) {
    i_nbr++;
    if (i_nbr > 7) i_nbr = 0;
    xm_stat[xmon_ch].next_i_nbr = i_nbr;
    xm_stat[xmon_ch].last_i_nbr = -1;
    xm_stat[xmon_ch].check_chksum = 0;
    return(xmon_ch);
  }
  /* check if counter is on the next turn */
  if (xm_stat[xmon_ch].last_i_nbr == i_nbr) {
    xm_stat[xmon_ch].check_chksum = 1;
    return(xmon_ch);
  }
  /* check if packet is next expected packet */
  if (xm_stat[xmon_ch].next_i_nbr == i_nbr) {
    xm_stat[xmon_ch].last_i_nbr = i_nbr;
    i_nbr++;
    if (i_nbr > 7) i_nbr = 0;
    xm_stat[xmon_ch].next_i_nbr = i_nbr;
    xm_stat[xmon_ch].check_chksum = 0;
    return(xmon_ch);
  }
  return(-1);
}

void write_mon_info(buffer,xmon_ch)
char *buffer;
int xmon_ch;
{
  int len;
  int chksum;
  int i;
  char xmon_error[] = "\rPackets lost\r";
  char errbuf[50];
  char buffer2[257];
  
  if (!xm_stat[xmon_ch].active) return;
  /* calculate checksum */
  chksum = 0;
  len = (*buffer) + 1;
  for (i=0;i<len;i++) {
    chksum += *(buffer + 1 + i);
  }
  /* perform checksum-test ? */
  if (xm_stat[xmon_ch].check_chksum) {
    if (chksum == xm_stat[xmon_ch].last_chksum)
      return; /* same packet, do nothing */
    /* some packets lost, start new or abort */
    if (xm_stat[xmon_ch].monbox) {
#ifdef USE_IFACE
      /* cancel boxmonitor */
      cancel_monbox(xmon_ch,xm_stat[xmon_ch].monbox_channel);
      return;
#endif
    }
    else {
      errbuf[0] = (char)(strlen(xmon_error) - 1);
      memcpy(errbuf+1,xmon_error,strlen(xmon_error));
      xmon_display_len(errbuf,xmon_ch);
    }
  }
  /* display packet on xmon-screen */
  if (xm_stat[xmon_ch].monbox) {
#ifdef USE_IFACE
    /* write to boxmonitor */
    if (xm_stat[xmon_ch].huffcod) {
      if (!decstathuf(buffer,buffer2))
        write_monbox(xm_stat[xmon_ch].monbox_channel,buffer2);
      else
        write_monbox(xm_stat[xmon_ch].monbox_channel,buffer);
    }
    else 
      write_monbox(xm_stat[xmon_ch].monbox_channel,buffer);
#endif  
  }
  else {
    if (xm_stat[xmon_ch].huffcod) {
      if (!decstathuf(buffer,buffer2))
        xmon_display_len(buffer2,xmon_ch);
      else
        xmon_display_len(buffer,xmon_ch);
    }
    else 
      xmon_display_len(buffer,xmon_ch);
  }
  xm_stat[xmon_ch].last_chksum = chksum;
  xm_stat[xmon_ch].last_received = time(NULL);
}

void analyse_mon_body(buffer)
char *buffer;
{
  int xmon_ch;
  char buffer2[257];
  
  if (!monheader_info.header_valid) return;
#ifdef USE_IFACE
  /* search body for box headers */
  xmon_ch = check_monbox(buffer);
  if (xmon_ch != -1) {
    if (xm_stat[xmon_ch].active) {
      xm_stat[xmon_ch].huffcod = 0;
      return;
    }
  }    
  /* now check if huffman coded and mailheader contained */
  if (!decstathuf(buffer,buffer2)) {
    xmon_ch = check_monbox(buffer2);
    if (xmon_ch != -1) {
      if (xm_stat[xmon_ch].active) {
        xm_stat[xmon_ch].huffcod = 1;
      }
    }
  }
#endif
  return;
}

static int gensum(string)
char *string;
{
  int sum;
  int i;
  
  sum = 0;
  for (i=0;i<strlen(string);i++) {
    sum += string[i];
  }
  return(sum);
}

void check_xmon_timeout()
{
  time_t cur_time;
  int i;
  
  cur_time = time(NULL);
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if ((cur_time - xm_stat[i].last_received) > XMON_TIMEOUT) {
        if (xm_stat[i].monbox) {
#ifdef USE_IFACE
          /* cancel boxmonitor */
          cancel_monbox(i,xm_stat[i].monbox_channel);
#endif
        }
      } 
    }
  }
}

#ifdef USE_IFACE

void monbox_close()
{
  int i;
  
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if (xm_stat[i].monbox) {
        /* cancel boxmonitor */
        cancel_monbox(i,xm_stat[i].monbox_channel);
      }
    } 
  }
}

#endif

#ifdef USE_IFACE

int open_monbox(monbox_channel,chksum)
int monbox_channel;
int chksum;
{
  int i;
  int xmon_ch;
  int i_nbr;

  xmon_ch = -1;
  for (i=0;i<MAX_XMON;i++) {
    if (!xm_stat[i].active) {
      xmon_ch = i;
      break;
    }
  }
  if (xmon_ch == -1) {
    return(-1);
  }

  xm_stat[xmon_ch].active = 1;
/*  xm_stat[xmon_ch].huffcod = 0; */
  xm_stat[xmon_ch].monbox = 1;
  xm_stat[xmon_ch].monbox_channel = monbox_channel;
  xm_stat[xmon_ch].srcsum = monheader_info.srcsum;
  strcpy(xm_stat[xmon_ch].srccall,monheader_info.srccall);
  xm_stat[xmon_ch].destsum = monheader_info.destsum;
  strcpy(xm_stat[xmon_ch].destcall,monheader_info.destcall);
  xm_stat[xmon_ch].last_i_nbr = monheader_info.last_i_nbr;
  i_nbr = monheader_info.last_i_nbr + 1;
  if (i_nbr > 7) i_nbr = 0;
  xm_stat[xmon_ch].next_i_nbr = i_nbr;
  xm_stat[xmon_ch].check_chksum = 1;
  xm_stat[xmon_ch].last_chksum = chksum;
  xm_stat[xmon_ch].screen = -1;
  xm_stat[xmon_ch].last_received = time(NULL);
  
  return(xmon_ch);
}

void close_monbox(xmon_ch)
int xmon_ch;
{
  xm_stat[xmon_ch].active = 0;
}

#endif

void cmd_extmon(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int screen;
  char call1[MAXCOLS+1];
  char call2[MAXCOLS+1];
  char call3[MAXCOLS+1];
  char call4[MAXCOLS+1];
  int res;
  int i;
  int j;
  int xmon_ch1;
  int xmon_ch2;
  int sum1;
  int sum2;
  int sum3;
  int sum4;
  int diffcall;
  char temptext[80];
  int found;
  int active;

  if (!xmon_flag) {
    cmd_display(mode,channel,xmon_xmondis_txt,1);
    return;
  }
  diffcall = 0;
  if ((mode == M_EXTMON) || (par1)) {
    res = sscanf(str,"%s %s %s %s",call1,call2,call3,call4);
    if ((res != 2) && (res != 4)) {
      cmd_display(mode,channel,xmon_fewpar_txt,1);
      return;
    }
    if (par1) {
      found = 0;
      for (j=0;((j<XMON_SCREENS) && !found);j++) {
        active = 0;
        for (i=0;((i<MAX_XMON) && !active);i++) {
          if (xm_stat[i].active) {
            if (xm_stat[i].screen == j) {
              active = 1;
            }
          }
        }
        if (!active) found = 1;
        screen = j;
      }
      if (!found) {
        cmd_display(mode,channel,xmon_nofree_txt,1);
        return;
      }
    }
    else {
      screen = xmon_screen;
    }
    if (res == 4) diffcall = 1;
  }
  else {
    res = sscanf(str,"%d %s %s %s %s",&screen,call1,call2,call3,call4);
    if ((res != 3) && (res != 5)) {
      cmd_display(mode,channel,xmon_fewpar_txt,1);
      return;
    }
    if (res == 5) diffcall = 1;
  }
  if ((screen < 0) || (screen >= XMON_SCREENS)) {
    cmd_display(mode,channel,xmon_illscr_txt,1);
    return;
  }
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if (xm_stat[i].screen == screen) {
        cmd_display(mode,channel,xmon_scruse_txt,1);
        return;
      }
    }
  }
  if ((strlen(call1) > 9) || (strlen(call2) > 9)) {
    cmd_display(mode,channel,xmon_illcall_txt,1);
    return;
  }
  if (diffcall) {
    if ((strlen(call3) > 9) || (strlen(call4) > 9)) {
      cmd_display(mode,channel,xmon_illcall_txt,1);
      return;
    }
  }
  xmon_ch1 = -1;
  xmon_ch2 = -1;
  for (i=0;i<MAX_XMON;i++) {
    if (!xm_stat[i].active) {
      if (xmon_ch1 == -1) {
        xmon_ch1 = i;
      }
      else {
        if (xmon_ch2 == -1) {
          xmon_ch2 = i;
        }
      }
    }
  }
  if ((xmon_ch1 == -1) || (xmon_ch2 == -1)) {
    cmd_display(mode,channel,xmon_noxmon_txt,1);
    return;
  }
  for (i=0;i<strlen(call1);i++) {
    call1[i] = toupper(call1[i]);
  }
  sum1 = gensum(call1);
  for (i=0;i<strlen(call2);i++) {
    call2[i] = toupper(call2[i]);
  }
  sum2 = gensum(call2);
  if (diffcall) {
    for (i=0;i<strlen(call3);i++) {
      call3[i] = toupper(call3[i]);
    }
    sum3 = gensum(call3);
    for (i=0;i<strlen(call4);i++) {
      call4[i] = toupper(call4[i]);
    }
    sum4 = gensum(call4);
  }

  /* check if connection is not already monitored */
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if ((xm_stat[i].srcsum == sum1) &&
          (xm_stat[i].destsum == sum2)) {
        if ((strcmp(xm_stat[i].srccall,call1) == 0) &&
            (strcmp(xm_stat[i].destcall,call2) == 0)) {
          cmd_display(mode,channel,xmon_conact_txt,1);
          return;
        }
      }
      if (diffcall) {
        if ((xm_stat[i].srcsum == sum3) &&
            (xm_stat[i].destsum == sum4)) {
          if ((strcmp(xm_stat[i].srccall,call3) == 0) &&
              (strcmp(xm_stat[i].destcall,call4) == 0)) {
            cmd_display(mode,channel,xmon_conact_txt,1);
            return;
          }
        }
      }
      else {
        if ((xm_stat[i].srcsum == sum2) &&
            (xm_stat[i].destsum == sum1)) {
          if ((strcmp(xm_stat[i].srccall,call2) == 0) &&
              (strcmp(xm_stat[i].destcall,call1) == 0)) {
            cmd_display(mode,channel,xmon_conact_txt,1);
            return;
          }
        }
      }
    }
  }

  xm_stat[xmon_ch1].active = 1;
  xm_stat[xmon_ch1].huffcod = 0;
  xm_stat[xmon_ch1].monbox = 0;
  xm_stat[xmon_ch1].srcsum = sum1;
  strcpy(xm_stat[xmon_ch1].srccall,call1);
  xm_stat[xmon_ch1].destsum = sum2;
  strcpy(xm_stat[xmon_ch1].destcall,call2);
  xm_stat[xmon_ch1].next_i_nbr = -1;
  xm_stat[xmon_ch1].last_i_nbr = -1;
  xm_stat[xmon_ch1].check_chksum = 0;
  xm_stat[xmon_ch1].screen = screen;
  xm_stat[xmon_ch1].attribute = att_normal;
  xm_stat[xmon_ch1].last_received = time(NULL);
  
  xm_stat[xmon_ch2].active = 1;
  xm_stat[xmon_ch2].huffcod = 0;
  xm_stat[xmon_ch2].monbox = 0;
  if (diffcall) {
    xm_stat[xmon_ch2].srcsum = sum3;
    strcpy(xm_stat[xmon_ch2].srccall,call3);
    xm_stat[xmon_ch2].destsum = sum4;
    strcpy(xm_stat[xmon_ch2].destcall,call4);
  }
  else {
    xm_stat[xmon_ch2].srcsum = sum2;
    strcpy(xm_stat[xmon_ch2].srccall,call2);
    xm_stat[xmon_ch2].destsum = sum1;
    strcpy(xm_stat[xmon_ch2].destcall,call1);
  }
  xm_stat[xmon_ch2].next_i_nbr = -1;
  xm_stat[xmon_ch2].last_i_nbr = -1;
  xm_stat[xmon_ch2].check_chksum = 0;
  xm_stat[xmon_ch2].screen = screen;
  xm_stat[xmon_ch2].attribute = att_monitor;
  xm_stat[xmon_ch2].last_received = time(NULL);

  if (par1) {
    sprintf(temptext,"%s%d%s",xmon_disp1_txt,screen,xmon_disp2_txt);
    cmd_display(mode,channel,temptext,1);
  }
  else {
    cmd_display(mode,channel,ok_text,1);
  }
  xmonstat_display(screen);
}
 
void cmd_endextmon(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int screen;
  int res;
  int i;
  int found;

  if (mode == M_EXTMON) {
    screen = xmon_screen;
  }
  else {
    res = sscanf(str,"%d",&screen);
    if (res != 1) {
      cmd_display(mode,channel,xmon_fewpar_txt,1);
      return;
    }
  }
  if ((screen < 0) || (screen >= XMON_SCREENS)) {
    cmd_display(mode,channel,xmon_illscr_txt,1);
    return;
  }
  found = 0;
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if (xm_stat[i].screen == screen) {
        found = 1;
        xm_stat[i].active = 0;
      }
    }
  }
  if (found) {
    cmd_display(mode,channel,xmon_close_txt,1);
  }
  else {
    cmd_display(mode,channel,xmon_notact_txt,1);
  }
  xmonstat_display(screen);
  statlin_update();
}

void cmd_extcomp(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int screen;
  int i;
  int found;
  int comp;

  if (mode != M_EXTMON) {
    cmd_display(mode,channel,xmon_ill_txt,1);
    return;
  }
  screen = xmon_screen;
  if ((screen < 0) || (screen >= XMON_SCREENS)) {
    cmd_display(mode,channel,xmon_illscr_txt,1);
    return;
  }
  if (len != 0) {
    for (i = 0; i < len; i++) str[i] = tolower(str[i]);

    comp = -1;
    
    if (len >= 2)
      if (strncmp(str,"on",2) == 0) comp = 1;

    if (len >= 3)
      if (strncmp(str,"off",3) == 0) comp = 0;

    if (comp == -1) {
      cmd_display(mode,channel,"INVALID VALUE",1);
      return;
    }
  }
  found = 0;
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if (xm_stat[i].screen == screen) {
        found = 1;
        if (len == 0) {
          if (xm_stat[i].huffcod == 0)
            cmd_display(mode,channel,"OFF",1);
          else
            cmd_display(mode,channel,"ON",1);
          return;
        }
        else xm_stat[i].huffcod = comp;
      }
    }
  }
  if (found) {
    cmd_display(mode,channel,ok_text,1);
  }
  else {
    cmd_display(mode,channel,xmon_notact_txt,1);
  }
  statlin_update();
}

/* returns 1 if extcomp active on screen */
int extcomp_active(screen)
int screen;
{
  int i;
 
  for (i=0;i<MAX_XMON;i++) {
    if (xm_stat[i].active) {
      if (xm_stat[i].screen == screen) {
        if (xm_stat[i].huffcod == 1)
        return(1);
      }
    }
  }
  return(0);
}

#ifdef USE_IFACE
void fill_xmon_call(channel,srccall,destcall)
int channel;
char *srccall;
char *destcall;
{
  if (xm_stat[channel].active) {
    strcpy(srccall,xm_stat[channel].srccall);
    strcpy(destcall,xm_stat[channel].destcall);
  }
  else {
    *srccall = '\0';
    *destcall = '\0';
  }
}

void fill_moni_call(srccall,destcall)
char *srccall;
char *destcall;
{
  if (monheader_info.header_valid) {
    strcpy(srccall,monheader_info.srccall);
    strcpy(destcall,monheader_info.destcall);
  }
  else {
    *srccall = '\0';
    *destcall = '\0';
  }
}
#endif

static void fill_call(call,shift_call)
char *call;
char *shift_call;
{
  char *callpos;
  char ch;
  int i;
  
  callpos = call;
  for (i=0;i<6;i++) {
    ch = ((*(shift_call + i)) >> 1) & 0x7f;
    if (ch != ' ') {
      *callpos = ch;
      callpos++;
    }
  }
  ch = (((*(shift_call + 6)) >> 1) & 0x0f);
  if (ch != 0) {
    *callpos = '-';
    callpos++;
    if (ch > 9) {
      *callpos = '1';
      callpos++;
      ch -= 10;
    }
    *callpos = ch + 0x30;
    callpos++;
  }
  *callpos = '\0';
}

int display_layer3(buffer)
char *buffer;
{
  char *bufcur;
  int len;
  char line[160];
  char ident[7];
  char call1[10];
  char call2[10];
  char call3[10];
  char call4[10];
  char flags[20];
  int end;
  int l3_opcode;
  
  bufcur = buffer + 1;
  len = (int)(*buffer) + 1;
  ident[6] = '\0';

  switch (*bufcur) {
  case 0xFF:
    bufcur++; /* skip 0xFF */
    len--;
    if (len < 6) return(1);
    strncpy(ident,bufcur,6); /* copy ident */
    ident[6] = '\0';
    len -= 6;
    bufcur += 6;
    sprintf(line,"Broadcast from %s",ident);
    moni_display2(line,0);
    if (len == 0) {
      moni_display2("No routing-table entries",0);
    }
    else {
      end = 0;
      while (!end) {
        if (len < 21) {
          moni_display2("Routing-table entries corrupt",0);
          return(1);
        }
        fill_call(call1,bufcur); /* copy call */
        len -= 7;
        bufcur += 7;
        strncpy(ident,bufcur,6); /* copy ident */
        ident[6] = '\0';
        len -= 6;
        bufcur += 6;
        fill_call(call2,bufcur); /* copy call */
        len -= 7;
        bufcur += 7;
        sprintf(line,"%s(%s) via %s quality: %u",call1,ident,call2,*bufcur);
        len--;
        bufcur++;
        moni_display2(line,0);
        if (len == 0) end = 1;
      }
    }
    break;
  default:
    if (len < L3_HEADERLEN) return(1);
    fill_call(call1,bufcur + L3_SRCCALL); /* copy call */
    fill_call(call2,bufcur + L3_DESTCALL); /* copy call */
    l3_opcode = *(bufcur + L3_OPCODE); /* opcode position */
    flags[0] = '\0';
    if (l3_opcode & 0x80)
      strcat(flags,", CHK");
    if (l3_opcode & 0x40)
      strcat(flags,", NAK");
    if (l3_opcode & 0x20)
      strcat(flags,", MOR");
    switch (l3_opcode & 0x0f) {
    case L3OP_CONREQ:
      if (len < L3_CONREQLEN) return(1);
      fill_call(call3,bufcur + L3_CALL3); /* copy call */
      fill_call(call4,bufcur + L3_CALL4); /* copy call */
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u, WS:%u%s, ConReq %s at %s)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),
        *(bufcur+L3_WINSIZE),flags,call3,call4);
      moni_display2(line,1);
      break;
    case L3OP_CONACK:
      if (len < L3_CONACKLEN) return(1);
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u, C-I:%u, C-ID:%u, WS:%u%s, ConAck)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),
        *(bufcur+L3_CI_2),*(bufcur+L3_CID_2),*(bufcur+L3_WINSIZE),flags);
      moni_display2(line,1);
      break;
    case L3OP_DISCREQ:
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u%s, DiscReq)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),flags);
      moni_display2(line,1);
      break;
    case L3OP_DISCACK:
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u%s, DiscAck)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),flags);
      moni_display2(line,1);
      break;
    case L3OP_INFO:
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u, TXNO:%u, RXNO:%u%s, Info)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),
        *(bufcur+L3_TXNO),*(bufcur+L3_RXNO),flags);
      moni_display2(line,1);
      if (len > L3_HEADERLEN)
        moni_display_len2(bufcur+L3_HEADERLEN,len-L3_HEADERLEN);
      break;
    case L3OP_INFOACK:
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u, RXNO:%u%s, InfoAck)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),
        *(bufcur+L3_RXNO),flags);
      moni_display2(line,1);
      break;
    default:
      sprintf(line,
        "(%s>%s  TTL:%u, C-I:%u, C-ID:%u, ??:%u, ??:%u%s, IllOpcode:%x)",
        call1,call2,*(bufcur+L3_TTL),*(bufcur+L3_CI),*(bufcur+L3_CID),
        *(bufcur+L3_CI_2),*(bufcur+L3_CID_2),flags,*(bufcur+L3_OPCODE));
      moni_display2(line,1);
      if (len > L3_HEADERLEN)
        moni_display_len2(bufcur+L3_HEADERLEN,len-L3_HEADERLEN);
      break;
    }
    break;
  }
  return(0);
}
