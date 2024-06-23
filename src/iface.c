/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for interface (iface.c)
   created: Mark Wahl DL4YBG 94/07/03
   updated: Mark Wahl DL4YBG 97/01/17
   updated: Matthias Hensler WS1LS 99/03/12
*/

#undef IFACE_DEBUG

#include "tnt.h"
#ifdef USE_IFACE
#include "ifacedef.h"
#include "iface.h"
#ifdef __GLIBC__xxx
#ifdef TNT_LINUX
#include <linux/in.h>
#endif
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <sys/un.h>
#include <signal.h>
#ifndef SOCK_STREAM
#include <socketbits.h>
#endif

typedef unsigned char boolean;
typedef unsigned char uchar;


#ifndef DPBOXT
extern void end_comp(int channel);
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void monbox_close();
extern void rem_data_display(int channel,char *buffer);
#endif
extern void cmd_display(int flag,int channel,char *buffer,int cr);
#ifndef DPBOXT
extern void get_call_xc(int channel,char *call);
extern void get_chanstr(char *qrg, char *chanstr);
extern int find_free_channel();
extern void update_owncall(int channel,char *call);
extern void cmd_xconnect_ext(int par1,int par2,int par3,int channel,
                         int len,int mode,char *str);
extern void abort_routing(char *call);
extern void cmd_xconnect(int par1,int par2,int channel,
                         int len,int mode,char *str);
extern void rem_data_display_buf(int channel,char *buffer,int len);
extern void dat_input(int channel,char *str,int len);
extern int calc_xframes(int channel);
extern void next_bid(int result,int channel);
#endif
#ifdef BCAST
extern void start_boxbcast(char *filename);
#endif
#ifndef DPBOXT
extern void end_ifacexconnect(int iface,int usernr);
extern void cmd_input(int channel,int mode,char *str,int len,int cscript);
#endif
#ifdef BCAST
extern void ibcastswitchtobc(short tnc, boolean y);
extern void ibcastpushunproto(short tnc);
extern void ibcastsetunproto(short tnc, char *unprstr);
extern void ibcastsend(short channel, short len, uchar *buffer);
extern void ibcastpopunproto(short tnc);
#endif
#ifndef DPBOXT
extern void copy_qrg(char qrg[MAXQRGS-1][20]);
extern void strip_call_log(char *call,int channel);
extern int shell_active(int channel);
extern void start_sf(int channel,int iface);
extern void init_monbox();
extern void init_boxcut();
extern void exit_boxcut();
extern void exit_monbox();
extern void get_call_log(char *call,int channel);
#endif
extern void real_screen(struct real_layout *layout);
extern void statlin_update();
extern int get_line(char *str,struct window *win);
extern void win_charout(char ch,struct window *win,int conv);
extern void win_charout_cntl(char ch,struct window *win);
extern void beep();
extern void win_attrib(char ch,struct window *win);
extern int wordwrap(char *str,struct window *win);
extern void win_textout_len(char *str,int len,struct window *win,int conv);
extern void write_mbfile(int len,int flag,char *str);
extern void win_stringout(char *str,struct window *win, int conv);
extern int open_window(int lines,struct window *win,int bot);
extern int close_window(struct window *win);


#ifndef DPBOXT
extern int tnc_channels;
extern int tnt_box_ssid;
extern struct rx_file *rx_file;
char noconn_text[80];
extern int frontend_active;
extern char frontend_para[];
extern int frontend_fd;
extern int frontend_sockfd;
int accept_uireq_flag;
char accuicall[10];
extern int head_valid;
extern int frame_ui;
extern int pid;
extern char destcall[];
extern int un_callcount;
extern int un_heardfrom;
extern char un_qrg[20];
extern char un_calls[10][10];
#endif
extern int use_select;
extern int tnt_daemon;
extern int act_mode;
extern int att_controlchar;
extern int att_normal;
extern int att_remote;
extern int att_cstatline;
extern int lines_mbinput;
extern int lines_mboutput;
extern int input_linelen;
extern int LINES,COLS;
extern int mbscr_divide;
extern int ptyecho_flag;
static char iface_err_txt[80];
static char iface_open_txt[80];
static char sock_err_txt[80];
static char conn_err_txt[80];
static char end_err_txt[80];
static char back_tnt_txt[80];
static char iface_trying_txt[80];
char iface_active_txt[80];
char box_socket[80];
char node_socket[80];
extern char sh_active_txt[];
static struct iface_list if_list[MAX_IFACE];
#ifdef DPBOXT
static struct iface_stat if_stat[1];
#else
static struct iface_stat *if_stat;
extern struct channel_stat *ch_stat;
#endif
int box_active_flag;
int box_busy_flag;
int box_iface;
int node_active_flag;
int node_iface;
static time_t last_iface_try;
static int clilen;
static struct sockaddr_un cli_addr;
static char boxconsole_call[256];

struct real_layout mb_layout[LAYOUTPARTS];
static struct window mb_prewin;
static struct window mb_statwin;
static struct window mb_textwin;
extern struct window statlin;

static void deactivate_program();
void send_command_packet_if();
void mb_display_buf();
void mb_stat_display();
void flush_buf();
int box_active();
int conv_name_to_iface();
#ifndef DPBOXT
void send_cstatus();
void send_huffstat();
#endif


/* close iface connection on channel */
void close_iface_con(channel,deact,disconnect)
int channel;
int deact;
int disconnect;
{
#ifndef DPBOXT
  char ans_str[80];
  int ans_len;
  int flag;
#endif

  if (if_stat[channel].iface != -1) {
    if (deact) deactivate_program(channel);
#ifndef DPBOXT
    if ((disconnect) && (channel != 0)) {
      if (if_stat[channel].direct_started) {
        end_comp(channel);
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
      }
      else {
        if ((if_stat[channel].box_connect == 1) ||
            (if_stat[channel].node_connect == 1)) {
          sprintf(ans_str,"TNT:%s> Reconnected to %s\r",
                  ch_stat[channel].curcall,ch_stat[channel].curcall);
          flag = 0;
          ans_len = strlen(ans_str);
          rem_data_display(channel,ans_str);
          queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
        }
        else {
          cmd_display(M_REMOTE,channel,back_tnt_txt,1);
        }
      }
    }
#endif
    flush_buf(channel,1);
    if_stat[channel].iface = -1;
    if (channel == 0)
      mb_stat_display();
  }
}

