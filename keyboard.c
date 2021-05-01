/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for keyboard input (keyboard.c)
   created: Mark Wahl DL4YBG 93/07/24
   updated: Mark Wahl DL4YBG 97/01/26

   08.01.00 hb9xar	Y2K fix: set_date sent 'number of years since 1900'
   25.03.04 hb9xar	include <time.h> for struct tm

*/

#include "tnt.h"
#include "keys.h"
#include "boxlist.h"
#ifndef DPBOXT
#include "xmon.h"
#endif

#include <time.h>

extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void sel_connect(int channel);
extern void statlin_update();
extern void cmd_display(int flag,int channel,char *buffer,int cr);
#ifndef DPBOXT
extern void open_dwnfile(int *state);
extern void frontend_exit(int deact);
extern void write_file(int channel,int len,int flag,char *str);
extern void sel_xmon(int xmon_channel);
extern int data_allowed(int channel);
#endif
extern void beep();
extern void rem_data_display(int channel,char *buffer);
#ifndef DPBOXT
extern void sel_monitor();
extern void sel_command();
extern void sel_heardlist();
#endif
extern void sel_boxlist();
extern void sel_help();
#ifdef USE_IFACE
extern void sel_mailbox();
#endif
#ifndef DPBOXT
extern void pre_charout_cntl(int channel,char ch);
extern int pre_newline(int channel,char *str);
extern void real_window_up(int part,int page);
extern void real_window_down(int part,int page);
extern void real_window_top(int part);
extern void real_window_bot(int part);
extern void pre_charout_nobnd(int channel,char ch);
extern void pre_charout(int channel,char ch);
extern int pre_lineend(int channel);
extern int pre_wordwrap(int channel,char *str);
extern void insert_cr_rx(int channel);
extern void rem_data_display_buf(int channel,char *buffer,int len);
extern int cmd_newline(int channel,char *str);
extern void cmd_charout_nobnd(int channel,char ch);
extern void cmd_charout(int channel,char ch);
extern int xm_newline(int screen,char *str);
extern void xm_charout_nobnd(int screen,char ch);
extern void xm_charout(int screen,char ch);
extern void heard_charout_cntl(char ch);
#endif
extern void help_charout_cntl(char ch);
extern void blist_analyse_line(int channel, int func);
extern void blist_charout_cntl(int channel,char ch);
#ifdef USE_IFACE
extern void mb_charout_cntl(char ch);
extern int mb_newline(char *str);
extern void mb_charout_nobnd(char ch);
extern void mb_charout(char ch);
extern int mb_lineend();
extern int mb_wordwrap(char *str);
extern void mb_input(char *str,int len);
extern void rem_mb_display_buf(char *buffer,int len);
#endif
extern void reinit_screen();


#ifdef DPBOXT

extern int act_channel;
extern int act_mode;
extern int act_state;
extern char *term;
extern char ok_text[];
extern char tnt_dir[];
extern char func_key_file[];
extern int LINES,COLS;
extern int tnt_daemon;
extern int use_select;

int chan_flag;
int chan_val1;
int tnc_command;

static int esc_flag;
static int pass_flag;
static int mb_pass_flag;
static int exit_flag;
static struct termios okbd_termios;
static struct termios nkbd_termios;
static char func_str[11];
static char input_str[MAXCOLS+1];
static char tnc_string[257];

#define MACRO_MIN 1
#define MACRO_MAX 12
struct macro_string { char string[100]; };
static struct macro_string macro_strings[MACRO_MAX+1];

extern void open_logfile();
extern void close_file();
void exit_tnt();
static void cmd_flag();
static void cmd_chdir();
static void cmd_cwdir();
extern void cmd_iface();
extern void cmd_endiface();
extern void cmd_actiface();
extern void cmd_deactiface();
extern void cmd_finiface();
extern void cmd_box();
extern void cmd_endbox();
extern void cmd_actbox();
extern void cmd_deactbox();
extern void cmd_finbox();
static void cmd_kmacro();
extern void cmd_logblist();
extern void cmd_blist();
extern void cmd_xblist();
static void cmd_value();
static void cmd_string();
extern void list_filenames();

#else /* DPBOXT */

extern int tnc_channels;
extern int act_channel;
extern int act_mode;
extern int act_state;
extern int xmon_screen;
extern int free_buffers;
extern struct channel_stat *ch_stat;
extern char *term;
extern char ok_text[];
extern char signon[];
extern int resync_count;
extern struct staterr staterr;
extern int script_channel;
extern char tnt_dir[];
extern char func_key_file[];
extern int LINES,COLS;
extern int tnt_comp;
extern int tnt_daemon;
extern int use_select;
extern int frontend_active;

int chan_flag;
int chan_val1;
int tnc_command;

static int esc_flag;
static int pass_flag;
#ifdef USE_IFACE
static int mb_pass_flag;
#endif /* USE_IFACE */
static int exit_flag;
static struct termios okbd_termios;
static struct termios nkbd_termios;
static char func_str[11];
static char input_str[MAXCOLS+1];
static char tnc_string[257];

#define MACRO_MIN 1
#define MACRO_MAX 12
struct macro_string { char string[100]; };
static struct macro_string macro_strings[MACRO_MAX+1];

extern void open_logfile();
extern void close_file();
extern void open_comscript();
extern void open_sendfile();
extern void break_send();
extern void rem_time();
extern void rem_cookie();
extern void rem_rtt();
extern void cmd_shell();
extern void cmd_endshell();
extern void cmd_redir();
#ifdef USE_SOCKET
extern void cmd_sockconn();
#endif /* USE_SOCKET */
void exit_tnt();
static void cmd_signon();
static void quit_tnt();
static void cmd_settime();
static void cmd_setdate();
static void cmd_flag();
static void cmd_mycall();
static void cmd_tnc();
extern void rem_ctext();
extern void rem_name();
static void cmd_chdir();
static void cmd_cwdir();
static void cmd_resync();
extern void cmd_xconnect();
extern void cmd_run();
#ifdef USE_SOCKET
extern void cmd_socket();
extern void cmd_endsock();
#endif /* USE_SOCKET */
#ifdef USE_IFACE
extern void cmd_iface();
extern void cmd_endiface();
extern void cmd_actiface();
extern void cmd_deactiface();
extern void cmd_finiface();
extern void cmd_box();
extern void cmd_endbox();
extern void cmd_actbox();
extern void cmd_deactbox();
extern void cmd_finbox();
extern void cmd_monboxlist();
extern void cmd_ldboxfil();
extern void cmd_node();
extern void cmd_endnode();
extern void cmd_actnode();
extern void cmd_deactnode();
extern void cmd_finnode();
#endif /* USE_IFACE */
static void cmd_kmacro();
extern void cmd_msend();
extern void cmd_extmon();
extern void cmd_endextmon();
extern void cmd_logblist();
extern void cmd_blist();
extern void cmd_xblist();
extern void open_monfile();
extern void close_monfile();
extern void open_xmonfile();
extern void close_xmonfile();
extern void cmd_concall();
extern void cmd_priv();
extern void cmd_loadpriv();
extern void cmd_listpriv();
extern void cmd_loadsys();
extern void cmd_listsys();
extern void cmd_ldnoremo();
extern void cmd_lstnorem();
extern void cmd_ldflchk();
extern void cmd_lstflchk();
extern void cmd_ldnotown();
extern void cmd_lsnotown();
extern void cmd_ldautostart();
extern void cmd_lsautostart();
extern void cmd_ldextrem();
extern void cmd_lsextrem();
static void cmd_value();
static void cmd_string();
extern void cmd_comp();
extern void cmd_extcomp();
extern void cmd_qrg();
extern void cmd_setacc();
static void cmd_free();
extern void send_bcfile();
extern void list_filenames();
extern void cmd_chanstat();
#ifdef USE_IFACE
extern void cmd_scanmbeacon();
extern void cmd_accuicall();
#endif /* USE_IFACE */
#ifdef BCAST
extern void cmd_bcrxstat();
extern void cmd_bctxstat();
#endif /* BCAST */
#endif /* DPBOXT */

