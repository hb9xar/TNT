/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for interfacing AX25-Kernel (ax25k.c)
   created: Mark Wahl DL4YBG 97/02/09
   updated: Johann Hanne DH3MB 99/01/02
   updated: Matthias Hensler WS1LS 99/03/12
*/

#include "tnt.h"

#ifdef USE_AX25K

/* poll status every 300ms */
#define AX25K_POLL 300000

#ifdef __GLIBC__xxx
#include <linux/if.h>
#include <linux/in.h>
#else
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#endif
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <linux/ax25.h>
#include <linux/netrom.h>
#ifndef SOCK_STREAM
#include <socketbits.h>
#endif

#ifndef SIOCAX25GETPARMS
#define NEW_KERNEL
#include <linux/rose.h>
#endif

/* includes from ax25 utilities */
#include <ax25/config.h>
#include <ax25/axutils.h>
#include <ax25/axconfig.h>
#include <ax25/nrconfig.h>
#include <ax25/version.h>
#include <ax25/procutils.h>

#include "ax25k.h"

extern void stat_display(int channel);
extern int next_qrg(char *qrg,char *port,int value);
extern int add_qrg(char *qrg,char *port);
extern void cmd_display(int flag,int channel,char *buffer,int cr);
extern void get_qrgpos(int *qrgpos,char *chanstr);
extern void statlin_update();
extern void next_command(int *state);
extern int find_call_channel(char *call);
extern void conn_boxcut(int channel);
extern void set_pwmode(int channel);
extern void set_remmode(int channel);
extern void set_flchkmode(int channel);
extern void data_display(int channel,char *buffer);
extern void action_on_connect(int channel);
extern void write_log(int channel);
extern void close_shell(int channel,int report,int disc);
extern void close_rxfile(int channel,int report);
extern void close_txfile(int channel,int report);
extern void close_xconnect(int channel,int success);
extern void clear_pwmode(int channel);
extern void close_iface_con(int channel,int deact,int disconnect);
extern void disc_boxcut(int channel);
extern void strip_call(char *call,int channel);
extern void next_connect(int channel);
extern void next_sendframe(int channel);
extern int decstathuf(char *src,char *dest);
extern void data_display_len(int channel, char *buffer);
extern void ax25_dump(unsigned char *data,int length,char *port);
extern int xconnect_first(int channel);
#ifdef USE_IFACE
struct sockaddr *build_sockaddr(const char *name, int *addrlen);
extern void send_cstatus(int channel, int frames);
extern int calc_xframes(int channel);
#endif
#ifdef TNT_SOUND
extern int play_sound(int);
#endif

extern struct channel_stat *ch_stat;
extern int tnc_channels;
extern int use_select;
extern int req_flag;
extern int act_state;
extern int is_root;

#ifdef NEW_KERNEL
enum {
  AX25_VALUES_IPDEFMODE,
  AX25_VALUES_AXDEFMODE,
  AX25_VALUES_BACKOFF,
  AX25_VALUES_CONMODE,
  AX25_VALUES_WINDOW,
  AX25_VALUES_EWINDOW,
  AX25_VALUES_T1,
  AX25_VALUES_T2,
  AX25_VALUES_T3,
  AX25_VALUES_N2,
  AX25_VALUES_DIGI,
  AX25_VALUES_IDLE,
  AX25_VALUES_PACLEN,
  AX25_VALUES_MAXQUEUE,
  AX25_VALUES_PROTOCOL,
  AX25_VALUES_DS_TIMEOUT,
  AX25_MAX_VALUES
};

static char * AXParamTable[AX25_MAX_VALUES] = {
  "ip_default_mode",
  "ax25_default_mode",
  "backoff_type",
  "connect_mode",
  "standard_window_size",
  "extended_window_size",
  "t1_timeout",
  "t2_timeout",
  "t3_timeout",
  "maximum_retry_count",
  "digipeat_mode",             /* gone with the wind... */
  "idle_timeout",
  "maximum_packet_length",
  "maxqueue",                  /* gone...  */
  "protocol",
  "dama_slave_timeout"
};
#endif

struct ax25k_stat {
  int fd;
  int af_type;
  struct full_sockaddr_ax25 ax25;
  int addrlen;
  int scan;
  char curcall[10];
  char port[10];
};

struct listen_stat {
  int axlfd;
  char call[10];
  char port[10];
  int mount_cnt;
  struct listen_stat *prev;
  struct listen_stat *next;
};

struct tnc_comm {
  char comm;
  void (*func)();
};

struct tnc_extcomm {
  char comm[10];
  void (*func)();
};

static struct ax25k_stat *ax25k_stat;
static struct listen_stat *listen_stat_root;
static int monfd;
static struct timeval last_ax25k_status;
char ax25k_port[80];
char moni_socket[80]="";
int ax25k_poll;
int fullmoni_flag;
int moni_para;

/* pseudo-TNC-variables */
int stamp_flag;

static char conn_str[] = "CONNECTED to";
static char disc_str[] = "DISCONNECTED fm";
static char notc_str[] = "CHANNEL NOT CONNECTED";
static char acon_str[] = "CHANNEL ALREADY CONNECTED";
static char invp_txt[] = "INVALID PARAMETERS";
static char invpo_txt[] = "INVALID PORT";
static char nwhc_txt[] = "NOT WHILE CONNECTED";
static char notl_err[] = "INTERNAL ERROR: NOT LISTENING";
static char canl_err[] = "CALLSIGN USED BY OTHER APPLICATION";
static char errparms_txt[] = "CANNOT GET/SET PARAMETERS";
static char diffport_txt[] = "CHANNEL CONNECTED ON DIFFERENT PORT";
static char valunk_txt[] = "VALUE UNKNOWN (KISS-PARAMETER)";

/* forward definitions */
static void analyse_monitor_ax25k(char *buffer,int len,
                                  struct sockaddr sa,int asize);
void gen_stamp(char *buffer,int type);


/* TNC command list */
static void ax_connect();
static void ax_disconnect();
static void ax_stamp();
static void ax_mycall();
static void ax_monitor();
static void ax_extcomm();
static void ax_tries();
static void ax_frack();
static void ax_digipeat();
static void ax_maxframe();
static void ax_txdelay();
static void ax_persist();
static void ax_slottime();

static struct tnc_comm tnc_comm[] = {
  {'C',ax_connect},
  {'D',ax_disconnect},
  {'F',ax_frack},
  {'I',ax_mycall},
  {'K',ax_stamp},
  {'M',ax_monitor},
  {'N',ax_tries},
  {'O',ax_maxframe},
  {'P',ax_persist},
  {'R',ax_digipeat},
  {'T',ax_txdelay},
  {'W',ax_slottime},
  {'@',ax_extcomm},
  {'#',ax_extcomm},
  {'\0',NULL}
};


/* TNC command list, extended commands */
static void ax_resptime();
static void ax_check();
static void ax_backoff();
static void ax_idle();
static void ax_paclen();
static void ax_fulldup();
static void ax_txtail();
static void ax_hardware();

static struct tnc_extcomm tnc_extcomm[] = {
  {"@D",ax_fulldup},
  {"@T2",ax_resptime},
  {"@T3",ax_check},
  {"@TA",ax_txtail},
  {"#BACKOFF",ax_backoff},
  {"#IDLE",ax_idle},
  {"#PACLEN",ax_paclen},
  {"#HARDWARE",ax_hardware},
  {"",NULL}
};

