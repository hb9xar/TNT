/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   include file for connect (connect.h)
   created: Mark Wahl DL4YBG 94/03/12
   updated: Mark Wahl DL4YBG 96/04/19
*/

#define T_NOCON 1
#define T_CON 2
#define T_RECON 3

#define C_NOTFOUND 1
#define C_DIRECT 2
#define C_ROUTE 3

#define CT_END 0
#define CT_CONNECT 1
#define CT_NODE 2
#define CT_USER 3

#define D_EOL -1

struct digiinfo {
  char *nextdigi;
  int nextlink;
};

#define MAXDIGIS 40
#define MAXBUFFS 20

struct xconn_stat {
  int active;
  int firstcon;
  int num_digi;
  int cur_digi;
  int first_digi;
  struct digiinfo digiinfo[MAXDIGIS];
  int num_alloc;
  char *alloc[MAXBUFFS];
  int ctype;
  int timeout;
  time_t starttime;
#ifdef USE_IFACE
  int inform_box;
  char callsign[10];
  int iface;
  int usernr;
#endif
#ifdef USE_SOCKET
  int inform_sock;
#endif
};

struct qrg_info {
  char qrg[20];
  char port_str[10];
};