/* values for flag command */
#define F_INFOBELL 0
#define F_CBELL 1
#define F_APPEND 2
#define F_COOKIE 3
#define F_REMOTE 4
#define F_UMLAUT 5
#define F_TXECHO 6
#define F_CTEXT 8
#define F_PTYECHO 9
#define F_AUTOBIN 10
#define F_XMON 11
#define F_MONBOX 12
#define F_AUTOBOX 13
#define F_HEARD 14
#define F_LAYER3 15
#define F_WORDWRAP 16
#define F_WHOLELINE 17
#define F_BSCRHOLD 18
#define F_TABEXP 19
#define F_NOACC 20
#define F_REMALLOW 21
#define F_SHPACSAT 22
#define F_BCREQUEST 23
#define F_DECBCAST 24
#define F_AUTOSTART 25
#define F_ACCUIREQ 26
#define F_AUTOYAPP 27
#define F_AUTO7PL 28
#define F_LOGBOOK 29

/* variables for flag command */
#ifdef DPBOXT
int append_flag;
extern int umlaut;
int txecho_flag;
int wordwrap_flag;
int wholeline_flag;
int bscrhold_flag;
int tabexp_flag;
#else
int infobell_flag;
int cbell_flag;
int append_flag;
int cookie_flag;
int remote_flag;
extern int umlaut;
int txecho_flag;
int ctext_flag;
int ptyecho_flag;
int autobin_flag;
int autoyapp_flag;
int auto7pl_flag;
extern int xmon_flag;
extern int mheard_flag;
#ifdef USE_IFACE
extern int monbox_flag;
extern int autobox_flag;
#endif
int layer3_flag;
int wordwrap_flag;
int wholeline_flag;
int bscrhold_flag;
int tabexp_flag;
int noacc_flag;
#ifdef BCAST
extern int shpacsat_flag;
extern int bcrequest_flag;
extern int decbcast_flag;
#endif
extern int autostart_flag;
#ifdef USE_IFACE
extern int accept_uireq_flag;
#endif
extern int logbook_flag;
#endif

/* values for value command */
#define V_LINELEN 0
#define V_MONLINES 1
#define V_CONDIV 2
#define V_XMONDIV 3
#define V_MBOXDIV 4
#define V_FPACLEN 5

/* variables for value command */
#ifdef DPBOXT
extern int input_linelen;
extern int mbscr_divide;
#else
extern int input_linelen;
extern int lines_moncon;
extern int scr_divide;
extern int xmon_scr_divide;
#ifdef USE_IFACE
extern int mbscr_divide;
#endif
extern int file_paclen;
#endif

#ifndef DPBOXT
/* values for string command */
#define S_NOACC 0
#define S_NOCONN 1
#define S_NOBOX 2
#define S_NONODE 3

/* variables for string command */
extern char rem_noacc_str[];
#ifdef USE_IFACE
extern char noconn_text[];
extern char rem_nobox_str[];
extern char rem_nonode_str[];
#endif
#endif