/*
 *	ax25 -> ascii conversion
 *      change: -0 is removed
 */
char *ax2asc_A(ax25_address *a)
{
	static char buf[11];
	char c, *s;
	int n;

	for (n = 0, s = buf; n < 6; n++) {
		c = (a->ax25_call[n] >> 1) & 0x7F;

		if (c != ' ') *s++ = c;
	}
	
	n = (a->ax25_call[6] >> 1) & 0x0F;
	if (n != 0) {
	
		*s++ = '-';

		if (n > 9) {
			*s++ = '1';
			n -= 10;
		}
	
		*s++ = n + '0';
	}
	*s++ = '\0';

	return buf;
}


void init_statline()
{
  int i;
  
  for (i=0;i<tnc_channels;i++) {
    stat_display(i);
  }
}

static int act_monitor()
{
  if (monfd != -1) {
    fprintf(stderr, "tnt: monitor already active\n");
    return 1;
  }

  if(moni_socket[0]==0) {
    int proto;

    if (!is_root) {
      moni_para = 0;
      return 0;
    }

    proto = ETH_P_AX25;
    if (fullmoni_flag) proto = ETH_P_ALL;

    if ((monfd = socket(AF_INET,SOCK_PACKET,htons(proto))) < 0) {
      fprintf(stderr, "tnt: socket: %s\n",strerror(errno));
      return 1;
    }
  }
  else
  {
    struct sockaddr *saddr;
    int servlen;

    saddr = build_sockaddr(moni_socket,&servlen);

    if (!saddr) {
      printf("tnt: invalid monitor-socket\n");
      return 1;
    }

    if ((monfd = socket(saddr->sa_family,SOCK_STREAM,0)) < 0) {
      fprintf(stderr,"tnt: error opening monitor-socket: %s\n",strerror(errno));
      return 1;
    }

    if (connect(monfd, saddr, servlen) < 0) {
      close(monfd);
      fprintf(stderr,"tnt: error connecting monitor-socket\n");
      monfd=-1;
      return 0;
    }
  }

  if (!use_select) {
    fcntl(monfd,F_SETFL,O_NONBLOCK);
  }
 
  return 0;
}

static int deact_monitor()
{
  if (monfd == -1) {
    fprintf(stderr, "tnt: monitor already deactived\n");
    return 1;
  }

  close(monfd);
  monfd = -1;

  return 0;
}

static int act_unproto()
{
  int unfd;
  char *addr;
  struct full_sockaddr_ax25 ax25;
  int addrlen;
  
  if (ax25k_stat[0].fd != -1) {
    fprintf(stderr, "tnt: unproto already active\n");
    return 1;
  }
  
  if ((addr = ax25_config_get_addr(ax25k_stat[0].port)) == NULL) {
    fprintf(stderr, "tnt: invalid AX25 port: %s\n",ax25k_stat[0].port);
    return 1;
  }
  
  ax25.fsa_ax25.sax25_family = AF_AX25;
  ax25.fsa_ax25.sax25_ndigis = 1;
  convert_call_entry(addr,ax25.fsa_digipeater[0].ax25_call);
  convert_call_entry(ch_stat[0].curcall,ax25.fsa_ax25.sax25_call.ax25_call);
  addrlen = sizeof(struct full_sockaddr_ax25);
  
  if ((unfd = socket(AF_AX25,SOCK_DGRAM,0)) == -1) {
    fprintf(stderr, "tnt: socket: %s\n",strerror(errno));
    return 1;
  }

  if (!use_select) {
    fcntl(unfd,F_SETFL,O_NONBLOCK);
  }

  if ((bind(unfd,(struct sockaddr *)&ax25,addrlen)) == -1) {
    fprintf(stderr, "tnt: bind: %s\n",strerror(errno));
    close(unfd);
    return 1;
  }
  
  ax25k_stat[0].fd = unfd;
  
  return 0;
}

static int deact_unproto()
{
  if (ax25k_stat[0].fd == -1) {
    fprintf(stderr, "tnt: unproto already deactived\n");
    return 1;
  }

  close(ax25k_stat[0].fd);
  ax25k_stat[0].fd = -1;

  return 0;
}


static int act_listen(call,port)
char *call;
char *port;
{
  int axlfd;
  char *addr;
  struct listen_stat *listen_stat_ptr;
  struct full_sockaddr_ax25 ax25;
  int addrlen;
  
  addr = ax25_config_get_addr(port);
  if (addr == NULL) {
    fprintf(stderr, "tnt: invalid AX25 port: %s\n",port);
    return 1;
  }
  
  listen_stat_ptr = listen_stat_root;

  while (listen_stat_ptr != NULL) {
    if ((strcmp(listen_stat_ptr->call,call) == 0) &&
        (strcmp(listen_stat_ptr->port,port) == 0)) {
      listen_stat_ptr->mount_cnt++;
      return 0;
    }
    listen_stat_ptr = listen_stat_ptr->next;
  }
  
  listen_stat_ptr = (struct listen_stat *)malloc(sizeof(struct listen_stat));
  if (listen_stat_ptr == NULL) {
    fprintf(stderr, "tnt: malloc-error in 'act_listen'\n");
    return 1;
  }
  
  ax25.fsa_ax25.sax25_family = AF_AX25;
  ax25.fsa_ax25.sax25_ndigis = 1;
  convert_call_entry(addr,ax25.fsa_digipeater[0].ax25_call);
  convert_call_entry(call,ax25.fsa_ax25.sax25_call.ax25_call);
  addrlen = sizeof(struct full_sockaddr_ax25);
  
  if ((axlfd = socket(AF_AX25,SOCK_SEQPACKET,0)) < 0) {
    fprintf(stderr, "tnt: socket: %s\n",strerror(errno));
    free(listen_stat_ptr);
    return 1;
  }

  if (!use_select) {
    fcntl(axlfd,F_SETFL,O_NONBLOCK);
  }
  
  if (bind(axlfd,(struct sockaddr *)&ax25,addrlen) < 0) {
    fprintf(stderr, "tnt: bind: %s\n",strerror(errno));
    close(axlfd);
    free(listen_stat_ptr);
    return 1;
  }
  
  if (listen(axlfd, SOMAXCONN) < 0) {
    fprintf(stderr, "tnt: listen: %s\n",strerror(errno));
    close(axlfd);
    free(listen_stat_ptr);
    return 1;
  }

  listen_stat_ptr->next = listen_stat_root;
  listen_stat_ptr->prev = NULL;
  if (listen_stat_root != NULL)
    listen_stat_root->prev = listen_stat_ptr;
  listen_stat_root = listen_stat_ptr;
  listen_stat_ptr->axlfd = axlfd;
  strcpy(listen_stat_ptr->call,call);
  strcpy(listen_stat_ptr->port,port);
  listen_stat_ptr->mount_cnt = 1;

  return 0;
}

