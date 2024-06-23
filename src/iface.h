/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1995 by Mark Wahl
   For license details see documentation
   include file for interface (iface.h)
   created: Mark Wahl DL4YBG 94/07/03
   updated: Mark Wahl DL4YBG 95/12/17
*/

#define MAX_IFACE 4


struct iface_list {
  int active;
  int sockfd;
  char name[80];
  int status;
  char unproto_address[256];
  IFACE_HEADER header;
  char buffer[256];
  int blocked;
  int sendlength;
  struct queue_entry *first_q;
  struct queue_entry *last_q;
};

struct buf_entry {
  int len;
  char *buf;
  struct buf_entry *next;
};

struct iface_stat {
  int iface;
  int usernr;
  int direct_started;
  int box_connect;
  int node_connect;
  struct buf_entry *first_q;
  struct buf_entry *last_q;
};