#ifndef DPBOXT
static struct com_list gen_com_list[] = {
  {"ALFDISP" ,4,0,NULL,"A"  ,0,0},
  {"A"       ,1,0,NULL,"A"  ,0,0},
  {"DAMADIS" ,4,0,NULL,"B"  ,0,0},
  {"B"       ,1,0,NULL,"B"  ,0,0},
  {"VERSION" ,1,0,NULL,"V"  ,0,0},
  {"CHECK"   ,2,0,NULL,"@T3",0,0},
  {"@T3"     ,3,0,NULL,"@T3",0,0},
  {"CONNECT" ,1,0,NULL,"C"  ,0,0},
  {"CTEXT"   ,2,0,NULL,"U"  ,0,0},
  {"U"       ,1,0,NULL,"U"  ,0,0},
  {"DIGIPEAT",4,0,NULL,"R"  ,0,0},
  {"R"       ,1,0,NULL,"R"  ,0,0},
  {"DISCONNE",1,0,NULL,"D"  ,0,0},
  {"DAYTIME" ,5,0,NULL,"K"  ,0,0},
  {"K"       ,1,0,NULL,"K"  ,0,0},
  {"ECHO"    ,1,0,NULL,"E"  ,0,0},
  {"FLOW"    ,2,0,NULL,"Z"  ,0,0},
  {"Z"       ,1,0,NULL,"Z"  ,0,0},
  {"FRACK"   ,1,0,NULL,"F"  ,0,0},
  {"FULLDUP" ,2,0,NULL,"@D" ,0,0},
  {"@D"      ,2,0,NULL,"@D" ,0,0},
  {"MAXFRAME",3,0,NULL,"O"  ,0,0},
  {"O"       ,1,0,NULL,"O"  ,0,0},
  {"MONITOR" ,1,0,NULL,"M"  ,0,0},
  {"MYCALL"  ,2,1,cmd_mycall,"I"  ,0,0},
  {"I"       ,1,0,NULL,"I"  ,0,0},
  {"MHEARD"  ,2,0,NULL,"H"  ,0,0},
  {"H"       ,1,0,NULL,"H"  ,0,0},
  {"PERSIST" ,1,0,NULL,"P"  ,0,0},
  {"RESPTIME",3,0,NULL,"@T2",0,0},
  {"@T2"     ,3,0,NULL,"@T2",0,0},
  {"RETRY"   ,2,0,NULL,"N"  ,0,0},
  {"N"       ,1,0,NULL,"N"  ,0,0},
  {"SLOTTIME",2,0,NULL,"W"  ,0,0},
  {"W"       ,1,0,NULL,"W"  ,0,0},
  {"TXDELAY" ,1,0,NULL,"T"  ,0,0},
  {"USERS"   ,2,0,NULL,"Y"  ,0,0},
  {"Y"       ,1,0,NULL,"Y"  ,0,0},
  {"XMITOK"  ,1,0,NULL,"X"  ,0,0},
  {"BUFFERS" ,3,0,NULL,"@B" ,0,0},
  {"@B"      ,2,0,NULL,"@B" ,0,0},
  {"A1SRTT"  ,6,0,NULL,"@A1",0,0},
  {"@A1"     ,3,0,NULL,"@A1",0,0},
  {"A2SRTT"  ,6,0,NULL,"@A2",0,0},
  {"@A2"     ,3,0,NULL,"@A2",0,0},
  {"A3SRTT"  ,6,0,NULL,"@A3",0,0},
  {"@A3"     ,3,0,NULL,"@A3",0,0},
  {"IPOLL"   ,3,0,NULL,"@I" ,0,0},
  {"@I"      ,2,0,NULL,"@I" ,0,0},
  {"8BIT"    ,3,0,NULL,"@M" ,0,0},
  {"@M"      ,4,0,NULL,"@M" ,0,0},
  {"VALCALL" ,3,0,NULL,"@V" ,0,0},
  {"@V"      ,4,0,NULL,"@V" ,0,0},
  {"CHANNEL" ,4,2,NULL,"" ,0,0},
  {"S"       ,1,2,NULL,"" ,0,0},
  {"TNC"     ,3,1,cmd_tnc,"",0,0},
  {"LOGQSO"  ,4,1,open_logfile,"",RX_NORM,RX_RCV | RX_SND},
  {"LOGREC"  ,4,1,open_logfile,"",RX_NORM,RX_RCV},
  {"LOGSND"  ,4,1,open_logfile,"",RX_NORM,RX_SND},
  {"READBIN" ,5,1,open_logfile,"",RX_BIN,RX_RCV},
  {"READ"    ,4,1,open_logfile,"",RX_PLAIN,RX_RCV},
  {"READABIN",5,1,open_logfile,"",RX_ABIN,RX_RCV},
  {"LOGABIN" ,4,1,open_logfile,"",RX_ABIN_Q,RX_RCV},
  {"READYAPP",5,1,open_logfile,"",RX_YAPP,RX_RCV},
  {"LOGMON"  ,4,1,open_monfile,"",RX_NORM,0},
  {"RDMON"   ,5,1,open_monfile,"",RX_PLAIN,0},
  {"RDMONBIN",6,1,open_monfile,"",RX_BIN,0},
  {"LOGXMON" ,4,1,open_xmonfile,"",RX_NORM,0},
  {"RDXMON"  ,6,1,open_xmonfile,"",RX_PLAIN,0},
  {"RDXMONBI",7,1,open_xmonfile,"",RX_BIN,0},
  {"CLOSE"   ,2,1,close_file,"",0,0},
  {"CLOSEMON",6,1,close_monfile,"",0,0},
  {"CLOSEXMO",6,1,close_xmonfile,"",0,0},
  {"SENDLOG" ,5,1,open_sendfile,"",TX_NORM,0},
  {"SENDBIN" ,5,1,open_sendfile,"",TX_BIN,0},
  {"SENDABIN",5,1,open_sendfile,"",TX_ABIN,0},
  {"SENDQBIN",5,1,open_sendfile,"",TX_ABINQ,0},
  {"SEND"    ,4,1,open_sendfile,"",TX_PLAIN,0},
  {"SENDYAPP",5,1,open_sendfile,"",TX_YAPP,0},
  {"SENDCOM" ,5,1,open_comscript,"",0,0},
#ifdef BCAST
  {"SENDBC"  ,5,1,send_bcfile,"",0,0},
#endif
  {"BREAK"   ,2,1,break_send,"",0,0},
  {"TIMESET" ,7,1,cmd_settime,"",0,0},
  {"DATESET" ,7,1,cmd_setdate,"",0,0},
  {"EXIT"    ,2,1,exit_tnt,"",0,0},
  {"QUIT"    ,4,1,quit_tnt,"",0,0},
  {"INFOBELL",4,1,cmd_flag,"",F_INFOBELL,0},
  {"CBELL"   ,2,1,cmd_flag,"",F_CBELL,0},
  {"APPEND"  ,3,1,cmd_flag,"",F_APPEND,0},
  {"COOKIE"  ,4,1,cmd_flag,"",F_COOKIE,0},
  {"REMOTE"  ,4,1,cmd_flag,"",F_REMOTE,0},
  {"UMLAUT"  ,3,1,cmd_flag,"",F_UMLAUT,0},
  {"TXECHO"  ,3,1,cmd_flag,"",F_TXECHO,0},
  {"CONTEXT" ,4,1,cmd_flag,"",F_CTEXT,0},
  {"PTYECHO" ,4,1,cmd_flag,"",F_PTYECHO,0},
  {"AUTOBIN" ,6,1,cmd_flag,"",F_AUTOBIN,0},
  {"AUTOYAPP",6,1,cmd_flag,"",F_AUTOYAPP,0},
  {"AUTO7PL" ,6,1,cmd_flag,"",F_AUTO7PL,0},
  {"LOGBOOK" ,7,1,cmd_flag,"",F_LOGBOOK,0},
  {"XMON"    ,4,1,cmd_flag,"",F_XMON,0},
  {"HEARD"   ,3,1,cmd_flag,"",F_HEARD,0},
#ifdef USE_IFACE
  {"MONBOX"  ,4,1,cmd_flag,"",F_MONBOX,0},
  {"AUTOBOX" ,6,1,cmd_flag,"",F_AUTOBOX,0},
#endif
  {"LAYER3"  ,3,1,cmd_flag,"",F_LAYER3,0},
  {"WORDWRAP",5,1,cmd_flag,"",F_WORDWRAP,0},
  {"WHOLELIN",5,1,cmd_flag,"",F_WHOLELINE,0},
  {"BSCRHOLD",5,1,cmd_flag,"",F_BSCRHOLD,0},
  {"TABEXP"  ,4,1,cmd_flag,"",F_TABEXP,0},
  {"NOACC"   ,5,1,cmd_flag,"",F_NOACC,0},
  {"REMALLOW",4,1,cmd_flag,"",F_REMALLOW,0},
#ifdef BCAST
  {"SHPACSAT",6,1,cmd_flag,"",F_SHPACSAT,0},
  {"BCRQST"  ,6,1,cmd_flag,"",F_BCREQUEST,0},
  {"DECBCAST",5,1,cmd_flag,"",F_DECBCAST,0},
#endif
  {"AUTOSTRT",5,1,cmd_flag,"",F_AUTOSTART,0},
#ifdef USE_IFACE
  {"ACCUIREQ",6,1,cmd_flag,"",F_ACCUIREQ,0},
  {"ACCUICAL",6,1,cmd_accuicall,"",0,0},
#endif
  {"LINELEN" ,3,1,cmd_value,"",V_LINELEN,0},
  {"MONLINES",4,1,cmd_value,"",V_MONLINES,0},
  {"CONDIV"  ,4,1,cmd_value,"",V_CONDIV,0},
  {"XMONDIV" ,5,1,cmd_value,"",V_XMONDIV,0},
#ifdef USE_IFACE
  {"MBOXDIV" ,5,1,cmd_value,"",V_MBOXDIV,0},
#endif
  {"FPACLEN" ,5,1,cmd_value,"",V_FPACLEN,0},
  {"SNOACC"  ,6,1,cmd_string,"",S_NOACC,0},
#ifdef USE_IFACE
  {"SNOCONN" ,7,1,cmd_string,"",S_NOCONN,0},
  {"SNOBOX"  ,6,1,cmd_string,"",S_NOBOX,0},
  {"SNONODE" ,7,1,cmd_string,"",S_NONODE,0},
  {"LMONBOX" ,4,1,cmd_monboxlist,"",0,0},
  {"LDBOXFIL",6,1,cmd_ldboxfil,"",0,0},
#endif
  {"SCOOKIE" ,3,1,rem_cookie,"",0,0},
  {"SCTEXT"  ,3,1,rem_ctext,"",0,0},
  {"RTT"     ,2,1,rem_rtt,"",0,0},
  {"NAME"    ,4,1,rem_name,"",0,0},
  {"STIME"   ,3,1,rem_time,"",0,0},
  {"SHELL"   ,2,1,cmd_shell,"",0,0},
  {"TSHELL"  ,3,1,cmd_shell,"",1,0},
  {"ROOTSH"  ,6,1,cmd_shell,"",2,0},
  {"TROOTSH" ,7,1,cmd_shell,"",3,0},
  {"ENDSHELL",4,1,cmd_endshell,"",0,0},
  {"RUN"     ,3,1,cmd_run,"",0,0},
  {"RUNT"    ,4,1,cmd_run,"",1,0},
  {"ENDRUN"  ,4,1,cmd_endshell,"",0,0},
#ifdef USE_SOCKET
  {"SOCKCON" ,5,1,cmd_sockconn,"",0,0},
  {"TSOCKCON",6,1,cmd_sockconn,"",1,0},
  {"ENDSOCKC",8,1,cmd_endshell,"",0,0},
#endif
  {"REDIR"   ,3,1,cmd_redir,"",0,0},
  {"ENDREDIR",4,1,cmd_endshell,"",0,0},
#ifdef USE_SOCKET
  {"SOCKET"  ,3,1,cmd_socket,"",0,0},
  {"ENDSOCK" ,4,1,cmd_endsock,"",0,0},
#endif
#ifdef USE_IFACE
  {"IFACE"   ,3,1,cmd_iface,"",0,0},
  {"ENDIFACE",5,1,cmd_endiface,"",0,0},
  {"ACTIF"   ,4,1,cmd_actiface,"",0,0},
  {"DEACTIF" ,4,1,cmd_deactiface,"",0,0},
  {"FINIFACE",5,1,cmd_finiface,"",0,0},
  {"BOX"     ,3,1,cmd_box,"",0,0},
  {"ENDBOX"  ,4,1,cmd_endbox,"",0,0},
  {"ACTBOX"  ,4,1,cmd_actbox,"",0,0},
  {"DEACTBOX",6,1,cmd_deactbox,"",0,0},
  {"FINBOX"  ,4,1,cmd_finbox,"",0,0},
  {"NODE"    ,3,1,cmd_node,"",0,0},
  {"ENDNODE" ,4,1,cmd_endnode,"",0,0},
  {"ACTNODE" ,4,1,cmd_actnode,"",0,0},
  {"DEACTNOD",6,1,cmd_deactnode,"",0,0},
  {"FINNODE" ,4,1,cmd_finnode,"",0,0},
#endif
  {"CD"      ,2,1,cmd_chdir,"",0,0},
  {"CWD"     ,3,1,cmd_cwdir,"",0,0},
  {"RESYNC"  ,4,1,cmd_resync,"",0,0},
  {"FREE"    ,4,1,cmd_free,"",0,0},
  {"XCONNECT",2,1,cmd_xconnect,"",0,0},
  {"KMACRO"  ,3,1,cmd_kmacro,"",0,0},
  {"MSEND"   ,3,1,cmd_msend,"",0,0},
  {"EXTMON"  ,3,1,cmd_extmon,"",0,0},
  {"EXTAMON" ,4,1,cmd_extmon,"",1,0},
  {"ENDEXTM" ,5,1,cmd_endextmon,"",0,0},
  {"LOGBLIST",4,1,cmd_logblist,"",0,0},
  {"BLIST"   ,3,1,cmd_blist,"",0,0},
  {"XBLIST"  ,3,1,cmd_xblist,"",0,0},
  {"CONCALL" ,4,1,cmd_concall,"",0,0},
  {"PRIV"    ,4,1,cmd_priv,"",0,0},
  {"LOADPRIV",8,1,cmd_loadpriv,"",0,0},
  {"LISTPRIV",8,1,cmd_listpriv,"",0,0},
  {"LOADSYS" ,7,1,cmd_loadsys,"",0,0},
  {"LISTSYS" ,7,1,cmd_listsys,"",0,0},
  {"LDNOREM" ,7,1,cmd_ldnoremo,"",0,0},
  {"LSTNOREM",8,1,cmd_lstnorem,"",0,0},
  {"LDFLCHK" ,7,1,cmd_ldflchk,"",0,0},
  {"LSTFLCHK",8,1,cmd_lstflchk,"",0,0},
  {"LDNOTOWN",8,1,cmd_ldnotown,"",0,0},
  {"LSNOTOWN",8,1,cmd_lsnotown,"",0,0},
  {"LDAUTOST",8,1,cmd_ldautostart,"",0,0},
  {"LSAUTOST",8,1,cmd_lsautostart,"",0,0},
  {"LDEXTREM",8,1,cmd_ldextrem,"",0,0},
  {"LSEXTREM",8,1,cmd_lsextrem,"",0,0},
  {"COMP"    ,4,1,cmd_comp,"",0,0},
  {"QRG"     ,3,1,cmd_qrg,"",0,0},
  {"SETACC"  ,4,1,cmd_setacc,"",0,0},
  {"LISTFILE",8,1,list_filenames,"",0,0},
  {"CSTATUS" ,2,1,cmd_chanstat,"",0,0},
#ifdef USE_IFACE
  {"SCANMBEA",5,1,cmd_scanmbeacon,"",0,0},
#endif
#ifdef BCAST
  {"BCRXSTAT",4,1,cmd_bcrxstat,"",0,0},
  {"BCTXSTAT",4,1,cmd_bctxstat,"",0,0},
#endif
  {"SIGNON"  ,4,1,cmd_signon,"",0,0},
  {""        ,0,-1,NULL,""   ,0,0}
};
#endif

