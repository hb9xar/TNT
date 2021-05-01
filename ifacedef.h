/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   include file for interface (ifacedef.h)
   created: Mark Wahl DL4YBG 94/11/27
   updated: Mark Wahl DL4YBG 96/09/23
*/

#define HEAD_LEN sizeof(IFACE_HEADER)
#define IFACE_PACLEN 256
#define MAX_LEN (HEAD_LEN + IFACE_PACLEN)
#define MAXQRGS 10
#define BULLIDLEN 12

typedef struct iface_header{
  char indicator;
  short channel;
  short usernr;
  unsigned short len;
} IFACE_HEADER;


#define LEN_SIMPLE sizeof(int)
#define LEN_CSTATUS (LEN_SIMPLE + sizeof(int))
#define LEN_BOXPBOXSF (LEN_SIMPLE + sizeof(int))
#define LEN_BOXPABORTSF (LEN_SIMPLE + sizeof(int))
#define LEN_HUFFSTAT (LEN_SIMPLE + sizeof(int))
#define LEN_ABINFILETOBOX (LEN_SIMPLE + sizeof(int) + sizeof(int))
#define DAT_QRGINFO (20 * (MAXQRGS + 1))
#define LEN_QRGINFO (LEN_SIMPLE + DAT_QRGINFO)
#define LEN_BULLID (LEN_SIMPLE + sizeof(int) + BULLIDLEN + 1)
#define LEN_SETRWMODE (LEN_SIMPLE + sizeof(int))
#define LEN_BOXISBUSY (LEN_SIMPLE + sizeof(int))
#define LEN_BCCALLBACK (LEN_SIMPLE + sizeof(long))
#define LEN_SETUNPROTO (LEN_SIMPLE + 20)
#define LEN_CONNECT (LEN_SIMPLE + sizeof(int) + 20)
#define LEN_TNTRESPONSE (LEN_SIMPLE + sizeof(int))
#define DAT_PATHINFO (10 * 10)
#define LEN_RXUNPROTOHEAD (LEN_SIMPLE + (3*sizeof(int)) + 20 + DAT_PATHINFO)

typedef struct iface_cmdbuf{
  int command;
  union {
    /* CMD_CONNECT */
    struct {
      int timeout;
      char qrg[20];
      char buffer[256-LEN_CONNECT];
    } connect;
    /* CMD_CSTATUS */
    int snd_frames;
    /* CMD_BOXPBOXSF */
    struct {
      int timeout;
      char buffer[256-LEN_BOXPBOXSF];
    } boxpboxsf;
    /* CMD_BOXPABORTSF */
    int immediate;
    /* CMD_ABINFILETOBOX */
    struct {
      int erase;
      int error;
      char buffer[256-LEN_ABINFILETOBOX];
    } abinfile_to_box;
    /* CMD_HUFFSTAT */
    int huffstat;
    /* CMD_QRGINFO */
    char qrg[MAXQRGS+1][20];
    /* CMD_BULLID */
    struct {
      int found;
      char bullid[BULLIDLEN+1];
    } bullid;
    /* CMD_SETRWMODE */
    int rwmode;
    /* CMD_BOXISBUSY */
    int boxisbusy;
    /* CMD_BCCALLBACK */
    long file_id; 
    /* CMD_SETUNPROTO */
    struct {
      char qrg[20];
      char address[256-LEN_SETUNPROTO];
    } setunproto;
    /* CMD_TNTRESPONSE */
    struct {
      int follows;
      char buffer[256-LEN_TNTRESPONSE];
    } tntresponse;
    /* CMD_RXUNPROTOHEAD */
    struct {
      int pid;
      int callcount;
      int heardfrom;
      char qrg[20];
      char calls[10][10];
    } rxunprotohead;
    /* GENERAL */
    char buffer[256-LEN_SIMPLE];
  } data;
} IFACE_CMDBUF;


#define BLOCKING_SIZE 2048

struct queue_entry {
  char *buffer;
  int len;
  struct queue_entry *next;
};

/* special definitions */
#define NO_CHANNEL -1
#define NO_USERNR -1

/* states for iface_list.active (0 = inactive) */
#define IF_NOTACTIVE 0
#define IF_ACTIVE 1
#define IF_TRYING 2

/* states of iface_list.indicator and first byte of iface-packet */
#define IF_COMMAND 1
#define IF_DATA 2
#define IF_UNPROTO 3

/* commands if IF_COMMAND */
#define CMD_ACTIVATE 1
#define CMD_DEACTIVATE 2
#define CMD_CONNECT 3
#define CMD_DISCONNECT 4
#define CMD_FINISH 5
#define CMD_CSTATUS 6
#define CMD_BLOCK 7
#define CMD_UNBLOCK 8
#define CMD_SORT_NEW_MAIL 9
#define CMD_SF_RX_EMT 10
#define CMD_BOXPBOXSF 11
#define CMD_BOXPRDISC 12
#define CMD_BOXPABORTSF 13
#define CMD_ABORT_ROUTING 14
#define CMD_START_SF 15
#define CMD_FINISH_PRG 16
#define CMD_ACT_RESP 17
#define CMD_ABINFILETOBOX 18
#define CMD_HUFFSTAT 19
#define CMD_QRGINFO 20
#define CMD_BULLID 21
#define CMD_STARTABIN 22
#define CMD_BOXTOUCH 23
#define CMD_SETRWMODE 24
#define CMD_BOXISBUSY 25
#define CMD_STARTBOXBC 26
#define CMD_BCCALLBACK 27
#define CMD_SETUNPROTO 28
#define CMD_NOSUCCESSCON 29
#define CMD_SUCCESSCON 30
#define CMD_TNTCOMMAND 31
#define CMD_TNTRESPONSE 32
#define CMD_ABORTCON 33
#define CMD_RXUNPROTOHEAD 34


#define BHMAGIC 4712

typedef struct bcast_headinfo {
  char tnc;
  char port;
  char qrg[20];
  long file_id;
  unsigned short file_type;
  char filename[256];
  char address[256];
  char bbs_source[256];
  char bbs_destination[256];
  char bbs_ax25uploader[256];
  time_t bbs_upload_time;
  time_t bbs_expire_time;
  char bbs_compression;
  char bbs_bid[256];
  char bbs_msgtype;
  char bbs_title[256];
  char bbs_fheader[256];
  unsigned short bodychecksum;
  unsigned char delete_after_tx;
  unsigned short magic;
} BCAST_HEADINFO;
