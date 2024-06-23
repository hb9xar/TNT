/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-96 by Mark Wahl
   For license details see documentation
   include file for shell login (shell.h)
   created: Mark Wahl DL4YBG 94/01/22
   updated: Mark Wahl DL4YBG 96/04/20
   updated: mayer hans oe1smc, 6.1.1999
*/

#define pty_name(name, prefix, num) \
  sprintf(name, "%s%c%x", prefix, 'p' + (num >> 4), num & 0xf)

#define uchar(x) ((x) & 0xff)

#define NUMPTY		256
#define MASTERPREFIX    "/dev/pty"
#define SLAVEPREFIX     "/dev/tty"

#ifdef GEN_NEW_USER
#define PWLOCKFILE      "/etc/ptmp"
#define PASSWDFILE      "/etc/passwd"
#ifndef MAXUID
#define MAXUID          4095
#endif
#endif


#define A_SHELL 1
#define A_REDIR 2
#define A_RUN 3
#define A_SOCKET 4
#define A_SOCKCONN 5


#ifdef HAVE_SOCKET

/* definitions for socket type */
#define STYP_AXSERV 1
#define STYP_NETCMD 2

typedef struct listen_socket {
  int sockfd;
  int type;
  int ax25special;
  char socketname[256];
  char mycall[10];
  struct listen_socket *prev;
  struct listen_socket *next;
} LISTEN_SOCKET;

/* definitions of login-status  */
/* for STYP_AXSERV */
#define LS_LOGIN 0
#define LS_PASSWD 1
#define LS_NOTACT 2
#define LS_SETUP 3
#define LS_ACTCMD 4
#define LS_ACTDAT 5
/* for STYP_NETCMD */
#define NC_COMMAND 0
#define NC_SETUP 1
#define NC_CONNECT 2

typedef struct active_socket {
  int sockfd;
  int type;
  int channel;
  char user_id[100];
  char user_pw[100];
  char connect[100];
  char mycall[10];
  int len_input_string;
  char input_string[256];
  int len_next_input;
  char next_input[256];
  int level;
  int login_stat;
  int lfcrconv;
  int ax25special;
  LISTEN_SOCKET *listen_socket;
  struct active_socket *prev;
  struct active_socket *next;
} ACTIVE_SOCKET;

#endif

struct shell_stat {
  int active;           /* flag if shell active */
  int mode;             /* mode during shell command */
  int pty;		/* pty file descriptor */
  int num;              /* pty number */
  char id[4];           /* pty id (last chars) */
  pid_t pid;            /* process id of child */
  int lfcrconv;         /* flag if lf -> cr and vice versa */
  int buflen;           /* number of chars in buffer */
  time_t bufupdate;     /* time of last char written to buffer */
  char buffer[PACKETSIZE];	/* buffer for chars from pty to tnc */
#ifdef HAVE_SOCKET
  ACTIVE_SOCKET *active_socket;	/* link to socket data structure */
#endif
  int direct_started;   /* flag if disconnect on end */
};