#ifdef USE_IFACE
/* commands allowed in mailbox screen */
static struct com_list mb_com_list[] = {
  {"LOGREC"  ,4,1,open_logfile,"",RX_NORM,RX_RCV},
  {"READBIN" ,5,1,open_logfile,"",RX_BIN,RX_RCV},
  {"READ"    ,4,1,open_logfile,"",RX_PLAIN,RX_RCV},
  {"CLOSE"   ,2,1,close_file,"",0,0},
  {"EXIT"    ,2,1,exit_tnt,"",0,0},
#ifndef DPBOXT
  {"QUIT"    ,4,1,quit_tnt,"",0,0},
#endif
  {"APPEND"  ,3,1,cmd_flag,"",F_APPEND,0},
  {"UMLAUT"  ,3,1,cmd_flag,"",F_UMLAUT,0},
  {"TXECHO"  ,3,1,cmd_flag,"",F_TXECHO,0},
#ifndef DPBOXT
  {"XMON"    ,4,1,cmd_flag,"",F_XMON,0},
  {"HEARD"   ,3,1,cmd_flag,"",F_HEARD,0},
  {"MONBOX"  ,4,1,cmd_flag,"",F_MONBOX,0},
  {"AUTOBOX" ,6,1,cmd_flag,"",F_AUTOBOX,0},
#endif
  {"WORDWRAP",5,1,cmd_flag,"",F_WORDWRAP,0},
  {"WHOLELIN",5,1,cmd_flag,"",F_WHOLELINE,0},
  {"BSCRHOLD",5,1,cmd_flag,"",F_BSCRHOLD,0},
  {"TABEXP"  ,4,1,cmd_flag,"",F_TABEXP,0},
  {"LINELEN" ,3,1,cmd_value,"",V_LINELEN,0},
#ifndef DPBOXT
  {"MONLINES",4,1,cmd_value,"",V_MONLINES,0},
  {"CONDIV"  ,4,1,cmd_value,"",V_CONDIV,0},
  {"XMONDIV" ,5,1,cmd_value,"",V_XMONDIV,0},
#endif
#ifdef USE_IFACE
  {"MBOXDIV" ,5,1,cmd_value,"",V_MBOXDIV,0},
#endif
#ifndef DPBOXT
  {"FPACLEN" ,5,1,cmd_value,"",V_FPACLEN,0},
  {"LMONBOX" ,4,1,cmd_monboxlist,"",0,0},
#endif
  {"IFACE"   ,3,1,cmd_iface,"",0,0},
  {"ENDIFACE",5,1,cmd_endiface,"",0,0},
  {"ACTIF"   ,4,1,cmd_actiface,"",0,0},
  {"DEACTIF" ,4,1,cmd_deactiface,"",0,0},
  {"FINIFACE",5,1,cmd_finiface,"",0,0},
  {"BOX"     ,3,1,cmd_box,"",0,0},
  {"ENDBOX"  ,4,1,cmd_endbox,"",0,0},
  {"ACTBOX"  ,4,1,cmd_actbox,"",0,0},
  {"DEACTBOX",6,1,cmd_deactbox,"",0,0},
  {"FINBOX"  ,4,1,cmd_finbox,"",0,0},
  {"CD"      ,2,1,cmd_chdir,"",0,0},
  {"CWD"     ,3,1,cmd_cwdir,"",0,0},
#ifndef DPBOXT
  {"RESYNC"  ,4,1,cmd_resync,"",0,0},
  {"FREE"    ,4,1,cmd_free,"",0,0},
#endif
  {"KMACRO"  ,3,1,cmd_kmacro,"",0,0},
#ifndef DPBOXT
  {"MSEND"   ,3,1,cmd_msend,"",0,0},
#endif
  {"LOGBLIST",4,1,cmd_logblist,"",0,0},
  {"BLIST"   ,3,1,cmd_blist,"",0,0},
  {"XBLIST"  ,3,1,cmd_xblist,"",0,0},
#ifndef DPBOXT
  {"SETACC"  ,4,1,cmd_setacc,"",0,0},
#endif
  {"LISTFILE",8,1,list_filenames,"",0,0},
  {""        ,0,-1,NULL,""   ,0,0}
};
#endif

#ifndef DPBOXT
/* commands allowed in extended monitor screen */
static struct com_list xm_com_list[] = {
  {"LOGXMON" ,4,1,open_xmonfile,"",RX_NORM,0},
  {"RDXMON"  ,6,1,open_xmonfile,"",RX_PLAIN,0},
  {"RDXMONBI",7,1,open_xmonfile,"",RX_BIN,0},
  {"CLOSEXMO",6,1,close_xmonfile,"",0,0},
  {"EXIT"    ,2,1,exit_tnt,"",0,0},
  {"QUIT"    ,4,1,quit_tnt,"",0,0},
  {"APPEND"  ,3,1,cmd_flag,"",F_APPEND,0},
  {"XMON"    ,4,1,cmd_flag,"",F_XMON,0},
  {"WORDWRAP",5,1,cmd_flag,"",F_WORDWRAP,0},
  {"WHOLELIN",5,1,cmd_flag,"",F_WHOLELINE,0},
  {"BSCRHOLD",5,1,cmd_flag,"",F_BSCRHOLD,0},
  {"TABEXP"  ,4,1,cmd_flag,"",F_TABEXP,0},
  {"LINELEN" ,3,1,cmd_value,"",V_LINELEN,0},
  {"MONLINES",4,1,cmd_value,"",V_MONLINES,0},
  {"CONDIV"  ,4,1,cmd_value,"",V_CONDIV,0},
  {"XMONDIV" ,5,1,cmd_value,"",V_XMONDIV,0},
#ifdef USE_IFACE
  {"MBOXDIV" ,5,1,cmd_value,"",V_MBOXDIV,0},
#endif
  {"FPACLEN" ,5,1,cmd_value,"",V_FPACLEN,0},
  {"CD"      ,2,1,cmd_chdir,"",0,0},
  {"CWD"     ,3,1,cmd_cwdir,"",0,0},
  {"KMACRO"  ,3,1,cmd_kmacro,"",0,0},
  {"MSEND"   ,3,1,cmd_msend,"",0,0},
  {"EXTMON"  ,3,1,cmd_extmon,"",0,0},
  {"EXTAMON" ,4,1,cmd_extmon,"",1,0},
  {"ENDEXTM" ,5,1,cmd_endextmon,"",0,0},
  {"EXTCOMP" ,4,1,cmd_extcomp,"",0,0},
  {"SETACC"  ,4,1,cmd_setacc,"",0,0},
  {""        ,0,-1,NULL,""   ,0,0}
};
#endif


