/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   include file for initialization (init.h)
   created: Mark Wahl DL4YBG 94/01/11
   updated: Mark Wahl DL4YBG 97/01/18
*/

/* files used for init */
#ifdef DPBOXT
#define INIT_FILE "dpboxt.ini"

/* default-values if init-file not found or corrupt */
#define DEF_USE_SELECT 1
#define DEF_TNT_HELP_FILE "dpboxt.hlp"
#define DEF_TNT_DIR ""
#define DEF_DOWNLOAD_DIR ""
#define DEF_BLIST_ADD_PLUS 0
#define DEF_FUNC_KEY_FILE "fkeys.tnt"
#define DEF_MACROTEXT_DIR ""
#define DEF_BOX_SOCKET "box/socket"

#else
#define INIT_FILE "tnt.ini"

/* default-values if init-file not found or corrupt */
#define DEF_USE_SELECT 1
#define UP_FILE "tnt.up"
#define DWN_FILE "tnt.dwn"
#define COOKIE_FILE "cookies.doc"
#define DEF_PROC_FILE "tnt.pid"
#define DEF_HELP_FILE "tntrem.hlp"
#define DEF_INFO_FILE "tntrem.inf"
#define DEF_TNT_HELP_FILE "tnt.hlp"
#define DEF_LOCK_FILE "/usr/spool/uucp/LCK..cua0"
#define DEF_NEWS_FILE_NAME "news.tnt"
#define DEF_NAME_FILE_NAME "names.tnt"
#define DEF_ROUTE_FILE_NAME "routes.tnt"
#define DEF_TNT_CTEXTFILE "ctext.tnt"
#define DEF_TNT_LOGBOOKFILE "log.tnt"
#ifdef USE_SOCKET
#define DEF_AUTOBOX_DIR "autobox.dir"
#define DEF_TNT_BOXENDER "boxender.tnt"
#define DEF_F6FBB_BOX "f6fbb.box"
#endif
#define DEF_TNT_PWFILE "pw.tnt"
#define DEF_TNT_SYSFILE "sys.tnt"
#define DEF_TNT_NOREMFILE "norem.tnt"
#define DEF_TNT_FLCHKFILE "flchk.tnt"
#define DEF_TNT_NOTOWNFILE "notown.tnt"
#define DEF_TNT_AUTOSTARTFILE "autostrt.tnt"
#define DEF_TNT_EXTREMOTEFILE "extrem.tnt"
#define DEF_SPEED B19200
#define DEF_SPEEDFLAG 0
#define DEF_TNC_CHANNELS 10
#define DEF_R_CHANNELS 5
#define DEF_FILE_PACLEN 255
#define DEF_TNT_COMP 1
#define DEF_DEVICE "/dev/cua0"
#define DEF_TNT_DIR ""
#define DEF_REMOTE_DIR ""
#define DEF_CTEXT_DIR ""
#define DEF_ABIN_DIR ""
#define DEF_RUN_DIR "bin/"
#define DEF_RESY_LOG_FILE ""
#ifdef BCAST
#define DEF_BCAST_LOG_FILE ""
#endif
#define DEF_UPLOAD_DIR ""
#define DEF_TNT_7PLUS_DIR ""
#define DEF_YAPP_DIR ""
#define DEF_DOWNLOAD_DIR ""
#define DEF_BLIST_ADD_PLUS 0
#define DEF_ALTSTAT 0
#define DEF_DISC_ON_START 0
#ifdef USE_SOCKET
#define DEF_SOCKPASS_FILE "netpass.tnt"
#endif
#ifdef GEN_NEW_USER
#define DEF_UNIX_NEW_USER 1
#define DEF_UNIX_USER_DIR "/tntusers/"
#define DEF_UNIX_FIRST_UID 410
#define DEF_UNIX_USER_GID 101
#endif
#define DEF_FUNC_KEY_FILE "fkeys.tnt"
#define DEF_MACROTEXT_DIR ""
#ifdef USE_IFACE
#define DEF_BOX_SOCKET "box/socket"
#define DEF_NEWMAILDIR "box/newmail/"
#define DEF_TNT_BOX_SSID 7
#define DEF_TNT_BOX_CALL ""
#define DEF_NODE_SOCKET "tntnode/socket"
#define DEF_TNT_NODE_SSID 9
#define DEF_TNT_NODE_CALL ""
#define DEF_FRONTEND_SOCKET "unix:tntsock"
#endif

#ifdef BCAST
#define DEF_TNT_BCTEMPDIR "/tmp/"
#define DEF_TNT_BCSAVEDIR "bcast/save/"
#define DEF_TNT_BCNEWMAILDIR "bcast/newmail/"
#endif

#ifdef TNT_CHAMBER
#define DEF_SCR_DIVIDE 5
#else
#define DEF_SCR_DIVIDE 3
#endif

#ifdef USE_SOCKET
#define DEF_FIXED_WAIT 1
#define DEF_AMOUNT_WAIT 20
#endif

#endif /* DPBOXT */

#define DEF_ATTC_NORMAL 0x07
#define DEF_ATTC_STATLINE 0x70
#define DEF_ATTC_MONITOR 0x08
#define DEF_ATTC_CSTATLINE 0x07
#define DEF_ATTC_CONTROLCHAR 0x08
#define DEF_ATTC_REMOTE 0x08
#define DEF_ATTC_SPECIAL 0x01

#define DEF_ATTM_NORMAL 0x00
#define DEF_ATTM_STATLINE 0x08
#define DEF_ATTM_MONITOR 0x10
#define DEF_ATTM_CSTATLINE 0x10
#define DEF_ATTM_CONTROLCHAR 0x02
#define DEF_ATTM_REMOTE 0x02
#define DEF_ATTM_SPECIAL 0x10

#ifndef DPBOXT
#define DEF_LINES_COMMAND 40
#define DEF_LINES_MONITOR 200
#define DEF_LINES_INPUT 10
#define DEF_LINES_OUTPUT 40
#define DEF_LINES_MONCON 0
#define DEF_LINES_R_INPUT 5
#define DEF_LINES_R_OUTPUT 20
#endif

#define DEF_INPUT_LINELEN 80
#define DEF_INSERTMODE 0

#ifdef USE_IFACE
#define DEF_LINES_MBINPUT 10
#define DEF_LINES_MBOUTPUT 500
#define DEF_MBSCR_DIVIDE 8
#endif

#ifndef DPBOXT
#define DEF_LINES_XMON 100
#define DEF_LINES_XMON_PRE 10
#define DEF_XMON_SCR_DIVIDE 5
#define DEF_NUM_HEARDENTRIES 50

#define DEF_REMOTE_USER "guest"

#define DEF_PTY_TIMEOUT 2
#endif
