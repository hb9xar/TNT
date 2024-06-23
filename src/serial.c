/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for serial port (serial.c)
   created: Mark Wahl DL4YBG 93/07/24
   updated: Mark Wahl DL4YBG 97/04/06
   updated: Matthias Hensler WS1LS 99/03/12
   updated: Berndt Josef Wulf VK5ABN 99/03/12
*/

#include "tnt.h"
#include "macro.h"

#ifdef HAVE_SOCKET
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
#endif
#ifndef SOCK_STREAM
#include <socketbits.h>
#endif

#include <ctype.h>

#undef HOSTDEBUG
#undef LOOK_OVERRUN

/* number of frames fetched from monitor before changing to other channel */
#define NUM_MONFRAMES 30
/* number of poll-cycles before data is resent to a former busy TNC */
#define WAIT_BUSY 40
/* number of xhost-polls before a status of all channels is requested */
#define NUM_XHOST_POLLS 40
/* minimum number of buffers for sending of data to tnc */
#define MINBUFFERS 250
/* minimum number of buffers for sendframe calculation */
#define ABSMINBUFFERS 100
/* number of frames to block interface */
#define MAX_SND_FRMS 7
/* number of broadcast-frames after which a normal frame is sent */
#define MAXBCASTTX 14

/* actual channel and mode */
extern int act_state;
extern int soft_tnc;
#ifndef DPBOXT
extern int tnc_channels;
#endif
extern int act_channel;
extern int act_mode;
extern char tnt_lockfile[];
extern int cookie_flag;
extern int ctext_flag;
extern int noacc_flag;
extern char resy_log_file[];
#ifdef DEBUGIO
extern char debugiof[] ; 
extern int fddebugio ; 
#endif
extern int box_active_flag;
extern int box_busy_flag;
extern int node_active_flag;
extern int altstat;
extern int use_select;
extern int ax25k_active;

#ifdef TNT_SOUND
extern int play_sound(int);
#endif
extern int xconnect_active(int channel);
extern void strip_call(char *call,int channel);
extern void strip_call_log(char *call,int channel);
extern int macro_getname(char *call,char *name);
extern int is_notown(char *callsign);
extern void statlin_update();
extern int xconnect_first(int channel);
extern void close_xconnect(int channel, int success);
extern void close_shell(int channel,int report,int disc);
extern void close_rxfile(int channel,int report);
extern void close_txfile(int channel,int report);
extern void clear_pwmode(int channel);
extern void write_log(int channel);
extern void cmd_display(int flag,int channel,char *buffer,int cr);
extern void next_command(int *state);
extern void open_upfile(int *state);
extern void stat_display(int channel);
extern void next_sendframe(int channel);
extern void set_pwmode(int channel);
extern void set_remmode(int channel);
extern void set_flchkmode(int channel);
#ifdef USE_IFACE
extern void close_iface_con(int channel,int deact,int disconnect);
extern void conn_boxcut(int channel);
extern void disc_boxcut(int channel);
extern void send_cstatus(int channel, int frames);
extern void cmd_box(int par1,int par2,int channel,
                    int len,int mode,char *str);
extern void rem_nobox(int channel);
extern void cmd_node(int par1,int par2,int channel,
                     int len,int mode,char *str);
extern void rem_nonode(int channel);
#endif
extern int do_autostart(char *callsign,int channel);
extern int rem_noacc(int channel);
extern void rem_ctext(int par1,int par2,int channel,
                      int len,int mode,char *str);
extern void rem_cookie(int par1,int par2,int channel,
                       int len,int mode,char *str);
extern void data_display(int channel,char *buffer);
extern void next_connect(int channel);
extern void moni_display(int channel, char *buffer);
extern void moni_display_len(int channel, char *buffer);
extern int decstathuf(char *src,char *dest);
extern void data_display_len(int channel, char *buffer);
extern int encstathuf(char *src,int srclen,char *dest,int *destlen);
extern void xcon_reset_first(int channel);
#ifdef HAVE_SOCKET
extern struct sockaddr *build_sockaddr(const char *name, int *addrlen);
#endif

#ifdef BCAST
extern void (*func_callback)(short chan, short free);
#endif

#ifdef USE_AX25K
extern void write_ax25k(int channel,char *buffer,int len);
extern void command_ax25k(int channel,char *data,int len);
#endif

static void send_block(int channel,char code,int len,char *data);
int queue_cmd_data(int channel,char code,int len,int flag,char *data);
int calc_xframes(int channel);

#ifdef HOSTDEBUG
#define HOSTBUFLEN 512

struct char_time {
  char ch;
  time_t time;
};

/* buffer for hostmode-tx-chars */
static struct char_time host_tx[HOSTBUFLEN];
static int host_tx_start;
static int host_tx_len;

/* buffer for hostmode-rx-chars */
static struct char_time host_rx[HOSTBUFLEN];
static int host_rx_start;
static int host_rx_len;
#endif

/* status of all channels */
struct channel_stat *ch_stat;
/* file descriptor for serial interface */
int serial;
/* file descriptor for lockfile */
int lock;
/* flag if resync active */
int resync;
/* flag if tnc busy */
int tnc_busy;
/* free buffers in tnc */
int free_buffers;
/* flag if buffers requested */
int buffers_req;
/* counter for resyncs */
int resync_count;
/* SSID of mailbox */
int tnt_box_ssid;
/* callsign/SSID of mailbox */
char tnt_box_call[10];
/* SSID of node */
int tnt_node_ssid;
/* callsign/SSID of node */
char tnt_node_call[10];
/* disconnect on startup */
int disc_on_start;
/* status errors */
struct staterr staterr;
/* type of send-queue */
int send_queue_type;
/* number of bcast-frames sent after last normal frame */
int bcasttx_frames;
/* an error occurred on interface to soft_tnc */
int softtnc_error;

#ifdef HAVE_SOCKET
/* wait after each transmitted paket to tfkiss */
int fixed_wait;
/* wait after 'amount_wait' characters transmitted to tfkiss */
int amount_wait;
#endif

#ifndef HAVE_USLEEP
  struct timeval timevalue;
#endif

/* txqueue for blocks to tnc */
static int block_buffer_len;	/* number of blocks in queue */
static struct block_buffer_ptr *block_buffer_root;
static struct block_buffer_ptr *block_buffer_last;

/* txqueue for blocks to tnc (broadcast) */
static int block_bcbuffer_len;	/* number of blocks in queue */
static struct block_buffer_ptr *block_bcbuffer_root;
static struct block_buffer_ptr *block_bcbuffer_last;

static int rcv_stat;
static int rcv_len;
static int rcv_pos; 
static char txbuffer[259];
static char txbuffer_save[259];
static char rxbuffer[259];
static int poll_channel;
static int poll_cycle;
static int req_info;
int req_flag;
static int req_info_save;
static int req_flag_save;
static int busy_count;
static int len_save;
static int ok_display;
static struct termios org_termios;
static struct termios wrk_termios;
static char xhost_list[259];
static char *xhost_ptr;
static int xhost_ava;
static int xhost_polls;
static int sent_bytes;
int softtnc_error;

static char hostmode_str[] = "\021\030\022\030\033JHOST 1\015"; 
                             /* ^Q^X^R^XESC at the beginning */
static char hostmode_exit[] = "\0\001\006JHOST 0";

static char resync_string[] = "\001\002INVALID COMMAND: ";

/* strings for broadcast push/pop functions */
static char pushed_pid[256];
static char pushed_monstat[256];
static char pushed_unproto[256];

#ifdef HOSTDEBUG
void debug_copy_rx(char *buffer, int len)
{
  int i;
  int hptr;

  for (i = 0;i < len;i++) {
    hptr = host_rx_start + host_rx_len;
    if (hptr >= HOSTBUFLEN) hptr -= HOSTBUFLEN;
    if (host_rx_len < HOSTBUFLEN) {
      host_rx_len++;
    }
    else {
      host_rx_start++;
      if (host_rx_start >= HOSTBUFLEN) host_rx_start -= HOSTBUFLEN;
    }
    host_rx[hptr].ch = buffer[i];
    host_rx[hptr].time = time(NULL);
  }
} 

void debug_copy_tx(char *buffer, int len)
{
  int i;
  int hptr;

  for (i = 0;i < len;i++) {
    hptr = host_tx_start + host_tx_len;
    if (hptr >= HOSTBUFLEN) hptr -= HOSTBUFLEN;
    if (host_tx_len < HOSTBUFLEN) {
      host_tx_len++;
    }
    else {
      host_tx_start++;
      if (host_tx_start >= HOSTBUFLEN) host_tx_start -= HOSTBUFLEN;
    }
    host_tx[hptr].ch = buffer[i];
    host_tx[hptr].time = time(NULL);
  }
} 
#endif

/* wait time us (linux: time raster 10 ms) */
void uwait(unsigned long time)
{
#ifdef HAVE_USLEEP
  usleep(time); /* sleep for 10ms */
#else
  timevalue.tv_usec = time; /* sleep for 10ms */
  timevalue.tv_sec = 0;
  select(0,(fd_set *) 0,
           (fd_set *) 0,
           (fd_set *) 0,
           &timevalue);
#endif
}

#ifndef DPBOXT
/* find a not connected channel, return -1 if none found */
int find_free_channel()
{
  int i;
  
  for (i=1;i<tnc_channels;i++) {
    if ((ch_stat[i].conn_state == CS_DISCON) && (!xconnect_active(i))) {
      return(i);
    }
  }
  return(-1);
}
#endif

