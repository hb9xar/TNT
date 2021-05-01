/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for display (display.c)
   created: Mark Wahl DL4YBG 93/08/01
   updated: Mark Wahl DL4YBG 97/02/02
*/

#include "tnt.h"
#ifndef DPBOXT
#include "xmon.h"
#endif


extern int insert_active();
#ifndef DPBOXT
extern int xconnect_active(int channel);
extern int extcomp_active(int screen);
#endif
extern void show_rxfile(int channel,char *str);
extern void show_txfile(int channel,char *str);
extern void win_stringout(char *str,struct window *win, int conv);
extern void real_screen(struct real_layout *layout);
#ifndef DPBOXT
extern void strip_call_log(char *call,int channel);
#ifdef USE_SOCKET
extern void sock_status_out(int channel,char *str);
#endif
extern void beep();
extern void write_file(int channel,int len,int flag,char *str);
extern void write_file_abin(int channel,int *len,int flag,char *str);
#endif
extern void win_attrib(char ch,struct window *win);
extern void win_textout_len(char *str,int len,struct window *win,int conv);
extern void win_charout_cntl(char ch,struct window *win);
#ifndef DPBOXT
extern void test_sysresponse(int channel,char *buffer,int len);
extern int act_abin_rx(int channel);
extern int wait_abin_rx(int channel);
#ifdef USE_IFACE
extern int box_active(int channel);
extern int iface_active(int channel);
extern void write_iface(int channel,int len,char *str);
#endif
extern int write_pty(int channel,int len,char *str);
extern void insert_cr_tx(int channel);
extern int shell_active(int channel);
#ifdef USE_IFACE
extern void boxcut(int channel,char *buffer,int len);
extern void boxcut_rest(int channel,char *buffer,int len);
extern void boxcut_nocr(int channel,char *buffer,int len);
#endif
extern void win_textout(char *str,struct window *win,int conv);
extern void connect_update(int channel,char *buffer,int len);
extern void scan_login_received(int channel,char *buffer,int len);
extern void scan_pw_request(int channel,char *buffer,int len);
extern int remote_input(int channel,char *buffer,int len);
#endif
extern void win_charout(char ch,struct window *win,int conv);
#ifdef USE_SOCKET
extern void out_cmd_socket(int channel,char *buffer);
#endif
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
#ifndef DPBOXT
extern void xm_pre_display(int screen,char *buffer,int cr);
extern void send_tntresponse(int channel,int mode,char *str,int cr);
#endif
#ifdef USE_IFACE
extern void mb_pre_display(char *buffer,int cr);
#endif
#ifndef DPBOXT
extern void write_monfile(int len,char *str,int append_cr);
extern int analyse_mon_header(char *buffer);
extern void write_mon_info(char *buffer,int xmon_ch);
extern void analyse_mon_body(char *buffer);
extern int display_layer3(char *buffer);
#ifdef BCAST
extern int analyse_bcast_body(char *buffer);
#endif
#ifdef USE_IFACE
extern void check_mbeacon(char *buffer);
extern void check_uireq(char *buffer);
#endif
#endif
extern int get_line(char *str,struct window *win);
extern int wordwrap(char *str,struct window *win);
extern void window_init();
extern int open_window(int lines,struct window *win,int bot);
#ifdef USE_IFACE
extern void reinit_mb();
#endif
extern void reinit_blist();
#ifndef DPBOXT
extern void reinit_xmon();
extern void reinit_mheard();
#endif
extern int close_window(struct window *win);
extern void window_exit();
#ifndef DPBOXT
extern int act_yapp_rx(int channel);
extern int act_yapp_tx(int channel);
extern void write_file_yapp(int channel, char *str, int len);
extern void ana_response_yapp(int channel,char *buffer,int len);
extern int check_autoyapp(int channel,char *buffer,int len);
#endif


#ifdef DPBOXT
extern int act_channel;
extern int act_mode;
extern struct channel_stat *ch_stat;
extern char signon[];
extern char add_signon[];
extern int is_root;
extern int chan_flag;
extern int chan_val1;
extern char att_normal;
extern char att_statline;
extern char att_monitor;
extern char att_cstatline;
extern char att_controlchar; 
extern char att_remote;
extern int input_linelen;
extern char tnt_dir[];
extern int LINES,COLS;
extern int bl_mode;
extern int tabexp_flag;
extern int ptyecho_flag;

static struct window helpwin;
struct window statlin;

static struct real_layout help_layout[LAYOUTPARTS];
extern struct real_layout (*boxlist_layout)[LAYOUTPARTS];
extern struct real_layout mb_layout[LAYOUTPARTS];
extern struct real_layout mbboxlist_layout[LAYOUTPARTS];
#else /* DPBOXT */

extern int tnc_channels;
extern int r_channels;
extern int act_channel;
extern int act_mode;
extern int xmon_screen;
extern struct channel_stat *ch_stat;
extern char signon[];
extern char add_signon[];
extern int is_root;
extern int chan_flag;
extern int chan_val1;
extern int resync;
extern char att_normal;
extern char att_statline;
extern char att_monitor;
extern char att_cstatline;
extern char att_controlchar; 
extern char att_remote;
extern int lines_command;
extern int lines_monitor;
extern int lines_input;
extern int lines_output;
extern int lines_moncon;
extern int lines_r_input;
extern int lines_r_output;
extern int input_linelen;
extern char rem_tnt_str[];
extern char rem_newlin_str[];
extern char tnt_dir[];
extern int infobell_flag;
extern int cbell_flag;
extern int layer3_flag;
extern int tabexp_flag;
extern int ptyecho_flag;
extern int LINES,COLS;
extern int tnc_busy;
extern struct tx_file tx_file_com;
#ifdef USE_IFACE
extern int bl_mode;
#endif

int scr_divide;
int altstat;
static int xmon_ch;
static int layer3_frame;
static struct window *prewin;
static struct window *statwin;
static struct window *textwin;
static struct window cmdwin;
static struct window monwin;
static struct window helpwin;
struct window statlin;

struct real_layout (*layout)[LAYOUTPARTS];
static struct real_layout cmd_layout[LAYOUTPARTS];
static struct real_layout mon_layout[LAYOUTPARTS];
static struct real_layout help_layout[LAYOUTPARTS];
extern struct real_layout xmon_layout[XMON_SCREENS][LAYOUTPARTS];
extern struct real_layout (*boxlist_layout)[LAYOUTPARTS];
extern struct real_layout heard_layout[LAYOUTPARTS];
#ifdef USE_IFACE
extern struct real_layout mb_layout[LAYOUTPARTS];
extern struct real_layout mbboxlist_layout[LAYOUTPARTS];
extern int scanmbea_valid;
#endif /* USE_IFACE */
#endif /* DPBOXT */

