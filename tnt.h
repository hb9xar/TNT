/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Main include file (tnt.h)
   created: Mark Wahl DL4YBG 93/08/01
   updated: Mark Wahl DL4YBG 97/01/21
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <string.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include "config.h"
#ifdef USE_HIBAUD
//&&&&#include <linux/fs.h>
#include <linux/tty.h>
#include <linux/serial.h>
#endif
#include "window.h"

#define PMODE 0600 /* mode to create file: owner rw, other none */
#ifdef TNT_CHAMBER
#define PMODE_REMOTE 0640 /* mode to create remote file */
#else
#define PMODE_REMOTE 0666 /* mode to create remote file: all rw */
#endif

/* time after which resync is started */
#define RESY_TIME 6

/* number of allowed wrong lines after password request //sys */
#define MAXPWLINES 20

/* control-codes for commands */
#ifdef TNT_CHAMBER
#define C_CUUP    '\006'	/* ^F */
#define C_EOLINE  '\005'	/* ^E */
#define C_DELLINE '\013'        /* ^K */
#else
#define C_CUUP    '\005'	/* ^E */
#define C_EOLINE  '\006'	/* ^F */
#define C_DELLINE '\031'        /* ^Y */
#endif
#define C_PAUSE   '\020'	/* ^P */
#define C_CHANNEL '\011'	/* ^I */
#define C_PASS    '\026'	/* ^V */
#define C_LINEUP  '\022'	/* ^R */
#define C_LINEDWN '\003'	/* ^C */
#define C_WINUP   '\027'	/* ^W */
#define C_WINDWN  '\032'	/* ^Z */
#define C_CULEFT  '\023'	/* ^S */
#define C_CURIGHT '\004'	/* ^D */
#define C_CUDWN   '\030'	/* ^X */
#define C_STLINE  '\001'	/* ^A */
#define C_INSERT  '\016'	/* ^N */
#define C_DELCHAR '\014'	/* ^L */
#define C_CUTOP   '\024'        /* ^T */
#define C_CUBOT   '\002'        /* ^B */

#define C_MONITOR '\240'	/* META-m */
#define C_CONNECT '\241'	/* META-q */
#define C_COMMAND '\242'	/* META-c */
#define C_EXTMON  '\243'        /* META-x */
#define C_MAILBOX '\244'        /* META-b */
#define C_BOXLIST '\245'        /* META-l */
#define C_MHEARD  '\246'        /* META-s */
#define C_HELP    '\247'        /* META-h */

/* length of TX-QUEUE (number of blocks to send to tnc) */
#define TXQUEUE 80
/* length of TX-QUEUE when blocking will be released (tx_hiwater) */
#define TXQUEUE_FREE 60
/* max number of waiting send frames for each channel */
#define MAX_FRAMES 8
/* max size of a packet */
#define PACKETSIZE 256

/* modes of display */
#define M_CONNECT 0
#define M_COMMAND 1
#define M_MONITOR 2
/* special modes */
#define M_CMDSCRIPT 3
#define M_REMOTE 4
#define M_REMOTE_TEMP 5
#define M_FRONTEND 6
#define M_SOCKET 7
#define M_EXTMON 8
#define M_MAILBOX 9
#define M_BOXLIST 10
#define M_HEARD 11
#define M_HELP 12
#define M_INTERFACE 13
#define M_PUSHPOP 14
#define M_IFACECMD 15

/* states of the main loop */
#define S_END 0
#define S_INIT 1
#define S_STAT 2
#define S_NORMAL 3
#define S_NOKEY 4
#define S_EXIT 5
#define S_TNT_UP 6
#define S_TNT_DWN 7
#define S_TSTXHOST 8

/* codes for blocks computer to TNC */
#define X_DATA 0
#define X_COMM 1

/* codes for blocks TNC to computer */
#define R_SUCC 0
#define R_SUCCMESS 1
#define R_FAILMESS 2
#define R_LINKSTAT 3
#define R_MONI 4
#define R_MONIMESS 5
#define R_MONIDATA 6
#define R_CHANDATA 7

/* states for blockreceiver */
#define R_CHANNEL 0 /* waiting for channel */
#define R_CODE 1    /* waiting for code */
#define R_LENGTH 2  /* waiting for length */
#define R_TOZERO 3  /* reading string to zero */
#define R_RDDATA 4  /* reading string of specified length */