#ifndef DPBOXT
#ifdef USE_AX25K
/* find a not connected channel, return -1 if none found */
int find_call_channel(call)
char *call;
{
  int i;
  
  for (i=1;i<tnc_channels;i++) {
    if ((ch_stat[i].conn_state == CS_DISCON) && 
        (!xconnect_active(i)) &&
        (strcmp(ch_stat[i].curcall,call) == 0)) {
      return(i);
    }
  }
  return(-1);
}
#endif
#endif

#ifndef DPBOXT
/* check if calls already used for connection */
static int check_double_conn(channel,srccall,destcall)
int channel;
char *srccall;
char *destcall;
{
  int i;
  char cmpcall[10];

  for (i=1;i<tnc_channels;i++) {
    if ((i != channel) && (ch_stat[i].conn_state != CS_DISCON)) {
      strip_call(cmpcall,i);
      if ((strcmp(destcall,cmpcall) == 0) &&
          (strcmp(srccall,ch_stat[i].curcall) == 0)) {
        /* double connection found */
        return(1);
      }
    }
  }
  /* no double connection found */
  return(0);
}

/* update own call on channel */
void update_owncall(channel,call)
int channel;
char *call;
{
  if ((channel < 1) || (channel >= tnc_channels) || (strlen(call) > 9))
    return;
  strcpy(ch_stat[channel].curcall,call);
}

/* update remote call on channel */
void update_remcall(channel,call)
int channel;
char *call;
{
  if ((channel < 1) || (channel >= tnc_channels) || (strlen(call) > 256))
    return;
  strcpy(ch_stat[channel].call,call);
}

void update_name(channel)
int channel;
{
  char call[10];
  char namestr[90];
  
  strip_call_log(call,channel);
  if ((macro_getname(call,namestr) == TNT_OK) && (strcmp(call,namestr) != 0)) {
    strcpy(ch_stat[channel].name,namestr);
  }
  else {
    ch_stat[channel].name[0] = '\0';
  }
}

/* update SSID if calls used for connection */
void update_ssid(channel,destcall)
int channel;
char *destcall;
{
  int i;
  char srccall_ssid[10];
  char srccall[10];
  char destcall_strip[10];
  char *dashptr;
  int call_ok;
  int ssid;
  int new_ssid;
  char cmdstr[256];
  int end;
  char ch;
  int offset;
  char *ptr;

  call_ok = 1;  
  /* copy own call */
  strcpy(srccall_ssid,ch_stat[channel].curcall);
  for (i=0;i<strlen(srccall_ssid);i++)
    srccall_ssid[i] = toupper(srccall_ssid[i]);
  strcpy(srccall,srccall_ssid);
  /* cut into call and ssid */
  dashptr = strchr(srccall,'-');
  if (dashptr != NULL) {
    ssid = atoi(dashptr + 1);
    *dashptr = '\0';
    if ((ssid < 0) || (ssid > 15)) { /* should not happen, but anyway */
      ssid = 0;
      new_ssid = 0;
      call_ok = 0;
      strcpy(srccall_ssid,srccall);
    }
  }
  else {
    ssid = 0;
  }
  
  /* remove channelnumber in front of callsign */
  offset = 0;
  ptr = strchr(destcall,':');
  if (ptr != NULL) {
    offset = ptr + 1 - destcall;
  }
  i = 0;
  end = 0;
  while (!end) {
    ch = toupper(destcall[i+offset]);
    if ((ch == ' ') || (ch == '\0') || (i > 8)) {
      ch = '\0';
      end = 1;
    }
    destcall_strip[i] = ch;
    i++;
  }
  
  for (;;) {
    if (check_double_conn(channel,srccall_ssid,destcall_strip) ||
        is_notown(srccall_ssid)) {
      if (call_ok) { /* first try with call of channel */
        new_ssid = 0;
        call_ok = 0;
      }
      else { /* next tries with new ssid */
        new_ssid++;
      }
      if (new_ssid == ssid) {
        new_ssid++;
      }
      if (new_ssid > 15)
        return; /* all ssids in use, leave ssid as it is */
      if (new_ssid)
        sprintf(srccall_ssid,"%s-%d",srccall,new_ssid);
      else
        strcpy(srccall_ssid,srccall);
    }
    else { /* free ssid found, update call if needed */
      if (!call_ok) { /* update own callsign */
        strcpy(cmdstr,"I");
        strcat(cmdstr,srccall_ssid);
        update_owncall(channel,srccall_ssid);
        queue_cmd_data(channel,X_COMM,strlen(cmdstr),M_CMDSCRIPT,cmdstr);
      }
      return;
    }
  }
}

/* init of tx-queue */
static void restart_tx_queue()
{
  int i;
  
  block_buffer_len = 0;
  block_buffer_root = NULL;
  block_buffer_last = NULL;
  block_bcbuffer_len = 0;
  block_bcbuffer_root = NULL;
  block_bcbuffer_last = NULL;
  for (i = 0; i < tnc_channels; i++) {
    ch_stat[i].snd_queue_frms = 0;
  }
}
#endif

/* dummy proc for alarm */
static void sigalarm()
{
}

#ifndef DPBOXT
/* opens serial port and set tnc to hostmode
   serstr holds serial device name,
   speed holds baudrate */
int init_serial(serstr,speed,speedflag,unlock)
char *serstr;
unsigned int speed;
int speedflag;
int unlock;
{
  int l;
  char c;
  int len;
#ifdef USE_HIBAUD
  struct serial_struct ser_io;
#endif
#ifdef HAVE_SOCKET
  struct sockaddr_un serv_addr;
  struct sockaddr *saddr;
  int servlen;
#endif

  if (unlock) unlink(tnt_lockfile);
  if ((lock = open(tnt_lockfile,O_CREAT|O_EXCL,0600)) == -1) {
    printf("Error: serial port locked by other user"
           " or unable to create lockfile\n");
    return(1);
  }
  close(lock);
  if (soft_tnc) {
#ifdef HAVE_SOCKET
    if (soft_tnc == 2) {
      saddr = build_sockaddr(serstr,&servlen);
      if (!saddr) {
        printf("Error: invalid parameters in socket definition"
               " for soft_tnc\n");
        return(1);
      }
    }
    else {
      /* fill structure "serv_addr" */
      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sun_family = AF_UNIX;
      strcpy(serv_addr.sun_path,serstr);
/*    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);*/
      servlen = sizeof(serv_addr);
      saddr = (struct sockaddr *)&serv_addr;
    }
    /* open socket */
    if ((serial = socket(saddr->sa_family,SOCK_STREAM,0)) < 0) {
      printf("Error: Cannot open socket to soft_tnc\n");
      unlink(tnt_lockfile);
      return(1);
    }
    signal(SIGALRM,sigalarm);
    if (saddr->sa_family == AF_UNIX)
      alarm(2);
    else
      alarm(10);
    /* connect soft_tnc */
    if (connect(serial, saddr, servlen) < 0) {
      close(serial);
      printf("Error: Cannot connect to soft_tnc\n");
      signal(SIGALRM,SIG_IGN);
      unlink(tnt_lockfile);
      return(1);
    }
    signal(SIGALRM,SIG_IGN);
    fcntl(serial,F_SETFL,O_NONBLOCK);
#endif
  }
  else {
    if ((serial = open(serstr,O_RDWR)) == -1) {
      printf("Error: can't open serial port\n");
      unlink(tnt_lockfile);
      return(1);
    }
    tcgetattr(serial,&org_termios);
#ifdef USE_HIBAUD
    if (speed == B38400) {
      if (ioctl(serial,TIOCGSERIAL, &ser_io) < 0) {
        printf("Error: can't get serial info\n");
        close(serial);
        unlink(tnt_lockfile);
        return(1);
      }
    }
#endif
    wrk_termios = org_termios;
    wrk_termios.c_cc[VTIME] = 0;
    wrk_termios.c_cc[VMIN] = 0;
    wrk_termios.c_iflag = 0;
#ifdef LOOK_OVERRUN
    wrk_termios.c_iflag |= (INPCK|IGNBRK);
#else
    wrk_termios.c_iflag |= IGNBRK;
#endif
    wrk_termios.c_oflag = 0;
    wrk_termios.c_lflag = 0;
    wrk_termios.c_cflag |= (CS8|CREAD|CLOCAL);
#ifdef HAVE_CRTSCTS
    wrk_termios.c_cflag &= ~(CSTOPB|PARENB|PARODD|CRTSCTS|HUPCL);
#else
    wrk_termios.c_cflag &= ~(CSTOPB|PARENB|PARODD|HUPCL);
#endif
    cfsetispeed(&wrk_termios,speed);
    cfsetospeed(&wrk_termios,speed);
#ifdef USE_HIBAUD
    if (speed == B38400) {
      ser_io.flags &= ~ASYNC_SPD_MASK;
      ser_io.flags |= speedflag;
      if (ioctl(serial,TIOCSSERIAL, &ser_io) < 0) {
        printf("Error: can't set serial info\n");
        tcsetattr(serial,TCSADRAIN,&org_termios);
        close(serial);
        unlink(tnt_lockfile);
        return(1);
      }
    }
#endif
    tcsetattr(serial,TCSADRAIN,&wrk_termios);
  }
  
  /* put tnc to hostmode */
  len = sizeof(hostmode_str) - 1;
  if (write(serial,hostmode_str,len) != len){
    printf("Error: can't write to serial port\n");
    return(1);
  }
#ifdef DEBUGIO
  wrdebugio ( "w ", hostmode_str,len ) ; 
#endif
  sleep(1);
  do {
    l = read(serial,&c,1);
  } while (l == 1);
  printf("TNC in Hostmode\n");
  if (use_select) {
    if (soft_tnc) {
#ifdef HAVE_SOCKET
      fcntl(serial,F_SETFL,0);
#endif
    }
    else {
      wrk_termios.c_cc[VTIME] = 0;
      wrk_termios.c_cc[VMIN] = 1;
      tcsetattr(serial,TCSADRAIN,&wrk_termios);
    }
  }
  /* clear pause-flag, cookie-send and mycall for all channels */
  /* clear display and log call and set start time */
  for (l = 0; l < tnc_channels; l++) {
    ch_stat[l].pause_flag = 0;
    ch_stat[l].sendcook = 0;
    ch_stat[l].contcon = 0;
    ch_stat[l].mycall[0] = '\0';
    ch_stat[l].log_call[0] = '\0';
    ch_stat[l].disp_call[0] = '\0';
    ch_stat[l].name[0] = '\0';
    ch_stat[l].start_time = time(NULL);
    ch_stat[l].state = 0;
    ch_stat[l].oldstate = 0;
    ch_stat[l].maxstate = 0;
    ch_stat[l].conn_state = CS_DISCON;
    ch_stat[l].ignore_state = 0;
    ch_stat[l].huffcod = 0;
    ch_stat[l].access_level = 0;
    ch_stat[l].pwwait = 0;
    ch_stat[l].flchkmode = 0;
    ch_stat[l].queue_act = 0;
  }
  resync_count = 0; /* no resyncs */
  staterr.st_mess = 0; /* reset error statistics */
  staterr.rcv_frms = 0;
  staterr.snd_frms = 0;
  staterr.unacked = 0;
  staterr.tries = 0;
  staterr.axstate = 0;
  staterr.free_buffers = 0;
  staterr.xhost = 0;
  tnc_busy = 0; /* TNC not busy */
#ifdef HOSTDEBUG
  host_tx_len = 0;
  host_tx_start = 0;
  host_rx_len = 0;
  host_rx_start = 0;
#endif
  bcasttx_frames = 0;
  send_queue_type = SQ_NORMAL;
  buffers_req = 0;
  free_buffers = -1;
  sent_bytes = 0;
  softtnc_error = 0;
  return(0);
}