/* close the iface-socket */
static void close_iface(iface,deact,disconnect,trying)
int iface;
int deact;
int disconnect;
int trying;
{
  int i;
  struct queue_entry *oldq_ptr;
  
  if (if_list[iface].active == IF_ACTIVE) {
#ifdef DPBOXT
    if (if_stat[0].iface == iface) {
      close_iface_con(0,deact,disconnect);
    }
    if (strcmp(if_list[iface].name,box_socket) == 0) {
      box_active_flag = 0;
      box_busy_flag = 0;
    }
#else
    for (i=0;i<tnc_channels;i++) {
      if (if_stat[i].iface == iface) {
        close_iface_con(i,deact,disconnect);
      }
    }
    if (strcmp(if_list[iface].name,box_socket) == 0) {
      monbox_close();
      box_active_flag = 0;
      box_busy_flag = 0;
    }
    if (strcmp(if_list[iface].name,node_socket) == 0) {
      node_active_flag = 0;
    }
#endif
    while (if_list[iface].first_q != NULL) {
      oldq_ptr = if_list[iface].first_q;
      free(oldq_ptr->buffer);
      if_list[iface].first_q = oldq_ptr->next;
      free((char *)oldq_ptr);
    }
    close(if_list[iface].sockfd);
    if (trying)
      if_list[iface].active = IF_TRYING;
    else
      if_list[iface].active = IF_NOTACTIVE;
  }
}

static void blocking_test(iface,len)
int iface;
int len;
{
  IFACE_CMDBUF command;
  
  if_list[iface].sendlength += len;
  if (if_list[iface].sendlength > BLOCKING_SIZE) {
    if_list[iface].blocked = 1;
    command.command = CMD_BLOCK;
    send_command_packet_if(NO_CHANNEL,NO_USERNR,LEN_SIMPLE,
                           (char *)&command,iface);
  }
}

static void unblocking(iface)
int iface;
{
  struct queue_entry *oldq_ptr;
  
  if (if_list[iface].active != IF_ACTIVE) return;
  if_list[iface].blocked = 0;
  if_list[iface].sendlength = 0;
  while ((!if_list[iface].blocked) &&
         (if_list[iface].first_q != NULL)) {
    oldq_ptr = if_list[iface].first_q;
    if (write(if_list[iface].sockfd,oldq_ptr->buffer,
              oldq_ptr->len) < oldq_ptr->len) {
      close_iface(iface,0,1,1);
      return;
    }
    blocking_test(iface,oldq_ptr->len);
    free(oldq_ptr->buffer);
    if_list[iface].first_q = oldq_ptr->next;
    free((char *)oldq_ptr);
  }
}

#ifndef DPBOXT
/* a connect was not successful, inform user */
void no_success_connect(int iface,int usernr)
{
  IFACE_CMDBUF command;

  if (if_list[iface].active != IF_ACTIVE) return;
  command.command = CMD_NOSUCCESSCON;
  send_command_packet_if(NO_CHANNEL,usernr,LEN_SIMPLE,
                         (char *)&command,iface);
}

/* a connect was successful, activate and inform user */
void success_connect(int iface,int usernr,int channel)
{
  int len;
  char call[256];
  IFACE_CMDBUF command;
        
  if (if_list[iface].active != IF_ACTIVE) return;
  /* fill command */
  command.command = CMD_SUCCESSCON;
  len = LEN_SIMPLE;
  /* fill callsign */
  get_call_xc(channel,call);
  strcpy(command.data.buffer,call);
  len += strlen(call);
  *(command.data.buffer + len - LEN_SIMPLE) = '\0';
  len++;
  send_command_packet_if((short)channel,usernr,len,(char *)&command,iface);
}

/* answer to a tnt-command via interface */
void send_tntresponse(int channel,int mode,char *str,int cr)
{
  int len;
  IFACE_CMDBUF command;
  short usernr;
  int hmode;
  int iface;

  hmode = mode / 65536;
  usernr = hmode & 0x000000FF;
  iface = (hmode / 256) & 0x000000FF;
  if (if_list[iface].active != IF_ACTIVE) return;
  /* fill command */
  command.command = CMD_TNTRESPONSE;
  len = LEN_TNTRESPONSE;
  command.data.tntresponse.follows = 0;
  strcpy(command.data.tntresponse.buffer,str);
  if (cr)
    strcat(command.data.tntresponse.buffer,"\r");
  len += strlen(command.data.tntresponse.buffer);
  *(command.data.tntresponse.buffer + len - LEN_TNTRESPONSE) = '\0';
  len++;
  send_command_packet_if((short)channel,usernr,len,(char *)&command,iface);
}

static void queue_if_data(int iface,int len,char *buffer)
{
  struct queue_entry *q_ptr;
  char *ptr;
  int res;

  if (if_list[iface].blocked) {
    q_ptr = (struct queue_entry *)malloc(sizeof(struct queue_entry));
    if (q_ptr == NULL) {
      close_iface(iface,0,1,1);
      return;
    }
    ptr = (char *)malloc(len+HEAD_LEN);
    if (ptr == NULL) {
      free(q_ptr);
      close_iface(iface,0,1,1);
      return;
    }
    memcpy(ptr,buffer,len+HEAD_LEN);
    q_ptr->buffer = ptr;
    q_ptr->len = len+HEAD_LEN;
    q_ptr->next = NULL;
    if (if_list[iface].first_q == NULL) {
      if_list[iface].first_q = q_ptr;
    }
    else {
      if_list[iface].last_q->next = q_ptr;
    }
    if_list[iface].last_q = q_ptr;
  }
  else {
    if ((res = write(if_list[iface].sockfd,
                   buffer,len+HEAD_LEN)) < len+HEAD_LEN) {
      close_iface(iface,0,1,1);
      return;
    }
    blocking_test(iface,len+HEAD_LEN);
  }
}

void check_uireq(char *buffer)
{
  int buflen;
  int len;
  IFACE_HEADER header;
  IFACE_CMDBUF command;
  char ifbuffer[MAX_LEN];
  int iface;

  if (!accept_uireq_flag) return;
  if (!head_valid) return;
  if (!frame_ui) return;
  if (pid != 0xf0) return;
  if (strcmp(destcall,accuicall) != 0) return;
  if (un_heardfrom < un_callcount) return;
  
  buflen = (*buffer) + 1;
  if (buflen < 12 || buflen > 13) return;
  if (buffer[1] != '?') return;

  iface = conv_name_to_iface(box_socket);
  if (iface == -1) return;
  if (if_list[iface].active != IF_ACTIVE) return;
  
  /* fill command */
  command.command = CMD_RXUNPROTOHEAD;
  len = LEN_RXUNPROTOHEAD;
  command.data.rxunprotohead.pid = pid;
  command.data.rxunprotohead.callcount = un_callcount;
  command.data.rxunprotohead.heardfrom = un_heardfrom;
  memcpy(command.data.rxunprotohead.qrg,un_qrg,20);
  memcpy(command.data.rxunprotohead.calls,un_calls,DAT_PATHINFO);

  header.indicator = IF_COMMAND;
  header.channel = NO_CHANNEL;
  header.usernr = NO_USERNR;
  header.len = len;
  memcpy(ifbuffer,(char *)&header,HEAD_LEN);
  memcpy(&ifbuffer[HEAD_LEN],(char *)&command,len);

  queue_if_data(iface,len,ifbuffer);

  if (if_list[iface].active != IF_ACTIVE) return;

  header.indicator = IF_UNPROTO;
  header.channel = NO_CHANNEL;
  header.usernr = NO_USERNR;
  header.len = buflen;
  memcpy(ifbuffer,(char *)&header,HEAD_LEN);
  memcpy(&ifbuffer[HEAD_LEN],buffer+1,buflen);

  queue_if_data(iface,buflen,ifbuffer);
}