void init_keyboard()
{
  txecho_flag = 1;
#ifndef DPBOXT
  ptyecho_flag = 1;
#endif
  esc_flag = 0;
  pass_flag = 0;
  wordwrap_flag = 1;
  wholeline_flag = 0;
  bscrhold_flag = 1;
#ifdef USE_IFACE
  mb_pass_flag = 0;
#endif
  chan_flag = 0;
  exit_flag = 0;

  if (!tnt_daemon) {
    tcgetattr(0,&okbd_termios);
    nkbd_termios = okbd_termios;
    if (use_select) {
      nkbd_termios.c_cc[VTIME] = 0;
      nkbd_termios.c_cc[VMIN] = 1;
    }
    else {
      nkbd_termios.c_cc[VTIME] = 0;
      nkbd_termios.c_cc[VMIN] = 0;
    }
    nkbd_termios.c_cc[VSTART] = -1;
    nkbd_termios.c_cc[VSTOP] = -1;
    nkbd_termios.c_iflag = 0;
    nkbd_termios.c_iflag |= (IGNBRK|ICRNL);
    nkbd_termios.c_oflag = 0;
    nkbd_termios.c_lflag = 0;
    nkbd_termios.c_cflag |= (CS8|CREAD|CLOCAL);
#ifdef HAS_CRTSCTS
    nkbd_termios.c_cflag &= ~(CSTOPB|PARENB|PARODD|CRTSCTS|HUPCL);
#else
    nkbd_termios.c_cflag &= ~(CSTOPB|PARENB|PARODD|HUPCL);
#endif
    tcsetattr(0,TCSADRAIN,&nkbd_termios);
  }
  cmd_kmacro(0,0,0,0,M_CMDSCRIPT,NULL); /* init keyboard macros */ 
}

void exit_keyboard()
{
  if (!tnt_daemon) {
    tcsetattr(0,TCSADRAIN,&okbd_termios);
  }
}

/* analyse input-string (command) */
void cmd_input(channel,mode,str,len,cscript)
int channel;
int mode;
char *str;
int len;
int cscript;
{
  char com_string[9];
  int found;
  int i,j;
  int invalid;
  int chan;
  struct com_list *com_list;
  
#ifdef DPBOXT
  com_list = mb_com_list;
#else
  if (mode == M_EXTMON)
    com_list = xm_com_list;
#ifdef USE_IFACE
  else if (mode == M_MAILBOX)
    com_list = mb_com_list;
#endif
  else
    com_list = gen_com_list;
#endif
  tnc_command = 0;
  invalid = 0;
  /* delete CR at end of string and return if empty */
  while ((str[len-1] == '\n') || (str[len-1] == CR)) {
    str[len-1] = '\0';
    len--;
  }
  if (len == 0) return;

  if (len > 79) {
    cmd_display(mode,channel,"Line too long, ignored",1);
    return;
  }

  i = 0;
  while ((i < 8) && (i < len) && (*str != SPACE)) {
    com_string[i] = *str++;
    if ((com_string[i] > 0x60) && (com_string[i] < 0x7E))
      com_string[i] &= 0xDF;
    i++;
  }
  com_string[i]= '\0';
  if ((i != len) && (*str != SPACE)) {
    invalid = 1;
  }
  len -= i;
  if (!invalid) {
    /* find string in command-list */
    found = 0;
    j = 0;
    while ((com_list[j].ext_com != -1) && !found) {
      if (strstr(com_list[j].string,com_string) == com_list[j].string) {
        /* string found, now check if length valid */
        if (strlen(com_string) >= com_list[j].len) {
          found = 1;
        }
      }
      if (!found) j++;
    }
    if (found) {
      /* delete spaces before parameters */
      while (*str == SPACE) {
        str++;
        len--;
      }
      switch (com_list[j].ext_com) {
      case 0: /* tnc command */
#ifndef DPBOXT
        tnc_command = 1;
        strcpy(tnc_string,com_list[j].new_string);
        if (len)
          strcat(tnc_string,str);
        len = strlen(tnc_string);
        queue_cmd_data(channel,X_COMM,len,mode,tnc_string);
#endif
        break;
      case 1: /* external command */
        (*com_list[j].func)(com_list[j].par1,com_list[j].par2,
                            channel,len,mode,str);
        break;
      case 2: /* change of channel */
#ifndef DPBOXT
        chan = atoi(str);
        if ((chan >= 0) && (chan < tnc_channels)) {
          if (cscript) { /* change command script channel */
            script_channel = chan;
          }
          else { /* change channel */
            sel_connect(chan);
            statlin_update();
          }
        }
#endif
        break;
      }
      return;
    }
  }
  cmd_display(mode,channel,"Invalid command",1);
  return;
}

void exit_tnt(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
#ifndef DPBOXT
  if (mode == M_REMOTE) {
    act_state = S_TNT_DWN;
    open_dwnfile(&act_state);
  }
  else {
    cmd_display(mode,channel,"Do you really want to exit (y/n)? ",0);
    exit_flag = 1;
  }
#else
  cmd_display(mode,channel,"Do you really want to exit (y/n)? ",0);
  exit_flag = 1;
#endif
}

#ifndef DPBOXT
void quit_tnt(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if (!tnt_daemon || (tnt_daemon && !frontend_active)) {
    cmd_display(mode,channel,"Invalid command",1);
    return;
  }
  frontend_exit(1);
  cmd_display(mode,channel,ok_text,1);
}
#endif

#ifndef DPBOXT
/* analyse input-string (data) */
void dat_input(channel,str,len)
int channel;
char *str;
int len;
{
  int flag;
  
  flag = 0;
  queue_cmd_data(channel,X_DATA,len,flag,str);
  write_file(channel,len,RX_SND,str);
}
#endif

