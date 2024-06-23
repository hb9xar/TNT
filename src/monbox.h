/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   include file for monitor boxfill (monbox.h)
   created: Mark Wahl DL4YBG 94/07/24
   updated: Mark Wahl DL4YBG 96/11/03
*/

#define FC_FILE 0600
#define _FNSIZE 120

#define Char char
#ifndef Static
# define Static static
#endif

typedef unsigned char boolean;
typedef unsigned char uchar;
typedef long bst;

#ifndef true
# define true 1
# define false 0
#endif

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

typedef enum {
  NOP, THEBOX_USER, W0RLI_SF, F6FBB_SF, F6FBB_USER, BAYCOM_USER, WAMPES_USER,
  W0RLI_USER, AA4RE_USER, F6FBB_USER_314
} cutboxtyp;

#define MAX_MONBOX 10

struct monbox_info {
  int active;
  short fd;
  int xmon_ch;
  cutboxtyp cuttype;
  short linect;
  char kopf[81];
  char betreff[81];
  char msgid[41];
  char fractline[257];
};

#define MAX_MULTIBOXLIST 60

struct multiboxlist {
  short dayct;
  char rubrik[9];
};

struct boxcut_info {
  boolean boxcut;
  short bcutchan;
  unsigned short bcutct;
  char bcutname[256];
  char bcutcall[12];
};