static int deact_listen(call,port)
char *call;
char *port;
{
  char *addr;
  struct listen_stat *listen_stat_ptr;

  addr = ax25_config_get_addr(port);
  if (addr == NULL) {
    fprintf(stderr, "tnt: invalid AX25 port: %s\n",port);
    return 1;
  }
  
  listen_stat_ptr = listen_stat_root;

  while (listen_stat_ptr != NULL) {
    if ((strcmp(listen_stat_ptr->call,call) == 0) &&
        (strcmp(listen_stat_ptr->port,port) == 0)) {
      listen_stat_ptr->mount_cnt--;
      if (listen_stat_ptr->mount_cnt == 0) {
        close(listen_stat_ptr->axlfd);
        if (listen_stat_ptr->next != NULL)
          listen_stat_ptr->next->prev = listen_stat_ptr->prev;
        if (listen_stat_ptr->prev == NULL)
          listen_stat_root = listen_stat_ptr->next;
        else
          listen_stat_ptr->prev->next = listen_stat_ptr->next;
        return 0;
      }
      return 0;
    }
    listen_stat_ptr = listen_stat_ptr->next;
  }

  fprintf(stderr, "tnt: call %s, port %s not listening\n",call,port);
  return 1;  
}

static int handle_allport(call,port_str,deact)
char *call;
char *port_str;
int deact;
{
  char qrg[20];
  char port[10];
  int res;
  int err;

  if (strcmp(port_str,"*") != 0) {
    if (deact) return(deact_listen(call,port_str));
    else return(act_listen(call,port_str));
  }
  res = 0;
  err = 0;
  while ((res != -1) && (!err)) {
    if ((res = next_qrg(qrg,port,res)) != -1) {
      if (deact) {
        err += deact_listen(call,port);
      }
      else {
        err += act_listen(call,port);
      }
    }
  }
  return(err > 0);
}

int init_ax25k()
{
  char *port;
  char *addr;
  int i;
  char *qrg;
  struct timezone tz;

  monfd = -1;
  moni_para = MONI_UNPR | MONI_INFO | MONI_SUPV | MONI_CONN;
  stamp_flag = 0;
  gettimeofday(&last_ax25k_status,&tz);
  ax25k_poll = AX25K_POLL;

  for (i=0;i<tnc_channels;i++) {
    ax25k_stat[i].fd = -1;
    ch_stat[i].pause_flag = 0;
    ch_stat[i].sendcook = 0;
    ch_stat[i].contcon = 0;
    ch_stat[i].mycall[0] = '\0';
    ch_stat[i].log_call[0] = '\0';
    ch_stat[i].disp_call[0] = '\0';
    ch_stat[i].name[0] = '\0';
    ch_stat[i].start_time = time(NULL);
    ch_stat[i].state = 0;
    ch_stat[i].oldstate = 0;
    ch_stat[i].maxstate = 0;
    ch_stat[i].conn_state = CS_DISCON;
    ch_stat[i].ignore_state = 0;
    ch_stat[i].huffcod = 0;
    ch_stat[i].access_level = 0;
    ch_stat[i].pwwait = 0;
    ch_stat[i].flchkmode = 0;
    strcpy(ch_stat[i].call,notc_str);
  }
  
  if (ax25_config_load_ports() == 0) {
    fprintf(stderr, "tnt: no AX.25 port data configured\n");
    return 1;
  }

  /*nr_config_load_ports();*/

  port = NULL;
  
  do {
    if ((port = ax25_config_get_next(port)) != NULL) {
      qrg = ax25_config_get_desc(port);
      add_qrg(qrg,port);
    }
  } while (port != NULL);

  addr = ax25_config_get_addr(ax25k_port);
  if (addr == NULL) {
    fprintf(stderr, "tnt: invalid default AX25 port: %s\n",port);
    return 1;
  }

  strcpy(ch_stat[0].curcall,addr);
  strcpy(ax25k_stat[0].curcall,addr);
  strcpy(ch_stat[0].mycall,addr);
  strcpy(ax25k_stat[0].port,ax25k_port);
  strcpy(ch_stat[0].call,"TNT");

  /* open unproto socket */
  act_unproto();
  
  /* set calls for all channels */
  for (i=1;i<tnc_channels;i++) {
    handle_allport(addr,"*",0);
    strcpy(ch_stat[i].curcall,addr);
    strcpy(ax25k_stat[i].curcall,addr);
    strcpy(ch_stat[i].mycall,addr);
  }

  if (act_monitor()) return 1;
  
  return 0;
}

void exit_ax25k()
{
  int i;
  
  deact_unproto();
  
  for (i=1;i<tnc_channels;i++) {
    if (ax25k_stat[i].fd != -1) close(ax25k_stat[i].fd);
    handle_allport(ax25k_stat[i].curcall,"*",1);
  }

  if(monfd!=-1)
    deact_monitor();
}

static char *strip_port(char *data,int len,char *port)
{
  char *ptr;
  int qrgpos;
  char qrg[20];
  char port_str[11];
  
  ptr = strchr(data,':');
  if (ptr != NULL) {
    if ((ptr - data) < 10) {
      strncpy(port_str,data,ptr+1 - data);
      port_str[ptr+1 - data] = '\0';
      qrgpos = -1;
      get_qrgpos(&qrgpos,port_str);
      if (qrgpos == -1) {
        return NULL;
      }
      if (next_qrg(qrg,port,qrgpos) == -1) {
        return NULL;
      }
      ptr++; 
    }
    else {
      return NULL;
    }
  }
  else {
    strcpy(port,ax25k_port);
    ptr = data;
  }
  return ptr;
}

static void reset_conn(channel)
int channel;
{
  ch_stat[channel].conn_state = CS_DISCON;
  strcpy(ch_stat[channel].call,notc_str);
  ch_stat[channel].name[0] = '\0';
}