/* Set permanent mycall */
void cmd_accuicall(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int i;

  if (len) {
    if (strcmp(str,"$") == 0) {
      accuicall[0] = '\0';
    }
    else {
      for (i=0;i<=9;i++) {
        accuicall[i] = toupper(str[i]);
      }
      accuicall[9] = '\0';
    }
    cmd_display(mode,channel,OK_TEXT,1);
  }
  else {
    if (accuicall[0] == '\0')
      cmd_display(mode,channel,"<empty>",1);
    else
      cmd_display(mode,channel,accuicall,1);
  }
}

#endif

/* analysis of received packet via interface */
static void packet_analysis(indicator,channel,usernr,len,buf,iface)
char indicator;
int channel;
int usernr;
int len;
char *buf;
int iface;
{
  int con_channel;
  char p1str[256];
  char p2str[256];
  char hlpstr[256];
  char h2[256];
  char *bufptr;
  IFACE_CMDBUF *rec_command;
  IFACE_CMDBUF command;
#ifndef DPBOXT
  int len2;
  int frames;
  int hmode;
#endif
  
#ifdef DPBOXT
  if ((channel != NO_CHANNEL) && (channel != 0)) return;
  if ((channel == 0) && (if_stat[channel].iface == -1)) return;
#else
  if ((channel != NO_CHANNEL) && (channel >= tnc_channels)) return;
  if ((channel < tnc_channels) && (if_stat[channel].iface == -1)) {
    /* for BULLID iface can be closed on channel */
    if (indicator != IF_COMMAND) return;
    if ((((IFACE_CMDBUF *)buf)->command != CMD_BULLID) &&
        (((IFACE_CMDBUF *)buf)->command != CMD_TNTCOMMAND)) return;
  }    
#endif
  switch (indicator) {
  case IF_COMMAND:
    rec_command = (IFACE_CMDBUF *)buf;
    switch (rec_command->command) {
    case CMD_CONNECT:
#ifndef DPBOXT
      bufptr = rec_command->data.connect.buffer;
      strcpy(p1str,bufptr);
      bufptr += strlen(p1str) + 1;
      strcpy(p2str,bufptr);
      get_chanstr(rec_command->data.connect.qrg,h2);
      
      con_channel = find_free_channel();
      if (con_channel == -1) {
        no_success_connect(iface,usernr);
      }
      else {
        strcpy(hlpstr,"I");
        strcat(hlpstr,p1str);
        queue_cmd_data(con_channel,X_COMM,strlen(hlpstr),M_CMDSCRIPT,hlpstr);
        update_owncall(con_channel,p1str);
        sprintf(hlpstr,"%s%s %d",h2,p2str,rec_command->data.connect.timeout);
        cmd_xconnect_ext(2,iface,usernr,
                     con_channel,strlen(hlpstr),M_CONNECT,hlpstr);
      }
#endif
      break;
    case CMD_DISCONNECT:
#ifndef DPBOXT
      if (channel == NO_CHANNEL) return;
      if (channel != 0) {
        end_comp(channel);
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
      }
      close_iface_con(channel,0,0);
#endif
      break;
    case CMD_FINISH:
      if (channel == NO_CHANNEL) return;
      close_iface_con(channel,0,1);
      break;
    case CMD_BLOCK:
      command.command = CMD_UNBLOCK;
      send_command_packet_if(NO_CHANNEL,NO_USERNR,LEN_SIMPLE,(char *)&command,iface);
      break;
    case CMD_UNBLOCK:
      unblocking(iface);
      break;
    case CMD_BOXPBOXSF:
#ifndef DPBOXT
      bufptr = rec_command->data.boxpboxsf.buffer;
      strcpy(p1str,bufptr);
      bufptr += strlen(p1str) + 1;
      strcpy(p2str,bufptr);
      
      con_channel = find_free_channel();
      if (con_channel == -1) {
        abort_routing(p2str);
      }
      else {
        strcpy(hlpstr,"I");
        strcat(hlpstr,p1str);
        queue_cmd_data(con_channel,X_COMM,strlen(hlpstr),M_CMDSCRIPT,hlpstr);
        update_owncall(con_channel,p1str);
        sprintf(hlpstr,"%s %d",p2str,rec_command->data.boxpboxsf.timeout);
        cmd_xconnect(1,iface,
                     con_channel,strlen(hlpstr),M_CONNECT,hlpstr);
      }
#endif
      break;
    case CMD_BOXPRDISC:
#ifndef DPBOXT
      if (channel == NO_CHANNEL) return;
      if (channel != 0) {
        end_comp(channel);
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
      }
#endif
      close_iface_con(channel,1,0);
      break;
    case CMD_BOXPABORTSF:
#ifndef DPBOXT
      if (channel == NO_CHANNEL) return;
      if (!rec_command->data.immediate) {
        end_comp(channel);
      }
      queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
      close_iface_con(channel,0,0);
      if (rec_command->data.immediate) {
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
      }
#endif
      break;
    case CMD_ACT_RESP:
      if (channel == NO_CHANNEL) return;
      if ((len-LEN_SIMPLE) > 0) {
        if (channel == 0) {
          mb_display_buf(rec_command->data.buffer,len-LEN_SIMPLE);
        }
#ifdef DPBOXT
      }
      if (usernr == NO_USERNR) {
        close_iface_con(channel,0,1);
      }
      else {
        if_stat[channel].usernr = usernr;
        flush_buf(channel,0);
      }
#else
        else {
          if (ptyecho_flag)
            rem_data_display_buf(channel,rec_command->data.buffer,len-LEN_SIMPLE);
          dat_input(channel,rec_command->data.buffer,len-LEN_SIMPLE);
        }
      }
      if (usernr == NO_USERNR) {
        close_iface_con(channel,0,1);
      }
      else {
        if_stat[channel].usernr = usernr;
        frames = calc_xframes(channel);
        send_cstatus(channel,frames);
        send_huffstat(channel);
        flush_buf(channel,0);
      }
#endif
      break;
    case CMD_HUFFSTAT:
#ifndef DPBOXT
      if ((channel > 0) && (channel < tnc_channels))
        ch_stat[channel].huffcod = rec_command->data.huffstat;
#endif
      break;
    case CMD_BULLID:
#ifndef DPBOXT
      if (channel == NO_CHANNEL) return;
      next_bid(rec_command->data.bullid.found,channel);
#endif
      break;
    case CMD_BOXISBUSY:
#ifndef DPBOXT
      if (rec_command->data.boxisbusy)
        box_busy_flag = 1;
      else
        box_busy_flag = 0;
#endif
      break;
    case CMD_STARTBOXBC:
#ifdef BCAST
      start_boxbcast(rec_command->data.buffer);
#endif    
      break;
    case CMD_SETUNPROTO:
#ifdef BCAST
      if (if_list[iface].active == IF_ACTIVE) {
        get_chanstr(rec_command->data.setunproto.qrg,hlpstr);
        strcat(hlpstr,rec_command->data.setunproto.address);
        strcpy(if_list[iface].unproto_address,hlpstr);
      }
#endif
      break;
    case CMD_ABORTCON:
#ifndef DPBOXT
      end_ifacexconnect(iface,usernr);
#endif
      break;
    case CMD_TNTCOMMAND:
#ifndef DPBOXT
      if (channel == NO_CHANNEL) return;
      if ((usernr == NO_USERNR) || (usernr == 0)) return;
      if ((len2 = strlen(rec_command->data.buffer)) == 0) return;
      hmode = ((usernr*65536) & 0x00FF0000) +
              ((iface*256*65536) & 0xFF000000) +
              M_IFACECMD;
      cmd_input(channel,hmode,rec_command->data.buffer,len2,0);
#endif
      break;
    default:
      break;
    }
    break;
  case IF_DATA:
    if (channel == NO_CHANNEL) return;
    if (channel == 0) {
      mb_display_buf(buf,len);
    }
#ifndef DPBOXT
    else {
      if (ptyecho_flag)
        rem_data_display_buf(channel,buf,len);
      dat_input(channel,buf,len);
    }
#endif
    break;
  case IF_UNPROTO:
#ifdef BCAST
    if (if_list[iface].active == IF_ACTIVE) {
      if (if_list[iface].unproto_address[0] != '\0') {
        ibcastswitchtobc(0,1);
        ibcastpushunproto(0);
        ibcastsetunproto(0,if_list[iface].unproto_address);
        if (ptyecho_flag)
          rem_data_display_buf(0,buf,len);
        ibcastsend(0,len,(uchar *)buf);
        ibcastpopunproto(0);
        ibcastswitchtobc(0,0);
      }
    }
#endif
    break;
  default:
    break;
  }
}