static int old_mode;
static int help_available;
char tnt_help_file[80];

#ifndef DPBOXT
static char *ax_state[] = {
  "************",
  " LINK SETUP ",
  " FRAME REJ *",
  " DISC. LINK ",
  " INFO XFER *",
  " REJECT SND ",
  " WAIT ACK **",
  " LOCAL BUSY ",
  " REM. BUSY *",
  " BOTH BUSY *",
  " LOCAL BUSY ",
  " REM. BUSY *",
  " BOTH BUSY *",
  " LOCAL BUSY ",
  " REM. BUSY *",
  " BOTH BUSY *"
};

static char *ax_state_alt[] = {
  " discon  ",
  " SET to  ",
  " FRMR fm ",
  " DRQ to  ",
  " IXF wid ",
  " REJ to  ",
  " WACK fm ",
  " LOCBUSY ",
  " BUSY fm ",
  " BUSY    ",
  " LOCBUSY ",
  " BUSY fm ",
  " BUSY    ",
  " LOCBUSY  ",
  " BUSY fm ",
  " BUSY    "
};
#endif
  
void statlin_update()
{
  int i;
  char c;
  char tmp;
  char *tmpstr;
  int channel;
  char ch_str[3];
  int ch_offset;
  
  tmpstr = (char *)malloc(COLS+2);
  switch (act_mode) {
#ifdef DPBOXT
  case M_BOXLIST:
    strcpy(tmpstr,"\015** BOXLIST *");
    tmp = '*';
    break;  
  case M_HELP:
    strcpy(tmpstr,"\015*** HELP ***");
    tmp = '*';
    break;  
  case M_MAILBOX:
    strcpy(tmpstr,"\015** MAILBOX *");
    if (insert_active())
      tmp = 'I';
    else 
      tmp = '*';
    break;  
  }
  strncat(tmpstr,&tmp,1);
  if (act_mode == M_MAILBOX) {
    strcat(tmpstr,"**************");
  }
  else if ((act_mode == M_BOXLIST) && (bl_mode == M_MAILBOX)) {
    strcat(tmpstr,"* MAILBOX ****");
  }
  show_rxfile(act_channel,tmpstr);
  tmp = '*';
  strncat(tmpstr,&tmp,1);
  show_txfile(act_channel,tmpstr);
  for (i=strlen(tmpstr);i<COLS+1;i++) {
    tmpstr[i] = '*';
  }
  tmpstr[COLS+1] = '\0';
#else
  case M_CONNECT:
    if (ch_stat[act_channel].huffcod)
      strcpy(tmpstr,"\015** CONN(H) *");
    else
      strcpy(tmpstr,"\015** CONNECT *");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else if (ch_stat[act_channel].pause_flag)
      tmp = 'P';
    else if (xconnect_active(act_channel))
      tmp = 'X';
    else if (insert_active())
      tmp = 'I';
    else
      tmp = '*';
    break;
      strncat(tmpstr,&tmp,1);
  case M_COMMAND:
    strcpy(tmpstr,"\015** COMMAND *");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else if (insert_active())
      tmp = 'I';
    else 
      tmp = '*';
    break;
  case M_MONITOR:
    strcpy(tmpstr,"\015** MONITOR *");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else if (ch_stat[0].pause_flag)
      tmp = 'P';
    else 
      tmp = '*';
    break;  
  case M_EXTMON:
    if (extcomp_active(xmon_screen))
      strcpy(tmpstr,"\015** EXTM(H) *");
    else
      strcpy(tmpstr,"\015** EXTMONI *");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else if (insert_active())
      tmp = 'I';
    else 
      tmp = '*';
    break;  
  case M_BOXLIST:
    strcpy(tmpstr,"\015** BOXLIST *");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else 
      tmp = '*';
    break;  
  case M_HEARD:
    strcpy(tmpstr,"\015*** HEARD **");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else 
      tmp = '*';
    break;  
  case M_HELP:
    strcpy(tmpstr,"\015*** HELP ***");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else 
      tmp = '*';
    break;  
#ifdef USE_IFACE
  case M_MAILBOX:
    strcpy(tmpstr,"\015** MAILBOX *");
    if (resync)
      tmp = 'S';
    else if (tnc_busy)
      tmp = 'B';
    else if (insert_active())
      tmp = 'I';
    else 
      tmp = '*';
    break;  
#endif
  }
  strncat(tmpstr,&tmp,1);
#ifdef USE_IFACE
  if ((act_mode == M_MAILBOX) && (!chan_flag)) {
    strcat(tmpstr,"**************");
  }
  else if ((act_mode == M_BOXLIST) && (bl_mode == M_MAILBOX) && (!chan_flag)) {
    strcat(tmpstr,"* MAILBOX ****");
  }
  else {
    strcat(tmpstr,"* CHANNEL ");
    if (chan_flag == 1) {
      strcat(tmpstr,"??");
    }
    else if (chan_flag == 2) {
      sprintf(ch_str,"%01d?",chan_val1);
      strcat(tmpstr,ch_str);
    }
    else {
      if (act_mode == M_EXTMON) {
        channel = xmon_screen;
      }
      else {
        channel = act_channel;
      }
      sprintf(ch_str,"%02d",channel);
      strcat(tmpstr,ch_str);
    }
    strcat(tmpstr," *");
  }
#else
  strcat(tmpstr,"* CHANNEL ");
  if (chan_flag) {
    strcat(tmpstr,"??");
  }
  else {
    if (act_mode == M_EXTMON) {
      channel = xmon_screen;
    }
    else {
      channel = act_channel;
    }
    sprintf(ch_str,"%0.2d",channel);
    strcat(tmpstr,ch_str);
  }
  strcat(tmpstr," *");