static void ax_connect(int channel,char *data,int len)
{
  int fd;
  struct full_sockaddr_ax25 ax25;
  int addrlen;
  struct full_sockaddr_ax25 ax25_d;
  int addrlen_d;
  int i;
  char *ptr;
  char port[10];
  char callsign[300];
  
  if (len == 0) {
    if ((ax25k_stat[channel].fd != -1) || (channel == 0))
      sprintf(callsign,"%s:%s",ax25k_stat[channel].port,ch_stat[channel].call);
    else
      sprintf(callsign,"%s",ch_stat[channel].call);
    cmd_display(req_flag,channel,callsign,1);
    return;
  }
  
  /* special handling for unproto channel */
  if (channel == 0) {
    ptr = strip_port(data,len,port);
    if (ptr == NULL) {
      cmd_display(req_flag,channel,invpo_txt,1);
      return;
    }
    for (i=0;i<strlen(ptr);i++) {
      ch_stat[0].call[i] = toupper(ptr[i]);
    }
    ch_stat[0].call[strlen(ptr)] = '\0';
    if (strcmp(port,ax25k_stat[0].port) != 0) {
      strcpy(ax25k_stat[0].port,port);
      deact_unproto();
      act_unproto();
    }
    cmd_display(req_flag,channel,OK_TEXT,1);
    return;
  }

  if (ax25k_stat[channel].fd != -1) {
    cmd_display(req_flag,channel,acon_str,1);
    return;
  }
  
  ptr = strip_port(data,len,port);
  if (ptr == NULL) {
    cmd_display(req_flag,channel,invpo_txt,1);
    reset_conn(channel);
    return;
  }
  
  addrlen_d = convert_call(ptr,&ax25_d);
  
  if (addrlen_d == -1) {
    cmd_display(req_flag,channel,invp_txt,1);
    reset_conn(channel);
    return;
  }
  
  fd = socket(AF_AX25,SOCK_SEQPACKET,0);
  if (fd < 0) {
    cmd_display(req_flag,channel,"Error: can't open socket",1);
    reset_conn(channel);
    return;
  }
  
  ax25.fsa_ax25.sax25_family = AF_AX25;
  ax25.fsa_ax25.sax25_ndigis = 1;
  convert_call_entry(ch_stat[channel].curcall,
                     ax25.fsa_ax25.sax25_call.ax25_call);
  convert_call_entry(ax25_config_get_addr(port),
                     ax25.fsa_digipeater[0].ax25_call);
  addrlen = sizeof(struct full_sockaddr_ax25);

  if (bind(fd,(struct sockaddr *)&ax25,addrlen) == -1) {
    close(fd);
    cmd_display(req_flag,channel,"Error: can't bind socket",1);
    reset_conn(channel);
    return;
  }
  
  i = TRUE;
  ioctl(fd,FIONBIO,&i);
  
  if (connect(fd,(struct sockaddr *)&ax25_d,addrlen_d)) {
    if (errno != EINPROGRESS) {
      close(fd);
      cmd_display(req_flag,channel,"Error: connect",1);
      reset_conn(channel);
      return;
    }
  }
  ax25k_stat[channel].fd = fd;
  ax25k_stat[channel].af_type = AF_AX25;
  memcpy(&(ax25k_stat[channel].ax25),(char *)&ax25_d,addrlen_d);
  ax25k_stat[channel].addrlen = addrlen_d;
  strcpy(ch_stat[channel].call,ax2asc_A(&(ax25_d.fsa_ax25.sax25_call)));
  i = 0;
  if (ax25_d.fsa_ax25.sax25_ndigis > 0) {
    strcat(ch_stat[channel].call," via");
  }
  while (i < ax25_d.fsa_ax25.sax25_ndigis) {
    strcat(ch_stat[channel].call," ");
    strcat(ch_stat[channel].call,ax2asc_A(&(ax25_d.fsa_digipeater[i])));
    i++;
  }
  ch_stat[channel].conn_state = CS_SETUP;
  ch_stat[channel].state = 1;
  strcpy(ax25k_stat[channel].port,port);
  statlin_update();
  stat_display(channel);
  cmd_display(req_flag,channel,OK_TEXT,1);
}

static void ax_disconnect(int channel,char *data,int len)
{
  if ((ax25k_stat[channel].fd != -1) &&
     (ch_stat[channel].conn_state != CS_DISCON)) {
    close(ax25k_stat[channel].fd);
    ax25k_stat[channel].fd = -1;
    cmd_display(req_flag,channel,OK_TEXT,1);
  }
  else {
    cmd_display(req_flag,channel,notc_str,1);
  }
}

static void ax_extcomm(int channel,char *data,int len)
{
  int i;
  int slen;
  
  i = 0;
  while (tnc_extcomm[i].comm[0] != '\0') {
    if ((slen = strlen(tnc_extcomm[i].comm)) <= len) {
      if (strncmp(tnc_extcomm[i].comm,data,slen) == 0) {
        while (*(data+slen) == ' ') slen++;
        (*tnc_extcomm[i].func)(channel,data+slen,len-slen);
        return;
      }
    }
    i++;
  }
  cmd_display(req_flag,channel,"Invalid command",1);
}

void command_ax25k(int channel,char *data,int len)
{
  int i;
  char buffer[257];
  int slen;
  
  if ((len == 0) || (len > 256)) {
    cmd_display(req_flag,channel,"Invalid command string",1);
    return;
  }
  
  /* put a NULL at string end */
  memcpy(buffer,data,len);
  buffer[len] ='\0';
  
  i = 0;
  while (tnc_comm[i].comm != '\0') {
    if (*data == tnc_comm[i].comm) {
      if ((*data == '#') || (*data == '@')) {
        (*tnc_comm[i].func)(channel,buffer,len);
      }
      else {
        slen = 1;
        while (*(buffer+slen) == ' ') slen++;
        (*tnc_comm[i].func)(channel,buffer+slen,len-slen);
      }
      next_command(&act_state);
      return;
    }
    i++;
  }
  cmd_display(req_flag,channel,"Invalid command",1);
  next_command(&act_state);
}

static void open_ax25k(fd,ax25,len,call,port)
int fd;
struct full_sockaddr_ax25 *ax25;
int len;
char *call;
char *port;
{
  int channel;
  char buffer[256];
  char stamp[40];
  int i;
  
  channel = find_call_channel(call);
  if (channel == -1) {
    close(fd);
    return;
  }
  ax25k_stat[channel].fd = fd;
  ax25k_stat[channel].af_type = AF_AX25;
  memcpy(&(ax25k_stat[channel].ax25),(char *)ax25,len);
  ax25k_stat[channel].addrlen = len;
  strcpy(ch_stat[channel].call,ax2asc_A(&(ax25->fsa_ax25.sax25_call)));
  i = 0;
  if (ax25->fsa_ax25.sax25_ndigis > 0) {
    strcat(ch_stat[channel].call," via");
  }
  while (i < ax25->fsa_ax25.sax25_ndigis) {
    strcat(ch_stat[channel].call," ");
    strcat(ch_stat[channel].call,ax2asc_A(&(ax25->fsa_digipeater[i])));
    i++;
  }
  strcpy(ax25k_stat[channel].port,port);
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
  ch_stat[channel].state = 4;
  ch_stat[channel].snd_frms = 0;
  #ifdef USE_IFACE
  conn_boxcut(channel);
  #endif
  set_pwmode(channel);
  ch_stat[channel].remote = 1;
  set_remmode(channel);
  set_flchkmode(channel);
  statlin_update();
  stat_display(channel);
  gen_stamp(stamp,ST_STAT);
  sprintf(buffer,"(%d) %s %s:%s%s",channel,conn_str,port,ch_stat[channel].call,stamp);
#ifdef TNT_SOUND
  play_sound(2);
#endif
  data_display(channel,buffer);
  action_on_connect(channel);
}

static void consucc_ax25k(int channel)
{
  char buffer[256];
  char stamp[40];
  int i;

  i = FALSE;
  ioctl(ax25k_stat[channel].fd,FIONBIO,&i);

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
  ch_stat[channel].snd_frms = 0;
  #ifdef USE_IFACE
  conn_boxcut(channel);
  #endif
  set_pwmode(channel);
  ch_stat[channel].remote = 1;
  set_remmode(channel);
  set_flchkmode(channel);
  statlin_update();
  stat_display(channel);
  gen_stamp(stamp,ST_STAT);
  sprintf(buffer,"(%d) %s %s:%s%s",channel,conn_str,ax25k_stat[channel].port,
          ch_stat[channel].call,stamp);
#ifdef TNT_SOUND
  play_sound(2);
#endif
  data_display(channel,buffer);
}