void keyboard_server(state,ch)
int *state;
char *ch;
{
  int i;
  int found;
  int analysed;
  int input_len;
  int num_func;
  struct func_keys *spec_keys;
  char msgs[100];
#ifndef DPBOXT
  int new_chan;
  int chan_val2;
#endif
  

  if (exit_flag) {
    switch (*ch) {
      case 'Y':
      case 'y':
#ifdef DPBOXT
        *state = S_END;
        exit_flag = 0;
        cmd_display(act_mode,0,"Y",1);
#else
        *state = S_TNT_DWN;
        open_dwnfile(state);
        exit_flag = 0;
        if (act_mode != M_EXTMON)
          cmd_display(act_mode,act_channel,"Y",1);
        else
          cmd_display(act_mode,xmon_screen,"Y",1);
#endif
        break;
      case 'N':
      case 'n':
        exit_flag = 0;
#ifdef DPBOXT
        cmd_display(act_mode,0,"N",1);
#else
        if (act_mode != M_EXTMON)
          cmd_display(act_mode,act_channel,"N",1);
        else
          cmd_display(act_mode,xmon_screen,"N",1);
#endif
        break;
    }
    return;
  }
#ifndef DPBOXT
  else if (chan_flag) {
    if (act_mode == M_EXTMON) {
      if ((*ch >= '0') && (*ch <= '9')) {
        chan_val1 = (int)((*ch) - '0');
        if (chan_val1 >= XMON_SCREENS) return;
        sel_xmon(chan_val1);
        chan_flag = 0;
        statlin_update();
        return;
      }
      else return;
    }
    else {
      if (((*ch >= '0') && (*ch <= '9')) || (*ch == BS) || (*ch == DEL)) {
        if ((*ch == BS) || (*ch == DEL)) {
          chan_flag = 1;
          statlin_update();
          return;
        }
        if (chan_flag == 1) {
          chan_val1 = (int)((*ch) - '0');
          if (chan_val1 > (tnc_channels / 10)) return;
          chan_flag++;
          statlin_update();
          return;
        }
        else {
          chan_val2 = (int)((*ch) - '0') + 10 * chan_val1;
          if (chan_val2 >= tnc_channels) return;
          sel_connect(chan_val2);
          chan_flag = 0;
          statlin_update();
          return;
        }
      }
      else return;
    }
  }
#endif
  if (esc_flag) {
    found = 0;
    if (strcmp(term,"xterm") == 0) {
      spec_keys = special_keys_xterm;
      num_func = NUMFUNC_XTERM;
    }
    else {
      spec_keys = special_keys;
      num_func = NUMFUNC;
    }
    func_str[esc_flag] = *ch;
    func_str[esc_flag + 1] = '\0';
    for (i = 0; (i < num_func) && (!found); i++) {
      if (strstr(spec_keys[i].code,func_str) == spec_keys[i].code) {
        found = 1;
        if (strlen(spec_keys[i].code) == (esc_flag + 1)) {
          esc_flag = 0;
          if (spec_keys[i].sel_channel == 2) {
#ifdef DPBOXT
            if (act_mode != M_MAILBOX)
              return;
#else
#ifdef USE_IFACE
            if ((act_mode != M_CONNECT) && (act_mode != M_COMMAND) &&
                (act_mode != M_MAILBOX))
#else
            if ((act_mode != M_CONNECT) && (act_mode != M_COMMAND))
#endif
              return;
            if (!data_allowed(act_channel)) {
              beep();
              return;
            }
#endif
            strcpy(msgs,macro_strings[(int)spec_keys[i].res_code].string);
            if(strcmp(msgs,"")) {
              if(msgs[0]==':') {
#ifdef DPBOXT
                cmd_display(act_mode,act_channel,msgs,1);
#else
	        if (act_mode == M_COMMAND)
	          cmd_display(act_mode,act_channel,&msgs[1],1);
                else
	          cmd_display(act_mode,act_channel,msgs,1);
#endif
	        cmd_input(act_channel,act_mode,&msgs[1],strlen(msgs)-1,0);
	        return;
	      }
#ifndef DPBOXT
	      else {
	        if (act_mode != M_CONNECT)
	          return;
	        if(ch_stat[act_channel].state < 4)
	          return;
	        if(msgs[strlen(msgs)-1]=='*') {
	          msgs[strlen(msgs)-1]=CR;
	          dat_input(act_channel,msgs,strlen(msgs));
	          if(txecho_flag)
	            rem_data_display(act_channel,msgs);
	        }
	        cmd_display(act_mode,act_channel,msgs,0);
	        return;
	      }
#endif
	    }
	    return;
          }
          if (spec_keys[i].sel_channel == 1) {
#ifndef DPBOXT
            new_chan = (int)spec_keys[i].res_code;
            if (act_mode == M_CONNECT) {
              if ((act_channel % 10) == new_chan)
                new_chan += 10 + (act_channel / 10) * 10;
              if (new_chan >= tnc_channels)
                return;
            }
            sel_connect(new_chan);
#endif
            return;
          }
          else {
            *ch = spec_keys[i].res_code;
          }
        }
        else {
          if (++esc_flag > 10) esc_flag = 0;
          return;
        }
      }
    }
    if (!found) {
      esc_flag = 0;
      return;
    }
  }
  if ((*ch < SPACE) || ((*ch >= '\240') && (*ch <= '\247'))) {
    analysed = 1;
    switch (*ch) {
      case ESC:
        func_str[0] = ESC;
        esc_flag = 1;
        break;
#ifndef DPBOXT
      case C_MONITOR:
        sel_monitor();
        break;
      case C_COMMAND:
        sel_command();
        break;
      case C_CONNECT:
        sel_connect(act_channel);
        break;
      case C_EXTMON:
        sel_xmon(xmon_screen);
        break;
      case C_MHEARD:
        sel_heardlist();
        break;
#endif
      case C_BOXLIST:
        sel_boxlist();
        break;
      case C_HELP:
        sel_help();
        break;
#ifdef USE_IFACE
      case C_MAILBOX:
        sel_mailbox();
        break;
#endif
#ifndef DPBOXT    
      case C_CHANNEL:
        chan_flag = 1;
        statlin_update();
        break;
#endif
      default:
        analysed = 0;
        break;
    }
    if (analysed) return;
  }
  if (*state == S_NORMAL) {
    switch (act_mode) {
#ifndef DPBOXT
    case M_CONNECT:
      input_len = 0;
      if (pass_flag) {
        pre_charout_cntl(act_channel,*ch);
        pass_flag = 0;
      }
      else if ((*ch < SPACE) || (*ch == ESC2) || (*ch == DEL)) {
        switch (*ch) {
          case LF:
            if (!data_allowed(act_channel)) {
              beep();
            }
            else {
              input_len = pre_newline(act_channel,input_str);
            }
            break;
          case C_PASS:
            pass_flag = 1;
            break;
          case C_PAUSE:
            ch_stat[act_channel].pause_flag ^= 1;
            statlin_update();
            break;
          case C_WINUP:
            real_window_up(2,1);
            break;
          case C_WINDWN:
            real_window_down(2,1);
            break;
          case C_LINEUP:
            real_window_up(2,0);
            break;
          case C_LINEDWN:
            real_window_down(2,0);
            break;
          case C_CUTOP:
            real_window_top(2);
            break;
          case C_CUBOT:
            real_window_bot(2);
            break;
          case C_CULEFT:
          case C_CUUP:
          case C_CUDWN:
          case C_STLINE:
          case C_EOLINE:
          case C_INSERT:
          case C_DELLINE:
          case C_DELCHAR:
          case BS:
          case DEL:
            pre_charout_nobnd(act_channel,*ch);
            break;
          case C_CURIGHT:
            pre_charout(act_channel,*ch);
            break;
          default:
            if ((wordwrap_flag) && (pre_lineend(act_channel)))
              input_len = pre_wordwrap(act_channel,input_str);
            pre_charout_cntl(act_channel,*ch);
            break;
        }
      }
      else {
        if ((wordwrap_flag) && (pre_lineend(act_channel)))
          input_len = pre_wordwrap(act_channel,input_str);
        pre_charout(act_channel,*ch);
      }
      if (input_len) {
        if ((input_len > 1) && (input_str[0] == ':'))
          cmd_input(act_channel,act_mode,input_str + 1,input_len - 1,0);
        else {
          dat_input(act_channel,input_str,input_len);
          if (txecho_flag) {
            insert_cr_rx(act_channel);
            rem_data_display_buf(act_channel,input_str,input_len);
          }
        }
      }
      break;
    case M_COMMAND:
      if ((*ch < SPACE) || (*ch == ESC2) || (*ch == DEL)) {
        switch (*ch) {
          case LF:
            input_len = cmd_newline(act_channel,input_str);
            cmd_input(act_channel,act_mode,input_str,input_len,0);
            break;
          case C_CULEFT:
          case C_CUUP:
          case C_CUDWN:
          case C_WINUP:
          case C_WINDWN:
          case C_CUTOP:
          case C_CUBOT:
          case C_STLINE:
          case C_EOLINE:
          case C_INSERT:
          case C_DELLINE:
          case C_DELCHAR:
          case BS:
          case DEL:
            cmd_charout_nobnd(act_channel,*ch);
            break;
          case C_CURIGHT:
            cmd_charout(act_channel,*ch);
            break;
        }
      }
      else {
        cmd_charout(act_channel,*ch);
      }
      break;
    case M_MONITOR:
      switch (*ch) {
        case C_PAUSE:
          ch_stat[0].pause_flag ^= 1;
          statlin_update();
          break;
        case C_WINUP:
          real_window_up(0,1);
          break;
        case C_WINDWN:
          real_window_down(0,1);
          break;
        case C_LINEUP:
        case C_CUUP:
          real_window_up(0,0);
          break;
        case C_LINEDWN:
        case C_CUDWN:
          real_window_down(0,0);
          break;
        case C_CUTOP:
          real_window_top(0);
          break;
        case C_CUBOT:
          real_window_bot(0);
          break;
      }
      break;
    case M_EXTMON:
      if ((*ch < SPACE) || (*ch == ESC2) || (*ch == DEL)) {
        switch (*ch) {
          case LF:
            input_len = xm_newline(xmon_screen,input_str);
            cmd_input(xmon_screen,act_mode,input_str,input_len,0);
            break;
          case C_WINUP:
            real_window_up(2,1);
            break;
          case C_WINDWN:
            real_window_down(2,1);
            break;
          case C_LINEUP:
            real_window_up(2,0);
            break;
          case C_LINEDWN:
            real_window_down(2,0);
            break;
           case C_CUTOP:
            real_window_top(2);
            break;
          case C_CUBOT:
            real_window_bot(2);
            break;
          case C_CULEFT:
          case C_CUUP:
          case C_CUDWN:
          case C_STLINE:
          case C_EOLINE:
          case C_INSERT:
          case C_DELLINE:
          case C_DELCHAR:
          case BS:
          case DEL:
            xm_charout_nobnd(xmon_screen,*ch);
            break;
          case C_CURIGHT:
            xm_charout(xmon_screen,*ch);
            break;
        }
      }
      else {
        xm_charout(xmon_screen,*ch);
      }
      break;
    case M_HEARD:
      switch (*ch) {
        case C_WINUP:
        case C_WINDWN:
        case C_CUUP:
        case C_CUDWN:
        case C_CUTOP:
        case C_CUBOT:
          heard_charout_cntl(*ch);
          break;
      }
      break;
#endif
    case M_HELP:
      switch (*ch) {
        case C_WINUP:
        case C_WINDWN:
        case C_CUUP:
        case C_CUDWN:
        case C_CUTOP:
        case C_CUBOT:
          help_charout_cntl(*ch);
          break;
      }
      break;
    case M_BOXLIST:
      switch (*ch) {
        case LF:
          blist_analyse_line(act_channel,FUNC_READ);
          break;
        case 'l':
        case 'L':
          blist_analyse_line(act_channel,FUNC_LIST);
          break;
        case 'e':
        case 'E':
          blist_analyse_line(act_channel,FUNC_ERASE);
          break;
        case 'k':
        case 'K':
          blist_analyse_line(act_channel,FUNC_KILL);
          break;
        case 't':
        case 'T':
          blist_analyse_line(act_channel,FUNC_TRANSFER);
          break;
        case '0':
          blist_analyse_line(act_channel,FUNC_LIFETIME);
          break;
        case C_CUUP:
        case C_CUDWN:
        case C_WINUP:
        case C_WINDWN:
        case C_CUTOP:
        case C_CUBOT:
          blist_charout_cntl(act_channel,*ch);
          break;
      }
      break;
#ifdef USE_IFACE
    case M_MAILBOX :
      input_len = 0;
      if (mb_pass_flag) {
        mb_charout_cntl(*ch);
        mb_pass_flag = 0;
      }
      else if ((*ch < SPACE) || (*ch == ESC2) || (*ch == DEL)) {
        switch (*ch) {
          case LF:
            input_len = mb_newline(input_str);
            break;
          case C_PASS:
            mb_pass_flag = 1;
            break;
          case C_PAUSE:
            break;
          case C_WINUP:
            real_window_up(2,1);
            break;
          case C_WINDWN:
            real_window_down(2,1);
            break;
          case C_LINEUP:
            real_window_up(2,0);
            break;
          case C_LINEDWN:
            real_window_down(2,0);
            break;
          case C_CUTOP:
            real_window_top(2);
            break;
          case C_CUBOT:
            real_window_bot(2);
            break;
          case C_CULEFT:
          case C_CUUP:
          case C_CUDWN:
          case C_STLINE:
          case C_EOLINE:
          case C_INSERT:
          case C_DELLINE:
          case C_DELCHAR:
          case BS:
          case DEL:
            mb_charout_nobnd(*ch);
            break;
          case C_CURIGHT:
            mb_charout(*ch);
            break;
          default:
            if ((wordwrap_flag) && (mb_lineend()))
              input_len = mb_wordwrap(input_str);
            mb_charout_cntl(*ch);
            break;
        }
      }
      else {
        if ((wordwrap_flag) && (mb_lineend()))
          input_len = mb_wordwrap(input_str);
        mb_charout(*ch);
      }
      if (input_len) {
        if ((input_len > 1) && (input_str[0] == ':'))
          cmd_input(act_channel,act_mode,input_str + 1,input_len - 1,0);
        else {
          mb_input(input_str,input_len);
          if (txecho_flag) {
            rem_mb_display_buf(input_str,input_len);
          }
        }
      }
      break;
#endif
    }
  }
}