#endif
  show_rxfile(act_channel,tmpstr);
  tmp = '*';
  strncat(tmpstr,&tmp,1);
  show_txfile(act_channel,tmpstr);
  strcat(tmpstr,"* ");
  ch_offset = (act_channel / 10) * 10;
  for (i = (0+ch_offset); i < (10+ch_offset); i++) {
    if (i >= tnc_channels) {
      tmp = SPACE;
    }
    else {
      if ((i == act_channel) && (act_mode == M_CONNECT)) {
        ch_stat[i].not_disp = 0;
      }
      if (ch_stat[i].not_disp) {
        c = '0';
        tmp = i+c-ch_offset;
      }
      else if (ch_stat[i].state) {
        tmp = '+';
      }
      else {
        tmp = SPACE;
      }
    }
    strncat(tmpstr,&tmp,1);
  }
  strcat(tmpstr," **");
  for (i=strlen(tmpstr);i<COLS+1;i++) {
    tmpstr[i] = '*';
  }
  tmpstr[COLS+1] = '\0';
#endif
  win_stringout(tmpstr,&statlin,0);
  free(tmpstr);
}

#ifndef DPBOXT
void newdata(channel)
int channel;
{
  ch_stat[channel].not_disp = 1;
  statlin_update();
}

void sel_monitor()
{
  if (act_mode == M_MONITOR) {
    switch (old_mode) {
    case M_COMMAND:
      real_screen(cmd_layout);
      break;
    case M_CONNECT:
      real_screen(layout[act_channel]);
      break;
    case M_EXTMON:
      real_screen(xmon_layout[xmon_screen]);
      break;
    case M_HEARD:
      real_screen(heard_layout);
      break;
    case M_HELP:
      real_screen(help_layout);
      break;
    case M_BOXLIST:
#ifdef USE_IFACE
      if (bl_mode == M_MAILBOX) {
        real_screen(mbboxlist_layout);
      }
      else {
        real_screen(boxlist_layout[act_channel]);
      }
#else
      real_screen(boxlist_layout[act_channel]);
#endif
      break;
#ifdef USE_IFACE
    case M_MAILBOX:
      real_screen(mb_layout);
      break;
#endif
    }
    act_mode = old_mode;
  }
  else {
    old_mode = act_mode;
    real_screen(mon_layout);
    act_mode = M_MONITOR;
  }
  statlin_update();
}

void sel_command()
{
  if (act_mode == M_COMMAND) return;
  real_screen(cmd_layout);
  act_mode = M_COMMAND;
  statlin_update();
}

void sel_connect(channel)
int channel;
{
  if ((channel == act_channel) && (act_mode == M_CONNECT)) return;  
  real_screen(layout[channel]);
  act_mode = M_CONNECT;
  act_channel = channel;
  statlin_update();
}

void stat_display_org(channel)
int channel;
{
  int stat_len;
  int snd_frms;
  char tmp;
  char *stat_str;
  
  stat_str = (char *)malloc(COLS+2);
  strcpy(stat_str,"");
  strcat(stat_str,"\015**");
  if (channel == 0) {
    /* for max length 160 chars */
    strncat(stat_str,"**************************************"
                     "****************************************"
                     "****************************************"
                     "****************************************",COLS - 2);
    stat_str[COLS+1] = '\0'; /* shorten string */
    memcpy(&stat_str[(COLS/2 - 9) + 1]," UNPROTO CHANNEL ",17);
  }
  else {
    strcat(stat_str,ax_state[ch_stat[channel].state & 0x0f]);
    strcat(stat_str,"**");
    if (ch_stat[channel].snd_frms) {
      strcat(stat_str," S=");
      snd_frms = ch_stat[channel].snd_frms + ch_stat[channel].snd_queue_frms;
      if (snd_frms > 9) {
        tmp = snd_frms/10+'0';
        strncat(stat_str,&tmp,1);
        tmp = snd_frms%10+'0';
        strncat(stat_str,&tmp,1);
        tmp = SPACE;
        strncat(stat_str,&tmp,1);
      }
      else {
        tmp = snd_frms+'0';
        strncat(stat_str,&tmp,1);
        strcat(stat_str," *");
      }
    }
    else strcat(stat_str,"******");
    strcat(stat_str,"**");
    if (ch_stat[channel].unacked) {
      strcat(stat_str," U=");
      tmp = ch_stat[channel].unacked+'0';
      strncat(stat_str,&tmp,1);
      tmp = SPACE;
      strncat(stat_str,&tmp,1);
    }
    else strcat(stat_str,"*****");
    strcat(stat_str,"**");
    if (ch_stat[channel].tries) {
      strcat(stat_str," R=");
      if (ch_stat[channel].tries > 9) {
        tmp = ch_stat[channel].tries/10+'0';
        strncat(stat_str,&tmp,1);
        tmp = ch_stat[channel].tries%10+'0';
        strncat(stat_str,&tmp,1);
        tmp = SPACE;
        strncat(stat_str,&tmp,1);
      }
      else {
        tmp = ch_stat[channel].tries+'0';
        strncat(stat_str,&tmp,1);
        strcat(stat_str," *");
      }
    }
    else strcat(stat_str,"******");
    strcat(stat_str,"** ");

    if (ch_stat[channel].disp_call[0] == '\0')   
      strncat(stat_str,ch_stat[channel].call,40);
    else
      strncat(stat_str,ch_stat[channel].disp_call,40);
    if ((stat_len = strlen(stat_str)) < (COLS + 1)) {
      strncat(stat_str,
              " ***************************************"
              "****************************************"
              "****************************************",
              COLS + 1 - stat_len);
    }
  }
  win_stringout(stat_str,&statwin[channel],0);
  free(stat_str);
}