static void close_ax25k(int channel)
{
  char buffer[256];
  char tmpstr[20];
  char stamp[40];
  
  close(ax25k_stat[channel].fd);
  gen_stamp(stamp,ST_STAT);
  sprintf(buffer,"(%d) %s %s:%s%s",channel,disc_str,ax25k_stat[channel].port,
          ch_stat[channel].call,stamp);
#ifdef TNT_SOUND
  play_sound(3);
#endif
  data_display(channel,buffer);
  ax25k_stat[channel].fd = -1;
  if (ch_stat[channel].conn_state == CS_CONN) {
    ch_stat[channel].end_time = time(NULL);
    if (!ch_stat[channel].flchkmode)
      write_log(channel);
  }
  ch_stat[channel].conn_state = CS_DISCON;
  close_shell(channel,0,0);
  close_rxfile(channel,0);
  close_txfile(channel,0);
  close_xconnect(channel,0);
  clear_pwmode(channel);
  #ifdef USE_IFACE
  close_iface_con(channel,1,0);
  disc_boxcut(channel);
  #endif
  strcpy(ch_stat[channel].call,notc_str);
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
  ch_stat[channel].state = 0;
  ch_stat[channel].snd_frms = 0;
  ch_stat[channel].tries = 0;
  ch_stat[channel].unacked = 0;
  if (ch_stat[channel].mycall[0] != '\0') {
    strcpy(tmpstr,"I");
    strcat(tmpstr,ch_stat[channel].mycall);
    req_flag = M_CMDSCRIPT;
    command_ax25k(channel,tmpstr,strlen(tmpstr));
  }
  statlin_update();
  stat_display(channel);
}

static void dissucc_ax25k(int channel)
{
  char buffer[256];
  char tmpstr[20];
  char stamp[40];

  gen_stamp(stamp,ST_STAT);  
  sprintf(buffer,"(%d) %s %s:%s%s",channel,disc_str,ax25k_stat[channel].port,
          ch_stat[channel].call,stamp);
#ifdef TNT_SOUND
  play_sound(3);
#endif
  data_display(channel,buffer);
  ax25k_stat[channel].fd = -1;
  if (ch_stat[channel].conn_state == CS_CONN) {
    ch_stat[channel].end_time = time(NULL);
    if (!ch_stat[channel].flchkmode)
      write_log(channel);
  }
  ch_stat[channel].conn_state = CS_DISCON;
  close_shell(channel,0,0);
  close_rxfile(channel,0);
  close_txfile(channel,0);
  close_xconnect(channel,0);
  clear_pwmode(channel);
  #ifdef USE_IFACE
  close_iface_con(channel,1,0);
  disc_boxcut(channel);
  #endif
  strcpy(ch_stat[channel].call,notc_str);
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
  ch_stat[channel].state = 0;
  ch_stat[channel].snd_frms = 0;
  ch_stat[channel].tries = 0;
  ch_stat[channel].unacked = 0;
  if (ch_stat[channel].mycall[0] != '\0') {
    strcpy(tmpstr,"I");
    strcat(tmpstr,ch_stat[channel].mycall);
    req_flag = M_CMDSCRIPT;
    command_ax25k(channel,tmpstr,strlen(tmpstr));
  }
  statlin_update();
  stat_display(channel);
}

void status_ax25k(int always)
{
  struct proc_ax25 *p;
  struct proc_ax25 *porg;
  char dcall[10],scall[10];
  char cdevice[4];
  int channel;
  int changed;
  int snd_frms,tries,unacked,axstate;
  struct timezone tz;
  struct timeval curr_timeval;
  int found;
#ifdef USE_IFACE
  int frames;
#endif
  
  gettimeofday(&curr_timeval,&tz);
  if (!always) {
    if (((curr_timeval.tv_sec - last_ax25k_status.tv_sec) < 1) &&
        ((curr_timeval.tv_usec - last_ax25k_status.tv_usec) < ax25k_poll))
      return;
  }
  last_ax25k_status.tv_sec = curr_timeval.tv_sec;
  last_ax25k_status.tv_usec = curr_timeval.tv_usec;

  p = read_proc_ax25();
  if (p == NULL) {
    fprintf(stderr,"tnt: error reading ax25-status (read_proc_ax25)\n");
  }
  else {
    for (channel=1;channel<tnc_channels;channel++) {
      ax25k_stat[channel].scan = 0;
      switch (ch_stat[channel].conn_state) {
      case CS_DISCON:
        if (ch_stat[channel].snd_frms) {
          ch_stat[channel].snd_frms = 0;
          stat_display(channel);
        }
        break;
      case CS_SETUP:
        if (!xconnect_first(channel)) {
          ax25k_stat[channel].scan = 1;
        }
        break;
      case CS_CONN:
        ax25k_stat[channel].scan = 1;
        break;
      }
    }
    porg = p;
    while (p != NULL) {
      if (strcmp(p->dest_addr,"*") == 0) {
        p = p->next;
        continue;
      }
      found = 0;
      channel = 1;
      while ((channel<tnc_channels) && (!found)) {
        if (!ax25k_stat[channel].scan) {
          channel++;
          continue;
        }
        strip_call(dcall,channel);
        if (strchr(dcall,'-') == NULL) strcat(dcall,"-0");
        strcpy(scall,ch_stat[channel].curcall);
        if (strchr(scall,'-') == NULL) strcat(scall,"-0");
        strcpy(cdevice,ax25_config_get_dev(ax25k_stat[channel].port));
        if ((strcmp(p->dest_addr,dcall) == 0) &&
            (strcmp(p->src_addr,scall) == 0) &&
            (strcmp(p->dev,cdevice) == 0)) {
          ax25k_stat[channel].scan = 0;
          found = 1;
          tries = p->n2count;
          if (p->vs < p->va)
            unacked = 8 + p->vs - p->va;
          else
            unacked = p->vs - p->va;
          snd_frms = p->sndq / 256;
          switch (p->st) {
          case 0:
          case 1:
            axstate = p->st;
            break;
          case 2:
            axstate = 3;
            break;
          case 3:
            axstate = 4;
            break;
          case 4:
            axstate = 6;
            break;
          default:
            axstate = 0;
            break;
          }
          
          changed = 0;
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
            ch_stat[channel].state = axstate;
            changed = 1;
          }
          if ((ch_stat[channel].conn_state == CS_SETUP) &&
              (ch_stat[channel].state >= 4)) {
            consucc_ax25k(channel);
          }
          if (ch_stat[channel].state > ch_stat[channel].maxstate)
            ch_stat[channel].maxstate = ch_stat[channel].state;
          if (changed) {
            stat_display(channel);
#ifdef USE_IFACE
            frames = calc_xframes(channel);
            send_cstatus(channel,frames);
#endif
          }
          if (ch_stat[channel].contcon) {
            next_connect(channel);
            ch_stat[channel].contcon = 0;
          }
          next_sendframe(channel);
        }
        channel++;
      }
      p = p->next;
    }
    free_proc_ax25(porg);
    for (channel=1;channel<tnc_channels;channel++) {
      if ((ch_stat[channel].conn_state != CS_DISCON) &&
          (ax25k_stat[channel].scan == 1)) {
        dissucc_ax25k(channel);
      }
    }
  }
}