void send_command_packet_if(channel,usernr,len,buf,iface)
short channel;
short usernr;
unsigned short len;
char *buf;
int iface;
{
  char buffer[MAX_LEN];
  int res;
  IFACE_HEADER header;

  if (if_list[iface].active != IF_ACTIVE) return;
  if ((iface > MAX_IFACE) || (len > IFACE_PACLEN)) return;
  header.indicator = IF_COMMAND;
  header.channel = channel;
  header.usernr = usernr;
  header.len = len;
  memcpy(buffer,(char *)&header,HEAD_LEN);
  memcpy(buffer + HEAD_LEN,buf,len);
  if ((res = write(if_list[iface].sockfd,buffer,len+HEAD_LEN)) < len+HEAD_LEN) {
    close_iface(iface,0,1,1);
    return;
  }
}

void send_command_packet(channel,usernr,len,buf)
short channel;
short usernr;
unsigned short len;
char *buf;
{
  char buffer[MAX_LEN];
  int iface;
  int res;
  IFACE_HEADER header;

  if (((iface = if_stat[channel].iface) == -1) || (len > IFACE_PACLEN)) return;
  if (if_list[iface].active != IF_ACTIVE) return;
  header.indicator = IF_COMMAND;
  header.channel = channel;
  header.usernr = usernr;
  header.len = len;
  memcpy(buffer,(char *)&header,HEAD_LEN);
  memcpy(buffer + HEAD_LEN,buf,len);
  if ((res = write(if_list[iface].sockfd,buffer,len+HEAD_LEN)) < len+HEAD_LEN) {
    close_iface(iface,0,1,1);
    return;
  }
}

#ifndef DPBOXT
/* channel status is sent via interface */
void send_cstatus(channel,frames)
int channel;
int frames;
{
  IFACE_CMDBUF command;

  if ((if_stat[channel].iface == -1) || (channel == 0)) return;
  command.command = CMD_CSTATUS;
  command.data.snd_frames = frames;
  send_command_packet((short)channel,if_stat[channel].usernr,
                      LEN_CSTATUS,(char *)&command);
}

/* huffman status is sent via interface */
void send_huffstat(channel)
int channel;
{
  IFACE_CMDBUF command;

  if ((if_stat[channel].iface == -1) || (channel == 0)) return;
  command.command = CMD_HUFFSTAT;
  command.data.huffstat = ch_stat[channel].huffcod; 
  send_command_packet((short)channel,if_stat[channel].usernr,
                      LEN_HUFFSTAT,(char *)&command);
}

/* send qrg-info via interface */
void send_qrginfo(iface)
int iface;
{
  IFACE_CMDBUF command;
  
  command.command = CMD_QRGINFO;
  copy_qrg(command.data.qrg);
  send_command_packet_if(NO_CHANNEL,NO_USERNR,
                         LEN_QRGINFO,(char *)&command,iface);
}

/* send new qrg-info to all interfaces */
void send_qrginfo_all()
{
  int i;
  
  for (i=0;i<MAX_IFACE;i++) {
    if (if_list[i].active == IF_ACTIVE) {
      send_qrginfo(i);
    }
  }
}

/* send bulletin-id query */
void check_bid(channel,bid)
int channel;
char *bid;
{
  IFACE_CMDBUF command;
  
  command.command = CMD_BULLID;
  command.data.bullid.found = 0;
  memcpy(command.data.bullid.bullid,bid,BULLIDLEN+1);
  send_command_packet_if(channel,NO_USERNR,
                         LEN_BULLID,(char *)&command,box_iface);
}

#endif

static void activate_program(channel)
int channel;
{
  int len;
  char callsign[256];
  IFACE_CMDBUF command;

  command.command = CMD_ACTIVATE;
  len = LEN_SIMPLE;
  if ((channel == 0) && (box_active(channel))) {
    if (*boxconsole_call != '\0') { /* fill callsign for boxlogin */
      strcpy(command.data.buffer,boxconsole_call);
      len += strlen(boxconsole_call);
      *(command.data.buffer + len - LEN_SIMPLE) = '\0';
      len++;
    }
  }
#ifndef DPBOXT
  else {
    /* fill callsign */
    get_call_log(callsign,channel);
    if (strlen(callsign) > 200) callsign[200] = '\0';
    strcpy(command.data.buffer,callsign);
    len += strlen(callsign);
    *(command.data.buffer + len - LEN_SIMPLE) = '\0';
    len++;
  }
#endif
  send_command_packet((short)channel,NO_USERNR,len,(char *)&command);
}