#ifndef DPBOXT
static void cmd_settime(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct tm timestr;
  time_t timeval;
  char tmpstr[10];
    
  timeval = time(&timeval);
  timestr = *localtime(&timeval);
  sprintf(tmpstr,"K %2.2u:%2.2u:%2.2u",
          timestr.tm_hour,timestr.tm_min,timestr.tm_sec);
  queue_cmd_data(channel,X_COMM,strlen(tmpstr),mode,tmpstr);
  cmd_display(mode,channel,tmpstr,1);
  tnc_command = 1;
}

static void cmd_setdate(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct tm timestr;
  time_t timeval;
  char tmpstr[10];
    
  timeval = time(&timeval);
  timestr = *localtime(&timeval);
  sprintf(tmpstr,"K %2.2u.%2.2u.%2.2u",
          timestr.tm_mday,timestr.tm_mon+1,timestr.tm_year%10);
  queue_cmd_data(channel,X_COMM,strlen(tmpstr),mode,tmpstr);
  cmd_display(mode,channel,tmpstr,1);
  tnc_command = 1;
}
#endif

static void cmd_flag(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int *flag;
  int i;
  char ch;
  
  switch (par1) {
#ifndef DPBOXT
  case F_INFOBELL:
    flag = &infobell_flag;
    break;
  case F_CBELL:
    flag = &cbell_flag;
    break;
#endif
  case F_APPEND:
    flag = &append_flag;
    break;
#ifndef DPBOXT
  case F_COOKIE:
    flag = &cookie_flag;
    break;
  case F_REMOTE:
    flag = &remote_flag;
    break;
#endif
  case F_UMLAUT:
    flag = &umlaut;
    break;
  case F_TXECHO:
    flag = &txecho_flag;
    break;
#ifndef DPBOXT
  case F_CTEXT:
    flag = &ctext_flag;
    break;
  case F_PTYECHO:
    flag = &ptyecho_flag;
    break;
  case F_AUTOBIN:
    flag = &autobin_flag;
    break;
  case F_AUTOYAPP:
    flag = &autoyapp_flag;
    break;
  case F_AUTO7PL:
    flag = &auto7pl_flag;
    break;
  case F_LOGBOOK:
    flag = &logbook_flag;
    break;
  case F_XMON:
    flag = &xmon_flag;
    break;
#ifdef USE_IFACE
  case F_MONBOX:
    flag = &monbox_flag;
    break;
  case F_AUTOBOX:
    flag = &autobox_flag;
    break;
#endif
  case F_HEARD:
    flag = &mheard_flag;
    break;
  case F_LAYER3:
    flag = &layer3_flag;
    break;
#endif
  case F_WORDWRAP:
    flag = &wordwrap_flag;
    break;
  case F_WHOLELINE:
    flag = &wholeline_flag;
    break;
  case F_BSCRHOLD:
    flag = &bscrhold_flag;
    break;
  case F_TABEXP:
    flag = &tabexp_flag;
    break;
#ifndef DPBOXT
  case F_NOACC:
    flag = &noacc_flag;
    break;
  case F_REMALLOW:
    if (ch_stat[channel].conn_state != CS_CONN) {
      cmd_display(mode,channel,"Only while connected",1);
      return;
    }
    flag = &ch_stat[channel].remote;
    break;
#ifdef BCAST
  case F_SHPACSAT:
    flag = &shpacsat_flag;
    break;
  case F_BCREQUEST:
    flag = &bcrequest_flag;
    break;
  case F_DECBCAST:
    flag = &decbcast_flag;
    break;
#endif
  case F_AUTOSTART:
    flag = &autostart_flag;
    break;
#ifdef USE_IFACE
  case F_ACCUIREQ:
    flag = &accept_uireq_flag;
    break;
#endif
#endif
  }
  if (len == 0) {
    if (*flag == 0)
      cmd_display(mode,channel,"OFF",1);
#ifndef DPBOXT
    else if ((*flag == 2) && ((par1 == F_INFOBELL) || (par1 == F_CBELL)))
      cmd_display(mode,channel,"OTHER",1);
#endif
    else
      cmd_display(mode,channel,"ON",1);
    return;
  }
  for (i = 0; i < len; i++) {
    ch = str[i];
    if ((ch > 0x40) && (ch < 0x5b)) {
      ch |= 0x20;
      str[i] = ch;
    }
  }    
  if (len >= 2) {
    if (strncmp(str,"on",2) == 0) {
      *flag = 1;
      cmd_display(mode,channel,"Ok",1);
      return;
    }
  } 
  if (len >= 3) {
    if (strncmp(str,"off",3) == 0) {
      *flag = 0;
      cmd_display(mode,channel,"Ok",1);
      return;
    }
  }
#ifndef DPBOXT
  if ((par1 == F_INFOBELL) || (par1 == F_CBELL)) {
    if (len >= 5) {
      if (strncmp(str,"other",5) == 0) {
        *flag = 2;
        cmd_display(mode,channel,"Ok",1);
        return;
      }
    }
  }
#endif
  cmd_display(mode,channel,"INVALID VALUE",1);
}