void stat_display_alt(channel)
int channel;
{
  int stat_len;
  int snd_frms;
  char tmpstring[256];
  char tmp;
  char *stat_str;
  
  stat_str = (char *)malloc(COLS+2);
  strcpy(stat_str,"");
  strcat(stat_str,"\015");
  if (channel == 0) {
    /* for max length 160 chars */
    strncat(stat_str,"****************************************"
                     "****************************************"
                     "****************************************"
                     "****************************************",COLS);
    stat_str[COLS+1] = '\0'; /* shorten string */
    memcpy(&stat_str[(COLS/2 - 9) + 1]," UNPROTO CHANNEL ",17);
  }
  else
   {
    strcat(stat_str,"Ch:");            /* CH:*/
                                       /* ChannelNr einblenden */    
    tmp = channel/10+'0';              /* HighByte */
    strncat(stat_str,&tmp,1);
    
    tmp = channel%10+'0';              /* LowByte */
    strncat(stat_str,&tmp,1);
    
    strcat(stat_str," Stat:");          /* Stat: */ 
    strcat(stat_str,ax_state_alt[ch_stat[channel].state & 0x0f]);
             
    /* Wenn Name vorhanden, Name ausgeben */
    if (ch_stat[channel].name[0] != '\0')
      {
       strncat(stat_str,ch_stat[channel].name,20);
       tmp = ':'; 
       strncat(stat_str,&tmp,1);
      }
    
    /*Distant call */
    strip_call_log(tmpstring,channel);
    if (strncmp(tmpstring,"CHANNEL",7) != 0)
       strcat(stat_str,tmpstring);
    strncat(stat_str,"          ",(9 - strlen(tmpstring)));
    
    /* Leerzeichen einfuegen */
    if ((stat_len = strlen(stat_str)) < (COLS - 26))
     {
      strncat(stat_str, "                                          "
                        "                                          "
                        "                                          "
                        "                                          ",
                        (COLS - 26 - stat_len));
                                                                                                                               }
                                                                                                                               
    
    
    /* MyCall */
    /* Wenn curcall mal staendig richtig gesetzt ist, kann man hier etwas
       einsparen. Eigentlich muesste ja noch auf Mycall=0 getestet werden
       um dann das Monitorcall anzuzeigen.*/
    strcat(stat_str,"My: ");
    if (ch_stat[channel].curcall[0] != '\0')
     {
      /* Currcall ausgeben */
      strncat(stat_str,ch_stat[channel].curcall,9);
      strncat(stat_str,"         ",(9 - strlen(ch_stat[channel].curcall)));
     }
     else
     {
      /* MyCall ausgeben */
      strncat(stat_str,ch_stat[channel].mycall,9);
      strncat(stat_str,"         ",(9 - strlen(ch_stat[channel].mycall)));
     }                      
     
    /* Linkstatus */  
    strcat(stat_str," AxSt:");
                                  
    /* Unsent */
    snd_frms = ch_stat[channel].snd_frms + ch_stat[channel].snd_queue_frms;
    /*High Nibble */
    tmp = snd_frms/10+'0';
    strncat(stat_str,&tmp,1);
    tmp = snd_frms%10+'0';
    strncat(stat_str,&tmp,1);
    tmp = '|';
    strncat(stat_str,&tmp,1);
    
    /* UnAck */
    tmp = ch_stat[channel].unacked/10+'0';
    strncat(stat_str,&tmp,1);
    tmp = ch_stat[channel].unacked%10+'0';
    strncat(stat_str,&tmp,1);
    tmp = '|';
    strncat(stat_str,&tmp,1);
    
    /* Retries */
    tmp = ch_stat[channel].tries/10+'0';
    strncat(stat_str,&tmp,1);
    tmp = ch_stat[channel].tries%10+'0';
    strncat(stat_str,&tmp,1);
    
  }
  win_stringout(stat_str,&statwin[channel],0);
  free(stat_str);
}

void stat_display(channel)
int channel;
{
  if (altstat)
    stat_display_alt(channel);
  else
    stat_display_org(channel);
}

static int output_screen(int channel)
{
#ifdef USE_IFACE
  return(ptyecho_flag || (!iface_active(channel) && !shell_active(channel)));
#else
  return(ptyecho_flag || (!shell_active(channel)));
#endif
}

void data_display(channel,buffer)
int channel;
char *buffer;
{
  char tmpstring[280];
  
  strcpy(tmpstring,"\r*** ");
  strcat(tmpstring,buffer);
  strcat(tmpstring,"\r");
#ifdef USE_SOCKET
  sock_status_out(channel,tmpstring);
#endif
  if (output_screen(channel)) {
    if ((cbell_flag == 1) ||
        ((cbell_flag == 2) && ((act_channel != channel) ||
         (act_mode != M_CONNECT))))
      beep();
    win_stringout(tmpstring,&textwin[channel],1);
    newdata(channel);
  }
  write_file(channel,strlen(tmpstring),RX_RCV,tmpstring);
}

void rem_data_display(channel,buffer)
int channel;
char *buffer;
{
  win_attrib(att_remote,&textwin[channel]);
  win_stringout(buffer,&textwin[channel],1);
  win_attrib(att_normal,&textwin[channel]);
}

void rem_data_display_buf(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  win_attrib(att_remote,&textwin[channel]);
  win_textout_len(buffer,len,&textwin[channel],1);
  win_attrib(att_normal,&textwin[channel]);
}

void rem_stat_display(channel,buffer)
int channel;
char *buffer;
{
  win_attrib(att_remote,&textwin[channel]);
  win_stringout(buffer,&textwin[channel],1);
  win_charout_cntl(C_DELLINE,&textwin[channel]);
  win_charout_cntl(C_STLINE,&textwin[channel]);
  win_attrib(att_normal,&textwin[channel]);
}