static void deactivate_program(channel)
int channel;
{
  IFACE_CMDBUF command;
  
  command.command = CMD_DEACTIVATE;
  send_command_packet((short)channel,if_stat[channel].usernr,
                      LEN_SIMPLE,(char *)&command);
}

/* write data to interface */
void write_iface(channel,len,str)
int channel;
int len;
char *str;
{
  int iface;
  char buffer[MAX_LEN];
  int res;
  char *ptr;
  struct buf_entry *b_ptr;
  struct queue_entry *q_ptr;
  IFACE_HEADER header;

  if (((iface = if_stat[channel].iface) == -1) || 
      (len > IFACE_PACLEN)) return;

  if (if_list[iface].active != IF_ACTIVE) return;  

  /* No response to activate program was received, queue all frames */  
  if (channel != NO_CHANNEL) {
    if (if_stat[channel].usernr == NO_USERNR) {
      b_ptr = (struct buf_entry *)malloc(sizeof(struct buf_entry));
      if (b_ptr == NULL) {
        close_iface(iface,0,1,1);
        return;
      }
      ptr = (char *)malloc(len);
      if (ptr == NULL) {
        free(b_ptr);
        close_iface(iface,0,1,1);
        return;
      }
      memcpy(ptr,str,len);
      b_ptr->buf = ptr;
      b_ptr->len = len;
      b_ptr->next = NULL;
      if (if_stat[channel].first_q == NULL) {
        if_stat[channel].first_q = b_ptr;
      }
      else {
        if_stat[channel].last_q->next = b_ptr;
      }
      if_stat[channel].last_q = b_ptr;
      return;
    }
  }

  header.indicator = IF_DATA;
  header.channel = (short) channel;
  if (channel == NO_CHANNEL)
    header.usernr = NO_USERNR;
  else
    header.usernr = if_stat[channel].usernr;
  header.len = len;
  memcpy(buffer,(char *)&header,HEAD_LEN);
  memcpy(&buffer[HEAD_LEN],str,len);
  if (if_list[iface].blocked) {
    q_ptr = (struct queue_entry *)malloc(sizeof(struct queue_entry));
    if (q_ptr == NULL) {
      close_iface(iface,0,1,1);
      return;
    }
    ptr = (char *)malloc(len+HEAD_LEN);
    if (ptr == NULL) {
      free(q_ptr);
      close_iface(iface,0,1,1);
      return;
    }
    memcpy(ptr,buffer,len+HEAD_LEN);
    q_ptr->buffer = ptr;
    q_ptr->len = len+HEAD_LEN;
    q_ptr->next = NULL;
    if (if_list[iface].first_q == NULL) {
      if_list[iface].first_q = q_ptr;
    }
    else {
      if_list[iface].last_q->next = q_ptr;
    }
    if_list[iface].last_q = q_ptr;
  }
  else {
    if ((res = write(if_list[iface].sockfd,
                   buffer,len+HEAD_LEN)) < len+HEAD_LEN) {
      close_iface(iface,0,1,1);
      return;
    }
    blocking_test(iface,len+HEAD_LEN);
  }
}

void flush_buf(channel,forget)
int channel;
int forget;
{
  struct buf_entry *b_ptr;
  
  if (if_stat[channel].iface == -1) return;
   
  while (if_stat[channel].first_q != NULL) {
    b_ptr = if_stat[channel].first_q;
    if_stat[channel].first_q = b_ptr->next;
    if (!forget && (if_stat[channel].usernr != NO_USERNR)) {
      write_iface(channel,b_ptr->len,b_ptr->buf);
    }
    free(b_ptr->buf);
    free(b_ptr);
  }
  if_stat[channel].last_q = NULL;
}

/* read data from interface */
static int read_data_iface(iface)
int iface;
{
  char buffer[MAX_LEN];
  char *bufptr;
  int len;
  int remain;
#ifdef IFACE_DEBUG
  unsigned short tmplen;
#endif
  

  if (if_list[iface].active != IF_ACTIVE) return(0);  
  len = read(if_list[iface].sockfd,buffer,MAX_LEN);
  if (len == -1) {
    if (errno != EAGAIN) {
      close_iface(iface,0,1,1);
    }
    return(0);
  }
  if (len == 0) {
    close_iface(iface,0,1,1);
  }
  bufptr = buffer;
  remain = len;
  while (remain) {
    if (if_list[iface].status < HEAD_LEN) {
      *(((char *)&if_list[iface].header) + if_list[iface].status) = *bufptr;
      if_list[iface].status++;
      remain--;
      bufptr++;
    }
    else {
      /* packet data */
      if (if_list[iface].header.len > 0) {
        if_list[iface].buffer[if_list[iface].status - HEAD_LEN] = *bufptr;
        if_list[iface].status++;
      }
      if ((if_list[iface].status - HEAD_LEN) == if_list[iface].header.len) {
        if_list[iface].status = 0;
#ifdef IFACE_DEBUG
        tmplen = 0;
        fprintf(stderr,"IF: %d, ind: %d, ch: %d, us: %d, len: %d\n",
                iface,
                if_list[iface].header.indicator,
                if_list[iface].header.channel,
                if_list[iface].header.usernr,
                if_list[iface].header.len);
        while (tmplen < if_list[iface].header.len) {
          fprintf(stderr,"%x ",if_list[iface].buffer[tmplen]);
          tmplen++;
        }
        fprintf(stderr,"\n");
#endif
        packet_analysis(if_list[iface].header.indicator,
                        if_list[iface].header.channel,
                        if_list[iface].header.usernr,
                        if_list[iface].header.len,
                        if_list[iface].buffer,
                        iface);
      }
      remain--;
      bufptr++;
    }
  }
  return(1);
}

/* dummy proc for alarm */
void sigalarm()
{
}

/* install link to other program */
void cmd_iface(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int sockfd;
  int servlen;
  struct sockaddr_un serv_addr;
  int iface;

  /* search, if iface already active or trying to connect */
  iface = 0;
  while (iface < MAX_IFACE) {
    if (if_list[iface].active != IF_NOTACTIVE) {
      if (strcmp(if_list[iface].name,str) == 0) {
        if (if_list[iface].active == IF_TRYING)
          break;
        cmd_display(mode,channel,iface_open_txt,1);
        return;
      }
    }
    iface++;
  }
  /* if iface not trying, find new entry */
  if (iface == MAX_IFACE) {  
    iface = 0;
    while (iface < MAX_IFACE) {
      if (if_list[iface].active == IF_NOTACTIVE) {
        break;
      }
      iface++;
    }
  }  
  if (iface == MAX_IFACE) {
    /* no iface entry left, exit */
    cmd_display(mode,channel,iface_err_txt,1);
    return;
  }

  if_list[iface].active = IF_TRYING;
  strcpy(if_list[iface].name,str);

  /* fill structure "serv_addr" */
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path,str);
/*servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);*/
  servlen = sizeof(serv_addr);  /* VK5ABN */
  /* open socket */
  if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
    cmd_display(mode,channel,sock_err_txt,1);
    return;
  }
  signal(SIGALRM,sigalarm);
  alarm(2);
  /* connect other program */
  if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
    close(sockfd);
    cmd_display(mode,channel,conn_err_txt,1);
    signal(SIGALRM,SIG_IGN);
    return;
  }
  signal(SIGALRM,SIG_IGN);
  if (!use_select) {
    fcntl(sockfd,F_SETFL,O_NONBLOCK);
  }
  if_list[iface].active = IF_ACTIVE;
  if_list[iface].sockfd = sockfd;
  if_list[iface].status = 0;
  if_list[iface].blocked = 0;
  if_list[iface].sendlength = 0;
  if_list[iface].unproto_address[0] = '\0';
  strcpy(if_list[iface].name,str);
  if (strcmp(if_list[iface].name,box_socket) == 0) {
    box_active_flag = 1;
    box_busy_flag = 0;
    box_iface = iface;
  }