static void cmd_value(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int *value;
  int max;
  int min;
  char rslt_str[80];
  int val;
  int res;
  int reinit;
  
  reinit = 0;
  switch (par1) {
  case V_LINELEN:
    value = &input_linelen;
    max = COLS;
    min = 80;
    break;
#ifndef DPBOXT
  case V_MONLINES:
    value = &lines_moncon;
    max = LINES - 10;
    min = 0;
    reinit = 1;
    break;
  case V_CONDIV:
    value = &scr_divide;
    max = 40;
    min = 2;
    reinit = 1;
    break;
  case V_XMONDIV:
    value = &xmon_scr_divide;
    max = 40;
    min = 2;
    reinit = 1;
    break;
#endif
#ifdef USE_IFACE
  case V_MBOXDIV:
    value = &mbscr_divide;
    max = 40;
    min = 2;
    reinit = 1;
    break;
#endif
#ifndef DPBOXT
  case V_FPACLEN:
    value = &file_paclen;
    if (tnt_comp)
      max = 255;
    else
      max = 256;
    min = 20;
    break;
#endif
  }
  if (len == 0) {
    sprintf(rslt_str,"%d",*value);
    cmd_display(mode,channel,rslt_str,1);
    return;
  }
  res = sscanf(str,"%d",&val);
  if (res == 1) {
    if ((val <= max) && (val >= min)) {
      *value = val;
      if (reinit) reinit_screen();
      cmd_display(mode,channel,"Ok",1);
      return;
    }
  }    
  cmd_display(mode,channel,"INVALID VALUE",1);
}

static void cmd_string(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
#ifndef DPBOXT
  char *chgstr;
  
  switch (par1) {
  case S_NOACC:
    chgstr = rem_noacc_str;
    break;
#ifdef USE_IFACE
  case S_NOCONN:
    chgstr = noconn_text;
    break;
  case S_NOBOX:
    chgstr = rem_nobox_str;
    break;
  case S_NONODE:
    chgstr = rem_nonode_str;
    break;
#endif
  }
  if (len == 0) {
    cmd_display(mode,channel,chgstr,1);
    return;
  }
  if (len > 80) {
    cmd_display(mode,channel,"String too long",1);
    return;
  }
  memcpy(chgstr,str,len);
  chgstr[len] = '\0';
  cmd_display(mode,channel,"Ok",1);
#endif
}

#ifndef DPBOXT
/* Set permanent mycall */
static void cmd_mycall(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int i;
  int j;
  
  strcpy(tnc_string,"I");
  if (len) {
    if (strcmp(str,"$") == 0) {
      ch_stat[channel].mycall[0] = '\0';
    }
    else {
      tnc_command = 1;
      for (i=0;i<=9;i++) {
        ch_stat[channel].mycall[i] = toupper(str[i]);
      }
      ch_stat[channel].mycall[9] = '\0';
      /* set mycall in all channels if channel is 0 */
      if (channel == 0) {
        for (j=1;j<tnc_channels;j++)
          strcpy(ch_stat[j].mycall,ch_stat[channel].mycall);
      }
      strcat(tnc_string,str);
      len = strlen(tnc_string);
      queue_cmd_data(channel,X_COMM,len,mode,tnc_string);
    }
  }
  else {
    if (ch_stat[channel].mycall[0] == '\0') 
      cmd_display(mode,channel,"<empty>",1);
    else
      cmd_display(mode,channel,ch_stat[channel].mycall,1);
  }
}

/* Send TNC command */
static void cmd_tnc(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if (len) {
    tnc_command = 1;
    strcpy(tnc_string,str);
    len = strlen(tnc_string);
    queue_cmd_data(channel,X_COMM,len,mode,tnc_string);
  }
  else {
    cmd_display(mode,channel,"INVALID TNC STRING",1);
  }
}
#endif

/* Change directory */
static void cmd_chdir(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int result;
  char *dir;

  dir = str;
  if (strlen(dir) == 0) {
    dir = getenv("HOME");
  }  
  if ((result = chdir(dir)) == -1) {
    cmd_display(mode,channel,"Can't change directory",1);
  }
  else {
    cmd_display(mode,channel,ok_text,1);
  }
}

/* Display working directory */
static void cmd_cwdir(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char dir[80];
  
  if (getcwd(dir,80) == NULL) {
    cmd_display(mode,channel,"Can't get directory name",1);
  }
  else {
    cmd_display(mode,channel,dir,1);
  }
}

#ifndef DPBOXT
/* Display number of resyncs up to now */
static void cmd_resync(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char number[80];
  
  sprintf(number,"Resyncs: %d",resync_count);
  cmd_display(mode,channel,number,1);
  if (staterr.st_mess) {
    sprintf(number,"<st_mess>: %d",staterr.st_mess);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.rcv_frms) {
    sprintf(number,"<rcv_frms>: %d",staterr.rcv_frms);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.snd_frms) {
    sprintf(number,"<snd_frms>: %d",staterr.snd_frms);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.unacked) {
    sprintf(number,"<unacked>: %d",staterr.unacked);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.tries) {
    sprintf(number,"<tries>: %d",staterr.tries);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.axstate) {
    sprintf(number,"<axstate>: %d",staterr.axstate);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.free_buffers) {
    sprintf(number,"<free_buffers>: %d",staterr.free_buffers);
    cmd_display(mode,channel,number,1);
  }
  if (staterr.xhost) {
    sprintf(number,"<xhost>: %d",staterr.xhost);
    cmd_display(mode,channel,number,1);
  }
}

/* Display number of free buffers */
static void cmd_free(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char number[20];
  
  sprintf(number,"%d",free_buffers);
  cmd_display(mode,channel,number,1);
}

/* show signon (TNT-version) */
static void cmd_signon(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_display(mode,channel,signon,1);
}
#endif

/* Read in keyboard-macros file */
static void cmd_kmacro(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x;
  FILE *fp;
  char tmp[100], tmp2[100];

  /* first delete all macros */
  for (x = MACRO_MIN;x <= MACRO_MAX;x++) {
    strcpy(macro_strings[x].string,"");
  }
  /* now open file and copy all macro-strings */
  strcpy(tmp,func_key_file);
  fp=fopen(tmp,"r");
  if(fp!=0) {
    while(!feof(fp)) {
      fgets(tmp,99,fp);
      if(feof(fp)) break;
      if(tmp[0]=='#') continue;
      tmp[strlen(tmp)-1]='\0';
      strcpy(tmp2,tmp);
      tmp2[2]='\0';
      x=atoi(tmp2);
      if ((x >= MACRO_MIN) && (x <= MACRO_MAX)) {
        strcpy(macro_strings[x].string,&tmp[3]);
      }
    }
    fclose(fp);
    cmd_display(mode,channel,ok_text,1);
    return;
  }
  cmd_display(mode,channel,"Can't load keyboard-macro-file",1);
}
