/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   include file for remote passwords (priv.h)
   created: Mark Wahl DL4YBG 94/12/21
   updated: Mark Wahl DL4YBG 97/01/22
*/

/* DXClusterergaenzungen Henning Folger, DL6DH, 06.02.1999 */

#define PW_NONE 0
#define PW_DIEBOX 1
#define PW_FLEXNET 2
#define PW_THENET 3
#define PW_BAYCOM 4
#define PW_MD2 5

#define PW_CLUSTER 6

/* Flags for THENET-Password */
#define PWTN_MORETRIES 1
#define PWTN_HIDESTRING 2
#define PWTN_HIDEPERFECT 4
#define PWTN_MAXVAL PWTN_MORETRIES+PWTN_HIDESTRING+PWTN_HIDEPERFECT
/* values for THENET-Password */
#define PWTN_TRIES 3
#define PWTN_STRLEN 72

struct pwmodes {
  char pwtype[20];
  int pwmode;
};

struct password_stat {
  int pwmode;
  struct calllist *entry;
  char box_login[20]; /* login time for DieBox */
  int pass_wait; /* waiting for password-generation string */
  int tries;
  int valid_try;
};

struct calllist {
  char callsign[10];
  int pwmode;
  char pw_file[80];
  int flags;
  char priv_string[40];
  struct calllist *next;
};

/* MD2.H - header file for MD2C.C
 */

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
   rights reserved.

   License to copy and use this software is granted for
   non-commercial Internet Privacy-Enhanced Mail provided that it is
   identified as the "RSA Data Security, Inc. MD2 Message Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

typedef struct {
  unsigned char state[16];                                 /* state */
  unsigned char checksum[16];                           /* checksum */
  unsigned int count;                 /* number of bytes, modulo 16 */
  unsigned char buffer[16];                         /* input buffer */
} MD2_CTX;