#ifndef DPBOXT
  if (strcmp(if_list[iface].name,node_socket) == 0) {
    node_active_flag = 1;
    node_iface = iface;
  }
  send_qrginfo(iface);
#endif
  cmd_display(mode,channel,OK_TEXT,1);
}

/* cut link to other program */
void cmd_endiface(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int iface;
  
  iface = 0;
  while (iface < MAX_IFACE) {
    if ((if_list[iface].active == IF_ACTIVE) ||
        (if_list[iface].active == IF_TRYING)) {
      if (strcmp(if_list[iface].name,str) == 0) {
        if (if_list[iface].active == IF_ACTIVE)
          close_iface(iface,1,1,0);
        else
          if_list[iface].active = IF_NOTACTIVE;
        cmd_display(mode,channel,OK_TEXT,1);
        return;
      }
    }
    iface++;
    if (iface == MAX_IFACE) {
      cmd_display(mode,channel,end_err_txt,1);
      return;
    }
  }
}

/* finish execution of other program */
void cmd_finiface(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int iface;
  IFACE_CMDBUF command;
  
  iface = 0;
  while (iface < MAX_IFACE) {
    if ((if_list[iface].active == IF_ACTIVE) ||
        (if_list[iface].active == IF_TRYING)) {
      if (strcmp(if_list[iface].name,str) == 0) {
        if (if_list[iface].active == IF_ACTIVE) {
          command.command = CMD_FINISH_PRG;
          send_command_packet_if(NO_CHANNEL,NO_USERNR,
                                 LEN_SIMPLE,(char *)&command,iface);
          close_iface(iface,0,1,0);
        }
        else
          if_list[iface].active = IF_NOTACTIVE;
        cmd_display(mode,channel,OK_TEXT,1);
        return;
      }
    }
    iface++;
    if (iface == MAX_IFACE) {
      cmd_display(mode,channel,end_err_txt,1);
      return;
    }
  }
}

/* activate link to other program */
void cmd_actiface_ext(par1,par2,par3,channel,len,mode,str)
int par1;
int par2;
int par3;
int channel;
int len;
int mode;
char *str;
{
  int iface;
#ifndef DPBOXT
  char boxcall[10];
  char *ptr;
  char ans_str[80];
  int ans_len;
  int flag;
#endif
  
  if (mode == M_MAILBOX) {
    channel = 0;
  }
#ifndef DPBOXT
  else {
    if (shell_active(channel)) {
      cmd_display(mode,channel,sh_active_txt,1);
      return;
    }
  }
#endif
  
  if (if_stat[channel].iface != -1) {
    cmd_display(mode,channel,iface_active_txt,1);
    return;
  }
  iface = 0;
  while (iface < MAX_IFACE) {
    if (if_list[iface].active == IF_ACTIVE) {
      if (strcmp(if_list[iface].name,str) == 0) {
        if_stat[channel].iface = iface;
        if_stat[channel].usernr = NO_USERNR;
        if_stat[channel].first_q = NULL;
        if_stat[channel].last_q = NULL;
        if_stat[channel].direct_started = par1;
        if_stat[channel].box_connect = 
          (strcmp(if_list[iface].name,box_socket) == 0);
#ifndef DPBOXT
        if_stat[channel].node_connect = 
          (strcmp(if_list[iface].name,node_socket) == 0);
#endif
#ifdef DPBOXT
        activate_program(channel);
        cmd_display(mode,channel,"Activating program",1);
        mb_stat_display();
#else
        if ((par2 == 1) && (channel != 0)) {
          start_sf(channel,iface);
          cmd_display(mode,channel,"Starting S&F",1);
        }
        else if ((par2 == 2) && (channel != 0)) {
          success_connect(iface,par3,channel);
          cmd_display(mode,channel,"Connect successful",1);
        }
        else {
          activate_program(channel);
          if ((mode == M_REMOTE) &&
              (if_stat[channel].direct_started == 0) &&
              (if_stat[channel].box_connect == 1)) {
            strcpy(boxcall,ch_stat[channel].curcall);
            ptr = strchr(boxcall,'-');
            if (ptr != NULL) *ptr = '\0';
            if (tnt_box_ssid != 0)
              sprintf(ans_str,"TNT:%s> Connected to %s-%d\r",
                      ch_stat[channel].curcall,boxcall,tnt_box_ssid);
            else
              sprintf(ans_str,"TNT:%s> Connected to %s\r",
                      ch_stat[channel].curcall,boxcall);
            flag = 0;
            ans_len = strlen(ans_str);
            rem_data_display(channel,ans_str);
            queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
          }
          else {
            cmd_display(mode,channel,"Activating program",1);
          }
        }
        if (channel == 0)
          mb_stat_display();
#endif
        return;
      }
    }
    iface++;
    if (iface == MAX_IFACE) {
      cmd_display(mode,channel,end_err_txt,1);
#ifndef DPBOXT
      if (par1) {
        cmd_display(M_INTERFACE,channel,noconn_text,1);
        end_comp(channel);
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
      }
#endif
      return;
    }
  }
}

/* activate link to other program */
void cmd_actiface(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_actiface_ext(par1,par2,0,channel,len,mode,str);
}

/* deactivate link to other program */
void cmd_deactiface(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int iface;

  if (mode == M_MAILBOX) channel = 0;  
  iface = 0;
  while (iface < MAX_IFACE) {
    if (if_list[iface].active == IF_ACTIVE) {
      if (strcmp(if_list[iface].name,str) == 0) {
        close_iface_con(channel,1,1);
        cmd_display(mode,channel,OK_TEXT,1);
        return;
      }
    }
    iface++;
    if (iface == MAX_IFACE) {
      cmd_display(mode,channel,end_err_txt,1);
      return;
    }
  }
}