/* put tnc to terminalmode and close serial port */
int exit_serial()
{
  int l;
  char c;
  int len;
  struct block_buffer_ptr *old_block_buffer;

  while (block_buffer_root != NULL) {
    old_block_buffer = block_buffer_root;
    block_buffer_root = old_block_buffer->next;
    free(old_block_buffer->data);
    free(old_block_buffer);
  }
  block_buffer_last = NULL;
  
  while (block_bcbuffer_root != NULL) {
    old_block_buffer = block_bcbuffer_root;
    block_bcbuffer_root = old_block_buffer->next;
    free(old_block_buffer->data);
    free(old_block_buffer);
  }
  block_bcbuffer_last = NULL;

  if (use_select) {
    if (soft_tnc) {
#ifdef HAVE_SOCKET
      if (!softtnc_error) {
        fcntl(serial,F_SETFL,O_NONBLOCK);
      }
#endif
    }
    else {
      wrk_termios.c_cc[VTIME] = 0;
      wrk_termios.c_cc[VMIN] = 0;
      tcsetattr(serial,TCSADRAIN,&wrk_termios);
    }
  }
  if ((!soft_tnc) || (!softtnc_error)) {
    /* get tnc back to terminalmode */
    len = sizeof(hostmode_exit);
    if (write(serial,hostmode_exit,len - 1) != len - 1){
      printf("Error: can't write to serial port\n");
      return(1);
    }
#ifdef DEBUGIO
    wrdebugio ( "w ", hostmode_exit,len - 1 ) ; 
#endif
    sleep(1);
    do {
      l = read(serial,&c,1);
    } while (l == 1);
    printf("TNC in terminalmode\n");
    if (!soft_tnc) {
      tcsetattr(serial,TCSADRAIN,&org_termios);
    }
  }
  close(serial);
  unlink(tnt_lockfile);
  return(0);
}