void handle_display_buffer(int channel,char **hlpbuffer,int *len)
{

  char buffer[513];
  int strlen;
  int len2;
  char *ptr;

#ifdef USE_IFACE
  /* handling when iface is active */
  if (iface_active(channel)) {
    write_iface(channel,*len,*hlpbuffer);
    if (output_screen(channel))
      win_textout_len(*hlpbuffer,*len,&textwin[channel],1);
    write_file(channel,*len,RX_RCV,*hlpbuffer);
    (*hlpbuffer) += *len;
    *len = 0;
    ch_stat[channel].oldbuflen = 0;
    ch_stat[channel].lastcr = 1;
    return;
  }
#endif

  /* handling when shell/socket is active */
  if (shell_active(channel)) {
    write_pty(channel,*len,*hlpbuffer);
    if (output_screen(channel))
      win_textout_len(*hlpbuffer,*len,&textwin[channel],1);
    write_file(channel,*len,RX_RCV,*hlpbuffer);
    (*hlpbuffer) += *len;
    *len = 0;
    ch_stat[channel].oldbuflen = 0;
    ch_stat[channel].lastcr = 1;
    return;
  }
  
  /* handling when autobin receive active */
  if (act_abin_rx(channel)) {
    len2 = *len;
    write_file_abin(channel,&len2,RX_RCV,*hlpbuffer);
    (*hlpbuffer) += len2;
    *len -= len2;
    ch_stat[channel].oldbuflen = 0;
    ch_stat[channel].lastcr = 1;
    return;
  }

  /* handling when YAPP receive is active */
  if (act_yapp_rx(channel)) {
    write_file_yapp(channel,*hlpbuffer,*len);
    (*hlpbuffer) += *len;
    *len = 0;
    ch_stat[channel].oldbuflen = 0;
    ch_stat[channel].lastcr = 1;
    return;
  }

  /* handling when YAPP transmit is active */
  if (act_yapp_tx(channel)) {
    ana_response_yapp(channel,*hlpbuffer,*len);
    (*hlpbuffer) += *len;
    *len = 0;
    ch_stat[channel].oldbuflen = 0;
    ch_stat[channel].lastcr = 1;
    return;
  }
  
  /* normal data handling */
  ptr = memchr(*hlpbuffer,CR,*len);
  if (ptr != NULL) {
    /* data up to CR to screen */
    strlen = (ptr + 1) - *hlpbuffer;
    insert_cr_tx(channel); 
    win_textout_len(*hlpbuffer,strlen,&textwin[channel],1);
    scan_pw_request(channel,*hlpbuffer,strlen);

    /* only if in a valid line */ 
    if (ch_stat[channel].lastcr) {
      if ((strlen + ch_stat[channel].oldbuflen) > 256) {
        if (ch_stat[channel].oldbuflen) {
          boxcut_nocr(channel,ch_stat[channel].oldbuf,
                      ch_stat[channel].oldbuflen);
          write_file(channel,ch_stat[channel].oldbuflen,RX_RCV,
                     ch_stat[channel].oldbuf);
        }
        ch_stat[channel].oldbuflen = 0;
        boxcut(channel,*hlpbuffer,strlen);
        write_file(channel,strlen,RX_RCV,*hlpbuffer);
      }
      else {
        /* build up a complete line in buffer */
        len2 = 0;
        if (ch_stat[channel].oldbuflen) {
          memcpy(buffer,ch_stat[channel].oldbuf,ch_stat[channel].oldbuflen);
          len2 += ch_stat[channel].oldbuflen;
          ch_stat[channel].oldbuflen = 0;
        }
        memcpy(buffer+len2,*hlpbuffer,strlen);
        len2 += strlen;
        buffer[len2] = '\0';
    
        if (len2 <= 256) {
          /* now handle line */
          if (ch_stat[channel].pwwait) {
            test_sysresponse(channel,buffer,len2);
            ch_stat[channel].pwwait--;
          }
          boxcut(channel,buffer,len2);
          connect_update(channel,buffer,len2);
          scan_login_received(channel,buffer,len2);
          if (!remote_input(channel,buffer,len2))
            write_file(channel,len2,RX_RCV,buffer);
        }
      }
    }
    else {
      memcpy(buffer,*hlpbuffer,strlen);
      buffer[strlen] = '\0';

      boxcut_rest(channel,buffer,strlen);
      write_file(channel,strlen,RX_RCV,buffer);
    }
    
    /* remove line from buffer */
    (*hlpbuffer) += strlen;
    *len -= strlen;
    ch_stat[channel].lastcr = 1;
    return;
  }
  else {
    /* data to screen */
    insert_cr_tx(channel); 
    win_textout_len(*hlpbuffer,*len,&textwin[channel],1);
    scan_pw_request(channel,*hlpbuffer,*len);

    if (check_autoyapp(channel,*hlpbuffer,*len)) {
      ch_stat[channel].oldbuflen = 0;
      ch_stat[channel].lastcr = 1;
    }
    else {
      if ((ch_stat[channel].lastcr) &&
          (*len + ch_stat[channel].oldbuflen <= 256)) {
          /* add to line in buffer */
          memcpy(ch_stat[channel].oldbuf+ch_stat[channel].oldbuflen,
                 *hlpbuffer,*len);
          ch_stat[channel].oldbuflen += *len;
          ch_stat[channel].lastcr = 1; /* still in valid line */
      }
      else {
        if (ch_stat[channel].oldbuflen) {
          boxcut_nocr(channel,ch_stat[channel].oldbuf,
                      ch_stat[channel].oldbuflen);
          write_file(channel,ch_stat[channel].oldbuflen,RX_RCV,
                     ch_stat[channel].oldbuf);
        }
        ch_stat[channel].oldbuflen = 0;
        ch_stat[channel].lastcr = 0;
        boxcut_nocr(channel,*hlpbuffer,*len);
        write_file(channel,*len,RX_RCV,*hlpbuffer);
      }
    }
    (*hlpbuffer) += *len;
    *len = 0;
    return;
  }
}

void data_display_len(channel,buffer)
int channel;
char *buffer;
{
  int len;
  char *hlpbuffer;
  
  if (output_screen(channel)) {
    if ((infobell_flag == 1) ||
        ((infobell_flag == 2) && ((act_channel != channel) ||
         (act_mode != M_CONNECT))))
      beep();
    newdata(channel);
  }
  len = (*buffer)+1;
  hlpbuffer = buffer+1;
  while (len > 0) {
    handle_display_buffer(channel,&hlpbuffer,&len);
  }
}

void rem_data_display_len(channel,buffer)
int channel;
char *buffer;
{
  win_attrib(att_remote,&textwin[channel]);
  win_textout(buffer,&textwin[channel],1);
  win_attrib(att_normal,&textwin[channel]);
}
#endif /* DPBOXT */

void cmd_display(flag,channel,buffer,cr)
int flag;
int channel;
char *buffer;
int cr;
{
  int ans_flag;
  int ans_len;
  char ans_str[256];
  int con_channel;
  
  switch (flag & 0x0000FFFF) {
#ifdef DPBOXT
  case M_MAILBOX:
    mb_pre_display(buffer,cr);
    break;
  default:
    break;
  }
#else
  case M_CMDSCRIPT:
    return;
  case M_PUSHPOP:
    return;
  case M_CONNECT:
    if (tx_file_com.type == -1) {
      con_channel = channel;
    }
    else {
      con_channel = act_channel;
    } 
    win_stringout(buffer,&prewin[con_channel],0);
    if (cr) win_charout(CR,&prewin[con_channel],0);
    break;
#ifdef USE_SOCKET
  case M_SOCKET:
    out_cmd_socket(channel,buffer);
    break;
#endif
  case M_REMOTE_TEMP:
  case M_REMOTE:
    strcpy(ans_str,rem_tnt_str);
    strcat(ans_str,buffer);
    strcat(ans_str,rem_newlin_str);
    ans_len = strlen(ans_str);
    ans_flag = 0;
    rem_data_display(channel,ans_str);
    queue_cmd_data(channel,X_DATA,ans_len,ans_flag,ans_str);
    break;
  case M_INTERFACE:
    strcpy(ans_str,buffer);
    strcat(ans_str,rem_newlin_str);
    ans_len = strlen(ans_str);
    ans_flag = 0;
    rem_data_display(channel,ans_str);
    queue_cmd_data(channel,X_DATA,ans_len,ans_flag,ans_str);
    break;
  case M_EXTMON:
    xm_pre_display(channel,buffer,cr);
    break;
#ifdef USE_IFACE
  case M_MAILBOX:
    mb_pre_display(buffer,cr);
    break;
#endif
  case M_IFACECMD:
#ifndef DPBOXT
    send_tntresponse(channel,flag,buffer,cr);
#endif
    break; 
  default:
    win_stringout(buffer,&cmdwin,0);
    if (cr) win_charout(CR,&cmdwin,0);
    break;
  }
