/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1995 by Mark Wahl
   For license details see documentation
   include file for extended monitor (xmon.h)
   created: Mark Wahl DL4YBG 94/07/17
   updated: Mark Wahl DL4YBG 95/03/16
*/

#ifdef USE_IFACE
#define MAX_XMON 20
#else
#define MAX_XMON 10
#endif
#define XMON_SCREENS 5

/* timeout if no new packet is received on monbox channel in seconds */
#define XMON_TIMEOUT 900

struct xmon_stat {
  int active;
  char srccall[10];
  char destcall[10];
  int srcsum;
  int destsum;
  int next_i_nbr;
  int last_i_nbr;
  int check_chksum;
  int last_chksum;
  int screen;
  int attribute;
  int monbox;
  int monbox_channel;
  time_t last_received;
  int huffcod;
};

struct monheader {
  int header_valid;
  char srccall[10];
  char destcall[10];
  int srcsum;
  int destsum;
  int last_i_nbr;
};

struct heardlist {
  int srcsum;
  int destsum;
  char *srccall;
  char *destcall;
  time_t first_heard;
  time_t last_heard;
  struct heardlist *previous;
  struct heardlist *next;
};

/* layer3 definitions */
#define L3_HEADERLEN 20
#define L3_CONACKLEN 21
#define L3_CONREQLEN 35

#define L3_SRCCALL 0
#define L3_DESTCALL 7
#define L3_TTL 14
#define L3_CI 15
#define L3_CID 16
#define L3_CI_2 17
#define L3_TXNO 17
#define L3_CID_2 18
#define L3_RXNO 18
#define L3_OPCODE 19
#define L3_WINSIZE 20
#define L3_CALL3 21
#define L3_CALL4 28

/* layer3 opcodes */
#define L3OP_CONREQ 1
#define L3OP_CONACK 2
#define L3OP_DISCREQ 3
#define L3OP_DISCACK 4
#define L3OP_INFO 5
#define L3OP_INFOACK 6

