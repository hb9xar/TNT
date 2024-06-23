/* include/config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
#define HAVE_UTIME_NULL 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the mkdir function.  */
#define HAVE_MKDIR 1

/* Define if you have the mktime function.  */
#define HAVE_MKTIME 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have <sys/signal.h> header file */
/* #undef HAVE_SYS_SIGNAL_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <termcap.h> header file.  */
#define HAVE_TERMCAP_H 1

/* Define if you have the <termio.h> header file.  */
#define HAVE_TERMIO_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the usleep function.  */
#define HAVE_USLEEP 1

/* Define if you have the srandom function.  */
#define HAVE_SRANDOM 1

/* Define if you have the index.  */
#define HAVE_INDEX 1

/* Define if you have the memmove.  */
#define HAVE_MEMMOVE 1

/* Define if using ncurses library */
#define HAVE_NCURSES 1

/* DEF_COLOR and DEF_TERMCAP must be set to the wanted default value,
   both values can be overridden with color and termcap in tnt.ini */
#define DEF_TERMCAP 0
#define DEF_COLOR 1

/* Define if you struct utmp has ut_host member.  */
#define HAVE_UT_HOST 1

/* Define if you struct utmp has ut_exit member.  */
#define HAVE_UT_EXIT 1

/* Define if CRTSCTS is available in tcsetattr.  */
#define HAVE_CRTSCTS 1

/* Define TNT_SOUND for audio support.  */
#define TNT_SOUND 1

/* Define DPBOXT to use code for dpboxt operation.  */
/* #undef DPBOXT */

/* Define if compiling with the Linux AX25K.  */
/* #undef USE_AX25K */

#ifndef DPBOXT
/* Define BCASE to include code for pacsat-broadcast. */
#define BCAST 1
#endif

/* Define USE_IFACE if you want to compile in software interface.  */
#define USE_IFACE 1

/* Define SPECIAL if if USE_SOCKET is defined and mail delivery 
   feature shall be used.  */
/* #undef SPECIAL */

/* Define GEN_NEW_USER for updating /etc/passwd on new user login.  */
#define GEN_NEW_USER 1

/* Define PWD_NOT_EMPTY if password entry must contain :,./:.  */
/* #undef PWD_NOT_EMPTY */

/* Define USE_HIBAUD if baudrates above 38400 (LINUX only)  */
#define USE_HIBAUD 1

/* Define if terminal sets cursor in new line if
   character in last column of the line is written.  */
#define DEF_AUTO_NEWLINE 0

/* Define if DEF_AUTO_NEW is set and terminal doesn't display
   character for the codes 128-160.  */
#define DEF_SUPP_HICNTL 0

/* Define file descriptor size XXXXXX */
/* #undef FD_SETSIZE */

/* DIR_STRING contains command line used for //DIR remote command.  */
#define DIR_STRING "ls -xaFT80 "

/* DIRL_STRING contains command line used for //DIRLONG remote command. */ 
#define DIRL_STRING "ls -laFT80 "

/* DIRRUN_STRING contains command line used for //RUN remote command. */ 
#define DIRRUN_STRING "ls -xT80 "

/* FREE_STRING contains command line used for //FREE remote command.  */
#define FREE_STRING "df 2>/dev/null > "

/* Define if on a POSIXised ISC UNIX.  */
/* #undef _POSIX_SOURCE */