void iface_fdset(max_fd,fdmask)
int *max_fd;
fd_set *fdmask;
{
  int iface;
  int fd;
  
  for (iface=0;iface<MAX_IFACE;iface++) {
    if (if_list[iface].active == IF_ACTIVE) {
      fd = if_list[iface].sockfd;
      FD_SET(fd,fdmask);
      if (fd > ((*max_fd) - 1)) {
        *max_fd = fd + 1;
      }
    }
  }
#ifndef DPBOXT
  if (tnt_daemon && !frontend_active) {
    FD_SET(frontend_sockfd,fdmask);
    if (frontend_sockfd > ((*max_fd) - 1)) {
      *max_fd = frontend_sockfd + 1;
    }
  }
#endif
}

int iface_receive(fdmask)
fd_set *fdmask;
{
  int iface;
  int result;
  
  result = 1;
  for (iface=0;iface<MAX_IFACE;iface++) {
    if (if_list[iface].active == IF_ACTIVE) {
      if (use_select) {
        if (FD_ISSET(if_list[iface].sockfd,fdmask)) {
          read_data_iface(iface);
          result = 0;
        }
      }
      else {
        if (read_data_iface(iface)) {
          result = 0;
        }
      }
    }
  }
#ifndef DPBOXT
  if (tnt_daemon && !frontend_active) {
    if (use_select) {
      if (FD_ISSET(frontend_sockfd,fdmask)) {
        clilen = sizeof(cli_addr);
        frontend_fd = accept(frontend_sockfd,
                             (struct sockaddr *) &cli_addr,
                             &clilen);
        if (frontend_fd >= 0) {
          frontend_active = 2;
          frontend_para[0] = '\0';
        }
        result = 0;
      }
    }
    else {
      clilen = sizeof(cli_addr);
      frontend_fd = accept(frontend_sockfd,
                           (struct sockaddr *) &cli_addr,
                           &clilen);
      if (frontend_fd >= 0) {
        fcntl(frontend_fd,F_SETFL,O_NONBLOCK);
        frontend_active = 2;
        frontend_para[0] = '\0';
        result = 0;
      }
    }
  }
#endif
  return(result);
}

void iface_trying()
{
  int iface;
  
  if ((time(NULL) - last_iface_try) < 120) return;
  for (iface=0;iface<MAX_IFACE;iface++) {
    if (if_list[iface].active == IF_TRYING) {
#ifdef DPBOXT
      cmd_display(M_MAILBOX,0,iface_trying_txt,1);
      cmd_iface(0,0,0,strlen(if_list[iface].name),
                      M_MAILBOX,
                      if_list[iface].name);
#else
      cmd_display(M_COMMAND,0,iface_trying_txt,1);
      cmd_iface(0,0,0,strlen(if_list[iface].name),
                      M_COMMAND,
                      if_list[iface].name);
#endif
    }
  }
  last_iface_try = time(NULL);
}

/* returns if iface active on channel */
int iface_active(channel)
int channel;
{
  return(if_stat[channel].iface != -1);
}

/* returns if box active on channel */
int box_active(channel)
int channel;
{
  if (if_stat[channel].iface == -1)
    return(0);
  return(if_stat[channel].box_connect);
}

#ifndef DPBOXT
/* returns if node active on channel */
int node_active(channel)
int channel;
{
  if (if_stat[channel].iface == -1)
    return(0);
  return(if_stat[channel].node_connect);
}

short give_usernr(channel)
int channel;
{
  if (if_stat[channel].iface == -1)
    return(NO_USERNR);
  return((short)if_stat[channel].usernr);
}
#endif

/* init of iface-variables */
void init_iface()
{
  int i;
  
  for (i=0;i<MAX_IFACE;i++) {
    if_list[i].active = IF_NOTACTIVE;
  }
#ifdef DPBOXT
  if_stat[0].iface = -1;
#else
  for (i=0;i<tnc_channels;i++) {
    if_stat[i].iface = -1;
  }
#endif
  strcpy(iface_err_txt,"All interfaces in use");
  strcpy(iface_open_txt,"Interface already open");
  strcpy(sock_err_txt,"Can't open stream socket, will try later again");
  strcpy(conn_err_txt,"Can't connect to program, will try later again");
  strcpy(end_err_txt,"Interface not active");
  strcpy(iface_active_txt,"Interface already active");
  strcpy(iface_trying_txt,"Try to connect to interface");
#ifndef DPBOXT
  strcpy(back_tnt_txt,"Back at TNT");
  strcpy(noconn_text,"Program not active");
#endif

  *boxconsole_call = '\0';
  box_active_flag = 0;
  box_busy_flag = 0;
  node_active_flag = 0;  
  last_iface_try = time(NULL);
#ifndef DPBOXT
  accept_uireq_flag = 0;
  accuicall[0] = '\0';
  init_monbox();
  init_boxcut();
#endif
  signal(SIGALRM,SIG_IGN);
}

/* exit of iface, close all sockets */
void exit_iface()
{
  int i;

#ifndef DPBOXT
  exit_boxcut();
  exit_monbox();
#endif
    
  for (i=0;i<MAX_IFACE;i++) {
    close_iface(i,1,1,0);
  }
  signal(SIGALRM,SIG_IGN);
}

/* activate box */
void cmd_actbox(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_iface(par1,par2,channel,strlen(box_socket),mode,box_socket);
}

/* deactivate box */
void cmd_deactbox(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_endiface(par1,par2,channel,strlen(box_socket),mode,box_socket);
}

/* finish box program */
void cmd_finbox(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_finiface(par1,par2,channel,strlen(box_socket),mode,box_socket);
}

/* start box */
void cmd_box(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int i;
  
  *boxconsole_call = '\0';
  if (str != NULL) {
    if (*str != '\0') {
      for (i = 0;str[i] != '\0';i++) boxconsole_call[i] = toupper(str[i]);
      boxconsole_call[strlen(str)] = '\0';
    }
  }
  cmd_actiface(par1,par2,channel,strlen(box_socket),mode,box_socket);
}

/* end box */
void cmd_endbox(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_deactiface(par1,par2,channel,strlen(box_socket),mode,box_socket);
}

/* activate node */
void cmd_actnode(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_iface(par1,par2,channel,strlen(node_socket),mode,node_socket);
}

/* deactivate node */
void cmd_deactnode(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_endiface(par1,par2,channel,strlen(node_socket),mode,node_socket);
}

/* finish node program */
void cmd_finnode(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_finiface(par1,par2,channel,strlen(node_socket),mode,node_socket);
}

/* start node */
void cmd_node(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_actiface(par1,par2,channel,strlen(node_socket),mode,node_socket);
}

/* end node */
void cmd_endnode(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_deactiface(par1,par2,channel,strlen(node_socket),mode,node_socket);
}

/* looking for iface of name str, if found, return iface-nr, else -1 */
int conv_name_to_iface(str)
char *str;
{
  int iface;
  iface = 0;
  while (iface < MAX_IFACE) {
    if (if_list[iface].active == IF_ACTIVE) {
      if (strcmp(if_list[iface].name,str) == 0) {
        return(iface);
      }
    }
    iface++;
  }
  return(-1);
}