/* try to resync tnc
   first up to 256 CNTL_A, until tnc responds
   second put tnc to hostmode and then up to 256 CNTL_A, until tnc responds
*/
void resync_tnc(state)
int *state;
{
  int l;
  char c;
  int len;
  int cnt;
  FILE *fp;
  struct tm cvtime;
  time_t ctime;
  char rec_buffer[256];
  int success;
  int i;
  char ch;
#ifdef HOSTDEBUG
  int hptr;
  char hostch;
#endif
  int disc;
  int disc_channel;
  char hex_str[80];
  char ascii_str[80];
  char hex[10];
  int res;

  if (soft_tnc && softtnc_error) {
    act_state = S_END;
    return;
  }
  disc = 0;
  resync_count++;
  fp = NULL;
  if (resy_log_file[0] != '\0') {  
    fp = fopen(resy_log_file,"a");
  }
#ifdef HOSTDEBUG
  if (fp != NULL) {
    fprintf(fp,"received characters before resync:\n");
    for (i = 0;i < host_rx_len;i++) {
      if (!(i % 16)) {
        sprintf(hex_str,"<%3.3x> : ",i);
        ascii_str[0] = '\0';
      }
      hptr = host_rx_start + i;
      if (hptr >= HOSTBUFLEN) hptr -= HOSTBUFLEN;
      hostch = host_rx[hptr].ch;
      if (!isprint((int)hostch))
        hostch = '.';
      ctime = host_rx[hptr].time;
      cvtime = *localtime(&ctime);
      sprintf(hex,"%2.2x ",host_rx[hptr].ch);
      strncat(ascii_str,&hostch,1);
      strcat(hex_str,hex);
      if (!((i + 1) % 16) || (i == (host_rx_len - 1))) {
        fprintf(fp,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u.%2.2u:\n"
                   "%-56.56s %-16.16s\n",
                cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
                cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec,
                hex_str,ascii_str);
      }
    }
    fprintf(fp,"transmitted characters before resync:\n");
    for (i = 0;i < host_tx_len;i++) {
      if (!(i % 16)) {
        sprintf(hex_str,"<%3.3x> : ",i);
        ascii_str[0] = '\0';
      }
      hptr = host_tx_start + i;
      if (hptr >= HOSTBUFLEN) hptr -= HOSTBUFLEN;
      hostch = host_tx[hptr].ch;
      if (!isprint((int)hostch))
        hostch = '.';
      ctime = host_tx[hptr].time;
      cvtime = *localtime(&ctime);
      sprintf(hex,"%2.2x ",host_tx[hptr].ch);
      strncat(ascii_str,&hostch,1);
      strcat(hex_str,hex);
      if (!((i + 1) % 16) || (i == (host_tx_len - 1))) {
        fprintf(fp,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u.%2.2u:\n"
                   "%-56.56s %-16.16s\n",
                cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
                cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec,
                hex_str,ascii_str);
      }
    }
  }
  host_tx_len = 0;
  host_tx_start = 0;
  host_rx_len = 0;
  host_rx_start = 0;
#endif
  if (fp != NULL) {
    ctime = time(NULL);
    cvtime = *localtime(&ctime);
    fprintf(fp,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u.%2.2u: Resync started\n",
          cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
          cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec);
    fprintf(fp,"State of hostmode-receiver:\n");
    switch (rcv_stat) {
    case R_CHANNEL:
      fprintf(fp,"waiting for channel\n");
      break;
    case R_CODE:
      fprintf(fp,"waiting for code, channel = %d\n",rxbuffer[0]);
      break;
    case R_LENGTH:
      fprintf(fp,"waiting for length, channel = %d, code = %d\n",
              rxbuffer[0],rxbuffer[1]);
      break;
    case R_TOZERO:
      fprintf(fp,"waiting for data or null, channel = %d, code = %d\n",
              rxbuffer[0],rxbuffer[1]);
      fprintf(fp,"received data (%d bytes): ",rcv_pos);
      if (rcv_pos) {
        for (i=0;i<rcv_pos;i++) {
          fprintf(fp,"<%x> ",rxbuffer[2+i]);
        }
        fprintf(fp,"\n");
        for (i=0;i<rcv_pos;i++) {
          ch = rxbuffer[2+i];
          if (!isalnum(ch)) ch = '.';
          fprintf(fp,"%c",ch);
        }
      }
      fprintf(fp,"\n");
      break;
    case R_RDDATA:
      fprintf(fp,"waiting for data, channel = %d, code = %d, length = %d\n",
              rxbuffer[0],rxbuffer[1],rxbuffer[2]);
      fprintf(fp,"received data (%d bytes, %d bytes left): ",rcv_pos,rcv_len);
      if (rcv_pos) {
        for (i=0;i<rcv_pos;i++) {
          fprintf(fp,"<%x> ",rxbuffer[3+i]);
        }
        fprintf(fp,"\n");
        for (i=0;i<rcv_pos;i++) {
          ch = rxbuffer[3+i];
          if (!isalnum(ch)) ch = '.';
          fprintf(fp,"%c",ch);
        }
      }
      fprintf(fp,"\n");
    }
  }
  if ((rcv_stat == R_LENGTH) || (rcv_stat == R_RDDATA)) {
    if ((rxbuffer[0] != 0) && 
        (rxbuffer[0] < tnc_channels) &&
        (rxbuffer[1] == R_CHANDATA)) {
      disc = 1;
      disc_channel = rxbuffer[0];
    }
  }
  resync = 1;
  success = 0;
  statlin_update();
  sleep(1);
  if (use_select) {
    if (soft_tnc) {
#ifdef HAVE_SOCKET
      fcntl(serial,F_SETFL,O_NONBLOCK);
#endif
    }
    else {
      wrk_termios.c_cc[VTIME] = 0;
      wrk_termios.c_cc[VMIN] = 0;
      tcsetattr(serial,TCSADRAIN,&wrk_termios);
    }
  }
  cnt = 0;
  *rec_buffer = '\0';
  do { /* send CNTL_A until tnc responds */
    res = write(serial,"\001",1);
    if (soft_tnc && (res != 1)) {
      softtnc_error = 1;
      *state = S_END;
      return;
    }
    cnt++;
    do {
      uwait(10000); /* sleep for 10 ms */
      l = read(serial,&c,1);
      if (l == 1) {
        if (fp != NULL)
          fprintf(fp,"<%x>",c);
        strncat(rec_buffer,&c,1);
        if (strncmp(rec_buffer,resync_string,strlen(rec_buffer)) != 0) {
          *rec_buffer = '\0';
        }
      }
    } while ((l == 1) && (strlen(rec_buffer) < strlen(resync_string)));
    if (strncmp(rec_buffer,resync_string,strlen(resync_string)) == 0)
      success = 1;
  } while (!success && (cnt <= 256));
  if (!success) {
    do {
      /* put tnc to hostmode */
      if (fp != NULL)
        fprintf(fp,"\nSwitching TNC to hostmode\n");
      len = sizeof(hostmode_str) - 1;
      res = write(serial,hostmode_str,len);
#ifdef DEBUGIO
      wrdebugio ( "w ", hostmode_str,len ) ; 
#endif
      if (soft_tnc && (res != len)) {
        softtnc_error = 1;
        *state = S_END;
        return;
      }
      sleep(1);
      do {
        uwait(10000); /* sleep for 10 ms */
        l = read(serial,&c,1);
        if (l == 1) {
          if (fp != NULL)
            fprintf(fp,"<%x>",c);
        }
      } while (l == 1);
      if (fp != NULL)
        fprintf(fp,"\nTry next Resync\n");
      cnt = 0;
      *rec_buffer = '\0';
      do { /* send CNTL_A until tnc responds */
        res = write(serial,"\001",1);
        if (soft_tnc && (res != 1)) {
          softtnc_error = 1;
          *state = S_END;
          return;
        }
        cnt++;
        do {
          uwait(10000); /* sleep for 10ms */
          l = read(serial,&c,1);
          if (l == 1) {
            if (fp != NULL)
              fprintf(fp,"<%x>",c);
            strncat(rec_buffer,&c,1);
            if (strncmp(rec_buffer,resync_string,strlen(rec_buffer)) != 0) {
              *rec_buffer = '\0';
            }
          }
        } while ((l == 1) && (strlen(rec_buffer) < strlen(resync_string)));
        if (strncmp(rec_buffer,resync_string,strlen(resync_string)) == 0)
          success = 1;
      } while (!success && (cnt <= 256));
    } while (!success);
  }
  if (fp != NULL)
    fprintf(fp,"\nReading remaining data\n");
  /* read all characters sent by tnc */
  do {
    l = read(serial,&c,1);
    if (l == 1) {
      if (fp != NULL)
        fprintf(fp,"<%x>",c);
    }
  } while (l == 1);
  if (fp != NULL) {
    fprintf(fp,"\nResync successful\n");
    fclose(fp);
  }
  if (use_select) {
    if (soft_tnc) {
#ifdef HAVE_SOCKET
      fcntl(serial,F_SETFL,0);
#endif
    }
    else {
      wrk_termios.c_cc[VTIME] = 0;
      wrk_termios.c_cc[VMIN] = 1;
      tcsetattr(serial,TCSADRAIN,&wrk_termios);
    }
  }
  rcv_stat = R_CHANNEL;
  resync = 0;
  statlin_update();
  req_info = RQ_CHSTAT;
  poll_channel = 0;
  poll_cycle = 0;
  xhost_ava = 0;
  xhost_polls = 0;
  bcasttx_frames = 0;
  send_queue_type = SQ_NORMAL;
  buffers_req = 0;
  free_buffers = -1;
  sent_bytes = 0;
  softtnc_error = 0;
  ok_display = 0;
  send_block(poll_channel,X_COMM,1,"L");
  *state = S_STAT;
  if (disc) {
    if (ch_stat[disc_channel].conn_state != CS_DISCON) {
      queue_cmd_data(disc_channel,X_COMM,1,M_CMDSCRIPT,"D");
#ifdef USE_IFACE
      close_iface_con(disc_channel,1,0);
#endif
    }
  }
}
#endif

void delay_send(int real_sent,int to_send)
{
#ifdef HAVE_SOCKET
  if (real_sent != to_send) {
    softtnc_error = 1;
    act_state = S_END;
    return;
  }
  if (fixed_wait) {
    uwait(10000);
  }
  else {
    sent_bytes += to_send;
    if (sent_bytes > amount_wait) {
      uwait(10000);
      sent_bytes = 0;
    }
  }
#endif
}

static void send_block(int channel,char code,int len,char *data)
{
  int res;
  FILE *fp;
  char hex_str[80];
  char ascii_str[80];
  char hex[10];
  struct tm cvtime;
  time_t ctime;
  int i;
  char *hptr;
  char hostch;

  if (ax25k_active) {
#ifdef USE_AX25K
    if (code == X_DATA) {
      write_ax25k(channel,data,len);
    }
    else {
      command_ax25k(channel,data,len);
    }
    return;
#endif
  }  
  if (len > PACKETSIZE) {
    fp = NULL;
    if (resy_log_file[0] != '\0') {  
      fp = fopen(resy_log_file,"a");
    }
    if (fp != NULL) {
      ctime = time(NULL);
      cvtime = *localtime(&ctime);
      fprintf(fp,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u.%2.2u: "
                 "maximum packet size exceeded: %i\n",
                  cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
                  cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec,
                  len);
#ifdef HOSTDEBUG
      fprintf(fp,"packet contents follow:\n");
      for (i = 0;i < len;i++) {
        if (!(i % 16)) {
          sprintf(hex_str,"<%3.3x> : ",i);
          ascii_str[0] = '\0';
        }
        hptr = data + i;
        hostch = *hptr;
        if (!isprint((int)hostch))
          hostch = '.';
        sprintf(hex,"%2.2x ",*hptr);
        strncat(ascii_str,&hostch,1);
        strcat(hex_str,hex);
        if (!((i + 1) % 16) || (i == (len - 1))) {
          fprintf(fp,"%-56.56s %-16.16s\n",
                  hex_str,ascii_str);
        }
      }
#endif
      fclose(fp);
    }
    return;
  }
  txbuffer[0] = (char)channel;
  txbuffer[1] = code;
  txbuffer[2] = (char)(len - 1);
  memcpy(txbuffer+3,data,len);
#ifdef HOSTDEBUG
  debug_copy_tx(txbuffer,len + 3);
#endif
  res = write(serial,txbuffer,len + 3);
#ifdef DEBUGIO
  wrdebugio ( "w ", txbuffer, len+3 ) ;
#endif 
  if (soft_tnc) {
    delay_send(res,len + 3);
  }
}

 
#ifdef DEBUGIO
wrdebugio ( art, txbuffer, len ) 
  char *art ;
  char *txbuffer ; 
  int len ; 
{
  char debbuffer[512] ; 
  int i, j ; 

  write ( fddebugio , art, strlen(art) ) ; 
  j=0 ;
  for ( i=0; i<len; i++ ) 
    {
      if ( ( (int)txbuffer[i] < 127 ) && ( (int)txbuffer[i] > 32 ) ) 
        {
          debbuffer[j++] = txbuffer[i] ; 
        } 
        else 
        {
          sprintf ( debbuffer+j, "<%2x>", (int)txbuffer[i] ) ; 
          j += 4 ;
        } ;
    } ;
  sprintf ( debbuffer+j, "\n" ) ; j++ ;
  write ( fddebugio , debbuffer, j ) ;
}
#endif
 