#endif
}

#ifndef DPBOXT  
void moni_display2(buffer,attrib)
char *buffer;
int attrib;
{
  if (attrib)
    win_attrib(att_monitor,&monwin);
  win_stringout(buffer,&monwin,0);
  if (attrib)
    win_attrib(att_normal,&monwin);
  win_charout(CR,&monwin,0);
  write_monfile(strlen(buffer),buffer,1);
}

void moni_display(channel,buffer)
int channel;
char *buffer;
{
  layer3_frame = 0;
  if (layer3_flag) {
    layer3_frame = (strstr(buffer,"pid CF") != NULL);
  }
  xmon_ch = analyse_mon_header(buffer);
  moni_display2(buffer,1);
}

void moni_display_len(channel,buffer)
int channel;
char *buffer;
{
  int append_cr;

  if (xmon_ch != -1) {
    write_mon_info(buffer,xmon_ch);
  }
  else {
    analyse_mon_body(buffer);
  }
  if (layer3_frame) {
    if (!display_layer3(buffer)) return;
  }
#ifdef USE_IFACE
  check_uireq(buffer);
#endif
#ifdef BCAST
  /* special display for broadcast-frames, including decoding */
  if (analyse_bcast_body(buffer)) return;
#endif
#ifdef USE_IFACE
  if (scanmbea_valid) check_mbeacon(buffer);
#endif
  win_textout(buffer,&monwin,0);
  append_cr = 0;
  if (*(buffer + *buffer + 1) != CR) {
    win_charout(CR,&monwin,0);
    append_cr = 1;
  }
  write_monfile((*buffer + 1),buffer+1,append_cr);
}

void moni_display_len2(buffer,len)
char *buffer;
int len;
{
  int append_cr;
  char *buffer2;

  buffer2 = (char *)malloc(len+1);
  *buffer2 = (char)(len - 1);
  memcpy(buffer2 + 1,buffer,len);
  win_textout(buffer2,&monwin,0);
  append_cr = 0;
  if (*(buffer + len - 1) != CR) {
    win_charout(CR,&monwin,0);
    append_cr = 1;
  }
  write_monfile(len,buffer,append_cr);
  free(buffer2);
}

int pre_newline(channel,str)
int channel;
char *str;
{
  int len;
  
  len = get_line(str,&prewin[channel]);
  win_charout(CR,&prewin[channel],0);
  return(len);
}

void pre_nextline(channel)
int channel;
{
  win_charout(CR,&prewin[channel],0);
}

void pre_charout(channel,ch)
int channel;
char ch;
{
  if (prewin[channel].column < input_linelen-1) {
    win_charout_cntl(ch,&prewin[channel]);
  }
  else {
    beep();
  }
}

void pre_charout_nobnd(channel,ch)
int channel;
char ch;
{
  win_charout_cntl(ch,&prewin[channel]);
}

void pre_charout_cntl(channel,ch)
int channel;
char ch;
{
  if (prewin[channel].column < input_linelen-1) {
    ch &= 0x1f;
    ch |= 0x40;
    win_attrib(att_controlchar,&prewin[channel]);
    win_charout_cntl(ch,&prewin[channel]);
    win_attrib(att_normal,&prewin[channel]);
  }
  else {
    beep();
  }
}

int pre_lineend(channel)
int channel;
{
  return(prewin[channel].column == input_linelen-1);
}

int pre_wordwrap(channel,str)
int channel;
char *str;
{
  return(wordwrap(str,&prewin[channel]));
}

int cmd_newline(channel,str)
int channel;
char *str;
{
  int len;
  char tmpstr[MAXCOLS+1];
  
  len = get_line(tmpstr,&cmdwin);
  win_charout(CR,&cmdwin,0);
  if (tmpstr[0] == ':') {
    strcpy(str,tmpstr+1);
    len--;
  }
  else
    strcpy(str,tmpstr);
  return(len);
}

void cmd_charout(channel,ch)
int channel;
char ch;
{
  if (cmdwin.column < input_linelen-1) {
    win_charout_cntl(ch,&cmdwin);
  }
  else {
    beep();
  }
}

void cmd_charout_nobnd(channel,ch)
int channel;
char ch;
{
  win_charout_cntl(ch,&cmdwin);
}
#endif /* DPBOXT */