/* file string with socket name of iface */
void get_iface_name(iface,str)
int iface;
char *str;
{
  if (if_list[iface].active == IF_ACTIVE) {
    strcpy(str,if_list[iface].name);
  }
  else {
    str[0] = '\0';
  }
}

/* procedures for mailbox-user-frontend */

void sel_mailbox()
{
  if (act_mode == M_MAILBOX) return;
  real_screen(mb_layout);
  act_mode = M_MAILBOX;
  statlin_update();
}

int mb_newline(str)
char *str;
{
  int len;
  
  len = get_line(str,&mb_prewin);
  win_charout(CR,&mb_prewin,0);
  return(len);
}

void mb_nextline()
{
  win_charout(CR,&mb_prewin,0);
}

void mb_charout(ch)
char ch;
{
  if (mb_prewin.column < input_linelen-1) {
    win_charout_cntl(ch,&mb_prewin);
  }
  else {
    beep();
  }
}

void mb_charout_nobnd(ch)
char ch;
{
  win_charout_cntl(ch,&mb_prewin);
}

void mb_charout_cntl(ch)
char ch;
{
  if (mb_prewin.column < input_linelen-1) {
    ch &= 0x1f;
    ch |= 0x40;
    win_attrib(att_controlchar,&mb_prewin);
    win_charout_cntl(ch,&mb_prewin);
    win_attrib(att_normal,&mb_prewin);
  }
  else {
    beep();
  }
}

int mb_lineend()
{
  return(mb_prewin.column == input_linelen-1);
}

int mb_wordwrap(str)
char *str;
{
  return(wordwrap(str,&mb_prewin));
}

void rem_mb_display_buf(buffer,len)
char *buffer;
int len;
{
  win_attrib(att_remote,&mb_textwin);
  win_textout_len(buffer,len,&mb_textwin,1);
  win_attrib(att_normal,&mb_textwin);
}

void mb_display_buf(buffer,len)
char *buffer;
int len;
{
  win_textout_len(buffer,len,&mb_textwin,1);
  write_mbfile(len,RX_RCV,buffer);
}

void mb_pre_display(buffer,cr)
char *buffer;
int cr;
{
  win_stringout(buffer,&mb_prewin,0);
  if (cr) win_charout(CR,&mb_prewin,0);
}

void mb_input(str,len)
char *str;
int len;
{
  write_iface(0,len,str);
}

void mb_stat_display()
{
  char *stat_str;
  int i;

  stat_str = (char *)malloc(COLS+2);
  strcpy(stat_str,"");
  strcat(stat_str,"\015**");
  strcat(stat_str,"************************************* ");
  if (if_stat[0].iface != -1)
    strcat(stat_str,"MAILBOX **************");
  else
    strcat(stat_str,"MAILBOX NOT CONNECTED ");
  strcat(stat_str,"******************");
  for (i=strlen(stat_str);i<COLS+1;i++) {
    stat_str[i] = '*';
  }
  stat_str[COLS+1] = '\0';
  win_stringout(stat_str,&mb_statwin,0);
  free(stat_str);
}

void init_mb()
{
  int j;
  
  if (((LINES/mbscr_divide-1) < 2) ||
      (mbscr_divide < 2 ) || (mbscr_divide > 40)) mbscr_divide = 3;
  for (j = 0; j < LAYOUTPARTS; j++) {
    mb_layout[j].win = NULL;
    mb_layout[j].first_real_line = 0;
    mb_layout[j].win_num_lines = 0;
    mb_layout[j].pagesize = 0;
  }
  open_window(lines_mbinput,&mb_prewin,1);
  open_window(1,&mb_statwin,1);
  win_attrib(att_cstatline,&mb_statwin);
  open_window(lines_mboutput,&mb_textwin,1);
  mb_layout[0].win = &mb_prewin;
  mb_layout[0].first_real_line = 0;
  mb_layout[0].win_num_lines = LINES/mbscr_divide-1;
  mb_layout[0].pagesize = LINES/mbscr_divide-2;
  mb_layout[1].win = &mb_statwin;
  mb_layout[1].first_real_line = LINES/mbscr_divide-1;
  mb_layout[1].win_num_lines = 1;
  mb_layout[1].pagesize = 0;
  mb_layout[2].win = &mb_textwin;
  mb_layout[2].first_real_line = LINES/mbscr_divide;
  mb_layout[2].win_num_lines = LINES-LINES/mbscr_divide-1;
  mb_layout[2].pagesize = LINES-LINES/mbscr_divide-4;
  mb_layout[3].win = &statlin;
  mb_layout[3].first_real_line = LINES-1;
  mb_layout[3].win_num_lines = 1;
  mb_layout[3].pagesize = 0;
 
  mb_stat_display();
#ifdef DPBOXT
  real_screen(mb_layout);
  statlin_update();
  cmd_actbox(0,0,0,0,M_CMDSCRIPT,NULL);
#endif
}

void reinit_mb()
{
  int j;
  
  if (((LINES/mbscr_divide-1) < 2) ||
      (mbscr_divide < 2 ) || (mbscr_divide > 40)) mbscr_divide = 3;
  for (j = 0; j < LAYOUTPARTS; j++) {
    mb_layout[j].win = NULL;
    mb_layout[j].first_real_line = 0;
    mb_layout[j].win_num_lines = 0;
    mb_layout[j].pagesize = 0;
  }
  mb_layout[0].win = &mb_prewin;
  mb_layout[0].first_real_line = 0;
  mb_layout[0].win_num_lines = LINES/mbscr_divide-1;
  mb_layout[0].pagesize = LINES/mbscr_divide-2;
  mb_layout[1].win = &mb_statwin;
  mb_layout[1].first_real_line = LINES/mbscr_divide-1;
  mb_layout[1].win_num_lines = 1;
  mb_layout[1].pagesize = 0;
  mb_layout[2].win = &mb_textwin;
  mb_layout[2].first_real_line = LINES/mbscr_divide;
  mb_layout[2].win_num_lines = LINES-LINES/mbscr_divide-1;
  mb_layout[2].pagesize = LINES-LINES/mbscr_divide-4;
  mb_layout[3].win = &statlin;
  mb_layout[3].first_real_line = LINES-1;
  mb_layout[3].win_num_lines = 1;
  mb_layout[3].pagesize = 0;
}

void exit_mb()
{
  close_iface_con(0,1,0);
  close_window(&mb_prewin);
  close_window(&mb_statwin);
  close_window(&mb_textwin);
}

#ifndef DPBOXT
void free_iface()
{
  free(if_stat);
}

int alloc_iface()
{
  if_stat = 
    (struct iface_stat *)malloc(tnc_channels * sizeof(struct iface_stat));
  return(if_stat == NULL);
}
#endif

#endif