#ifndef DPBOXT
static int block_receiver(rxbuffer,len,buffer)
char *rxbuffer;
int len;
char *buffer;
{
  int block_ok;
  int i;
  int error;
  FILE *fp;
  struct tm cvtime;
  time_t ctime;
  
  block_ok = 0;  
  i = 0;
  error = 0;
#ifdef DEBUGIO
  wrdebugio ( "r ", rxbuffer,len ) ; 
#endif
  while ((!block_ok) && (i < len) && (!error)) {
    switch (rcv_stat) {
    case R_CHANNEL:
      if ((buffer[i] >= tnc_channels) && (buffer[i] != 255)) {
        error = 1;
        rcv_stat = R_CHANNEL;
      }
      else {
        /* channel 255 only on response to XHOST-request */
        if ((buffer[i] == 255) && (req_info != RQ_XHOST)) {
          error = 1;
          rcv_stat = R_CHANNEL;
        }
        else {
          rxbuffer[0] = buffer[i];
          rcv_stat = R_CODE;
          i++;
        }
      }
      break;
    case R_CODE:
      if (buffer[i] > R_CHANDATA) {
        error = 2;
        rcv_stat = R_CHANNEL;
      }
      else {
        if ((rxbuffer[0] == 255) &&
            ((buffer[i] != R_SUCCMESS) && (buffer[i] != R_FAILMESS))) {
          error = 2;
          rcv_stat = R_CHANNEL;
        }
        else {
          rxbuffer[1] = buffer[i];
          i++;
          switch (rxbuffer[1]) {
          case R_SUCC:
            rcv_stat = R_CHANNEL;
            block_ok = 1;
            break;
          case R_SUCCMESS:
          case R_FAILMESS:
          case R_LINKSTAT:
          case R_MONI:
          case R_MONIMESS:
            rcv_stat = R_TOZERO;
            rcv_pos = 0;
            break;
          case R_MONIDATA:
          case R_CHANDATA:
            rcv_stat = R_LENGTH;
            break;
          }
        }
      }
      break;
    case R_LENGTH:
      rxbuffer[2] = buffer[i];
      rcv_len = (int)rxbuffer[2] + 1;
      rcv_stat = R_RDDATA;
      rcv_pos = 0;
      i++;
      break;
    case R_TOZERO:
      if ((rxbuffer[2+rcv_pos] = buffer[i]) == '\0') {
        rcv_stat = R_CHANNEL;
        i++;
        block_ok = 1;
      }
      else {
        rcv_pos++;
        i++;
        if (rcv_pos == 256) {
          error = 4;
          rcv_stat = R_CHANNEL;
        }
      }
      break;
    case R_RDDATA:
      rxbuffer[3+rcv_pos] = buffer[i];
      rcv_len--;
      rcv_pos++;
      i++;
      if (rcv_len == 0) {
        block_ok = 1;
        rcv_stat = R_CHANNEL;
      }
      break;  
    }
  }
  if ((i < len) && (block_ok)) error = 3;
  if (error) {
    fp = NULL;
    if (resy_log_file[0] != '\0') {  
      fp = fopen(resy_log_file,"a");
    }
    if (fp != NULL) {
      ctime = time(NULL);
      cvtime = *localtime(&ctime);
      fprintf(fp,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u / ",
              cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
              cvtime.tm_hour,cvtime.tm_min);
      switch (error ) {
      case 1:
        fprintf(fp,"Illegal channel: %x\n",buffer[i]);
        break;
      case 2:
        fprintf(fp,"Illegal command code: %x\n",buffer[i]);
        break;
      case 3:
        fprintf(fp,"Additional data after block: ");
          while (i < len) {
            fprintf(fp,"%x,",buffer[i]);
            i++;
          }
          fprintf(fp,"\n");
        break;
      case 4:
        fprintf(fp,"String too long, remaining data: ");
          while (i < len) {
            fprintf(fp,"%x,",buffer[i]);
            i++;
          }
          fprintf(fp,"\n");
        break;
      }
      fclose(fp);
    }
  }
  return(block_ok);
}

static int fill_status(channel,buffer)
int channel;
char *buffer;
{
  int res;
  int changed;
  int st_mess;
  int rcv_frms;
  int snd_frms;
  int unacked;
  int tries;
  int axstate;
  
  changed = 0;
  if (channel != 0) {
    res = sscanf(buffer,"%d %d %d %d %d %d",&st_mess,&rcv_frms,&snd_frms,
                 &unacked,&tries,&axstate);
    if (res != 6) return(changed);
  }
  else {
    res = sscanf(buffer,"%d %d",&st_mess,&rcv_frms);
    if (res != 2) return(changed);
  }
  
  /* security checks because of received garbage in case of resync */
  if ((st_mess < 0) || (st_mess > 1000)) {
    st_mess = 10;
    staterr.st_mess++;
  }
  if ((rcv_frms < 0) || (rcv_frms > 1000)) {
    rcv_frms = 10;
    staterr.rcv_frms++;
  }
  if (channel != 0) {
    if ((snd_frms < 0) || (snd_frms > 40)) {
      snd_frms = 40;
      staterr.snd_frms++;
    }
    if ((unacked < 0) || (unacked > 7)) {
      unacked = 7;
      staterr.unacked++;
    }
    if ((tries < 0) || (tries > 100)) {
      tries = 10;
      staterr.tries++;
    }
    if ((axstate < 0) || (axstate > 15)) {
      axstate = 0;
      staterr.axstate++;
    }
  }

  ch_stat[channel].st_mess = st_mess;

  /* only up to 10 monitorframes at one time */
  if ((channel == 0) && (rcv_frms > NUM_MONFRAMES))
    ch_stat[channel].rcv_frms = NUM_MONFRAMES;
  else
    ch_stat[channel].rcv_frms = rcv_frms;

  if (channel != 0) {
    if (ch_stat[channel].snd_frms != snd_frms) {
      ch_stat[channel].snd_frms = snd_frms;
      changed = 1;
    }
    if (ch_stat[channel].unacked != unacked) {
      ch_stat[channel].unacked = unacked;
      changed = 1;
    }
    if (ch_stat[channel].tries != tries) {
      ch_stat[channel].tries = tries;
      changed = 1;
    }
    ch_stat[channel].oldstate = ch_stat[channel].state;
    if (ch_stat[channel].state != axstate) {
      ch_stat[channel].oldstate = ch_stat[channel].state;
      ch_stat[channel].state = axstate;
      changed = 1;
    }
    if (ch_stat[channel].state > ch_stat[channel].maxstate)
      ch_stat[channel].maxstate = ch_stat[channel].state;
  }
  return(changed);
}

/* results of status message analysis */
#define R_CONN 1
#define R_DISC 2
#define R_RESET 3
#define R_BUSY 4
#define R_FAIL 5

static int analyse_stat_msg(buffer)
char *buffer;
{
  if (strstr(buffer,"CONNECTED to ") != NULL) return(R_CONN);
  if (strstr(buffer,"DISCONNECTED fm ") != NULL) return(R_DISC);
  if (strstr(buffer,"LINK RESET ") != NULL) return(R_RESET);
  if (strstr(buffer,"BUSY fm") != NULL) return(R_BUSY);
  if (strstr(buffer,"LINK FAILURE with ") != NULL) return(R_FAIL);
  return(0);
}

static void disc_to_conn(channel)
int channel;
{
  ch_stat[channel].conn_state = CS_CONN;
  ch_stat[channel].sendcook = 1;
  ch_stat[channel].start_time = time(NULL);
  ch_stat[channel].contcon = 1;
  ch_stat[channel].lastcr = 1;
  ch_stat[channel].oldbuflen = 0;
  ch_stat[channel].huffcod = 0;
  ch_stat[channel].access_level = 0;
  ch_stat[channel].pwwait = 0;
  ch_stat[channel].flchkmode = 0;
#ifdef USE_IFACE
  conn_boxcut(channel);
#endif  
}

static void setup_to_conn(channel)
int channel;
{
  if (xconnect_first(channel)) {
    /* a connect request during connect setup of xconnect */
    close_xconnect(channel,0);
    ch_stat[channel].sendcook = 1;
  }
  ch_stat[channel].conn_state = CS_CONN;
  ch_stat[channel].start_time = time(NULL);
  ch_stat[channel].contcon = 1;
  ch_stat[channel].lastcr = 1;
  ch_stat[channel].oldbuflen = 0;
  ch_stat[channel].huffcod = 0;
  ch_stat[channel].access_level = 0;
  ch_stat[channel].pwwait = 0;
  ch_stat[channel].flchkmode = 0;
#ifdef USE_IFACE
  conn_boxcut(channel);
#endif  
}

static int setup_to_disc(channel)
int channel;
{
  int block_send;
  char tmpstr[10];
  
  ch_stat[channel].conn_state = CS_DISCON;
  block_send = 0;
  close_shell(channel,0,0);
  close_rxfile(channel,0);
  close_txfile(channel,0);
  close_xconnect(channel,0);
  clear_pwmode(channel);
#ifdef USE_IFACE
  close_iface_con(channel,1,0);
  disc_boxcut(channel);
#endif  
  ch_stat[channel].disp_call[0] = '\0';
  ch_stat[channel].name[0] = '\0';
  ch_stat[channel].log_call[0] = '\0';
  ch_stat[channel].maxstate = 0;
  ch_stat[channel].huffcod = 0;
  ch_stat[channel].access_level = 0;
  ch_stat[channel].pwwait = 0;
  ch_stat[channel].remote = 0;
  ch_stat[channel].flchkmode = 0;
  ch_stat[channel].lastcr = 0;
  ch_stat[channel].oldbuflen = 0;
  statlin_update();
  if (ch_stat[channel].mycall[0] != '\0') {
    strcpy(tmpstr,"I");
    strcat(tmpstr,ch_stat[channel].mycall);
    send_block(channel,X_COMM,strlen(tmpstr),tmpstr);
    req_info = RQ_MYCALL;
    req_flag = M_CMDSCRIPT;
    block_send = 1;
  }
  return(block_send);
}