int init_screen()
{
  int i;
  int j;

#ifdef DPBOXT
  tabexp_flag = 1;
  window_init();
  open_window(1,&statlin,1);
  act_channel = 0;
  act_mode = M_MAILBOX;
  old_mode = M_MAILBOX;
  win_attrib(att_statline,&statlin);
  statlin_update();
  return(0);
}
#else
  tabexp_flag = 1;
  infobell_flag = 0;
  cbell_flag = 1;
  layer3_flag = 1;  
  if ((input_linelen > COLS) || (input_linelen < 80)) input_linelen = COLS;
  for (j = 0; j < LAYOUTPARTS; j++) {
    cmd_layout[j].win = NULL;
    cmd_layout[j].first_real_line = 0;
    cmd_layout[j].win_num_lines = 0;
    cmd_layout[j].pagesize = 0;
    mon_layout[j].win = NULL;
    mon_layout[j].first_real_line = 0;
    mon_layout[j].win_num_lines = 0;
    mon_layout[j].pagesize = 0;
  }
  for (i = 0; i < tnc_channels; i++) {
    for (j = 0; j < LAYOUTPARTS; j++) {
      layout[i][j].win = NULL;
      layout[i][j].first_real_line = 0;
      layout[i][j].win_num_lines = 0;
      layout[i][j].pagesize = 0;
    }
  }
  window_init();
  open_window(1,&statlin,1);
  open_window(lines_command,&cmdwin,1);
  cmd_layout[0].win = &cmdwin;
  cmd_layout[0].first_real_line = 0;
  cmd_layout[0].win_num_lines = LINES-1;
  cmd_layout[0].pagesize = LINES-4;
  cmd_layout[1].win = &statlin;
  cmd_layout[1].first_real_line = LINES-1;
  cmd_layout[1].win_num_lines = 1;
  cmd_layout[1].pagesize = 0;
  open_window(lines_monitor,&monwin,1);
  mon_layout[0].win = &monwin;
  mon_layout[0].first_real_line = 0;
  mon_layout[0].win_num_lines = LINES-1;
  mon_layout[0].pagesize = LINES-4;
  mon_layout[1].win = &statlin;
  mon_layout[1].first_real_line = LINES-1;
  mon_layout[1].win_num_lines = 1;
  mon_layout[1].pagesize = 0;
  if ((((LINES-lines_moncon)/scr_divide-1) < 2) ||
      (scr_divide < 2) || (scr_divide > 40)) scr_divide = 3;
  for (i = 0; i < tnc_channels; i++) {
    if (i && (i < r_channels)) { /* full windowsize */
      open_window(lines_input,&prewin[i],1);
      open_window(lines_output,&textwin[i],1);
    }
    else { /* reduced windowsize */
      open_window(lines_r_input,&prewin[i],1);
      open_window(lines_r_output,&textwin[i],1);
    }
    open_window(1,&statwin[i],1);
    win_attrib(att_cstatline,&statwin[i]);
    layout[i][0].win = &prewin[i];
    layout[i][0].first_real_line = 0;
    layout[i][0].win_num_lines = (LINES-lines_moncon)/scr_divide-1;
    layout[i][0].pagesize = (LINES-lines_moncon)/scr_divide-2;
    layout[i][1].win = &statwin[i];
    layout[i][1].first_real_line = (LINES-lines_moncon)/scr_divide-1;
    layout[i][1].win_num_lines = 1;
    layout[i][1].pagesize = 0;
    layout[i][2].win = &textwin[i];
    layout[i][2].first_real_line = (LINES-lines_moncon)/scr_divide;
    layout[i][2].win_num_lines =
      (LINES-lines_moncon)-(LINES-lines_moncon)/scr_divide-1;
    layout[i][2].pagesize =
      (LINES-lines_moncon)-(LINES-lines_moncon)/scr_divide-4;
    layout[i][3].win = &statlin;
    layout[i][3].first_real_line = (LINES-lines_moncon)-1;
    layout[i][3].win_num_lines = 1;
    layout[i][3].pagesize = 0;
    if (lines_moncon) {
      layout[i][4].win = &monwin;
      layout[i][4].first_real_line = (LINES-lines_moncon);
      layout[i][4].win_num_lines = lines_moncon;
      layout[i][4].pagesize = lines_moncon - 1;
    }
  }
  real_screen(cmd_layout);
  act_mode = M_COMMAND;
  old_mode = M_COMMAND;
  act_channel = 1;
  xmon_ch = -1;
  layer3_frame = 0;
  win_attrib(att_statline,&statlin);
  for (i = 0; i < tnc_channels;i++) {
    ch_stat[i].not_disp = 0;
  }
  statlin_update();
  if (!is_root) {
    cmd_display(M_COMMAND,1,signon,0);
    cmd_display(M_COMMAND,1,add_signon,1);
  }
  else {
    cmd_display(M_COMMAND,1,signon,1);
  }
  return(0);
}
#endif

void reinit_help();

void reinit_screen()
{
  int i;
  int j;

#ifdef DPBOXT
  if (input_linelen > COLS) input_linelen = COLS;
  reinit_help();
  reinit_mb();
  reinit_blist();

  switch (act_mode) {
  case M_MAILBOX:  
    real_screen(mb_layout);
    break;
  case M_BOXLIST:
    real_screen(mbboxlist_layout);
    break;
  case M_HELP:  
    real_screen(help_layout);
    break;
  }
#else
  if (input_linelen > COLS) input_linelen = COLS;
  for (j = 0; j < LAYOUTPARTS; j++) {
    cmd_layout[j].win = NULL;
    cmd_layout[j].first_real_line = 0;
    cmd_layout[j].win_num_lines = 0;
    cmd_layout[j].pagesize = 0;
    mon_layout[j].win = NULL;
    mon_layout[j].first_real_line = 0;
    mon_layout[j].win_num_lines = 0;
    mon_layout[j].pagesize = 0;
  }
  for (i = 0; i < tnc_channels; i++) {
    for (j = 0; j < LAYOUTPARTS; j++) {
      layout[i][j].win = NULL;
      layout[i][j].first_real_line = 0;
      layout[i][j].win_num_lines = 0;
      layout[i][j].pagesize = 0;
    }
  }
  cmd_layout[0].win = &cmdwin;
  cmd_layout[0].first_real_line = 0;
  cmd_layout[0].win_num_lines = LINES-1;
  cmd_layout[0].pagesize = LINES-4;
  cmd_layout[1].win = &statlin;
  cmd_layout[1].first_real_line = LINES-1;
  cmd_layout[1].win_num_lines = 1;
  cmd_layout[1].pagesize = 0;

  mon_layout[0].win = &monwin;
  mon_layout[0].first_real_line = 0;
  mon_layout[0].win_num_lines = LINES-1;
  mon_layout[0].pagesize = LINES-4;
  mon_layout[1].win = &statlin;
  mon_layout[1].first_real_line = LINES-1;
  mon_layout[1].win_num_lines = 1;
  mon_layout[1].pagesize = 0;

  if ((((LINES-lines_moncon)/scr_divide-1) < 2) ||
      (scr_divide < 2) || (scr_divide > 40)) scr_divide = 3;
  for (i = 0; i < tnc_channels; i++) {
    layout[i][0].win = &prewin[i];
    layout[i][0].first_real_line = 0;
    layout[i][0].win_num_lines = (LINES-lines_moncon)/scr_divide-1;
    layout[i][0].pagesize = (LINES-lines_moncon)/scr_divide-2;
    layout[i][1].win = &statwin[i];
    layout[i][1].first_real_line = (LINES-lines_moncon)/scr_divide-1;
    layout[i][1].win_num_lines = 1;
    layout[i][1].pagesize = 0;
    layout[i][2].win = &textwin[i];
    layout[i][2].first_real_line = (LINES-lines_moncon)/scr_divide;
    layout[i][2].win_num_lines =
      (LINES-lines_moncon)-(LINES-lines_moncon)/scr_divide-1;
    layout[i][2].pagesize =
      (LINES-lines_moncon)-(LINES-lines_moncon)/scr_divide-4;
    layout[i][3].win = &statlin;
    layout[i][3].first_real_line = (LINES-lines_moncon)-1;
    layout[i][3].win_num_lines = 1;
    layout[i][3].pagesize = 0;
    if (lines_moncon) {
      layout[i][4].win = &monwin;
      layout[i][4].first_real_line = (LINES-lines_moncon);
      layout[i][4].win_num_lines = lines_moncon;
      layout[i][4].pagesize = lines_moncon - 1;
    }
  }
  reinit_help();
#ifdef USE_IFACE
  reinit_mb();
#endif
  reinit_blist();
  reinit_xmon();
  reinit_mheard();  

  switch (act_mode) {
  case M_COMMAND:  
    real_screen(cmd_layout);
    break;
  case M_MONITOR:  
    real_screen(mon_layout);
    break;
  case M_CONNECT:  
    real_screen(layout[act_channel]);
    break;
  case M_EXTMON:  
    real_screen(xmon_layout[xmon_screen]);
    break;
#ifdef USE_IFACE
  case M_MAILBOX:  
    real_screen(mb_layout);
    break;
#endif
  case M_BOXLIST:
#ifdef USE_IFACE
    if (bl_mode == M_MAILBOX)  
      real_screen(mbboxlist_layout);
    else
      real_screen(boxlist_layout[act_channel]);
#else
    real_screen(boxlist_layout[act_channel]);
#endif
    break;
  case M_HEARD:  
    real_screen(heard_layout);
    break;
  case M_HELP:  
    real_screen(help_layout);
    break;
  }
#endif
}