/* which information is requested */
#define RQ_NORMAL 0
#define RQ_CHSTAT 1
#define RQ_CALL 2
#define RQ_CURCALL 3
#define RQ_OWNCALL 4
#define RQ_XHOST 5
#define RQ_BUFFERS 6
#define RQ_MYCALL 7
#define RQ_CONNECT 8
#define RQ_RECONNECT 9
#define RQ_REDISC 10
#define RQ_PID 11
#define RQ_MONSTAT 12
#define RQ_UNPROTO 13
#define RQ_BCBUFFERS 14

/* connect-states 
   (channel_stat.conn_state) */
#define CS_DISCON 0
#define CS_SETUP 1
#define CS_CONN 2


typedef struct yapptype {
  int state;
  int init;
  int yappc;
  int total;
  int filelength;
  int delete;
  int write;
  int maxfill;
  int seekpos;
  int channel;
  int outlen;
  int outbufptr;
  int buflen;
  int progress;
  int startval;
  int curval;
  char outbuffer[512];
  char buffer[1024];
  unsigned short fdate;
  unsigned short ftime;
  char sendbuffer[256];
  int sendbuflen;
  time_t bufupdate;
} yapptype;


struct channel_stat {
  int st_mess;
  int rcv_frms;
  int snd_frms;
  int unacked;
  int tries;
  int state;
  int conn_state;
  char mycall[10];
  char curcall[10];
  char call[257];
  char call_save[257];
  char name[90];
  int not_disp;
  int snd_queue_frms;
  int oldstate;
  int sendcook;
  char log_call[31];
  char disp_call[31];
  int contcon;
  time_t start_time;
  time_t end_time;
  int maxstate;
  int ignore_state;
  int lastcr;
  char oldbuf[256];
  int oldbuflen;
  int huffcod;
  int access_level;
  int pwwait;
  char pwstr[6];
  int rootlevel;
  int remote;
  int pause_flag;
  int flchkmode;
};

struct block_buffer_ptr {
  int channel;
  char code;
  int len;
  int flag;
  char *data;
  struct block_buffer_ptr *next;
};

struct com_list {
  char string[9];
  int len;
  int ext_com;
  void (*func)();
  char new_string[4];
  int par1;
  int par2;
};

struct tx_file {
  char name[160];
  int type;
  FILE *fp;
  int fd;
  int flag;
  int mode;
  int len;
  unsigned int crc;
  int len_tmp;
  unsigned int crc_tmp;
  int wait_ok;
  time_t start_time;
  time_t end_time;
  yapptype *yapp;
};

struct rx_file {
  char name[160];
  int type;
  FILE *fp;
  int fd;
  int flag;
  int mode;
  int len;
  unsigned int crc;
  int len_tmp;
  unsigned int crc_tmp;
  int wait_bin;
  time_t start_time;
  time_t end_time;
  char binheader[80];
  time_t last_touch;
  yapptype *yapp;
};

/* receive file types */
#define RX_NORM 0	/* Logfile (ASCII) */
#define RX_BIN 1	/* Image */
#define RX_ABIN 2	/* Auto-Bin protocol */
#define RX_PLAIN 3	/* Image with CR to LF translation */
#define RX_ABIN_Q 4     /* Auto-Bin protocol, no additional responses */
#define RX_ABIN_E 5     /* RX_ABIN_Q only for box and boxcut */
#define RX_YAPP 6       /* receive using YAPP */
#define RX_AUTO7P 7     /* auto 7PLUS reception */

/* transmit file types */
#define TX_NORM 0	/* ASCII-file */
#define TX_BIN 1	/* Image */
#define TX_ABIN 2	/* Auto-Bin protocol */
#define TX_PLAIN 3	/* Image with CR to LF translation */
#define TX_SCRIPT 4	/* Command-script */
#define TX_ABINQ 5      /* Auto-Bin protocol, no wait for ok, no statistics */
#define TX_YAPP 6       /* transmit using YAPP */

/* flag for logging data */
#define RX_RCV 0x01	/* received data */
#define RX_SND 0x02	/* sent data */

/* types of send_queue */
#define SQ_NORMAL 0     /* normal queue */
#define SQ_BCAST 1      /* queue for broadcastframes */

/* states of YAPP state machine */
#define YAPPSTATE_S     0
#define YAPPSTATE_SH    1
#define YAPPSTATE_SD    2
#define YAPPSTATE_SE    3
#define YAPPSTATE_ST    4
#define YAPPSTATE_R     5
#define YAPPSTATE_RH    6
#define YAPPSTATE_RD    7

struct func_keys {
  char code[11];
  int sel_channel;
  char res_code;
};

struct staterr {
  unsigned int st_mess;
  unsigned int rcv_frms;
  unsigned int snd_frms;
  unsigned int unacked;
  unsigned int tries;
  unsigned int axstate;
  unsigned int free_buffers;
  unsigned int xhost;
};