static int conn_to_disc(channel)
int channel;
{
  int block_send;
  char tmpstr[10];
  
  ch_stat[channel].conn_state = CS_DISCON;
  block_send = 0;
  ch_stat[channel].end_time = time(NULL);
  if (!ch_stat[channel].flchkmode)
    write_log(channel);
  close_shell(channel,0,0);
  close_rxfile(channel,0);
  close_txfile(channel,0);
  close_xconnect(channel,0);
  clear_pwmode(channel);
#ifdef USE_IFACE
  close_iface_con(channel,1,0);
  disc_boxcut(channel);
#endif  
  ch_stat[channel].disp_call[0] = '\0';
  ch_stat[channel].name[0] = '\0';
  ch_stat[channel].log_call[0] = '\0';
  ch_stat[channel].maxstate = 0;
  ch_stat[channel].huffcod = 0;
  ch_stat[channel].access_level = 0;
  ch_stat[channel].pwwait = 0;
  ch_stat[channel].remote = 0;
  ch_stat[channel].flchkmode = 0;
  ch_stat[channel].lastcr = 0;
  ch_stat[channel].oldbuflen = 0;
  statlin_update();
  if (ch_stat[channel].mycall[0] != '\0') {
    strcpy(tmpstr,"I");
    strcat(tmpstr,ch_stat[channel].mycall);
    send_block(channel,X_COMM,strlen(tmpstr),tmpstr);
    req_info = RQ_MYCALL;
    req_flag = M_CMDSCRIPT;
    block_send = 1;
  }
  return(block_send);
}

static int update_conn_state(channel,buffer)
int channel;
char *buffer;
{
  int result;
  int block_send;
  
  block_send = 0;
  result = analyse_stat_msg(buffer);
  if (result) {
#ifdef TNT_SOUND
    switch (result) {    /* Soundhandling */
      case R_CONN: { play_sound(2); break; }
      case R_DISC: { play_sound(3); break; }
      case R_RESET: { play_sound(4); break; }
      case R_BUSY: { play_sound(5); break; }
      case R_FAIL: { play_sound(6); break; }
    }
#endif
    switch (ch_stat[channel].conn_state) {
    case CS_DISCON:
      switch (result) {
      case R_CONN:
      case R_RESET:
        disc_to_conn(channel);
        break;
      case R_DISC:
      case R_BUSY:
      case R_FAIL:
        /* nothing to do */
        break;
      }
      break;
    case CS_SETUP:
      switch (result) {
      case R_CONN:
      case R_RESET:
        setup_to_conn(channel);
        break;
      case R_DISC:
      case R_BUSY:
      case R_FAIL:
        block_send = setup_to_disc(channel);
        break;
      }
      break;
    case CS_CONN:
      switch (result) {
      case R_CONN:
      case R_RESET:
        /* nothing to do */
        break;
      case R_DISC:
      case R_BUSY:
      case R_FAIL:
        block_send = conn_to_disc(channel);
        break;
      }
      break;
    }
  }
  return(block_send);
}

void action_on_connect(int channel)
{
  int i;
  int ssid;
  int access;

  if (ch_stat[channel].sendcook) {
    ssid=0;
    for(i=0;i<strlen(ch_stat[channel].curcall);i++) {
      if(ch_stat[channel].curcall[i]=='-') {
        ssid=atoi(&ch_stat[channel].curcall[i+1]);
        break;
      }
    }
        
    if (ch_stat[channel].flchkmode) {
      ch_stat[channel].remote = 0;
    }
#ifdef USE_IFACE
    else if (((tnt_box_call[0] != '\0') && 
              (strcmp(tnt_box_call,ch_stat[channel].curcall) == 0)) ||
             ((tnt_box_call[0] == '\0') && 
              (ssid == tnt_box_ssid))) {
      if (box_active_flag && !box_busy_flag) {
        cmd_box(1,0,channel,0,M_CMDSCRIPT,NULL);
      }
      else {
        rem_nobox(channel);
      }
    }
    else if (((tnt_node_call[0] != '\0') && 
              (strcmp(tnt_node_call,ch_stat[channel].curcall) == 0)) ||
             ((tnt_node_call[0] == '\0') && 
              (ssid == tnt_node_ssid))) {
      if (node_active_flag) {
        cmd_node(1,0,channel,0,M_CMDSCRIPT,NULL);
      }
      else {
        rem_nonode(channel);
      }
    }
#endif
    else if (do_autostart(ch_stat[channel].curcall,channel)) {
      /* program already started */
    }
    else {
      access = 1;
      if (noacc_flag) {
        access = rem_noacc(channel);
      }
      if (access) {
        if (ctext_flag)
          rem_ctext(0,0,channel,0,M_CONNECT,NULL);
        else if (cookie_flag)
          rem_cookie(1,0,channel,0,0,NULL);
      }
    }
    ch_stat[channel].sendcook = 0;
  }
}