void exit_screen()
{
  int i;

#ifdef DPBOXT
  close_window(&statlin);
#else  
  close_window(&cmdwin);
  close_window(&monwin);
  close_window(&statlin);
  for (i = 0; i < tnc_channels; i++) {
    close_window(&prewin[i]);
    close_window(&statwin[i]);
    close_window(&textwin[i]);
  }
#endif  
  window_exit();
}

/* display line on help-screen */
static void help_textout_len(buf,len)
char *buf;
int len;
{
  win_textout_len(buf,len,&helpwin,1);
}

/* character to help-screen */
void help_charout_cntl(ch)
char ch;
{
  win_charout_cntl(ch,&helpwin);
}

/* select help screen */
void sel_help()
{
  if ((!help_available) || (act_mode == M_HELP)) return;
  real_screen(help_layout);
  act_mode = M_HELP;
  statlin_update();
}

#define BUFSIZE 512

/* init of help-screen */
void init_help()
{
  char helpfile[160];
  int helpfd;
  int file_len;
  int end;
  int len2;
  char buf[BUFSIZE];
  int i;
  int j;
  int lines;
  char *linebuf;
  int linelen;

  
  help_available = 0;
  strcpy(helpfile,tnt_help_file);
  helpfd = open(helpfile,O_RDONLY); 
  if (helpfd == -1) {
    /* file does not exist */
    return;
  }
  
  /* counting number of LFs in file to get number of lines */
  file_len = 0;
  lines = 0;
  end = 1;
  while (end) {
    len2 = read(helpfd,buf,BUFSIZE);
    file_len += len2;
    for (i=0;i<len2;i++) {
      if (buf[i] == '\n') lines++;
    }
    if (len2 < BUFSIZE) end = 0;
  }
  lines += 1; /* just to be sure... */

  for (j = 0; j < LAYOUTPARTS; j++) {
    help_layout[j].win = NULL;
    help_layout[j].first_real_line = 0;
    help_layout[j].win_num_lines = 0;
    help_layout[j].pagesize = 0;
  }
  open_window(lines,&helpwin,0);
  help_layout[0].win = &helpwin;
  help_layout[0].first_real_line = 0;
  help_layout[0].win_num_lines = LINES-1;
  help_layout[0].pagesize = LINES-4;
  help_layout[1].win = &statlin;
  help_layout[1].first_real_line = LINES-1;
  help_layout[1].win_num_lines = 1;
  help_layout[1].pagesize = 0;
  
  /* seek to file begin */
  lseek(helpfd,0,SEEK_SET);
  linelen = 0;
  end = 1;
  linebuf = (char *)malloc(COLS+2);
  while (end) {
    len2 = read(helpfd,buf,BUFSIZE);
    for (i=0;i<len2;i++) {
      if (buf[i] == '\n') {
        linebuf[linelen] = '\r';
        linelen++;
        help_textout_len(linebuf,linelen);
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
    help_textout_len(linebuf,linelen);
    linelen = 0;
  }
  close(helpfd);
  free(linebuf);
  helpwin.line = 0;
  if ((helpwin.phys_line = helpwin.line + helpwin.line_offset) >=
       helpwin.num_lines)
    helpwin.phys_line -= helpwin.num_lines;
  helpwin.column = 0;
  help_available = 1;
}

void reinit_help()
{
  int j;
  
  if (help_available) {
    for (j = 0; j < LAYOUTPARTS; j++) {
      help_layout[j].win = NULL;
      help_layout[j].first_real_line = 0;
      help_layout[j].win_num_lines = 0;
      help_layout[j].pagesize = 0;
    }
    help_layout[0].win = &helpwin;
    help_layout[0].first_real_line = 0;
    help_layout[0].win_num_lines = LINES-1;
    help_layout[0].pagesize = LINES-4;
    help_layout[1].win = &statlin;
    help_layout[1].first_real_line = LINES-1;
    help_layout[1].win_num_lines = 1;
    help_layout[1].pagesize = 0;
  }
}

/* exit of help-screen */
void exit_help()
{
  if (help_available) {
    close_window(&helpwin);
  }
}

#ifndef DPBOXT
void free_display()
{
  free(prewin);
  free(statwin);
  free(textwin);
}

int alloc_display()
{
  prewin = (struct window *)malloc(tnc_channels * sizeof(struct window));
  statwin = (struct window *)malloc(tnc_channels * sizeof(struct window));
  textwin = (struct window *)malloc(tnc_channels * sizeof(struct window));
  layout = (struct real_layout (*) [LAYOUTPARTS])
    malloc(tnc_channels * sizeof(struct real_layout [LAYOUTPARTS]));
  return((prewin == NULL) ||
         (statwin == NULL) ||
         (textwin == NULL) ||
         (layout == NULL));
}
#endif