void write_ax25k(int channel,char *buffer,int len)
{
  int result;
  int dlen;
  struct full_sockaddr_ax25 dest;
  
  if (ax25k_stat[channel].fd == -1) return;
  if (channel == 0) {
    if ((dlen = convert_call(ch_stat[0].call,&dest)) == -1) {
      fprintf(stderr, "tnt: Can't convert unproto address\n");
      return;
    }
    if (sendto(ax25k_stat[0].fd,buffer,len,0,
               (struct sockaddr *)&dest,dlen) == -1) {
      fprintf(stderr, "tnt: sendto: %s\n",strerror(errno));
    }
    return;
  }
  result = write(ax25k_stat[channel].fd,buffer,len);
  if (result == -1) close_ax25k(channel);
}

static int read_ax25k(int channel)
{
  int displayed;
  int len;
  char buffer[PACKETSIZE+1];
  char huffbuffer[PACKETSIZE+1];
  
  len = read(ax25k_stat[channel].fd,buffer+1,PACKETSIZE);
  if (len <= 0) {
    if ((len == -1) && (errno == EAGAIN)) { /* no data */
      return(0);
    }
    else { /* EOF or error */
      close_ax25k(channel);
      return(1);
    }
  }
  /* update status if first successful read from socket */
  if (ch_stat[channel].conn_state != CS_CONN) {
    status_ax25k(1);
  }
  buffer[0] = (char)len - 1;
  displayed = 0;
  if (ch_stat[channel].huffcod) {
    if (!decstathuf(buffer,huffbuffer)) {
      data_display_len(channel,huffbuffer);
      displayed = 1;
    }
  }
  if (!displayed) {
    data_display_len(channel,buffer);
  }
  return(1);
}

#define MONBUFLEN 1500

static int readmon_ax25k()
{
  char monbuf[MONBUFLEN];
  int size;
  struct sockaddr sa;
  int asize;

  if(monfd==-1)
    return(1);

  asize = sizeof(sa);
  size = recvfrom(monfd,monbuf,sizeof(monbuf),0,&sa,&asize);
  if (size <= 0) {
    if (!((size == -1) && (errno == EAGAIN))) {
      fprintf(stderr, "tnt: Error reading from monitor socket - monitor disabled\n");
      deact_monitor();
    }
    return(0);
  }
  analyse_monitor_ax25k(monbuf,size,sa,asize);
  return(1);
}

void ax25k_fdset(max_fd,fdmask)
int *max_fd;
fd_set *fdmask;
{
  int i;
  struct listen_stat *listen_stat_ptr;
  
  listen_stat_ptr = listen_stat_root;
  while (listen_stat_ptr != NULL) {
    FD_SET(listen_stat_ptr->axlfd,fdmask);
    if (listen_stat_ptr->axlfd > ((*max_fd) - 1)) {
      *max_fd = listen_stat_ptr->axlfd + 1;
    }
    listen_stat_ptr = listen_stat_ptr->next;
  }

  if(monfd != -1) { /* DH3MB */
    FD_SET(monfd,fdmask);
    if (monfd > ((*max_fd) - 1)) {
      *max_fd = monfd + 1;
    }
  }

  for (i=1;i<tnc_channels;i++) {
    if (ax25k_stat[i].fd != -1) {
      FD_SET(ax25k_stat[i].fd,fdmask);
      if (ax25k_stat[i].fd > ((*max_fd) - 1)) {
        *max_fd = ax25k_stat[i].fd + 1;
      }
    }
  }
}

int ax25k_receive(fdmask)
fd_set *fdmask;
{
  int newfd;
  int addrlen;
  struct full_sockaddr_ax25 ax25;
  int i;
  int result;
  struct listen_stat *listen_stat_ptr;
  
  result = 1;
  if (use_select) {
    listen_stat_ptr = listen_stat_root;
    while (listen_stat_ptr != NULL) {
      if (FD_ISSET(listen_stat_ptr->axlfd,fdmask)) {
        i = TRUE;
        ioctl(listen_stat_ptr->axlfd,FIONBIO,&i);
        addrlen = sizeof(struct full_sockaddr_ax25);
        newfd = accept(listen_stat_ptr->axlfd,(struct sockaddr *)&ax25,
                       &addrlen);
        i = FALSE;
        ioctl(listen_stat_ptr->axlfd,FIONBIO,&i);
        if (newfd != -1) {
          open_ax25k(newfd,&ax25,addrlen,listen_stat_ptr->call,
                     listen_stat_ptr->port);
        }
        result = 0;
      }
      listen_stat_ptr = listen_stat_ptr->next;
    }

    if (monfd != -1 && FD_ISSET(monfd,fdmask)) {
      readmon_ax25k();
      result = 0;
    }
  }
  else {
    listen_stat_ptr = listen_stat_root;
    while (listen_stat_ptr != NULL) {
      if ((newfd = accept(listen_stat_ptr->axlfd,
                          (struct sockaddr *)&ax25,&addrlen)) >= 0) {
        fcntl(newfd,F_SETFL,O_NONBLOCK);
        open_ax25k(newfd,&ax25,addrlen,listen_stat_ptr->call,
                   listen_stat_ptr->port);
        result = 0;
      }
      listen_stat_ptr = listen_stat_ptr->next;
    }

    if (readmon_ax25k()) {
      result = 0;
    }
  }
  for (i=1;i<tnc_channels;i++) {
    if (ax25k_stat[i].fd != -1) {
      if (use_select) {
        if (FD_ISSET(ax25k_stat[i].fd,fdmask)) {
          read_ax25k(i);
          result = 0;
        }
      }
      else {
        if (read_ax25k(i)) {
          result = 0;
        }
      }
    }
  }
  return(result);
}

void free_ax25k()
{
  free(ax25k_stat);
}

int alloc_ax25k()
{
  ax25k_stat = (struct ax25k_stat *)
    malloc(tnc_channels * sizeof(struct ax25k_stat));
  return(ax25k_stat == NULL);
}

static void analyse_monitor_ax25k(buffer,len,sa,asize)
char *buffer;
int len;
struct sockaddr sa;
int asize;
{
  char *port;
  struct ifreq ifr;
  
  if (fullmoni_flag) {
    strcpy(ifr.ifr_name,sa.sa_data);
    if (ioctl(monfd,SIOCGIFHWADDR,&ifr) < 0) {
      fprintf(stderr, "tnt: Cannot get HWADDR\n");
    }
    else {
      if (ifr.ifr_hwaddr.sa_family == AF_AX25) {
        if ((buffer[0] & 0x0f) == 0) {
          if (moni_socket[0]==0) { /* DH3MB */
            port = ax25_config_get_name(sa.sa_data);
            if (port == NULL) port = sa.sa_data;
          } else
            port = 0;
          ax25_dump((unsigned char *)buffer+1,len-1,port);
        }
      }
/*    if (len > 16) {
        -* BPQ Ether header is missing here *-
        if (buffer[12] == 0x08 && buffer[13] == 0xFF) {
          if (moni_socket[0]==0) { *//* DH3MB *//*
            port = ax25_config_get_name(sa.sa_data);
            if (port == NULL) port = sa.sa_data;
          } else
            port = 0;
          ax25_dump((unsigned char *)buffer+16,
                    buffer[14] + buffer[15] * 256 - 5,port);
        }
      } */
    }
  }
  else {
    if ((buffer[0] & 0x0f) == 0) {
      if (moni_socket[0]==0) { /* DH3MB */
        port = ax25_config_get_name(sa.sa_data);
        if (port == NULL) port = sa.sa_data;
      } else
        port = 0;
      ax25_dump((unsigned char *)buffer+1,len-1,port);
    }
  }
}