static int block_decoder(rxbuffer,state)
char *rxbuffer;
int *state;
{
  char rxbuffercopy[259];
  int channel;
  int block_send;
  int res;
#ifdef USE_IFACE
  int frames;
#endif
  char huffbuffer[PACKETSIZE+1];
  int displayed;
#ifdef BCAST
  short dummychan;
#endif
   
  memcpy(rxbuffercopy,rxbuffer,259);
  block_send = 0; 
  channel = rxbuffer[0];
  switch (rxbuffer[1]) {
  case R_SUCC:
    if (ok_display) {
      cmd_display(req_flag,channel,OK_TEXT,1);
    }
    if (req_info == RQ_OWNCALL) {
      send_block(channel,X_COMM,1,"I");
      req_info = RQ_CURCALL;
      block_send = 1;
    }
    else if (req_info == RQ_MYCALL) {
      send_block(channel,X_COMM,1,"C");
      req_info = RQ_CALL;
      block_send = 1;
    }
    next_command(state);
    break;
  case R_SUCCMESS:
    switch (req_info) {
    case RQ_NORMAL:
      cmd_display(req_flag,channel,rxbuffer+2,1);
      next_command(state);
      break;
    case RQ_REDISC:
      cmd_display(req_flag,channel,rxbuffer+2,1);
      block_send = setup_to_disc(channel);
      next_command(state);
      break;
    case RQ_BUFFERS:
      if (sscanf(rxbuffer+2,"%d",&free_buffers) != 1) {
        free_buffers = -1;
      }
      else {
        if ((free_buffers < 0) || (free_buffers > 100000)) {
          free_buffers = -1;
          staterr.free_buffers++;
        }
      }
      break;
#ifdef BCAST
    case RQ_BCBUFFERS:
      if (sscanf(rxbuffer+2,"%d",&free_buffers) != 1) {
        free_buffers = -1;
      }
      else {
        dummychan = 0;
        (*func_callback)(dummychan,(short)free_buffers);
      }
      break;
#endif
    case RQ_PID:
      strcpy(pushed_pid,rxbuffer+2);
      break;
    case RQ_MONSTAT:
      strcpy(pushed_monstat,rxbuffer+2);
      break;
    case RQ_UNPROTO:
      strcpy(pushed_unproto,rxbuffer+2);
      break;
    case RQ_XHOST:
      if (*state == S_TSTXHOST) {
        xhost_ava = 1;
        *state = S_TNT_UP;
        open_upfile(state);
      }
      strcpy(xhost_list,rxbuffer+2);
      xhost_ptr = xhost_list;
      if (*xhost_ptr != 0) {
        if (*xhost_ptr > tnc_channels) {
          *xhost_ptr = 0;
          staterr.xhost++;
        }
        else {
          send_block((*xhost_ptr)-1,X_COMM,1,"L");
          req_info = RQ_CHSTAT;
          block_send = 1;
          xhost_ptr++;
        }
      }
      break;
    case RQ_CHSTAT:
      res = fill_status(channel,rxbuffer+2);
      if (((res) && (*state != S_STAT)) ||
          (*state == S_STAT)) {
        stat_display(channel);
#ifdef USE_IFACE
        frames = calc_xframes(channel);
        send_cstatus(channel,frames);
#endif 
      }
      if (*state == S_STAT) {
        if ((ch_stat[channel].state >= 4) && 
            (ch_stat[channel].conn_state != CS_CONN)) {
          setup_to_conn(channel);
          if (disc_on_start) {
            queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
          }
        }
        send_block(channel,X_COMM,1,"C");
        req_info = RQ_CALL;
        block_send = 1;
      }
      else {
        if (!ch_stat[channel].pause_flag) {
          if ((ch_stat[channel].rcv_frms) || (ch_stat[channel].st_mess)) {
            send_block(channel,X_COMM,1,"G");
            block_send = 1;
          }
        }
      }
      next_sendframe(channel);
      break;
    case RQ_CALL:
      strcpy(ch_stat[channel].call,rxbuffer+2);
      stat_display(channel);
      if ((ch_stat[channel].conn_state == CS_CONN) &&
          (strcmp(ch_stat[channel].call,"CHANNEL NOT CONNECTED") != 0)) {
        set_pwmode(channel);
        ch_stat[channel].remote = 1;
        set_remmode(channel);
        set_flchkmode(channel);
      }
      else {
        clear_pwmode(channel);
        ch_stat[channel].remote = 0;
        ch_stat[channel].flchkmode = 1;
        ch_stat[channel].sendcook = 0;
      }
      send_block(channel,X_COMM,1,"I");
      req_info = RQ_CURCALL;
      block_send = 1;
      break;
    case RQ_CURCALL:
      /* copy call of startup to mycall */
      if (ch_stat[channel].mycall[0] == '\0')
        strcpy(ch_stat[channel].mycall,rxbuffer+2);
      strcpy(ch_stat[channel].curcall,rxbuffer+2);
      if (altstat) {
        update_name(channel);
        stat_display(channel);
      }
      action_on_connect(channel);
      if (!ch_stat[channel].pause_flag) {
        if ((ch_stat[channel].rcv_frms) || (ch_stat[channel].st_mess)) {
          send_block(channel,X_COMM,1,"G");
          block_send = 1;
        }
      }
      break;
    }
    break;
  case R_FAILMESS:
    if (strcmp(rxbuffer+2,"TNC BUSY - LINE IGNORED") == 0) {
      tnc_busy = 1;
      busy_count = WAIT_BUSY; /* nbr of times before sending resumed */
      req_info_save = req_info;
      req_flag_save = req_flag;
      len_save = (int)(txbuffer[2] + 1); /* get length of packet */
      memcpy(txbuffer_save,txbuffer,len_save + 3); /* save packet */
      statlin_update();
      break;
    }
    switch (req_info) {
    case RQ_XHOST:
      if (*state == S_TSTXHOST) {
        xhost_ava = 0;
        *state = S_TNT_UP;
        open_upfile(state);
        poll_channel = 0;
        send_block(poll_channel,X_COMM,1,"L");
        req_info = RQ_CHSTAT;
        block_send = 1;
      }
      break;
    case RQ_MYCALL:
      send_block(channel,X_COMM,1,"C");
      req_info = RQ_CALL;
      block_send = 1;
      break;
    case RQ_BUFFERS:
      free_buffers = -1;
      break;
#ifdef BCAST
    case RQ_BCBUFFERS:
      free_buffers = -1;
      break;
#endif
    case RQ_CONNECT:
      cmd_display(req_flag,channel,rxbuffer+2,1);
      block_send = setup_to_disc(channel);
      next_command(state);
      break;
    case RQ_RECONNECT:
      strcpy(ch_stat[channel].call,ch_stat[channel].call_save);
      cmd_display(req_flag,channel,rxbuffer+2,1);
      next_command(state);
      break;
    case RQ_REDISC:
      cmd_display(req_flag,channel,rxbuffer+2,1);
      block_send = setup_to_disc(channel);
      next_command(state);
      break;
    case RQ_PID:
      strcpy(pushed_pid,"240");
      break;
    case RQ_MONSTAT:
      strcpy(pushed_monstat,"UISC");
      break;
    case RQ_UNPROTO:
      strcpy(pushed_unproto,"CQ");
      break;
    default:
      cmd_display(req_flag,channel,rxbuffer+2,1);
      next_command(state);
      break;
    }
    break;
  case R_LINKSTAT:
    data_display(channel,rxbuffer+2);
    block_send = update_conn_state(channel,rxbuffer+2);
    ch_stat[channel].st_mess--;
    if (ch_stat[channel].contcon) {
      next_connect(channel);
      ch_stat[channel].contcon = 0;
    }
    if (!block_send) {
      send_block(channel,X_COMM,1,"C");
      req_info = RQ_CALL;
      block_send = 1;
    }
    break;
  case R_MONI:
    if (!ch_stat[channel].pause_flag) {
      if ((--ch_stat[channel].rcv_frms) || (ch_stat[channel].st_mess)) {
        send_block(channel,X_COMM,1,"G");
        block_send = 1;
      }
    }
    moni_display(channel,rxbuffercopy+2);
    break;
  case R_MONIMESS:
    send_block(channel,X_COMM,1,"G");
    block_send = 1;
    moni_display(channel,rxbuffercopy+2);
    break;
  case R_MONIDATA:
    if (!ch_stat[channel].pause_flag) {
      if ((--ch_stat[channel].rcv_frms) || (ch_stat[channel].st_mess)) {
        send_block(channel,X_COMM,1,"G");
        block_send = 1;
      }  
    }
    moni_display_len(channel,rxbuffercopy+2);
    break;
  case R_CHANDATA:
#ifdef TNT_SOUND
/*    play_sound(1); */ /* now disabled! */
#endif
    if (!ch_stat[channel].pause_flag) {
      if ((--ch_stat[channel].rcv_frms) || (ch_stat[channel].st_mess)) {
        send_block(channel,X_COMM,1,"G");
        block_send = 1;
      }
    }
    displayed = 0;
    if (ch_stat[channel].huffcod) {
      if (!decstathuf(rxbuffercopy+2,huffbuffer)) {
        data_display_len(channel,huffbuffer);
        displayed = 1;
      }
    }
    if (!displayed)
      data_display_len(channel,rxbuffercopy+2);
    break;
  }
  return(block_send);
}

/* put block to tx-queue */
int queue_cmd_data(int channel,char code,int len,int flag,char *data)
{
  int newlen;
  char *newdata;
  int is_huff;
  char huffbuffer[PACKETSIZE];
  struct block_buffer_ptr *head;
  char *buffer;
  
  if ((code == X_DATA) && (channel != 0)) {
    ch_stat[channel].snd_queue_frms++;
    if (free_buffers != -1)
      free_buffers -= (len / 32 + 1);
    stat_display(channel);
  }
  is_huff = 0;
  if ((ch_stat[channel].huffcod) && (code == X_DATA)) {
    is_huff = 1;
    newdata = huffbuffer;
    if (encstathuf(data,len,newdata,&newlen)) {
      is_huff = 0;
    }
  }
  if (!is_huff) {
    newdata = data;
    newlen = len;
  }

  /* allocate new bufferelement */
  head = (struct block_buffer_ptr *) malloc(sizeof(struct block_buffer_ptr));
  if (head == NULL) return(1);
  buffer = (char *) malloc(newlen);
  if (buffer == NULL) {
    free(head);
    return(1);
  }

  head->channel = channel;
  head->code = code;
  head->len = newlen;
  head->flag = flag;
  head->data = buffer;
  head->next = NULL;
  memcpy(buffer,newdata,newlen);
  if (send_queue_type == SQ_BCAST) {
    block_bcbuffer_len++;
    if (block_bcbuffer_root == NULL) {
      block_bcbuffer_root = head;
      block_bcbuffer_last = head;
    }
    else {
      block_bcbuffer_last->next = head;
      block_bcbuffer_last = head;
    }
  }
  else { /* SQ_NORMAL */
    block_buffer_len++;
    if (block_buffer_root == NULL) {
      block_buffer_root = head;
      block_buffer_last = head;
    }
    else {
      block_buffer_last->next = head;
      block_buffer_last = head;
    }
  }
  return(0);
}

/* sends block out of txqueue */
static int send_cmd_data()
{
  int channel;
  char code;
  int len;
  int flag;
  struct block_buffer_ptr *old_block_buffer;
  struct block_buffer_ptr **buffer_root;
  struct block_buffer_ptr **buffer_last;
  int *buffer_len;
  char call[256];
  int i;
  int new_buffer_len;
  char new_buffer[257];
  
  if ((block_buffer_len) || (block_bcbuffer_len)) {
    new_buffer_len = 0;
    if ((block_bcbuffer_len) && (bcasttx_frames < MAXBCASTTX)) {
      buffer_root = &block_bcbuffer_root;
      buffer_last = &block_bcbuffer_last;
      buffer_len = &block_bcbuffer_len;
      bcasttx_frames++;
    }
    else if (block_buffer_len) {
      buffer_root = &block_buffer_root;
      buffer_last = &block_buffer_last;
      buffer_len = &block_buffer_len;
      bcasttx_frames = 0;
    }
    else {
      buffer_root = &block_bcbuffer_root;
      buffer_last = &block_bcbuffer_last;
      buffer_len = &block_bcbuffer_len;
      bcasttx_frames = 0;
    }
    channel = (*buffer_root)->channel;
    code = (*buffer_root)->code;
    len = (*buffer_root)->len;
    flag = (*buffer_root)->flag;
    req_info = RQ_NORMAL;
    if (code == X_COMM) { 
      switch ((*buffer_root)->data[0]) {
      case 'C':
        if ((channel == 0) && (len == 1) && (flag == M_PUSHPOP)) {
          req_info = RQ_UNPROTO;
        }
        if ((channel == 0) && (len == 5) && (flag == M_PUSHPOP)) {
          if (strncmp((*buffer_root)->data,"Cpush",5) == 0) {
            sprintf(new_buffer,"C%s",pushed_unproto);
            new_buffer_len = strlen(new_buffer);
          }
        }
        else if ((channel) && (len > 1)) {
          switch (ch_stat[channel].conn_state) {
          case CS_DISCON:
            ch_stat[channel].conn_state = CS_SETUP;
            for (i=0;i<len-1;i++) {
              call[i] = toupper((*buffer_root)->data[1+i]);
            }
            call[len-1] = '\0';
            update_remcall(channel,call);
            if (altstat) {
              update_name(channel);
            }
            req_info = RQ_CONNECT;
            break;
          case CS_SETUP:
            if (xconnect_first(channel)) {
              req_info = RQ_CONNECT;
              xcon_reset_first(channel);
            }
            else {
              strcpy(ch_stat[channel].call_save,ch_stat[channel].call);
              for (i=0;i<len-1;i++) {
                call[i] = toupper((*buffer_root)->data[1+i]);
              }
              call[len-1] = '\0';
              update_remcall(channel,call);
              if (altstat) {
                update_name(channel);
              }
              req_info = RQ_RECONNECT;
            }
            break;
          }
        }
        break;
      case 'D':
        if (channel) {
          if (ch_stat[channel].conn_state == CS_DISCON) {
            req_info = RQ_REDISC;
          }
        }
        break;
      case 'I':
        if ((len > 1) && (req_info != RQ_MYCALL))
          req_info = RQ_OWNCALL;
        break;
      case 'M':
        if ((channel == 0) && (len == 1) && (flag == M_PUSHPOP))
          req_info = RQ_MONSTAT;
        else if ((channel == 0) && (len == 5) && (flag == M_PUSHPOP)) {
          sprintf(new_buffer,"M%s",pushed_monstat);
          new_buffer_len = strlen(new_buffer);
        }
        break;
      case '@':
        if (len == 2) {
          if (((*buffer_root)->data[1] == 'P') &&
              (channel == 0) && (flag == M_PUSHPOP))
            req_info = RQ_PID;
#ifdef BCAST
          else if (((*buffer_root)->data[1] == 'B') &&
              (channel == 0) && (flag == M_PUSHPOP))
            req_info = RQ_BCBUFFERS;
        }
        else if (len == 6) {
          if (((*buffer_root)->data[1] == 'B') &&
              (channel == 0) && (flag == M_PUSHPOP)) {
            sprintf(new_buffer,"@B%s",pushed_monstat);
            new_buffer_len = strlen(new_buffer);
          }
#endif
        }
        break;
      }
    }
    if (code == X_COMM) req_flag = flag;
    else req_flag = M_COMMAND;
    if (new_buffer_len)
      send_block(channel,code,new_buffer_len,new_buffer);
    else
      send_block(channel,code,len,(*buffer_root)->data);
    (*buffer_len)--;
    old_block_buffer = *buffer_root;
    *buffer_root = old_block_buffer->next;
    if (*buffer_root == NULL)
      *buffer_last = NULL;
    free(old_block_buffer->data);
    free(old_block_buffer);
    
    if (code == X_COMM) {
      return(1);
    }
    else { /* X_DATA */
      if (channel != 0) {
        ch_stat[channel].snd_queue_frms--;
        ch_stat[channel].snd_frms++;
        stat_display(channel);
      }
      return(0);
    }
  }
  return(0);
}

