/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Main config file (config.h)
   created: Mark Wahl DL4YBG 94/02/01
   updated: Mark Wahl DL4YBG 97/01/26
   updated: mayer hans oe1smc, 6.1.1999
*/

/* if code used for dpboxt (terminal for dpbox) */
#ifdef DPBOXT_LINUX
#define DPBOXT
#define TNT_LINUX
#else
#undef DPBOXT
#endif

#ifdef DPBOXT_SOLARIS
#define DPBOXT
#define TNT_SOLARIS
#else
#undef DPBOXT
#endif

/* Operating System definition done in Makefile
   use -DTNT_xxx as defines
   
   Current implementations:
   Linux:	TNT_LINUX
   ISC:		TNT_ISC
   NetBSD:      TNT_NETBSD (not verified)
   SOLARIS      TNT_SOLARIS  ( verfied by hans, oe1smc )

*/

/* Special Features definition */

/* define HAS_USLEEP if usleep() is available,
   else select with timeout is used
   
   define HAS_SRANDOM if srandom() is available,
   else srand() is used

   define HAS_INDEX if index() is availbale,
   else strchr() is used

   define HAS_MEMMOVE if memmove() is available,
   else function is emulated by a temporary buffer

   define DEF_AUTO_NEWLINE to 1 if your terminal sets the cursor
   in the new line, if a character in the last column of the line
   was written, else set to 0.

   if DEF_AUTO_NEWLINE is set to 1 and the terminal don't display any
   character for the codes 128-160 DEF_SUPP_HICNTL must be set to 1, else
   set to 0.

   DEF_COLOR and DEF_TERMCAP must be set to the wanted default value,
   both values can be overridden with color and termcap in tnt.ini
            
   define HAS_CRTSCTS if CRTSCTS is available in tcsetattr
   else flag not used
       
   define USE_SOCKET if INET-Sockets available

   define USE_IFACE if you want to use the interface to other programs
   
   define HAS_UTHOST if ut_host entry in utmp available
   
   define HAS_UTEXIT if ut_exit entry in utmp available
   
   DIR_STRING contains command line used for //DIR remote command
 
   DIRL_STRING contains command line used for //DIRLONG remote command

   DIRRUN_STRING contains command line used for //RUN remote command,
   if no program to run is specified (-> dir of all prgs in run_dir). 
 
   FREE_STRING contains command line used for //FREE remote command

   define SPECIAL if USE_SOCKET is defined and mail delivery feature
   shall be used.   

   define GEN_NEW_USER if code for updating /etc/passwd if a new user
   logs in shall be used. PWD_NOT_EMPTY must be defined if password-entry
   must contain :,./:
   
   define USE_HIBAUD if baudrates above 38400 baud shall be used
   (Linux implementation)
   
   define BCAST if you want to include code for pacsat-broadcast
*/

#ifdef TNT_LINUX
#ifdef DPBOXT
#undef BCAST
#else
#define BCAST
#endif
#define HAS_USLEEP
#define HAS_SRANDOM
#define HAS_INDEX
#define HAS_MEMMOVE
#define HAS_CRTSCTS
#define HAS_UTHOST
#undef HAS_UTEXIT
#define USE_SOCKET
#define USE_IFACE
#undef SPECIAL
#define GEN_NEW_USER
#undef PWD_NOT_EMPTY
#define USE_HIBAUD
#define DEF_AUTO_NEWLINE 0
#define DEF_SUPP_HICNTL 0
#define DEF_TERMCAP 0
#define DEF_COLOR 1
#define DIR_STRING "ls -xaFT80 "
#define DIRL_STRING "ls -laFT80 "
#define DIRRUN_STRING "ls -xT80 "
#define FREE_STRING "df 2>/dev/null > "
#include <termcap.h>
#ifndef __GLIBC__xxx
#include <sys/wait.h>
#endif
#include <linux/wait.h>
#ifndef errno
#include <errno.h>
#endif
#endif

#ifdef TNT_ISC
#undef BCAST
#undef HAS_USLEEP
#undef HAS_SRANDOM
#undef HAS_INDEX
#undef HAS_MEMMOVE
#undef HAS_CRTSCTS
#undef HAS_UTHOST
#undef HAS_UTEXIT
#define USE_SOCKET
#undef USE_IFACE
#define SPECIAL
#define GEN_NEW_USER
#define PWD_NOT_EMPTY
#undef USE_HIBAUD
#define DEF_AUTO_NEWLINE 1
#define DEF_SUPP_HICNTL 1
#define DEF_TERMCAP 1
#define DEF_COLOR 0
#define DIR_STRING "ls -lF "
#define DIRL_STRING "ls -lF "
#define DIRRUN_STRING "ls "
#define FREE_STRING "/bin/df2 2>/dev/null > "
#include <termio.h>
#include <sys/bsdtypes.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stream.h>
#include <sys/ptem.h>
#define FD_SETSIZE 256
#endif

#ifdef TNT_NETBSD
#define BCAST
#define HAS_USLEEP
#define HAS_SRANDOM
#define HAS_INDEX
#define HAS_MEMMOVE
#define HAS_CRTSCTS
#define HAS_UTHOST
#undef HAS_UTEXIT
#define USE_SOCKET
#define USE_IFACE
#undef SPECIAL
#define GEN_NEW_USER
#undef PWD_NOT_EMPTY
#undef USE_HIBAUD
#define DEF_AUTO_NEWLINE 0
#define DEF_SUPP_HICNTL 0
#define DEF_TERMCAP 0
#define DEF_COLOR 1
#define DIR_STRING "ls -xaFT80 "
#define DIRL_STRING "ls -laFT80 "
#define DIRRUN_STRING "ls -xT80 "
#define FREE_STRING "df 2>/dev/null > "
#include <termcap.h>
#include <sys/wait.h>
#endif

#ifdef TNT_SOLARIS
#ifdef DPBOXT
#undef BCAST
#else
#define BCAST
#endif
#undef HAS_USLEEP
#undef HAS_SRANDOM
#undef HAS_INDEX
#undef HAS_MEMMOVE
#undef HAS_CRTSCTS
#undef HAS_UTHOST
#undef HAS_UTEXIT
#define USE_SOCKET
#define USE_IFACE
#define SPECIAL
#define GEN_NEW_USER
#define PWD_NOT_EMPTY
#undef USE_HIBAUD
#define DEF_AUTO_NEWLINE 1
#define DEF_SUPP_HICNTL 1
#define DEF_TERMCAP 1
#define DEF_COLOR 0
#define DIR_STRING "ls -lF "
#define DIRL_STRING "ls -lF "
#define DIRRUN_STRING "ls "
#define FREE_STRING "/bin/df2 2>/dev/null > "
#include <termio.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <sys/stream.h>
#include <sys/ptem.h>
#endif