void gen_stamp(char *buffer,int type)
{
  struct tm cvtime;
  time_t ctime;
  
  buffer[0] = '\0';
  if (!stamp_flag && (type != ST_EVER)) return;
  if (type == ST_MONI && stamp_flag == 1) return;
  ctime = time(NULL);
  cvtime = *localtime(&ctime);
  sprintf(buffer," - %2.2u.%2.2u.%2.2u %2.2u:%2.2u:%2.2u",
          cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
          cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec);
}

static void ax_stamp(int channel,char *data,int len)
{
  char temp[40];
  char answer[257];
  int value;
  int res;
  
  if (len == 0) {
    gen_stamp(temp,ST_EVER);
    sprintf(answer,"%d%s",stamp_flag,temp);
    cmd_display(req_flag,channel,answer,1);
    return;
  }
  res = sscanf(data,"%d%s",&value,temp);
  if (res == 1) {
    if ((value >= 0) && (value <= 2)) {
      stamp_flag = value;
      cmd_display(req_flag,channel,OK_TEXT,1);
      return;
    }
  }
  cmd_display(req_flag,channel,invp_txt,1);
}

static void ax_mycall(int channel,char *data,int len)
{
  char string[257];
  int res;
  int slen;
  int i;
  char curcall[20];
  int ssid;
  char *ptr;
    
  if (len == 0) {
    cmd_display(req_flag,channel,ch_stat[channel].curcall,1);
    return;
  }
  if (channel != 0) {
    if ((ch_stat[channel].conn_state == CS_CONN) ||
        ((ch_stat[channel].conn_state == CS_SETUP) &&
         (!xconnect_first(channel)))) {
      cmd_display(req_flag,channel,nwhc_txt,1);
      return;
    }
  }
  res = sscanf(data,"%s",string);
  if (res == 1) {
    ssid = 0;
    ptr = strchr(string,'-');
    if (ptr != NULL) {
      *ptr = '\0';
      res = sscanf(ptr+1,"%d",&ssid);
      if (res != 1) ssid = -1;
    }
    slen = strlen(string);
    if ((slen <= 6) && (ssid >= 0) && (ssid <= 15)) {
      if (ssid == 0)
        sprintf(curcall,"%s",string);
      else
        sprintf(curcall,"%s-%d",string,ssid);
      for (i=0;i<strlen(curcall);i++) {
        curcall[i] = toupper(curcall[i]);
      }
      if (channel == 0) {
        strcpy(ax25k_stat[channel].curcall,curcall);
        strcpy(ch_stat[channel].curcall,curcall);
        deact_unproto();
        act_unproto();
        cmd_display(req_flag,channel,OK_TEXT,1);
        return;
      }
      else {
        if (handle_allport(ax25k_stat[channel].curcall,"*",1)) {
          cmd_display(req_flag,channel,notl_err,1);
          return;
        }
        if (handle_allport(curcall,"*",0)) {
          handle_allport(ax25k_stat[channel].curcall,"*",0);
          strcpy(ch_stat[channel].curcall,curcall);
          cmd_display(req_flag,channel,canl_err,1);
          return;
        }
        strcpy(ax25k_stat[channel].curcall,curcall);
        strcpy(ch_stat[channel].curcall,curcall);
        cmd_display(req_flag,channel,OK_TEXT,1);
        return;
      }
    }
  }
  cmd_display(req_flag,channel,invp_txt,1);
}

static void ax_monitor(int channel,char *data,int len)
{
  char string[257];
  char moni_str[10];
  int res;
  int slen;
  int i;
    
  if (len == 0) {
    if (moni_para) {
      moni_str[0] = '\0';
      if ((moni_para & MONI_UNPR)) strcat(moni_str,"U");
      if ((moni_para & MONI_INFO)) strcat(moni_str,"I");
      if ((moni_para & MONI_SUPV)) strcat(moni_str,"S");
      if ((moni_para & MONI_CONN)) strcat(moni_str,"C");
    }
    else {
      strcpy(moni_str,"N");
    }
    cmd_display(req_flag,channel,moni_str,1);
    return;
  }
  res = sscanf(data,"%s",string);
  if (res == 1) {
    slen = strlen(string);
    if ((slen > 0) && (len <= 4)) {
      for (i=0;i<slen;i++) string[i] = toupper(string[i]);
      if (strchr(string,'N') != NULL) {
        if (monfd!=-1) deact_monitor();
        moni_para = 0;
      }
      else {
        if (monfd==-1) act_monitor();
        moni_para = 0;
        if (strchr(string,'U') != NULL) moni_para |= MONI_UNPR;
        if (strchr(string,'I') != NULL) moni_para |= MONI_INFO;
        if (strchr(string,'S') != NULL) moni_para |= MONI_SUPV;
        if (strchr(string,'C') != NULL) moni_para |= MONI_CONN;
      }
      cmd_display(req_flag,channel,OK_TEXT,1);
      return;
    }
  }
  cmd_display(req_flag,channel,invp_txt,1);
}

static int handle_parms(port,value_nr,value,change)
char *port;
int value_nr;
unsigned short *value;
int change;
{
#ifdef NEW_KERNEL
/* dl1bke 970319: set AX.25 parameters for newer kernels through
 *               /proc/sys/net/ax25... Sadly we cannot use
 *               sysctl() as I forgot an entry that gives back
 *               the device name, argh. OTOH this hack doesn't look
 *               that bad, anyway...
 */
       char *dev;
       char path[128];
       FILE *fp;

       if ((dev = ax25_config_get_dev(port)) == NULL)
               return -1;

       sprintf(path, PROC_AX25_SYSCTL_DIR"/%s/%s", dev, AXParamTable[value_nr]);

       if (!change)
       {
               fp = fopen(path, "r");
               if (fp == NULL)
                       return -1;
               fscanf(fp, "%hd", value);
       } else {
               fp = fopen(path, "w");
               if (fp == NULL)
                       return -1;
               fprintf(fp, "%d\n", *value);
       }

       fclose(fp);
       return 0;
#else
  struct ax25_parms_struct ax25_parms;
  ax25_address callsign;
  char *addr;
  int sfd;

  if (value_nr >= AX25_MAX_VALUES) return -1;
  if ((addr = ax25_config_get_addr(port)) == NULL) {
    return -1;
  }
  if (convert_call_entry(addr,callsign.ax25_call) == -1) {
    return -1;
  }
  if ((sfd = socket(AF_AX25,SOCK_SEQPACKET,0)) < 0) {
    return -1;
  }
  ax25_parms.port_addr = callsign;
  if (ioctl(sfd,SIOCAX25GETPARMS,&ax25_parms) != 0) {
    close(sfd);
    return -1;
  }
  if (!change) {
    *value = ax25_parms.values[value_nr];
  }
  else {
    ax25_parms.values[value_nr] = *value;
    if (ioctl(sfd,SIOCAX25SETPARMS,&ax25_parms) != 0) {
      close(sfd);
      return -1;
    }
  }
  close(sfd);
  return 0;
#endif
}