/* sets channel to next connected channel, returns 1 if no channel left */          
static int next_conn_channel(int *channel)
{
  while ((*channel) < tnc_channels) {
    if (ch_stat[*channel].conn_state != CS_DISCON) {
      return(0);
    }
    (*channel)++;
  }
  (*channel) = 0;
  return(1);
}

static void poll_tnc(state)
int *state;
{
  if (buffers_req) { /* free buffers were requested */
    buffers_req = 0;
    if (*state == S_STAT) { /* xhost-test after startup */
      *state = S_TSTXHOST;
      send_block(255,X_COMM,1,"G");
      req_info = RQ_XHOST;
    }
    else if (xhost_ava) { /* xhost-poll */
      send_block(255,X_COMM,1,"G");
      req_info = RQ_XHOST;
      if (poll_cycle) {
        xhost_polls++;
        poll_cycle = 0;
      }
      else xhost_polls = 1;
    }
    else { /* ordinary poll */
      poll_channel = 0;
      poll_cycle = 0;
      send_block(poll_channel,X_COMM,1,"L");
      req_info = RQ_CHSTAT;
    }
  }
  else if (poll_cycle) { /* special poll cycle for connected channels */
    if (!next_conn_channel(&poll_channel)) { /* any connected chan left */
      send_block(poll_channel,X_COMM,1,"L");
      req_info = RQ_CHSTAT;
      poll_channel++;
    }
    else { /* continue with xhost poll */
      send_block(0,X_COMM,2,"@B");
      req_info = RQ_BUFFERS;
      buffers_req = 1;
    }
  }
  else if ((xhost_ava) && (xhost_polls != -1)) { /* xhost poll */
    if (*xhost_ptr != 0) { /* xhost returned channels with data */
      send_block((*xhost_ptr)-1,X_COMM,1,"L");
      req_info = RQ_CHSTAT;
      xhost_ptr++;
    }
    else if (xhost_polls >= NUM_XHOST_POLLS) { /* now an ordinary poll */
      poll_cycle = 0;
      poll_channel = 0;
      send_block(poll_channel,X_COMM,1,"L");
      req_info = RQ_CHSTAT;
      xhost_polls = -1;
    }
    else if ((xhost_polls % 4) == 0) { /* start special poll for conn. ch */
      poll_channel = 1;
      if (!next_conn_channel(&poll_channel)) { /* any connected chan. left */
        send_block(poll_channel,X_COMM,1,"L");
        req_info = RQ_CHSTAT;
        poll_cycle = 1;
        poll_channel++;
      }
      else { /* continue with xhost poll */
        send_block(255,X_COMM,1,"G");
        req_info = RQ_XHOST;
        xhost_polls++;
      }
    }
    else { /* xhost poll */
      send_block(255,X_COMM,1,"G");
      req_info = RQ_XHOST;
      xhost_polls++;
    }
  }
  else { /* ordinary poll, next channel */
    poll_channel++;
    if (poll_channel >= tnc_channels) { /* all channels polled */
      send_block(0,X_COMM,2,"@B");
      req_info = RQ_BUFFERS;
      buffers_req = 1;
    }
    else { /* next channel */
      send_block(poll_channel,X_COMM,1,"L");
      req_info = RQ_CHSTAT;
    }
  }
}

int calc_maxframes(channel)
int channel;
{
  int max_frames;
  int conn_channels;
  int i;
  
  if (ax25k_active) {
    return(MAX_FRAMES-1);
  }
  conn_channels = 0;
  for (i=1;i<tnc_channels;i++) {
    if (ch_stat[i].conn_state != CS_DISCON) conn_channels++;
  }
  if (conn_channels == 0) conn_channels = 1;
  max_frames = MAX_FRAMES - 1;
  if (free_buffers != -1) {
    if (free_buffers < MINBUFFERS) return(0);
    max_frames = ((free_buffers - ABSMINBUFFERS) / conn_channels) / 8;
    if (max_frames > MAX_FRAMES - 1)
      max_frames = MAX_FRAMES -1;
  }
  return(max_frames);
}

int calc_xframes(channel)
int channel;
{
  int max_frames;
  int frames;
  
  max_frames = calc_maxframes(channel);
  frames = ch_stat[channel].snd_frms+ch_stat[channel].snd_queue_frms;
  if (frames > max_frames) {
    return(MAX_SND_FRMS);
  }
  return(MAX_SND_FRMS - (max_frames - frames));
}

int data_allowed(channel)
int channel;
{
  if (channel == 0)
    return(1);
  else
    return((ch_stat[channel].snd_frms+ch_stat[channel].snd_queue_frms) <
            MAX_FRAMES);
}

int senddata_allowed(channel)
int channel;
{
  int max_frames;
  
  max_frames = calc_maxframes(channel);
  return((ch_stat[channel].snd_frms+ch_stat[channel].snd_queue_frms) <
          max_frames);
}

int serial_server(state,buffer,len)
int *state;
char *buffer;
int len;
{
  int res;
  int block_ava;
  
  if (*state == S_INIT) {
    restart_tx_queue();
    rcv_stat = R_CHANNEL;
    req_info = RQ_CHSTAT;
    poll_channel = 0;
    poll_cycle = 0;
    xhost_ava = 0;
    xhost_polls = 0;
    ok_display = 0;
    send_block(poll_channel,X_COMM,1,"L");
    *state = S_STAT;
  }
  else {
#ifdef HOSTDEBUG
    debug_copy_rx(buffer,len);
#endif
    block_ava = block_receiver(rxbuffer,len,buffer);
    if (block_ava == -1) return(1);
    if (block_ava == 1) {
      if (!block_decoder(rxbuffer,state)) {
        if (((block_buffer_len) || (block_bcbuffer_len)) && (!busy_count)) {
          ok_display = send_cmd_data();
        }
        else {
          if (tnc_busy) { /* was TNC busy? */
            busy_count--; /* one wait less */
            if (busy_count == 0) { /* now resend ignored data */
              tnc_busy = 0;
              statlin_update();
              memcpy(txbuffer,txbuffer_save,len_save + 3);
#ifdef HOSTDEBUG
              debug_copy_tx(txbuffer_save,len_save + 3);
#endif
              res = write(serial,txbuffer_save,len_save + 3);
#ifdef DEBUGIO
  	      wrdebugio ( "w ", txbuffer_save,len_save + 3 ) ; 
#endif
              if (soft_tnc) {
                delay_send(res,len_save + 3);
              }
              ok_display = (txbuffer_save[1] == X_COMM);
              req_info = req_info_save;
              req_flag = req_flag_save;
              return(0);
            }
          }
          poll_tnc(state);
          ok_display = 0;
        }
      }  
    }   
  }
  return(0);
}

#ifdef USE_AX25K
void handle_tx_queue()
{
  while ((block_buffer_len) || (block_bcbuffer_len)) {
    send_cmd_data();
  }
}
#endif

void free_serial()
{
  free(ch_stat);
}

int alloc_serial()
{
  ch_stat = (struct channel_stat *)
    malloc(tnc_channels * sizeof(struct channel_stat));
  return(ch_stat == NULL);
}
#endif