static void ax_value(int channel,char *data,int len,int value_nr,
                     int sockopt_nr,int minval,int maxval)
{
  char *ptr;
  char port[10];
  int tmp;
  char tmpc;
  unsigned short value;
  int value_int;
  int value_len;
  int res;
  char answer[20];
  int err;

  ptr = strip_port(data,len,port);
  if (ptr == NULL) {
    cmd_display(req_flag,channel,invpo_txt,1);
    return;
  }

  if (strlen(ptr) == 0) {
    if ((channel != 0) && (ch_stat[channel].conn_state != CS_DISCON) &&
        (sockopt_nr != -1)) {
      if ((strcmp(ax25k_stat[channel].port,port) != 0) &&
          (ptr != data)) {
        cmd_display(req_flag,channel,diffport_txt,1);
        return;
      }
      err = getsockopt(ax25k_stat[channel].fd,SOL_AX25,sockopt_nr,
                       &value_int,&value_len);
      if (!err) {
        value = value_int;
        if (sockopt_nr == AX25_BACKOFF) {
          if (value) value = 'E';
          else value = 'L';
        }
      }
    }
    else {
      err = handle_parms(port,value_nr,&value,0);
#ifdef NEW_KERNEL
      if (sockopt_nr == AX25_BACKOFF) {
        if (value) value = 'E';
        else value = 'L';
      }
#endif
    }
    if (err == -1) {
      cmd_display(req_flag,channel,errparms_txt,1);
      return;
    }
    if (value_nr == AX25_VALUES_BACKOFF) {
      sprintf(answer,"%c",value);
    }
    else {
      sprintf(answer,"%d",value);
    }
    cmd_display(req_flag,channel,answer,1);
  }
  else {
    err = 0;
    if (value_nr == AX25_VALUES_BACKOFF) {
      res = sscanf(ptr,"%c",&tmpc);
      if (res != 1) {
        err = 1;
      }
      else {
        value = toupper(tmpc);
        if ((value != 'E') && (value != 'L')) {
          err = 1;
        }
      }
    }
    else {
      res = sscanf(ptr,"%d",&tmp);
      if (res != 1) {
        err = 1;
      }
      else {
        value = tmp;
        if ((value < minval) || (value > maxval)) {
          err = 1;
        }
      }
    }
    if (err) {
      cmd_display(req_flag,channel,invp_txt,1);
      return;
    }
    if ((channel != 0) && (ch_stat[channel].conn_state != CS_DISCON) &&
        (sockopt_nr != -1)) {
      if ((strcmp(ax25k_stat[channel].port,port) != 0) &&
          (ptr != data)) {
        cmd_display(req_flag,channel,diffport_txt,1);
        return;
      }
      if (sockopt_nr == AX25_BACKOFF) {
        if (value == 'E') value = 1;
        else value = 0;
      }
      value_int = value;
      value_len = sizeof(value_int);
      err = setsockopt(ax25k_stat[channel].fd,SOL_AX25,sockopt_nr,
                       &value_int,value_len);
    }
    else {
#ifdef NEW_KERNEL
      if (sockopt_nr == AX25_BACKOFF) {
        if (value == 'E') value = 1;
        else value = 0;
      }
#endif
      err = handle_parms(port,value_nr,&value,1);
    }
    if (err == -1) {
      cmd_display(req_flag,channel,errparms_txt,1);
      return;
    }
    cmd_display(req_flag,channel,OK_TEXT,1);
  }
}

static void ax_tries(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_N2,AX25_N2,1,31);
}

static void ax_frack(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_T1,AX25_T1,1,65535);
}

static void ax_resptime(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_T2,AX25_T2,1,65535);
}

static void ax_check(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_T3,AX25_T3,1,65535);
}

static void ax_maxframe(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_WINDOW,AX25_WINDOW,1,7);
}

static void ax_digipeat(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_DIGI,-1,0,3);
}

static void ax_backoff(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_BACKOFF,AX25_BACKOFF,0,0);
}

static void ax_idle(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_IDLE,AX25_IDLE,0,100);
}

static void ax_paclen(int channel,char *data,int len)
{
  ax_value(channel,data,len,AX25_VALUES_PACLEN,AX25_PACLEN,22,256);
}

static int handle_kissparms(port,value_nr,value)
char *port;
int value_nr;
int value;
{
  struct sockaddr sa;
  char *addr;
  int sfd;
  int proto;
  char buffer[2];
  int buflen;

  if (value_nr >= AX25_MAX_VALUES) return -1;
  if ((addr = ax25_config_get_addr(port)) == NULL) {
    return -1;
  }
  proto = ETH_P_AX25;
  if ((sfd = socket(AF_INET,SOCK_PACKET,htons(proto))) < 0) {
    return -1;
  }
  strcpy(sa.sa_data,ax25_config_get_dev(port));
  buffer[0] = value_nr;
  buffer[1] = value;
  buflen = 2;
  if (sendto(sfd,buffer,buflen,0,&sa,sizeof(struct sockaddr)) == -1) {
    close(sfd);
    return -1;
  }
  close(sfd);
  return 0;
}

static void ax_kissvalue(int channel,char *data,int len,int value_nr,
                     int minval,int maxval)
{
  char *ptr;
  char port[10];
  int value;
  int res;
  int err;

  ptr = strip_port(data,len,port);
  if (ptr == NULL) {
    cmd_display(req_flag,channel,invpo_txt,1);
    return;
  }
  
  if ((ptr == data) && (channel != 0) &&
      (ch_stat[channel].conn_state != CS_DISCON)) {
    strcpy(port,ax25k_stat[channel].port);
  }

  if (strlen(ptr) == 0) {
    cmd_display(req_flag,channel,valunk_txt,1);
  }
  else {
    err = 0;
    res = sscanf(ptr,"%d",&value);
    if (res != 1) {
      err = 1;
    }
    else {
      if ((value < minval) || (value > maxval)) {
        err = 1;
      }
    }
    if (err) {
      cmd_display(req_flag,channel,invp_txt,1);
      return;
    }
    err = handle_kissparms(port,value_nr,value);
    if (err == -1) {
      cmd_display(req_flag,channel,errparms_txt,1);
      return;
    }
    cmd_display(req_flag,channel,OK_TEXT,1);
  }
}

#define PARAM_TXDELAY 1
#define PARAM_PERSIST 2
#define PARAM_SLOTTIME 3
#define PARAM_TXTAIL 4
#define PARAM_FULLDUP 5
#define PARAM_HARDWARE 6

static void ax_txdelay(int channel,char *data,int len)
{
  ax_kissvalue(channel,data,len,PARAM_TXDELAY,0,255);
}

static void ax_persist(int channel,char *data,int len)
{
  ax_kissvalue(channel,data,len,PARAM_PERSIST,0,255);
}

static void ax_slottime(int channel,char *data,int len)
{
  ax_kissvalue(channel,data,len,PARAM_SLOTTIME,0,255);
}

static void ax_txtail(int channel,char *data,int len)
{
  ax_kissvalue(channel,data,len,PARAM_TXTAIL,0,255);
}

static void ax_fulldup(int channel,char *data,int len)
{
  ax_kissvalue(channel,data,len,PARAM_FULLDUP,0,1);
}

static void ax_hardware(int channel,char *data,int len)
{
  ax_kissvalue(channel,data,len,PARAM_HARDWARE,0,255);
}

#endif
