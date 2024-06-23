/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for remote (remote.c)
   created: Mark Wahl DL4YBG 94/01/16
   updated: Mark Wahl DL4YBG 97/02/16
   updated: Matthias Hensler WS1LS 99/03/10
   updated: Johann Hanne DH3MB 98/12/28
*/

#include "tnt.h"
#include "macro.h"

#ifdef TNT_SOLARIS
#include <strings.h>
#endif

#define COR_TELL_TEXT "tnt-tellfile corrupt!"
extern void cmd_display(int flag,int channel,char *buffer,int cr);
extern void beep();
#ifdef USE_IFACE
extern int boxcut_active(int channel);
#endif
extern void data_display_len2(int channel,char *buffer);
extern void rem_data_display(int channel,char *buffer);
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void write_file(int channel,int len,int flag,char *str);
extern void close_rxfile2(int channel,int report,int rename);
extern void close_txfile(int channel,int report);
extern int shell_active(int channel);
extern void drop_priv(int mode,int channel,int *uid,int *gid);
extern void rest_priv(int mode,int channel,int uid,int gid);
extern int gen_cookie(char *tmpname,int headfoot);
extern int replace_macros(int in,int out,char *othercall,
                          char *mycall,int channel, time_t start_time);
extern int macro_getname(char *call,char *name);
extern int find_line_for_call(char *call,char *line);
extern int delete_line_for_call(char *call);
extern int add_line_for_call(char *call,char *name);
extern void statlin_update();
extern int sel_remhlist(int fd1, int flag, char *str);
extern int xconnect_active(int);
#ifdef USE_IFACE
extern void send_huffstat(int channel);
#endif
extern void cmd_input(int channel,int mode,char *str,int len,int cscript);
extern void write_file_yapp(int channel, char *str, int len);

extern int rem_datei_display();

extern struct channel_stat *ch_stat;
extern char rem_ver_str[];
extern struct tx_file *tx_file;
extern struct rx_file *rx_file;
extern int remote_flag;
extern int cookie_flag;
extern int ctext_flag;
extern int noacc_flag;
extern int autobin_flag;
extern int autoyapp_flag;
extern int auto7pl_flag;
extern int pgpauto_flag;
extern int pgpkeyadd_flag;
extern int pgpusegpg_flag;
extern int soundon_flag;
extern char tnt_ctextfile[];
extern char tnt_qtextfile[80];
extern char news_file_name[];
extern char run_dir[];
extern char macrotext_dir[];
extern char download_dir[];
extern char tnt_7plus_dir[];
extern int file_paclen;
extern int tnt_comp;
extern int tnc_channels;
extern char tnt_telltext_file[];
#ifdef TNT_SOUND
extern char tnt_sound_file[];
extern char sound_dir[];
#endif
extern char tnt_session_log[];
extern char route_file_name[80];

time_t sysopactiv;
time_t tnt_startup;
long int session_sek;
char tnt_info_message[80];
char queue_activ;
int autostart_flag;
char rem_tnt_str[80];
char rem_inv_str[80];
char rem_len_str[80];
char rem_newlin_str[80];
char rem_info_file[80];
char rem_help_file[80];
char rem_dis_str[80];
char rem_noacc_str[80];
char rem_nobox_str[80];
char rem_nonode_str[80];
char rem_cls_str[80];
char rem_brk_str[80];
char rem_wri_str[80];
char rem_wyapp_str[80];
char rem_nodir_str[80];
char rem_error_str[80];
char rem_nocook_str[80];
char rem_name_err[80];
char rem_name_str1[80];
char rem_name_str2[80];
char comp_usage_txt[80];
char comp_disable_txt[80];
char tnt_dir[80];
char remote_dir[80];
char ctext_dir[80];
char abin_start_txt[80];
char rem_send_abort[80];
char tnt_sysfile[80];
char tnt_noremfile[80];
char tnt_flchkfile[80];
char tnt_notownfile[80];
char tnt_autostartfile[80];
char tnt_extremotefile[80];
char chs_header[80];
char chs_dash[80];
char no_act_conn[80];

#ifdef TNT_SOUND
#define SOUND_MAX 8
  time_t sound_mod = 0;
  char   sound_data[SOUND_MAX][150] = { 0,0,0,0,0,0,0,0 };
#endif

struct sysoplist {
  char callsign[10];
  char sys_file[80];
  int access_level;
  struct sysoplist *next;
};

static struct sysoplist *sysoplist_root;

struct noremlist {
  char callsign[10];
  struct noremlist *next;
};

static struct noremlist *noremlist_root;

struct flchklist {
  char callsign[10];
  struct flchklist *next;
};

static struct flchklist *flchklist_root;

struct notownlist {
  char callsign[10];
  struct notownlist *next;
};

static struct notownlist *notownlist_root;

struct autostartlist {
  char callsign[10];
  char commstring[79];
  struct autostartlist *next;
};

static struct autostartlist *autostartlist_root;

struct extremotelist {
  char commstring[9];
  int level;
  int minlen;
  char command[80];
  struct extremotelist *next;
};

static struct extremotelist *extremotelist_root;

int check_call_type();
#ifdef TNT_SOUND
int play_sound(int);
#endif
static void rem_echo();
static void rem_version();
void rem_time();
static void rem_disc();
static void rem_quit();
static void rem_info();
static void rem_help();
void queue_tellmsg(); /* Msg auf allen Kanaelen ausgeben */
void rem_act();
void rem_rtt();
void rem_setmsg();
void rem_session();
void init_session();
void exit_session();
void cmd_tellmsg();
int queue_tellinfo();
void rem_tell_da();
void rem_tell_weg();
void rem_tell_600();
void rem_tell_klo();
void rem_tell_gnd();
void rem_onactiv();
void rem_sendonact();
void rem_chat();
void rem_heardlist();
static void rem_ring();
void rem_dir();
static void rem_free();
void rem_cookie();
extern void open_sendfile();
extern void open_logfile();
extern void close_file();
extern void break_send();
extern void cmd_shell();
extern void cmd_run();
#ifdef HAVE_SOCKET
extern void cmd_sockconn();
#endif
void rem_name();
static void rem_news();
#ifdef USE_IFACE
extern void cmd_box();
#endif
void cmd_comp();
void cmd_sysop();
void cmd_command();
void cmd_chanstat();
void strip_call();
void strip_call_log();

struct rem_list {
  char string[9];
  int len;
  int ext_com;
  void (*func)();
  char new_string[4];
  int par1;
  int par2;
  int access_level;
  int quiet_on_autostart;
};

/* position of command RUN in table */
#define CMD_RUN 0

static struct rem_list remote_list[] = {
  {"RUN"     ,3,1,cmd_run,"",0,0,0,1},
  {"RUNT"    ,4,1,cmd_run,"",1,0,0,1},
  {"VERSION" ,1,1,rem_version,"",0,0,0,0},
  {"ECHO"    ,1,1,rem_echo,"",0,0,1,0},
  {"TIME"    ,1,1,rem_time,"",0,0,0,0},
  {"READ"    ,1,1,open_sendfile,"",TX_PLAIN,0,0,0},
  {"RPRG"    ,2,1,open_sendfile,"",TX_ABIN,0,0,0},
  {"RYAPP"   ,2,1,open_sendfile,"",TX_YAPP,0,0,0},
  {"WRITE"   ,1,2,open_logfile,"",RX_PLAIN,RX_RCV,0,0},
  {"WPRG"    ,2,2,open_logfile,"",RX_ABIN,RX_RCV,0,0},
  {"WYAPP"   ,2,2,open_logfile,"",RX_YAPP,RX_RCV,0,0},
  {"CLOSE"   ,2,1,close_file,"",0,0,0,0},
  {"BREAK"   ,2,1,break_send,"",0,0,0,0},
  {"DISCONNE",1,1,rem_disc,"",0,0,0,0},
  {"QUIT"    ,1,1,rem_quit,"",0,0,0,0},
  {"INFO"    ,1,1,rem_info,"",0,0,0,0},
  {"HELP"    ,1,1,rem_help,"",0,0,0,0},
  {"ACT"     ,3,1,rem_act,"",0,0,0,0},
  {"ONACT"   ,3,1,rem_onactiv,"",0,0,0,0},
  {"SESSION" ,3,1,rem_session,"",0,0,0,0},
  {"CHAT"    ,2,1,rem_chat,"",0,0,0,0},
  {"MSG"     ,3,1,rem_chat,"",0,0,0,0},  /* Sorry DH3MB, I also wrote a cmd
                                            like this, WS1LS */
  {"MHEARD"  ,2,1,rem_heardlist,"",0,0,0,0},
  {"RTT"     ,2,1,rem_rtt,"",0,0,0,0},
  {"RING"    ,2,1,rem_ring,"",0,0,0,0},
  {"CAT"     ,3,1,rem_dir,"",1,0,0,0},
  {"DIR"     ,3,1,rem_dir,"",0,0,0,0},
  {"DIRLONG" ,4,1,rem_dir,"",1,0,0,0},
  {"FREE"    ,3,1,rem_free,"",0,0,0,0},
  {"COOKIE"  ,4,1,rem_cookie,"",0,0,0,0},
  {"SHELL"   ,2,1,cmd_shell,"",0,0,1,1},
  {"TSHELL"  ,3,1,cmd_shell,"",1,0,1,1},
  {"ROOTSH"  ,6,1,cmd_shell,"",2,0,2,1},
  {"TROOTSH" ,7,1,cmd_shell,"",3,0,2,1},
#ifdef HAVE_SOCKET
  {"SOCKET"  ,6,1,cmd_sockconn,"",0,0,1,1},
  {"TSOCKET" ,7,1,cmd_sockconn,"",1,0,1,1},
#endif
  {"NAME"    ,1,1,rem_name,"",0,0,0,0},
  {"NEWS"    ,4,1,rem_news,"",0,0,0,0},
  {"COMP"    ,4,1,cmd_comp,"",0,0,3,0},
  {"SYSOP"   ,3,1,cmd_sysop,"",0,0,0,0},
  {"COMMAND" ,3,1,cmd_command,"",0,0,1,0},
  {"CSTAT"   ,2,1,cmd_chanstat,"",0,0,0,0},
#ifdef USE_IFACE  
  {"BOX"     ,2,1,cmd_box,"",0,0,0,0},
#endif
  {""        ,0,-1,NULL,"",0,0,0,0}
};

/* analyse one line of sysop file */
static int sysline_analyse(str1,str2,val1,sysoplist_ptr)
char *str1;
char *str2;
int val1;
struct sysoplist **sysoplist_ptr;
{
  struct sysoplist *sysoplist_wrk;
  int i;
  
  if (strlen(str1) > 9) return(1); /* callsign maximum 9 characters */
  sysoplist_wrk = (struct sysoplist *)malloc(sizeof(struct sysoplist));
  for (i=0;i<strlen(str1);i++) {
    sysoplist_wrk->callsign[i] = toupper(str1[i]);
  }
  sysoplist_wrk->callsign[i] = '\0';
  strcpy(sysoplist_wrk->sys_file,str2);
  sysoplist_wrk->access_level = val1;
  sysoplist_wrk->next = NULL;
  if (sysoplist_root == NULL) {
    sysoplist_root = sysoplist_wrk;
    *sysoplist_ptr = sysoplist_wrk;
  }
  else {
    (*sysoplist_ptr)->next = sysoplist_wrk;
    *sysoplist_ptr = sysoplist_wrk;
  }
  return(0);
} 

/* load sysop file */
static void load_sysfile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  char str1[83];
  char str2[83];
  int val1;
  int rslt;
  FILE *sys_file_fp;
  struct sysoplist *sysoplist_cur;

  strcpy(file_str,tnt_sysfile);
  if (!(sys_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  sysoplist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,sys_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          rslt = sscanf(line,"%s %s %d",str1,str2,&val1);
          switch (rslt) {
          case EOF:
            break;
          case 3:
            if (sysline_analyse(str1,str2,val1,&sysoplist_cur)) {
              file_corrupt = 1;
              file_end = 1;
            }
            break;
          default:
            file_corrupt = 1;
            file_end = 1;
            break;
          }
        }
      }
    }
  }
  fclose(sys_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    cmd_display(M_COMMAND,0,
      "WARNING: sysfile is in wrong format, wrong line:",1);
    cmd_display(M_COMMAND,0,line,1);
    return;
  }
}

static void clear_sysfile()
{
  struct sysoplist *sysoplist_wrk;
  struct sysoplist *sysoplist_tmp;
  
  sysoplist_wrk = sysoplist_root;
  while (sysoplist_wrk != NULL) {
    sysoplist_tmp = sysoplist_wrk;
    sysoplist_wrk = sysoplist_tmp->next;
    free(sysoplist_tmp);
  }
  sysoplist_root = NULL;
}

/* load noremote file */
static void load_noremfile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  int i;
  FILE *norem_file_fp;
  struct noremlist *noremlist_cur;
  struct noremlist *noremlist_wrk;
  char *ptr;

  strcpy(file_str,tnt_noremfile);
  if (!(norem_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  noremlist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,norem_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          ptr = strchr(line,'\n');
          if (ptr != NULL) *ptr = '\0';
          if (strlen(line) <= 9) {
            noremlist_wrk = 
              (struct noremlist *)malloc(sizeof(struct noremlist));
            for (i=0;i<strlen(line);i++) {
              noremlist_wrk->callsign[i] = toupper(line[i]);
            }
            noremlist_wrk->callsign[i] = '\0';
            noremlist_wrk->next = NULL;
            if (noremlist_root == NULL) {
              noremlist_root = noremlist_wrk;
              noremlist_cur = noremlist_wrk;
            }
            else {
              noremlist_cur->next = noremlist_wrk;
              noremlist_cur = noremlist_wrk;
            }
          }
          else {
            file_corrupt = 1;
            file_end = 1;
          }
        }
      }
    }
  }
  fclose(norem_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    cmd_display(M_COMMAND,0,
      "WARNING: noremfile is in wrong format, wrong line:",1);
    cmd_display(M_COMMAND,0,line,1);
    return;
  }
}

static void clear_noremfile()
{
  struct noremlist *noremlist_wrk;
  struct noremlist *noremlist_tmp;
  
  noremlist_wrk = noremlist_root;
  while (noremlist_wrk != NULL) {
    noremlist_tmp = noremlist_wrk;
    noremlist_wrk = noremlist_tmp->next;
    free(noremlist_tmp);
  }
  noremlist_root = NULL;
}

/* load flexnet-check file */
static void load_flchkfile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  int i;
  FILE *flchk_file_fp;
  struct flchklist *flchklist_cur;
  struct flchklist *flchklist_wrk;
  char *ptr;

  strcpy(file_str,tnt_flchkfile);
  if (!(flchk_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  flchklist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,flchk_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          ptr = strchr(line,'\n');
          if (ptr != NULL) *ptr = '\0';
          if (strlen(line) <= 9) {
            flchklist_wrk = 
              (struct flchklist *)malloc(sizeof(struct flchklist));
            for (i=0;i<strlen(line);i++) {
              flchklist_wrk->callsign[i] = toupper(line[i]);
            }
            flchklist_wrk->callsign[i] = '\0';
            flchklist_wrk->next = NULL;
            if (flchklist_root == NULL) {
              flchklist_root = flchklist_wrk;
              flchklist_cur = flchklist_wrk;
            }
            else {
              flchklist_cur->next = flchklist_wrk;
              flchklist_cur = flchklist_wrk;
            }
          }
          else {
            file_corrupt = 1;
            file_end = 1;
          }
        }
      }
    }
  }
  fclose(flchk_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    cmd_display(M_COMMAND,0,
      "WARNING: flchkfile is in wrong format, wrong line:",1);
    cmd_display(M_COMMAND,0,line,1);
    return;
  }
}

static void clear_flchkfile()
{
  struct flchklist *flchklist_wrk;
  struct flchklist *flchklist_tmp;
  
  flchklist_wrk = flchklist_root;
  while (flchklist_wrk != NULL) {
    flchklist_tmp = flchklist_wrk;
    flchklist_wrk = flchklist_tmp->next;
    free(flchklist_tmp);
  }
  flchklist_root = NULL;
}

/* load not-own-call file */
static void load_notownfile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  int i;
  FILE *notown_file_fp;
  struct notownlist *notownlist_cur;
  struct notownlist *notownlist_wrk;
  char *ptr;

  strcpy(file_str,tnt_notownfile);
  if (!(notown_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  notownlist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,notown_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          ptr = strchr(line,'\n');
          if (ptr != NULL) *ptr = '\0';
          if (strlen(line) <= 9) {
            notownlist_wrk = 
              (struct notownlist *)malloc(sizeof(struct notownlist));
            for (i=0;i<strlen(line);i++) {
              notownlist_wrk->callsign[i] = toupper(line[i]);
            }
            notownlist_wrk->callsign[i] = '\0';
            notownlist_wrk->next = NULL;
            if (notownlist_root == NULL) {
              notownlist_root = notownlist_wrk;
              notownlist_cur = notownlist_wrk;
            }
            else {
              notownlist_cur->next = notownlist_wrk;
              notownlist_cur = notownlist_wrk;
            }
          }
          else {
            file_corrupt = 1;
            file_end = 1;
          }
        }
      }
    }
  }
  fclose(notown_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    cmd_display(M_COMMAND,0,
      "WARNING: notownfile is in wrong format, wrong line:",1);
    cmd_display(M_COMMAND,0,line,1);
    return;
  }
}

static void clear_notownfile()
{
  struct notownlist *notownlist_wrk;
  struct notownlist *notownlist_tmp;
  
  notownlist_wrk = notownlist_root;
  while (notownlist_wrk != NULL) {
    notownlist_tmp = notownlist_wrk;
    notownlist_wrk = notownlist_tmp->next;
    free(notownlist_tmp);
  }
  notownlist_root = NULL;
}

/* load autostart file */
static void load_autostartfile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  FILE *autostart_file_fp;
  char *ptr;
  struct autostartlist *autostartlist_wrk;
  struct autostartlist *autostartlist_cur;
  int i;
  char callsign[10];
  char commstring[80];

  strcpy(file_str,tnt_autostartfile);
  if (!(autostart_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  autostartlist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,autostart_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          ptr = strchr(line,'\n');
          if (ptr != NULL) *ptr = '\0';
          ptr = line;
          i = 0;
          while ((*ptr == ' ') || (*ptr == TAB)) ptr++;
          while ((*ptr != ' ') && (*ptr != TAB) && 
                 (*ptr != '\0') && (i < 10)) {
            callsign[i] = toupper(*ptr);
            ptr++;
            i++;
          }
          callsign[i] = '\0';
          i = 0;
          if ((*ptr == ' ') || (*ptr == TAB)) {
            while ((*ptr == ' ') || (*ptr == TAB)) ptr++;
            while ((*ptr != '\0') && (i < 79)) {
              commstring[i] = *ptr;
              ptr++;
              i++;
            }
          }
          commstring[i] = '\0';

          if ((strlen(callsign) > 0) && (strlen(commstring) > 0)) {
            autostartlist_wrk = 
              (struct autostartlist *)malloc(sizeof(struct autostartlist));
            strcpy(autostartlist_wrk->callsign,callsign);
            strcpy(autostartlist_wrk->commstring,commstring);
            autostartlist_wrk->next = NULL;
            if (autostartlist_root == NULL) {
              autostartlist_root = autostartlist_wrk;
              autostartlist_cur = autostartlist_wrk;
            }
            else {
              autostartlist_cur->next = autostartlist_wrk;
              autostartlist_cur = autostartlist_wrk;
            }
          }
        }
      }
    }
  }
  fclose(autostart_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    cmd_display(M_COMMAND,0,
      "WARNING: autostartfile is in wrong format, wrong line:",1);
    cmd_display(M_COMMAND,0,line,1);
    return;
  }
}

static void clear_autostartfile()
{
  struct autostartlist *autostartlist_wrk;
  struct autostartlist *autostartlist_tmp;
  
  autostartlist_wrk = autostartlist_root;
  while (autostartlist_wrk != NULL) {
    autostartlist_tmp = autostartlist_wrk;
    autostartlist_wrk = autostartlist_tmp->next;
    free(autostartlist_tmp);
  }
  autostartlist_root = NULL;
}

/* load not-own-call file */
static void load_extremotefile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  int i;
  FILE *extremote_file_fp;
  struct extremotelist *extremotelist_cur;
  struct extremotelist *extremotelist_wrk;
  char *ptr;
  char commstring[9];
  int level;
  int minlen;
  char command[80];

  strcpy(file_str,tnt_extremotefile);
  if (!(extremote_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  extremotelist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,extremote_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          ptr = strchr(line,'\n');
          if (ptr != NULL) *ptr = '\0';
          
          ptr = line;
          i = 0;
          while ((*ptr == ' ') || (*ptr == TAB)) ptr++;
          while ((*ptr != ' ') && (*ptr != TAB) && 
                 (*ptr != '\0') && (i < 8)) {
            commstring[i] = toupper(*ptr);
            ptr++;
            i++;
          }
          commstring[i] = '\0';
          level = -1;
          if ((*ptr == ' ') || (*ptr == TAB)) {
            while ((*ptr == ' ') || (*ptr == TAB)) ptr++;
            if ((*ptr >= '0') && (*ptr <= '4')) {
              level = *ptr - '0';
              ptr++;
            }
          }
          minlen = -1;
          if ((*ptr == ' ') || (*ptr == TAB)) {
            while ((*ptr == ' ') || (*ptr == TAB)) ptr++;
            if ((*ptr >= '1') && (*ptr <= '8')) {
              minlen = *ptr - '0';
              ptr++;
            }
          }
          i = 0;
          if ((*ptr == ' ') || (*ptr == TAB)) {
            while ((*ptr == ' ') || (*ptr == TAB)) ptr++;
            while ((*ptr != '\0') && (i < 79)) {
              command[i] = *ptr;
              ptr++;
              i++;
            }
          }
          command[i] = '\0';
          if ((strlen(commstring) > 0) && (strlen(command) > 0) &&
               (level != -1) && (minlen != -1)) {
            extremotelist_wrk = 
              (struct extremotelist *)malloc(sizeof(struct extremotelist));
            strcpy(extremotelist_wrk->commstring,commstring);
            extremotelist_wrk->level = level;
            extremotelist_wrk->minlen = minlen;
            strcpy(extremotelist_wrk->command,command);
            extremotelist_wrk->next = NULL;
            if (extremotelist_root == NULL) {
              extremotelist_root = extremotelist_wrk;
              extremotelist_cur = extremotelist_wrk;
            }
            else {
              extremotelist_cur->next = extremotelist_wrk;
              extremotelist_cur = extremotelist_wrk;
            }
          }
          else {
            file_corrupt = 1;
            file_end = 1;
          }
        }
      }
    }
  }
  fclose(extremote_file_fp);
  if (file_corrupt) {
    if (line == NULL) line[0] = '\0';
    cmd_display(M_COMMAND,0,
      "WARNING: extremotefile is in wrong format, wrong line:",1);
    cmd_display(M_COMMAND,0,line,1);
    return;
  }
}

static void clear_extremotefile()
{
  struct extremotelist *extremotelist_wrk;
  struct extremotelist *extremotelist_tmp;
  
  extremotelist_wrk = extremotelist_root;
  while (extremotelist_wrk != NULL) {
    extremotelist_tmp = extremotelist_wrk;
    extremotelist_wrk = extremotelist_tmp->next;
    free(extremotelist_tmp);
  }
  extremotelist_root = NULL;
}


void init_remote()
{
  int t;
  
  t = (int)time(NULL);
  srand(t);
  autostart_flag = 0;
  remote_flag = 0;
  cookie_flag = 0;
  ctext_flag = 0;
  noacc_flag = 0;
  autobin_flag = 1;
  autoyapp_flag = 0;
  auto7pl_flag = 0;
  pgpauto_flag = 0;
  pgpkeyadd_flag = 1;
  pgpusegpg_flag = 0;
  strcpy(rem_tnt_str,"<TNT>: ");
  strcpy(rem_inv_str,"Invalid command !");
  strcpy(rem_len_str,"Line too long, ignored");
  strcpy(rem_newlin_str,"\015");
  strcpy(rem_dis_str,"\01573!\015");
  strcpy(rem_cls_str,"File closed");
  strcpy(rem_brk_str,"Transmission aborted");
  strcpy(rem_wri_str,"File open, end with //close");
  strcpy(rem_wyapp_str,"YAPP-reception activated");
  strcpy(rem_nodir_str,"No such file or directory or no permission");
  strcpy(rem_error_str,"Function not successful");
  strcpy(rem_nocook_str,"Cookie-file not existing");
  strcpy(rem_name_err,"Error writing to names-file");
  strcpy(rem_name_str1,"Thanks, ");
  strcpy(rem_name_str2,", your name has been stored");
  strcpy(abin_start_txt,"AutoBIN-receive started, filename: ");
  strcpy(rem_send_abort,"AutoBIN-send aborted");
  strcpy(comp_usage_txt,"Usage: //COMP ON/OFF");
  strcpy(comp_disable_txt,"Sorry, compression not possible");
  strcpy(rem_noacc_str,"Sorry, no access to this system");
  strcpy(rem_nobox_str,"Sorry, box is down, please try later");
  strcpy(rem_nonode_str,"Sorry, node is down, please try later");
  strcpy(chs_header,"Channel   From        To          Starttime");
  strcpy(chs_dash,  "------------------------------------------------");
  strcpy(no_act_conn,"No stations connected");
  sysoplist_root = NULL;
  load_sysfile();
  noremlist_root = NULL;
  load_noremfile();
  flchklist_root = NULL;
  load_flchkfile();
  notownlist_root = NULL;
  load_notownfile();
  autostartlist_root = NULL;
  load_autostartfile();
  extremotelist_root = NULL;
  load_extremotefile();
}

void exit_remote()
{
  clear_sysfile();
  clear_noremfile();
  clear_flchkfile();
  clear_notownfile();
  clear_autostartfile();
  clear_extremotefile();
}

static char *delete_path(char *filename)
{
  char *ptr1;
  char *ptr2;
  char *ptr;
  
  ptr1 = strrchr(filename,'\\');
  ptr2 = strrchr(filename,'/');
  if ((ptr1 == NULL) && (ptr2 == NULL)) {
    ptr = strchr(filename,':');
    if (ptr == NULL)
      ptr = filename;
    else
      ptr++;
  }
  else if (ptr1 == NULL) ptr = ptr2 + 1;
  else if (ptr2 == NULL) ptr = ptr1 + 1;
  else if (ptr1 > ptr2) ptr = ptr1 + 1;
  else ptr = ptr2 + 1;
  return(ptr);
}

/* check for yapp start sequence */
int check_autoyapp(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char hlpstr[3];
  
  if (!autoyapp_flag) return(0);
  if (len < 2) return(0);
  if ((buffer[len-2] != 0x05) || (buffer[len-1] != 0x01)) return(0);
  open_logfile(RX_YAPP,RX_RCV,channel,0,M_CMDSCRIPT,"");
  rem_data_display(channel,rem_newlin_str);
  hlpstr[0] = 0x05;
  hlpstr[1] = 0x01;
  hlpstr[2] = '\0';
  write_file_yapp(channel,hlpstr,2);
  return(1);
}

/* handle 7plus strings/reception */
static int handle_7pl(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  int offset;
  char filename[257];
  char filename2[257];
  char hlpstring[257];
  int i;
  int typ;
  int part;
  int errflag;
  int res;
  char *ptr1;
  char *ptr2;
  char *q;
    
  if (rx_file[channel].type == RX_AUTO7P) {
    /* already receiving file, just check for endstring */
    if (strncmp(buffer," stop_",6) == 0) {
      write_file(channel,len,RX_RCV,buffer);
      close_rxfile2(channel,0,0);
    }
    return(1);
  }
  /* code from 7plus extract.c follows here */
  /* (c) Axel Bauda, DG1BBQ */
  /* This is necessary, because of some strange BBS in the UK that keeps
   *        stripping the space in the first line. */
  offset=0;
  typ = 0;
  errflag = 0;
  if (!strncmp (buffer, " go_", 4))
    offset = 4;
  if (!strncmp (buffer, "go_", 3))
    offset = 3;
  if (offset) { /* header valid */
    if (!strncmp (buffer+offset, "7+.", 3)) {
      /* It's a code file */
      /* Get filename from header. Create output filename. */
      res = sscanf(buffer+offset+3,"%d%s%s%s",
             &part, hlpstring, hlpstring, filename2);
      if (res == 4) {
        if ((q = strrchr (filename2, '.')) != NULL)
           *q = '\0';
        filename2[8] = '\0';
        if (strstr(buffer, "of 001"))
          sprintf(hlpstring, ".7PL");
        else
          sprintf(hlpstring, ".P%02x", part);
        strcat(filename2,hlpstring);
        typ = 1; /* is a code file */
      }
    }
    /* OK, then it could be an ERR or COR file.
     *          Careful! It could also be a marked textfile */
    if (!strncmp(buffer+offset,"text.",5) && (strstr(buffer,".ERR"))) {
      res = sscanf(buffer+offset+6,"%12s", filename2);
      if (res == 1) typ = 2; /* ERR */
    }
    if (!strncmp(buffer+offset,"text.",5) && (strstr(buffer,".COR"))) {
      res = sscanf(buffer+offset+6,"%12s", filename2);
      if (res == 1) typ = 3; /* COR */
    }
    /* It could also be an info file accompanying the code file */
    if (!strncmp (buffer+offset, "info.", 5)) {
      res = sscanf(buffer+offset+6,"%12s", filename2);
      if (res == 1) typ = 4;
    }
    if (typ) {
      ptr1 = strrchr(filename2,'/');
      if (ptr1 == NULL) ptr1 = filename2;
      ptr2 = strrchr(ptr1,'\\');
      if (ptr2 == NULL) ptr2 = ptr1;
      strcpy(filename,tnt_7plus_dir);
      strcat(filename,ptr2);
      for (i = 0;i < strlen(filename);i++)
        filename[i] = tolower(filename[i]);

      i = 1;
      open_logfile(RX_AUTO7P,RX_RCV,channel,strlen(filename),
                   M_CONNECT,filename);
      errflag = (rx_file[channel].type != RX_AUTO7P);

      q = strchr(filename,'.');
      if ((typ != 1) && (q != NULL)) *q = '\0'; /* do not remove .p0x */

      if (errflag) {  /* File already exists */
        do {
          switch(typ) {
          case 1:       /* code file  */
            sprintf(hlpstring,"%s.%02x",filename,i++);
            break;
          case 2:       /* ERR-File */
            sprintf(hlpstring,"%s.e%02x",filename,i++);
            break;
          case 3:       /* COR-File */
            sprintf(hlpstring,"%s.c%02x",filename,i++);
            break;
          case 4:       /* Inf-File */
            sprintf(hlpstring,"%s.i%02x",filename,i++);
            break;
          }
          open_logfile(RX_AUTO7P,RX_RCV,channel,strlen(hlpstring),
                       M_CONNECT,hlpstring);
          errflag = (rx_file[channel].type != RX_AUTO7P);
        } while ((i < 255) && errflag);
      }
      if (!errflag) return(1);
    }
  }
  return(0);
}


/* handle PGP strings/reception
 * for now we are only looking for signed messages and keyblocks
 * returns !0  if buffer handled
 * dg9ep 7-98
 */
static int handle_pgp(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char filename[257];
  char tmpstdoutname[257];
  char tmpstderrname[257];
  char cmdline[300];
  char sTmp[300];
  int i;
  int typ;
  int part;
  int errflag;
  int res;
  int ans_len;
  int result;

  if (rx_file[channel].type == RX_PGP) {
    /* already receiving file, just check for endstring */
    typ = 0;
    if        (!strncmp(buffer,"-----END PGP SIGNATURE-----",       27)) {
       typ = PGP_SIGNED_MESSAGE;
    } else if (!strncmp(buffer,"-----END PGP PUBLIC KEY BLOCK-----",34)) {
       typ = PGP_PUBLIC_KEY_BLOCK;
    }
    if ( typ > 0 ) {
      /* end of Message, now process the input */
      write_file(channel,len,RX_RCV,buffer);
      strcpy(filename,rx_file[channel].name);
      close_rxfile2(channel,0,0);

      strcpy(tmpstdoutname,"/tmp/tnt_pgp_out.XXXXXX");
      mkstemp(tmpstdoutname);
      strcpy(tmpstderrname,"/tmp/tnt_pgp_err.XXXXXX");
      mkstemp(tmpstderrname);

      if( pgpusegpg_flag ) {
          /* build cmdline for gpg */
          strcpy(cmdline,"gpg --batch -o /dev/null ");
          if( typ == PGP_PUBLIC_KEY_BLOCK ) { /* Keyblock */
              if( pgpkeyadd_flag ) {  /* add keys to keyring */
                 strcat(cmdline,"--import -v ");
              } else { /* show keys only */
                 strcat(cmdline,"-v ");
                 rem_data_display(channel,"(these keys will _not_ be added to your keyring)\r");
              }
          }
          strcat(cmdline,filename);
      } else {
          /* build cmdline for pgp */
          strcpy(cmdline,"pgp +batchmode +verbose=0 -o /dev/null "); /* $TODO -> Outfile auch PGP vorwerfen */
          if( typ == PGP_PUBLIC_KEY_BLOCK ) {
              if( pgpkeyadd_flag ) {  /* add keys to keyring */
                 strcat(cmdline," +interactive=off +force -ka ");
              } else { /* show keys only */
                 rem_data_display(channel,"(keys will not be added to keyring)\r");
              }
          }
          strcat(cmdline,filename);
      }
      strcat(cmdline," > ");
      strcat(cmdline,tmpstdoutname);
      strcat(cmdline," 2> ");  /* btw: gpg directs its output to stderr ... */
      strcat(cmdline,tmpstderrname);

      /* strcpy(sTmp,cmdline);
       * rem_data_display(channel,sTmp);
       * rem_data_display(channel,"\r");
       */

      result = system(cmdline);

      /* Ausgabe von pgp anzeigen */
      rem_datei_display(channel,tmpstdoutname);
      rem_datei_display(channel,tmpstderrname);
      unlink(tmpstdoutname);
      unlink(tmpstderrname);

      unlink(filename);
    }
    return(1);
  }

  typ = 0;
  if        (!strncmp (buffer, "-----BEGIN PGP SIGNED MESSAGE-----",34)) {
      /* It's a signed file */
      typ = PGP_SIGNED_MESSAGE;
  } else if (!strncmp (buffer, "-----BEGIN PGP PUBLIC KEY BLOCK-----",36)) {
      typ = PGP_PUBLIC_KEY_BLOCK;
  }
  if( typ ) { /* start recording pgp-data */
      /* Get tmp. filename */
      strcpy(filename,"/tmp/tnt_pgp.XXXXXX");
      mkstemp(filename);

      open_logfile(RX_PGP,RX_RCV,channel,strlen(filename),
                   M_CONNECT,filename);
      errflag = (rx_file[channel].type != RX_PGP);
      if (!errflag) return(1);
  }
  return(0);
}




/* check packet, if special strings are contained */
static int check_spec_strings(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char date_str[257];
  char filename[257];
  char reststring[257];
  char *restptr;
  int wait_bin;
  int start_bin;
  int umw;
  char tmpstr[10];
  int flag;
  char *ptr;
  char file_str[256];
  int i;
  int fd;
  int display;
  
  display = 0;

  /* 7plus handling */
  if (((rx_file[channel].type == -1) && auto7pl_flag) ||
      (rx_file[channel].type == RX_AUTO7P)) {
    if (handle_7pl(channel,buffer,len)) return(0);
  }

  /* PGP handling  dg9ep */
  if (((rx_file[channel].type == -1) && pgpauto_flag) ||
       (rx_file[channel].type == RX_PGP)) {
    if (handle_pgp(channel,buffer,len)) return(0);
  }

  wait_bin = ((rx_file[channel].type == RX_ABIN) ||
              (rx_file[channel].type == RX_ABIN_Q)) && 
             (rx_file[channel].wait_bin);
#ifdef USE_IFACE
  start_bin = (rx_file[channel].type == -1) && 
              (autobin_flag || boxcut_active(channel));
#else
  start_bin = (rx_file[channel].type == -1) && (autobin_flag);
#endif
  if (!wait_bin && !start_bin) return(0);
  if (wait_bin) {
    if (strncmp(buffer,"#ABORT#",7) == 0) {
      close_rxfile2(channel,0,1);
      cmd_display(M_REMOTE,channel,"AutoBIN-receive aborted",1);
      return(0);
    }
  }
  if (len < 6) return(0);
  if (strncmp(buffer,"#BIN#",5) != 0) return(0);
  umw = sscanf(buffer,"#BIN#%d#|%d#$%s",&rx_file[channel].len,
               &rx_file[channel].crc,reststring);
  if (umw <= 0) return(0);
  if (rx_file[channel].len <= 0) return(0);
  if (umw == 3) {
    if ((restptr = strchr(reststring,'#')) != NULL) {
      strncpy(date_str,reststring,restptr-reststring);
      date_str[restptr-reststring] = '\0';
      strcpy(filename,restptr+1);
      if (filename[0] != '\0') 
        umw = 4;
    }
    else {
      strcpy(date_str,reststring);
    }
  }
  if (start_bin) {
    switch (umw) {
    case 1:
      sprintf(rx_file[channel].binheader,"#BIN#%d",rx_file[channel].len);
      break;
    case 2:
      sprintf(rx_file[channel].binheader,"#BIN#%d#|%d",rx_file[channel].len,
              rx_file[channel].crc);
      break;
    case 3:
      sprintf(rx_file[channel].binheader,"#BIN#%d#|%d#$%s",rx_file[channel].len,
              rx_file[channel].crc,date_str);
      break;
    case 4:
      if (strlen(filename) > 45) {
        ptr = delete_path(filename);
      }
      else
        ptr = filename;
      sprintf(rx_file[channel].binheader,"#BIN#%d#|%d#$%s#%s",
              rx_file[channel].len,rx_file[channel].crc,date_str,ptr);
      break;
    }
    /* try to open file with received name */
    if (umw != 4) {
      strcpy(filename,download_dir);
      strcat(filename,"abinXXXXXX");
      mkstemp(filename);
      ptr = strrchr(filename,'/') + 1;
    }
    else {
      for (i=0;i<strlen(filename);i++) {
        filename[i] = tolower(filename[i]);
      }
      ptr = delete_path(filename);
      strcpy(file_str,download_dir);
      strcat(file_str,ptr);
      if ((fd = open(file_str,O_RDONLY)) != -1) {
        close(fd);
        strcpy(filename,download_dir);
        strcat(filename,"abinXXXXXX");
        mkstemp(filename);
        ptr = strrchr(filename,'/') + 1;
      }
    }
#ifdef USE_IFACE
    if (boxcut_active(channel))
      open_logfile(RX_ABIN_E,RX_RCV,channel,strlen(ptr),M_CONNECT,ptr);
    else
      open_logfile(RX_ABIN_Q,RX_RCV,channel,strlen(ptr),M_CONNECT,ptr);
#else
    open_logfile(RX_ABIN_Q,RX_RCV,channel,strlen(ptr),M_CONNECT,ptr);
#endif
    strcpy(file_str,abin_start_txt);
    strcat(file_str,ptr);
    strcat(file_str,rem_newlin_str);
    display = 1;
  }
  if (umw == 1) rx_file[channel].crc = 0;
  rx_file[channel].crc_tmp = 0;
  rx_file[channel].len_tmp = 0;
  rx_file[channel].wait_bin = 0;
  flag = 0;
  rx_file[channel].start_time = time(NULL);
  if (display)
    rem_data_display(channel,file_str);
  strcpy(tmpstr,"#OK#\015");
  rem_data_display(channel,tmpstr);
  queue_cmd_data(channel,X_DATA,strlen(tmpstr),flag,tmpstr);
  return(1);
}

int analyse_remstr(channel,remstring,autost)
int channel;
char *remstring;
int autost;
{
  int len;
  int found;
  char com_string[11];
  int i,j;
  int level;
  struct extremotelist *extremotelist_wrk;
  int invalid;
  int result;
  int flag;
  char *str;
  int par2;
  int mode;
  char tmp_str[80];

  level = -1;
  invalid = 0;
  result = 0;
  str = remstring;
  i = 0;
  len = strlen(str);

  if (len > 79) {
    if (ch_stat[channel].remote) {
      strcpy(str,rem_tnt_str);
      strcat(str,rem_len_str);
      strcat(str,rem_newlin_str);
      len = strlen(str);
      flag = 0;
      rem_data_display(channel,str);
      queue_cmd_data(channel,X_DATA,len,flag,str);
    }
    return(result);
  }

  while ((i < 8) && (i < len) && (*str != SPACE)) {
    com_string[i] = *str++;
    if ((com_string[i] > 0x60) && (com_string[i] < 0x7E))
      com_string[i] &= 0xDF;
    i++;  
  }
  com_string[i] = '\0';
  if ((i != len) && (*str != SPACE)) {
    invalid = 1;
  }
  len -= i;
  if (!invalid) {
    /* find string in extended remote command list */
    if (extremotelist_root != NULL) {
      extremotelist_wrk = extremotelist_root;
      while (extremotelist_wrk != NULL) {
        if (strlen(com_string) >= extremotelist_wrk->minlen) {
          if (strncmp(extremotelist_wrk->commstring,
                      com_string,strlen(com_string)) == 0) {
            level = extremotelist_wrk->level;
            i = 0;
            str = remstring;
            strcpy(str,extremotelist_wrk->command);
            len = strlen(str);
            while ((i < 8) && (i < len) && (*str != SPACE)) {
              com_string[i] = *str++;
              if ((com_string[i] > 0x60) && (com_string[i] < 0x7E))
                com_string[i] &= 0xDF;
              i++;  
            }
            com_string[i] = '\0';
            if ((i != len) && (*str != SPACE)) {
              invalid = 1;
            }
            len -= i;
            break;
          }
        }
        extremotelist_wrk = extremotelist_wrk->next;
      }
    }
  }
  if (!invalid) {
    /* check if numeric follow (chat) */
    if( (com_string[0] >= '1') && (com_string[0] <= '9') ) {
      if( (com_string[1] == '\0') || (com_string[2] == '\0') ) {
        j = com_string[0] - '0';
        if(com_string[1] != '\0') {
          if( (com_string[1] < '0') || (com_string[1] > '9') ) invalid=1;
          j = j * 10;
          j += (com_string[1] - '0');
        }
        if(!invalid) {
          if( (j<1) || (j>=tnc_channels) ) {
            strcpy(tmp_str,"<TNT>:Channel out of range.");
            strcat(tmp_str, rem_newlin_str);
            len=strlen(tmp_str);
            rem_data_display(channel, tmp_str);
            queue_cmd_data(channel,X_DATA,len,flag,tmp_str);
            return(result);
          }
          while(*str == SPACE) {
            str++;
            len--;
          }
          if(len == 0) {
            strcpy(tmp_str,"<TNT>:missing text");
            strcat(tmp_str, rem_newlin_str);
            rem_data_display(channel, tmp_str);
            queue_cmd_data(channel,X_DATA,strlen(tmp_str),flag,tmp_str);
            return(result);
          }
          if(ch_stat[j].conn_state != CS_CONN) {
            sprintf(tmp_str,"<TNT>:channel %d not connected", j);
            strcat(tmp_str, rem_newlin_str);
            rem_data_display(channel, tmp_str);
            queue_cmd_data(channel,X_DATA,strlen(tmp_str),flag,tmp_str);
            return(result);
          }
          strip_call_log(com_string,j);
          if((tx_file[j].type != -1) || (rx_file[j].type != -1) ||
             (xconnect_active(j)) || (check_call_type(com_string))) {
            sprintf(tmp_str,"<TNT>:sorry no chat to %s", com_string);
            strcat(tmp_str, rem_newlin_str);
            rem_data_display(channel, tmp_str);
            queue_cmd_data(channel,X_DATA,strlen(tmp_str),flag,tmp_str);
            return(result);
          }
          strip_call_log(com_string,channel);
          sprintf(tmp_str,"<Msg de %d:%s, reply with \"//%d ...\">",
                  channel,com_string,channel);
          strcat(tmp_str,rem_newlin_str);
          strcat(tmp_str,str);
          strcat(tmp_str,rem_newlin_str);
          rem_data_display(j, tmp_str);
          queue_cmd_data(j,X_DATA,strlen(tmp_str),flag,tmp_str);
          return(result);
        }
        strcpy(tmp_str,"<TNT>: illegal channel");
        strcat(tmp_str,rem_newlin_str);
        rem_data_display(channel, tmp_str);
        queue_cmd_data(channel,X_DATA,strlen(tmp_str),flag,tmp_str);
        return(result);
      }
    }

    /* find string in remote-command-list */
    found = 0;
    j = 0;
    while ((remote_list[j].ext_com != -1) && !found) {
      if (strstr(remote_list[j].string,com_string)
          == remote_list[j].string) {
        /* string found, now check if length valid */
        if (strlen(com_string) >= remote_list[j].len) {
          found = 1;
        }
      }
      if (!found) j++;
    }
    if (found) {
      if (level == -1) level = remote_list[j].access_level;
      if ((ch_stat[channel].remote &&
           (ch_stat[channel].access_level >= level)) || (level == 3)) { 
        /* delete spaces before parameters */
        while (*str == SPACE) {
          str++;
          len--;
        }
        if (remote_list[j].ext_com == 2) result = 1;
        if (remote_list[j].ext_com) {
          par2 = remote_list[j].par2; 
          mode = M_REMOTE;
          if (autost == 1) {
            if (remote_list[j].quiet_on_autostart) {
              mode = M_CMDSCRIPT;
              par2 = 1;
            }
          }
          (*remote_list[j].func) (remote_list[j].par1,par2,
                                  channel,len,mode,str);
        }
        return(result);
      }
    }
    else {
      if ((ch_stat[channel].remote &&
          (ch_stat[channel].access_level >= remote_list[CMD_RUN].access_level)) ||
          (remote_list[CMD_RUN].access_level == 3)) {
        par2 = remote_list[CMD_RUN].par2; 
        mode = M_REMOTE;
        if (autost == 1) {
          if (remote_list[CMD_RUN].quiet_on_autostart) {
            mode = M_CMDSCRIPT;
            par2 = 1;
          }
        }
        cmd_run(remote_list[CMD_RUN].par1,par2,
                channel,strlen(remstring),mode,remstring);
        return(result);
      }
    }
  }
  if (autost) {
    queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
  }
  else {
    if (ch_stat[channel].remote) {
      strcpy(str,rem_tnt_str);
      strcat(str,rem_inv_str);
      strcat(str,rem_newlin_str);
      len = strlen(str);
      flag = 0;
      rem_data_display(channel,str);
      queue_cmd_data(channel,X_DATA,len,flag,str);
    }
  }
  return(result);
}

/* analyse remote-input-string */
/* if result != 0 the string must not be written to a file */
int remote_input(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char strbuf[257];
  int result;
  char ans_str[82];
  

  /* first scan for #BIN# */
  if (check_spec_strings(channel,buffer,len)) return(1);

  /* no remote if autobin-send */
  if ((tx_file[channel].type == TX_ABIN) ||
      (tx_file[channel].type == TX_ABINQ)) {
    if (tx_file[channel].wait_ok) {
      if (len >= 4) {
        if (strncmp(buffer,"#OK#",4) == 0) {
          tx_file[channel].wait_ok = 0;
          tx_file[channel].start_time = time(NULL);
          return(0);
        }
      }
    }
    if (len >= 7) {
      if (strncmp(buffer,"#ABORT#",7) == 0) {
        close_txfile(channel,0);
        if (tx_file[channel].type == TX_ABIN) {
          cmd_display(M_REMOTE,channel,rem_send_abort,1);
        }
        else {
          strcpy(ans_str,rem_send_abort);
          strcat(ans_str,rem_newlin_str);
          rem_data_display(channel,ans_str);
        }
        return(0);
      }
    }
    return(0);
  }
  /* no remote if autobin-receive */
  if ((rx_file[channel].type == RX_ABIN) ||
      (rx_file[channel].type == RX_ABIN_Q) ||
      (rx_file[channel].type == RX_ABIN_E)) {
    return(0);
  }
  if (!remote_flag) return(0);
  /* remote commands only if still connected */
  if (ch_stat[channel].conn_state != CS_CONN) return(0);
  result = 0;
  /* check if remote command */
  if (len >= 3) {
    if (strncmp(buffer,"//",2) == 0) {
      /* remote only for terminals, not for nodes and boxes */
      strip_call_log(ans_str,channel);
      if( (check_call_type(ans_str)) && (strncmp(buffer,"//COMP",6)!=0) )
           return(0);
      strcpy(strbuf,buffer+2);
      strbuf[len-3] = '\0';
      result = analyse_remstr(channel,strbuf,0);
    }
  }
  return(result);
}

static void rem_echo(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int flag;
  char ans_str[256];
  int ans_len;
  
  flag = 0;
  strcpy(ans_str,str);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
}

static void rem_ring(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int flag;
  char ans_str[80] = "<TNT>: Bell has rung..";
  int ans_len;
  int numring;
  
  flag = 0;
#ifdef TNT_SOUND
  if(play_sound(7) == 1) {
#endif
    for (numring = 1; numring < 15; numring++) {
      beep();
      usleep(100*1000);  
    } 
#ifdef TNT_SOUND
  }
#endif
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
}

void rem_session(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  time_t tnt_session;
  char tmp_str[80];
  char ans_str[160];
  long int session_time;

  tnt_session = time(&tnt_session);
  session_time = tnt_session - tnt_startup;
  sprintf(tmp_str,"<TNT>: Lifetime of this session: %dd %dh %dm %ds",
                  (session_time / 86400), ((session_time / 3600)%24),
                  ((session_time / 60)%60), (session_time % 60));
  strcpy(ans_str, tmp_str);
  strcat(ans_str, rem_newlin_str);

  session_time = (tnt_session - tnt_startup) + session_sek;
  sprintf(tmp_str,"       Lifetime of TNT:          %dd %dh %dm %ds",
                  (session_time / 86400), ((session_time / 3600)%24),
                  ((session_time / 60)%60), (session_time % 60));
  strcat(ans_str, tmp_str);
  strcat(ans_str, rem_newlin_str);
  rem_data_display(channel, ans_str);
  queue_cmd_data(channel,X_DATA,strlen(ans_str),0,ans_str);
}

void rem_act(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char ans_str[80];
  char tmp_str[20];
  long diffsec;
  time_t acttime;
  int days, hour, min;
  int ans_len;

  acttime = time(&acttime);
  diffsec = acttime - sysopactiv;

  strcpy(ans_str,"<TNT>: Die letzte Sysopaktivitaet war vor ");
  days = (diffsec / 86400);
  hour = (diffsec / 3600) % 24;
  min  = (diffsec / 60)   % 60;
  diffsec %= 60;
  if(days>0) sprintf(tmp_str,"%dd %dh %dm %ds",days,hour,min,diffsec);
  else if(hour>0) sprintf(tmp_str,"%dh %dm %ds",hour,min,diffsec);
  else if(min>0) sprintf(tmp_str,"%dm %ds",min,diffsec);
  else sprintf(tmp_str,"%d Sekunden.",diffsec);
  strcat(ans_str,tmp_str);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,0,ans_str);
}

void rem_setmsg(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char ans_str[80];

  if(strlen(str)==0) {
    sprintf(ans_str,"MSG: %s",tnt_info_message); }
  else {
    if((strcmp(str,"0")==0) || (strcmp(str,"1")==0)) {
      tnt_info_message[0]='\0';
      strcpy(ans_str,"<OK>: string clear");
    } else {
      sprintf(ans_str,"<OK>:%s",str);
      strcpy(tnt_info_message,str);
    }
  }
  ans_str[78] = '\0';
  cmd_display(mode,channel,ans_str,1);
}

void rem_rtt(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int flag;
  char ans_str[80];
  int ans_len;
  char *substr;
  long tmplong;  
  char *p1, *p2;  
  time_t t;
  struct tm *timep;
  char hour, min, sec;
  long diffsec;
  
  
  if (ch_stat[channel].conn_state != CS_CONN) {
    cmd_display(mode,channel,"Only while connected",1);
    return;
  }
  flag = 0;
  strcpy(ans_str, str);
  if (len >= 41) { 
    p1 = strtok(ans_str, "$");
    p2 = strtok(NULL, " ");
    if (p2 != NULL)  {
      tmplong = strtoul(p2, 0, 16);
      if (tmplong > 0) {
        t = time(&t);
        timep = localtime(&t);
        diffsec = t - tmplong;
        hour = diffsec / 3600;
        min =  (diffsec % 3600)/ 60;
        sec = diffsec % 60;
        sprintf(ans_str, "<TNT>: Link time %02d:%02d:%02d (%02d.%02d.%04d %02d:%02d:%02d)", 
                hour, min, sec,
                timep->tm_mday,timep->tm_mon+1,timep->tm_year+1900,
                timep->tm_hour,timep->tm_min,timep->tm_sec);
        substr = ans_str;
      }
      else substr = "<TNT>: RTT failure!"; 
    } 
    else substr = "<TNT>: RTT failure!";
  } 
  else {
    t = time(&t);
    timep = localtime(&t);
    sprintf(ans_str, "//ECHO //RTT Link time $%X (%02d.%02d.%04d %02d:%02d:%02d)",
            (unsigned int)t, timep->tm_mday, timep->tm_mon+1, timep->tm_year+1900,
            timep->tm_hour, timep->tm_min, timep->tm_sec);
    substr= ans_str;
  }   
  strcpy(ans_str,substr);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
}

static void rem_version(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int flag;
  char ans_str[80];
  int ans_len;
  
  flag = 0;
  strcpy(ans_str,rem_tnt_str);
  strcat(ans_str,rem_ver_str);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
}

void rem_time(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct tm timestr;
  time_t timeval;
  char tmpstr[40];
  char ans_str[80];
  int ans_len;
  int flag;
  
  timeval = time(&timeval);
  timestr = *localtime(&timeval);
  sprintf(tmpstr,"%2.2u/%2.2u/%2.2u %2.2u:%2.2u:%2.2u",
          timestr.tm_year,timestr.tm_mon+1,timestr.tm_mday,
          timestr.tm_hour,timestr.tm_min,timestr.tm_sec);
  flag = 0;
  strcpy(ans_str,rem_tnt_str);
  strcat(ans_str,tmpstr);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
}

void end_comp(channel)
int channel;
{
  int flag;
  char ans_str[11];
  int ans_len;

  if ((channel < 1) || (channel > tnc_channels)) return;
  flag = 0;
  if (ch_stat[channel].huffcod) {
    strcpy(ans_str,"\r//COMP 0\r");
    ans_len = strlen(ans_str);
    rem_data_display(channel,ans_str);
    queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
  }
}

/* send disc */
static void rem_disc(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  ch_stat[channel].queue_act = 0; /* Keine Auftraege mehr bearbeiten */
  end_comp(channel);
  queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
}

/* send qtext and disc, some routines stolen from DL4YBG */
static void rem_quit(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  char string[260];
  char tmpname[20];
  int result;
  int fd1;
  int fd2;
  int i;
  int end;
  int flag;
  char ans_str[10];
  char call[10];
  int ans_len;
  FILE *qtf;
  int tmp_byte;
  
  strcpy(tmpname,"/tmp/tntqtxXXXXXX");
  mkstemp(tmpname);

  strip_call_log(call,channel);
  flag = 0;

  strcpy(string,tnt_qtextfile);
  fd1 = open(string,O_RDONLY);
  if(fd1 < 0) {
    ans_len = strlen(rem_dis_str);
    rem_data_display(channel,rem_dis_str);
    queue_cmd_data(channel,X_DATA,ans_len,flag,rem_dis_str);
  } else {
    fd2 = open(tmpname,O_RDWR|O_CREAT,PMODE);
    if(fd2 < 0) {
      close(fd1);
      ans_len = strlen(rem_dis_str);
      rem_data_display(channel,rem_dis_str);
      queue_cmd_data(channel,X_DATA,ans_len,flag,rem_dis_str);
    } else {
      result = replace_macros(fd1,fd2,call,
                            ch_stat[channel].curcall,channel,
                            ch_stat[channel].start_time);
      close(fd1);
      close(fd2);

      if (result > 0) {
        qtf=fopen(tmpname,"rb");
        string[0]='\0';
        while((tmp_byte=fgetc(qtf))!= -1) {
          if(tmp_byte == '\n') strcat(string,rem_newlin_str);
          else { /* umstaendlich :) */
            sprintf(call, "%c", tmp_byte);
            strcat(string, call);
          }
      /*  else strncat(string, &tmp_byte, 1); */
          ans_len = strlen(string);
          if(ans_len > 250) {
            rem_data_display(channel,string);
            queue_cmd_data(channel,X_DATA,ans_len,flag,string);
            string[0]='\0';
            ans_len=0;
          }
        }
        if(ans_len > 0) {
          rem_data_display(channel,string);
          queue_cmd_data(channel,X_DATA,ans_len,flag,string);
        }
        fclose(qtf);
        unlink(tmpname);
      }
      else {
        unlink(tmpname);
        if (result == 1)
        cmd_display(mode,channel,rem_nocook_str,1);
        else
        cmd_display(mode,channel,rem_error_str,1);
      }
    }
  }
  ch_stat[channel].queue_act = 0; /* Keine Auftraege mehr */
  end_comp(channel);
  strcpy(ans_str,"D");
  ans_len = strlen(ans_str);
  queue_cmd_data(channel,X_COMM,ans_len,M_CMDSCRIPT,ans_str);
}

static void rem_info(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1,x_par2;
  char ans_str[160];
  int ans_len;

  strcpy(ans_str,rem_info_file);  
  ans_len = strlen(ans_str);
  x_par1 = TX_NORM;
  x_par2 = 1;
  open_sendfile(x_par1,x_par2,channel,ans_len,mode,ans_str);
}

static void rem_help(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1,x_par2;
  char ans_str[160];
  int ans_len;

  strcpy(ans_str,rem_help_file);  
  ans_len = strlen(ans_str);
  x_par1 = TX_NORM;
  x_par2 = 1;
  open_sendfile(x_par1,x_par2,channel,ans_len,mode,ans_str);
}

static void rem_news(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1,x_par2;
  char ans_str[160];
  int ans_len;

  strcpy(ans_str,news_file_name);  
  ans_len = strlen(ans_str);
  x_par1 = TX_NORM;
  x_par2 = 1;
  open_sendfile(x_par1,x_par2,channel,ans_len,mode,ans_str);
}

void rem_dir(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  int ans_len;
  char string[80];
  char tmpname[20];
  int result;
  int uid,gid;

#ifdef HAVE_INDEX
  if (index(str,'/') != NULL) {
#else
  if (strchr(str,'/') != NULL) {
#endif
    cmd_display(mode,channel,rem_nodir_str,1);
    return;
  }
  strcpy(tmpname,"/tmp/tntdirXXXXXX");
  mkstemp(tmpname);
  if (par2)
    strcpy(string,DIRRUN_STRING);
  else if (par1)   
    strcpy(string,DIRL_STRING);
  else
    strcpy(string,DIR_STRING);
  if (par2)
    strcat(string,run_dir);
  else
    strcat(string,remote_dir);
  strcat(string,str);
  strcat(string," >");
  strcat(string,tmpname);
  strcat(string," 2>/dev/null");
  drop_priv(mode,channel,&uid,&gid);
  result = system(string);
  rest_priv(mode,channel,uid,gid);
  if (!result) {
    ans_len = strlen(tmpname);
    x_par1 = TX_NORM;
    open_sendfile(x_par1,par2,channel,ans_len,M_REMOTE_TEMP,tmpname);
  }
  else {
    unlink(tmpname);
    cmd_display(mode,channel,rem_nodir_str,1);
  }
}

static void rem_free(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  int ans_len;
  char string[80];
  char tmpname[20];
  int result;

  strcpy(tmpname,"/tmp/tntfreXXXXXX");
  mkstemp(tmpname);
  strcpy(string,FREE_STRING);
  strcat(string,tmpname);
  result = system(string);
  if (!result) {
    ans_len = strlen(tmpname);
    x_par1 = TX_NORM;
    open_sendfile(x_par1,par2,channel,ans_len,M_REMOTE_TEMP,tmpname);
  }
  else {
    unlink(tmpname);
    cmd_display(mode,channel,rem_error_str,1);
  }
}

/* send one cookie */
/* if par1 != 0: header and footer output */
void rem_cookie(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  int ans_len;
  char tmpname[20];
  int result;
  
  strcpy(tmpname,"/tmp/tntcooXXXXXX");
  mkstemp(tmpname);
  result = gen_cookie(tmpname,par1);
  if (!result) {
    ans_len = strlen(tmpname);
    x_par1 = TX_NORM;
    open_sendfile(x_par1,par2,channel,ans_len,M_REMOTE_TEMP,tmpname);
  }
  else {
    unlink(tmpname);
    if (result == 1)
      cmd_display(mode,channel,rem_nocook_str,1);
    else
      cmd_display(mode,channel,rem_error_str,1);
  }
}

/* put callsign of remote station on channel to call */
/* call of station to which physically connected (AX25) */
void strip_call(call,channel)
char *call;
int channel;
{
  int i;
  int offset;
  char *ptr;
   
  offset = 0;
  ptr = strchr(ch_stat[channel].call,':');
  if (ptr != NULL) {
    offset = ptr + 1 - ch_stat[channel].call;
  } 
  for (i = 0; i < 9; i++) {
    call[i] = ch_stat[channel].call[i+offset];
    if (call[i] == '\0') {
      break;
    }
    if (call[i] == ' ') {
      call[i] = '\0';
      break;
    } 
  }
  if (i == 9) call[i] = '\0';
}

/* put callsign of remote station on channel to */
/* call of station to which logically connected (DIGIPEATER) */
void strip_call_log(call,channel)
char *call;
int channel;
{
  int i;
  int offset;
  char *colon;

  if (ch_stat[channel].disp_call[0] == '\0') {
    strip_call(call,channel);
    return;
  }   
  offset = 0;
  colon = strchr(ch_stat[channel].disp_call,':');
  if (colon != NULL) {
    offset = colon + 1 - ch_stat[channel].disp_call; 
  }  
  for (i = 0; i < 9; i++) {
    call[i] = toupper(ch_stat[channel].disp_call[i+offset]);
    if (call[i] == '\0') {
      break;
    }
    if (call[i] == ' ') {
      call[i] = '\0';
      break;
    } 
  }
  if (i == 9) call[i] = '\0';
}

/* put callsign of remote station on channel to */
/* call of station to which logically connected */
/* no deleting of ssid and digipeater path */
void get_call_log(call,channel)
char *call;
int channel;
{
  int i;
  int offset;
  char *colon;

  if (ch_stat[channel].disp_call[0] == '\0') {
    offset = 0; 
    colon = strchr(ch_stat[channel].call,':');
    if (colon != NULL) {
      offset = colon + 1 - ch_stat[channel].call;
    } 
    strcpy(call,&ch_stat[channel].call[offset]);
    return;
  }   
  offset = 0;
  colon = strchr(ch_stat[channel].disp_call,':');
  if (colon != NULL) {
    offset = colon + 1 - ch_stat[channel].disp_call; 
  }  
  for (i = 0; i < 9; i++) {
    call[i] = toupper(ch_stat[channel].disp_call[i+offset]);
    if (call[i] == '\0') {
      break;
    }
    if (call[i] == ' ') {
      call[i] = '\0';
      break;
    } 
  }
  if (i == 9) call[i] = '\0';
}

/* send the ctext */
void rem_ctext(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  int ans_len;
  char string[160];
  char tmpname[20];
  int result;
  int fd1;
  int fd2;
  char call[10];
  char call2[10];
  int i;
  int end;

  strcpy(tmpname,"/tmp/tntctxXXXXXX");
  mkstemp(tmpname);

  strip_call(call,channel);
  i = 0;
  end = 0;
  do {
    if ((call2[i] = tolower(call[i])) == '-') {
      call2[i] = '\0';
      end = 1;
    }
    i++;
  } while ((i <= 9) && (!end));

  strcpy(string,ctext_dir);
  strcat(string,call2);
  strcat(string,".ctx");
  fd1 = open(string,O_RDONLY);
  if (fd1 < 0) {
    strcpy(string,tnt_ctextfile);
    fd1 = open(string,O_RDONLY);
    if(fd1 < 0) return;
  }

  fd2 = open(tmpname,O_RDWR|O_CREAT,PMODE);
  if(fd2 < 0) { close(fd1); return; }

  result = replace_macros(fd1,fd2,call,
                          ch_stat[channel].curcall,channel,
                          ch_stat[channel].start_time);

  close(fd1);
  close(fd2);

  if (result > 0) {
    ans_len = strlen(tmpname);
    x_par1 = TX_NORM;
    open_sendfile(x_par1,par2,channel,ans_len,M_REMOTE_TEMP,tmpname);
  }
  else {
    unlink(tmpname);
    if (result == 1)
      cmd_display(mode,channel,rem_nocook_str,1);
    else
      cmd_display(mode,channel,rem_error_str,1);
  }
}

/* update names file */
void rem_name(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char call[10];
  char tempstr[250];
  char namestr[90];

  if (ch_stat[channel].conn_state != CS_CONN) {
    cmd_display(mode,channel,"Only while connected",1);
    return;
  }
  strip_call_log(call,channel);
  if (len == 0) { /* no name specified */
    if (macro_getname(call,namestr) == TNT_OK) {
      cmd_display(mode,channel,namestr,1);
      return;
    }
    else {
      cmd_display(mode,channel,"No access to names database",1);
      return;
    }
  }
  /* test if entry in namesfile */
  if (find_line_for_call(call,namestr) == LINE_FOUND) {
    if (delete_line_for_call(call) > 0) {
      cmd_display(mode,channel,rem_name_err,1);
      return;
    }
  }
  if (add_line_for_call(call,str) > 0) {
    cmd_display(mode,channel,rem_name_err,1);
    return;
  }
  if (mode == M_REMOTE) {
    strcpy(tempstr,rem_name_str1);
    strcat(tempstr,str);
    strcat(tempstr,rem_name_str2);
    cmd_display(mode,channel,tempstr,1);
  }
  else {
    cmd_display(mode,channel,OK_TEXT,1);
  }
}

/* send a macro-textfile */
void cmd_msend(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  int ans_len;
  char string[160];
  char tmpname[20];
  int result;
  int fd1;
  int fd2;
  char call[10];

  strcpy(tmpname,"/tmp/tntmtxXXXXXX");
  mkstemp(tmpname);

#ifdef HAVE_INDEX
  if (index(str,'/') != NULL) {
#else
  if (strchr(str,'/') != NULL) {
#endif
    strcpy(string,str);
  }
  else {
    strcpy(string,macrotext_dir);
    strcat(string,str);
  }
  fd1 = open(string,O_RDONLY);
  if (fd1 < 0) {
    cmd_display(mode,channel,"Can't open file",1);
    return;
  }

  fd2 = open(tmpname,O_RDWR|O_CREAT,PMODE);
  if(fd2 < 0) { close(fd1); return; }

  strip_call_log(call,channel);
  result = replace_macros(fd1,fd2,call,
                          ch_stat[channel].curcall,channel,
                          ch_stat[channel].start_time);

  close(fd1);
  close(fd2);

  if (result > 0) {
    ans_len = strlen(tmpname);
    x_par1 = TX_NORM;
    open_sendfile(x_par1,par2,channel,ans_len,M_REMOTE_TEMP,tmpname);
    cmd_display(mode,channel,OK_TEXT,1);
  }
  else {
    unlink(tmpname);
    if (result == 1)
      cmd_display(mode,channel,rem_nocook_str,1);
    else
      cmd_display(mode,channel,rem_error_str,1);
  }
}

/* huffman en/decoding */
void cmd_comp(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char buffer[20];
  int i;

  if ((file_paclen > 255) || (!tnt_comp)){
    cmd_display(mode,channel,comp_disable_txt,1);
    return;
  }  
  if (len) {
    for (i=0;i<len;i++)
      str[i] = toupper(str[i]);
  }
  switch (len) {
  case 1:
    if (mode != M_REMOTE) {
      break;
    }
    if (str[0] == '1') {
      ch_stat[channel].huffcod = 1;
      statlin_update();
#ifdef USE_IFACE
      send_huffstat(channel);
#endif      
      return;
    }
    if (str[0] == '0') {
      ch_stat[channel].huffcod = 0;
      statlin_update();
#ifdef USE_IFACE
      send_huffstat(channel);
#endif      
      return;
    }
    break;
  case 2:
    if (strcmp(str,"ON") == 0) {
      if (mode == M_REMOTE) {
        strcpy(buffer,"\r//COMP 1\r");
        rem_data_display(channel,buffer);
        queue_cmd_data(channel,X_DATA,strlen(buffer),0,buffer);
      }
      else {
        cmd_display(mode,channel,OK_TEXT,1);
      }
      ch_stat[channel].huffcod = 1;
      statlin_update();
#ifdef USE_IFACE
      send_huffstat(channel);
#endif      
      return;
    }
    break;
  case 3:
    if (strcmp(str,"OFF") == 0) {
      if (mode == M_REMOTE) {
        strcpy(buffer,"\r//COMP 0\r");
        rem_data_display(channel,buffer);
        queue_cmd_data(channel,X_DATA,strlen(buffer),0,buffer);
      }
      else {
        cmd_display(mode,channel,OK_TEXT,1);
      }
      ch_stat[channel].huffcod = 0;
      statlin_update();
#ifdef USE_IFACE
      send_huffstat(channel);
#endif      
      return;
    }
    break;
  }
  if (mode == M_REMOTE) {
    cmd_display(mode,channel,comp_usage_txt,1);
  }
  else {
    if (len == 0) {
      if (ch_stat[channel].huffcod == 0)
        cmd_display(mode,channel,"OFF",1);
      else
        cmd_display(mode,channel,"ON",1);
      return;
    }
    else {
      cmd_display(mode,channel,"INVALID VALUE",1);
    }
  }
}

/* display/change access level for remote commands */
void cmd_setacc(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int i;
  int j;
  int found;
  int res;
  char com_string[80];
  char tmpstr[80];
  
  res = sscanf(str,"%s %s",com_string,tmpstr);
  if (res) {
    /* find string in remote-command-list */
    for (i=0;i<strlen(com_string);i++)
      com_string[i] = toupper(com_string[i]);
    found = 0;
    j = 0;
    while ((remote_list[j].ext_com != -1) && !found) {
      if (strstr(remote_list[j].string,com_string)
          == remote_list[j].string) {
        /* string found, now check if length valid */
        if (strlen(com_string) >= remote_list[j].len) {
          found = 1;
        }
      }
      if (!found) j++;
    }
    if (found) {
      if (res == 2) {
        for (i=0;i<strlen(tmpstr);i++)
          tmpstr[i] = toupper(tmpstr[i]);
        if (strncmp("NORMAL",tmpstr,strlen(tmpstr)) == 0) {
          remote_list[j].access_level = 0;
          cmd_display(mode,channel,OK_TEXT,1);
          return;
        }
        if (strncmp("SYSOP",tmpstr,strlen(tmpstr)) == 0) {
          remote_list[j].access_level = 1;
          cmd_display(mode,channel,OK_TEXT,1);
          return;
        }
        if (strncmp("ROOT",tmpstr,strlen(tmpstr)) == 0) {
          remote_list[j].access_level = 2;
          cmd_display(mode,channel,OK_TEXT,1);
          return;
        }
        if (strncmp("ALWAYS",tmpstr,strlen(tmpstr)) == 0) {
          remote_list[j].access_level = 3;
          cmd_display(mode,channel,OK_TEXT,1);
          return;
        }
        if (strncmp("NEVER",tmpstr,strlen(tmpstr)) == 0) {
          remote_list[j].access_level = 4;
          cmd_display(mode,channel,OK_TEXT,1);
          return;
        }
      }
      else {
        switch (remote_list[j].access_level) {
        case 0:
          cmd_display(mode,channel,"NORMAL",1);
          break;
        case 1:
          cmd_display(mode,channel,"SYSOP",1);
          break;
        case 2:
          cmd_display(mode,channel,"ROOT",1);
          break;
        case 3:
          cmd_display(mode,channel,"ALWAYS",1);
          break;
        case 4:
          cmd_display(mode,channel,"NEVER",1);
          break;
        }
        return;
      }
    }
  }
  cmd_display(mode,channel,
       "Usage: SETACC <remote_command> [NORMAL/SYSOP/ROOT/ALWAYS/NEVER]",1);
}

int randint(my_range)
short my_range;
{
  int i,j;
  
  i = RAND_MAX / my_range;
  i *= my_range;
  while ((j = rand()) >= i) continue;
  return (j % i) % my_range;
}

short tnt_random(short low, short hiw)
{
  short erg;
  
  erg = randint(hiw - low + 1);
  return (erg + low);
}

/* password for sysop-permissions */
void cmd_sysop(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char ans_str[80];
  int ans_len;
  int i;
  int j;
  int flag;
  int ok;
  int nr;
  char hs[10];
  char call[10];
  int pwnr[5];
  char password[133];
  int found;
  struct sysoplist *sysoplist_wrk;
  FILE *fd;
  char *ptr;
  
  strip_call_log(call,channel);
  ptr = strchr(call,'-');
  if (ptr != NULL) *ptr = '\0';
  sysoplist_wrk = sysoplist_root;
  found = 0;
  while ((!found) && (sysoplist_wrk != NULL)) {
    if (strcmp(call,sysoplist_wrk->callsign) == 0) {
      found = 1;
      if((fd = fopen(sysoplist_wrk->sys_file,"r")) != NULL) {
        fgets(password,132,fd);
        fclose(fd);
        ptr = strchr(password,'\r');
        if (ptr != NULL) *ptr = '\0';
        ptr = strchr(password,'\n');
        if (ptr != NULL) *ptr = '\0';
      }
      else {
        found = 0;
        sysoplist_wrk = sysoplist_wrk->next;
      }
    }
    else {
      sysoplist_wrk = sysoplist_wrk->next;
    }
  }
  if (!found) {
    strcpy(ans_str,rem_tnt_str);
    strcat(ans_str,rem_inv_str);
    strcat(ans_str,rem_newlin_str);
    ans_len = strlen(ans_str);
    flag = 0;
    rem_data_display(channel,ans_str);
    queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
    return;
  }
  sprintf(ans_str,"TNT:%s>",ch_stat[channel].curcall);
  for (i=0;i<5;i++) {
    do {
      nr = tnt_random(1,strlen(password));
      ok = 1;
      if (i > 0) {
        for (j=0;j<i;j++) {
          if (pwnr[j] == nr)
            ok = 0;
        }
      }
    } while (!(ok && (password[nr-1] != ' ')));
    ch_stat[channel].pwstr[i] = password[nr-1];
    pwnr[i] = nr;
    sprintf(hs," %d",nr);
    strcat(ans_str,hs);
  }
  ch_stat[channel].pwstr[5] = '\0';
  ch_stat[channel].pwwait = MAXPWLINES;
  ch_stat[channel].rootlevel = sysoplist_wrk->access_level;
  flag = 0;
  strcat(ans_str,"\r");
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
}

/* reload password file */
void cmd_loadsys(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  clear_sysfile();  
  load_sysfile();
  cmd_display(mode,channel,OK_TEXT,1);
  return;
}

/* list password file data */
void cmd_listsys(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct sysoplist *sysoplist_wrk;
  char disp_line[83];
  
  if (sysoplist_root == NULL) {
    cmd_display(mode,channel,"No lines found",1);
    return;
  }
  sysoplist_wrk = sysoplist_root;
  while (sysoplist_wrk != NULL) {
    sprintf(disp_line,"%s %s %d",
            sysoplist_wrk->callsign, sysoplist_wrk->sys_file,
            sysoplist_wrk->access_level);
    cmd_display(mode,channel,disp_line,1);
    sysoplist_wrk = sysoplist_wrk->next;
  }
}

void test_sysresponse(int channel,char *buffer,int len)
{
  char string[257];
  char *ptr;
  
  memcpy(string,buffer,len);
  string[len] = '\0';
  ptr = strchr(string,'\r');
  if (ptr != NULL) *ptr = '\0';
  ptr = strchr(string,'\n');
  if (ptr != NULL) *ptr = '\0';
  ptr = strstr(string,ch_stat[channel].pwstr);
  if (ptr != NULL) {
    ch_stat[channel].access_level = 1 + ch_stat[channel].rootlevel;
    
  }
}

int rem_noacc(int channel)
{
  int flag;
  char ans_str[81];
  int ans_len;
  char call[10];
  struct sysoplist *sysoplist_wrk;
  int found;
  char *ptr;
  
  strip_call_log(call,channel);
  ptr = strchr(call,'-');
  if (ptr != NULL) *ptr = '\0';
  sysoplist_wrk = sysoplist_root;
  found = 0;
  while ((!found) && (sysoplist_wrk != NULL)) {
    if (strcmp(call,sysoplist_wrk->callsign) == 0) {
      found = 1;
    }
    else {
      sysoplist_wrk = sysoplist_wrk->next;
    }
  }
  if (found) return(1);

  flag = 0;
  strcpy(ans_str,rem_noacc_str);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
  end_comp(channel);
  strcpy(ans_str,"D");
  ans_len = strlen(ans_str);
  queue_cmd_data(channel,X_COMM,ans_len,M_CMDSCRIPT,ans_str);
  return(0);
}

static void rem_noiface(int channel,char *str)
{
  int flag;
  char ans_str[81];
  int ans_len;
  
  flag = 0;
  strcpy(ans_str,str);
  strcat(ans_str,rem_newlin_str);
  ans_len = strlen(ans_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,ans_len,flag,ans_str);
  end_comp(channel);
  strcpy(ans_str,"D");
  ans_len = strlen(ans_str);
  queue_cmd_data(channel,X_COMM,ans_len,M_CMDSCRIPT,ans_str);
}

void rem_nobox(int channel)
{
  rem_noiface(channel,rem_nobox_str);
}

void rem_nonode(int channel)
{
  rem_noiface(channel,rem_nonode_str);
}

/* execute tnt command from remote */
void cmd_command(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_input(channel,mode,str,len,0);
}

/* reload no remote file */
void cmd_ldnoremo(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  clear_noremfile();  
  load_noremfile();
  cmd_display(mode,channel,OK_TEXT,1);
  return;
}

/* list no remote file data */
void cmd_lstnorem(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct noremlist *noremlist_wrk;
  char disp_line[83];
  
  if (noremlist_root == NULL) {
    cmd_display(mode,channel,"No lines found",1);
    return;
  }
  noremlist_wrk = noremlist_root;
  while (noremlist_wrk != NULL) {
    sprintf(disp_line,"%s",noremlist_wrk->callsign);
    cmd_display(mode,channel,disp_line,1);
    noremlist_wrk = noremlist_wrk->next;
  }
}

void set_remmode(channel)
int channel;
{
  char callsign[10];
  struct noremlist *noremlist_wrk;
  int found;
  
  strip_call_log(callsign,channel);
  noremlist_wrk = noremlist_root;
  found = 0;
  while ((!found) && (noremlist_wrk != NULL)) {
    if (strcmp(callsign,noremlist_wrk->callsign) == 0) {
      found = 1;
      ch_stat[channel].remote = 0;
    }
    else {
      noremlist_wrk = noremlist_wrk->next;
    }
  }
}

/* reload flexnet-check file */
void cmd_ldflchk(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  clear_flchkfile();  
  load_flchkfile();
  cmd_display(mode,channel,OK_TEXT,1);
  return;
}

/* list flexnet-check file data */
void cmd_lstflchk(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct flchklist *flchklist_wrk;
  char disp_line[83];
  
  if (flchklist_root == NULL) {
    cmd_display(mode,channel,"No lines found",1);
    return;
  }
  flchklist_wrk = flchklist_root;
  while (flchklist_wrk != NULL) {
    sprintf(disp_line,"%s",flchklist_wrk->callsign);
    cmd_display(mode,channel,disp_line,1);
    flchklist_wrk = flchklist_wrk->next;
  }
}

void set_flchkmode(channel)
int channel;
{
  char callsign[10];
  struct flchklist *flchklist_wrk;
  int found;
  
  ch_stat[channel].flchkmode = 0;
  strip_call_log(callsign,channel);
  flchklist_wrk = flchklist_root;
  found = 0;
  while ((!found) && (flchklist_wrk != NULL)) {
    if (strcmp(callsign,flchklist_wrk->callsign) == 0) {
      found = 1;
      ch_stat[channel].flchkmode = 1;
    }
    else {
      flchklist_wrk = flchklist_wrk->next;
    }
  }
}

/* reload not-own-calls file */
void cmd_ldnotown(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  clear_notownfile();  
  load_notownfile();
  cmd_display(mode,channel,OK_TEXT,1);
  return;
}

/* list not-own-calls file data */
void cmd_lsnotown(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct notownlist *notownlist_wrk;
  char disp_line[83];
  
  if (notownlist_root == NULL) {
    cmd_display(mode,channel,"No lines found",1);
    return;
  }
  notownlist_wrk = notownlist_root;
  while (notownlist_wrk != NULL) {
    sprintf(disp_line,"%s",notownlist_wrk->callsign);
    cmd_display(mode,channel,disp_line,1);
    notownlist_wrk = notownlist_wrk->next;
  }
}

/* return 1 if call is not-own-call */
int is_notown(char *callsign)
{
  struct notownlist *notownlist_wrk;
  
  notownlist_wrk = notownlist_root;
  while (notownlist_wrk != NULL) {
    if (strcmp(callsign,notownlist_wrk->callsign) == 0) {
      return(1);
    }
    else {
      notownlist_wrk = notownlist_wrk->next;
    }
  }
  return(0);
}

/* reload autostart file */
void cmd_ldautostart(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  clear_autostartfile();  
  load_autostartfile();
  cmd_display(mode,channel,OK_TEXT,1);
  return;
}

/* list autostart data */
void cmd_lsautostart(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char disp_line[83];
  int found;
  struct autostartlist *autostartlist_wrk;
  
  found = 0;
  autostartlist_wrk = autostartlist_root;
  while (autostartlist_wrk != NULL) {
    found = 1;
    sprintf(disp_line,"%-9.9s %s",autostartlist_wrk->callsign,
                                  autostartlist_wrk->commstring);
    cmd_display(mode,channel,disp_line,1);
    autostartlist_wrk = autostartlist_wrk->next;
  } 
  if (!found) {
    cmd_display(mode,channel,"No lines found",1);
    return;
  }
}

/* start command for call, if any */
int do_autostart(char *callsign,int channel)
{
  struct autostartlist *autostartlist_wrk;
  char commstring[79];
  
  if (!autostart_flag) return(0);
  autostartlist_wrk = autostartlist_root;
  while (autostartlist_wrk != NULL) {
    if (strcmp(callsign,autostartlist_wrk->callsign) == 0) {
      strcpy(commstring,autostartlist_wrk->commstring);
      analyse_remstr(channel,commstring,1);
      return(1);
    }
    autostartlist_wrk = autostartlist_wrk->next;
  }
  return(0);
}

/* reload extended remote commands file */
void cmd_ldextrem(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  clear_extremotefile();  
  load_extremotefile();
  cmd_display(mode,channel,OK_TEXT,1);
  return;
}

/* list extended remote commands file data */
void cmd_lsextrem(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  struct extremotelist *extremotelist_wrk;
  char disp_line[83];
  char level[10];
  char commstring[9];
  int i;
  char ch;
  int end;
  
  if (extremotelist_root == NULL) {
    cmd_display(mode,channel,"No lines found",1);
    return;
  }
  strcpy(disp_line,"Alias    Level  Command");
  cmd_display(mode,channel,disp_line,1);
  strcpy(disp_line,"----------------------------------------"
                   "---------------------------------------");
  cmd_display(mode,channel,disp_line,1);
  extremotelist_wrk = extremotelist_root;
  while (extremotelist_wrk != NULL) {
    switch (extremotelist_wrk->level) {
    case 0:
      strcpy(level,"NORMAL");
      break;
    case 1:
      strcpy(level,"SYSOP ");
      break;
    case 2:
      strcpy(level,"ROOT  ");
      break;
    case 3:
      strcpy(level,"ALWAYS");
      break;
    case 4:
      strcpy(level,"NEVER ");
      break;
    }
    
    end = 0;
    for (i = 0;i < 8;i++) {
      ch = extremotelist_wrk->commstring[i];
      if ((ch == '\0') || end) {
        ch = ' ';
        end = 1;
      }
      if (i >= extremotelist_wrk->minlen) {
        commstring[i] = tolower(ch);
      }
      else {
        commstring[i] = ch;
      }
    }
    commstring[8] = '\0';
    
    sprintf(disp_line,"%s %s %s",commstring,level,extremotelist_wrk->command);
    cmd_display(mode,channel,disp_line,1);
    extremotelist_wrk = extremotelist_wrk->next;
  }
}

/* show channelstatus */
void cmd_chanstat(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int x_par1;
  int ans_len;
  char tmpname[20];
  int result;
  int no_connection;
  char destcall[10];
  int i;
  FILE *fp;
  struct tm cvtime;
  char hs[256];

  result = 1;
  fp = NULL;
  if (mode == M_REMOTE) {
    strcpy(tmpname,"/tmp/tntchsXXXXXX");
    mkstemp(tmpname);
    fp = fopen(tmpname,"w");
  }
  if ((fp != NULL) || (mode != M_REMOTE)) {
    result = 0;
    no_connection = 1;
    for (i=1;i<tnc_channels;i++) {
      if (ch_stat[i].conn_state == CS_CONN) {
        if (no_connection) {
          if (mode == M_REMOTE) {
            strcpy(hs,chs_header);
            strcat(hs,"\n");
            fputs(hs,fp);
            strcpy(hs,chs_dash);
            strcat(hs,"\n");
            fputs(hs,fp);
          }
          else {
            cmd_display(mode,channel,chs_header,1);
            cmd_display(mode,channel,chs_dash,1);
          }
          no_connection = 0;
        }
        strip_call_log(destcall,i);
        cvtime = *localtime(&ch_stat[i].start_time);
        if (mode == M_REMOTE) {
          fprintf(fp,
            "  %2.2d      %-9.9s > %-9.9s   %2.2u.%2.2u.%2.2u %2.2u:%2.2u\n",
            i,ch_stat[i].curcall,destcall,
            cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
            cvtime.tm_hour,cvtime.tm_min);
        }
        else {
          sprintf(hs,
            "  %2.2d      %-9.9s > %-9.9s   %2.2u.%2.2u.%2.2u %2.2u:%2.2u",
            i,ch_stat[i].curcall,destcall,
            cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
            cvtime.tm_hour,cvtime.tm_min);
          cmd_display(mode,channel,hs,1);
        }
      }
    }
    if (no_connection) {
      result = 1;
    }
    else {
      if (mode == M_REMOTE)
        fprintf(fp,"\n");
      else {
        *hs = '\0';
        cmd_display(mode,channel,hs,1);
      }
    }
    if (mode == M_REMOTE) fclose(fp);
  }
  if (!result) {
    if (mode == M_REMOTE) {
      ans_len = strlen(tmpname);
      x_par1 = TX_NORM;
      open_sendfile(x_par1,par2,channel,ans_len,M_REMOTE_TEMP,tmpname);
    }
  }
  else {
    if (mode == M_REMOTE) {
      unlink(tmpname); 
      cmd_display(mode,channel,rem_error_str,1);
    }
    else {
      cmd_display(mode,channel,no_act_conn,1);
    }
  }
}

/* Session-Handling by WS1LS, WSP */
void init_session()
{
  FILE *f1;

  tnt_startup = time(&tnt_startup); /* Zeit des TNT-Starts */
  session_sek = 0;

  if((f1=fopen(tnt_session_log,"rb"))!=NULL) {
    fscanf(f1,"%d",&session_sek);
    fclose(f1);
  }
}

void exit_session()
{
  FILE *f1;
  time_t tnt_shutdown;

  tnt_shutdown = time(&tnt_shutdown); /* Ende der TNT-Session */
  session_sek = session_sek + (tnt_shutdown - tnt_startup);

  if((f1=fopen(tnt_session_log,"wb"))!=NULL) {
    fprintf(f1,"%d\n",session_sek); /* Gesamte Session speichern */
    fclose(f1);
  }
}

void queue_tellmsg(char *tmp_msg) /* Msg auf alle Kanaele ausgeben */
{
  unsigned char i,j;
  char tell_msg[200];
  char tmp_str[20];
  struct tm *timestr;
  time_t timeval;

  for(i=1; i<tnc_channels; i++) {
    strip_call_log(tmp_str,i);
    if((ch_stat[i].conn_state == CS_CONN) && (tx_file[i].type == -1) &&
      (rx_file[i].type == -1) && (check_call_type(tmp_str)==0)) {
      if(! xconnect_active(i)) { /* Nicht den Connectaufbau stoeren */
        tell_msg[0]='\0';
             /* Eigene Makro-Konvertierung:
                %n Name, %t Zeit, %c Call, %g/%b Bell, %k Kanal */
        for(j=0; j<strlen(tmp_msg); j++) {
          if(tmp_msg[j]=='%') {
            switch(tmp_msg[j+1]) {
              case 'n' : /* Name */
              case 'N' :
                strcat(tell_msg,ch_stat[i].name);
                break;
              case 'c' : /* Call */
              case 'C' :
                strip_call_log(tmp_str,i);
                strcat(tell_msg,tmp_str);
                break;
              case 'g' : /* Klingel */
              case 'G' :
              case 'b' :
              case 'B' :
                sprintf(tmp_str,"%c",0x07);
                strcat(tell_msg,tmp_str);
                break;
              case 't' : /* Zeit */
              case 'T' :
                  timeval = time(&timeval);
                  timestr = localtime(&timeval);
                  sprintf(tmp_str,"%2.2u:%2.2u:%2.2u",
                          timestr->tm_hour,timestr->tm_min,timestr->tm_sec);
                  strcat(tell_msg,tmp_str);
                  break;
              case 'k' : /* Kanal */
              case 'K' :
                  sprintf(tmp_str,"%d",i);
                  strcat(tell_msg,tmp_str);
                  break;
              default :
                  sprintf(tmp_str,"%%%c",tmp_msg[j+1]);
                  strcat(tell_msg,tmp_str);
              }
            j++;
            } else {
              sprintf(tmp_str,"%c",tmp_msg[j]);
              strcat(tell_msg,tmp_str); }
          }
        tell_msg[78]='\0';
        strcat(tell_msg, rem_newlin_str);

        rem_data_display(i, tell_msg);
        queue_cmd_data(i,X_DATA,strlen(tell_msg),0,tell_msg);
      }
    }
  }
}

void cmd_tellmsg(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if(strlen(str)==0) {
    cmd_display(mode,channel,"missing message",1);
  } else {
    queue_tellmsg(str); }
}

int queue_tellinfo(text,channel,mode) /* Ausgabe eines bestimmten Textes */
int text;
int channel;
int mode;
{
  FILE *tellfile;
  char ans1_str[80];
  char ans2_str[80];
  char tmp_str[85];
  int tmp_nr;
  char tmp_valid;
  char ans0_str[80];
  char i,j;

  if((tellfile=fopen(tnt_telltext_file,"r"))==NULL) return(1);
  tmp_valid=0;
  while(fgets(tmp_str,85,tellfile)!=NULL) {
    if((tmp_str[0]!='#') && (tmp_str[0]!='\0') && (tmp_str[0]!='\n')) {
      if(sscanf(tmp_str,"%d %s", &tmp_nr, &ans0_str) != 2) {
        fclose(tellfile);
        return(1); }
      for(i=0; tmp_str[i]!=' '; i++);
      i++;
      for(j=0; ((tmp_str[i]!='\0') && (tmp_str[i]!='\n')); i++) {
        ans0_str[j]=tmp_str[i];
        j++; }
    ans0_str[j]='\0';
    if(tmp_nr == text) {
      strcpy(ans1_str,ans0_str);
      tmp_valid = tmp_valid | 1; }
    if(tmp_nr == text+1) {
      strcpy(ans2_str,ans0_str);
      tmp_valid = tmp_valid | 2; }
    }
  }
  fclose(tellfile);
  if(tmp_valid != 3) return(1);
  if(strcmp(ans1_str,"0")!=0) queue_tellmsg(ans1_str);
  if(strcmp(ans2_str,"0")!=0) strcpy(tnt_info_message,ans2_str);
  if(strcmp(ans2_str,"1")==0) tnt_info_message[0]='\0';
  if(strcmp(ans2_str,"0")!=0) {
    strcpy(ans1_str,"<OK>");
    if(strcmp(ans2_str,"1")!=0) {
      strcat(ans1_str,":");
      strcat(ans1_str,ans2_str);
      ans2_str[78]='\0';
    }
    cmd_display(mode,channel,ans1_str,1);
  }
  return(0);
}

void rem_tell_da(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if(queue_tellinfo(1,channel,mode) ==1)
     cmd_display(mode,channel,COR_TELL_TEXT,1);
}

void rem_tell_weg(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if(queue_tellinfo(3,channel,mode) ==1)
    cmd_display(mode,channel,COR_TELL_TEXT,1);
}

void rem_tell_600(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if(queue_tellinfo(5,channel,mode) ==1)
    cmd_display(mode,channel,COR_TELL_TEXT,1);
}

void rem_tell_klo(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if(queue_tellinfo(7,channel,mode) ==1)
    cmd_display(mode,channel,COR_TELL_TEXT,1);
}

void rem_tell_gnd(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  if(queue_tellinfo(9,channel,mode) ==1)
    cmd_display(mode,channel,COR_TELL_TEXT,1);
}

void rem_onactiv(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char ans_str[80];

  if(strlen(str)==0) {
    strcpy(ans_str,"<TNT>: ONActivitytext disabled");
    ch_stat[channel].queue_act=0;
  } else {
    ch_stat[channel].queue_act=1;
    queue_activ=1;
    strcpy(ch_stat[channel].onact_text,str);
    strcpy(ans_str,"<TNT>: ONActivitytext enabled");
  }
  strcat(ans_str,rem_newlin_str);
  rem_data_display(channel,ans_str);
  queue_cmd_data(channel,X_DATA,strlen(ans_str),0,ans_str);
}

void rem_sendonact()
{
  int i;
  char ans_str[80];

  queue_activ=0;

  for(i=1; i<tnc_channels; i++) {
    if(ch_stat[i].queue_act!=0) {
      if((tx_file[i].type!=-1) || (rx_file[i].type!=-1)) queue_activ=1;
      else {
        strcpy(ans_str,ch_stat[i].onact_text);
        ans_str[78]='\0';
        strcat(ans_str,rem_newlin_str);
        ch_stat[i].queue_act=0;
        rem_data_display(i,ans_str);
        queue_cmd_data(i,X_DATA,strlen(ans_str),0,ans_str);
      }
    }
  }
}

void rem_chat(par1,par2,channel,len,mode,str) /* Chatten */
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int port;
  char rem_port_info[10];
  char chat_text[80];
  int i,j;
  char ans_str[130];
  char from_call[7];
  char to_call[7];

  strcpy(chat_text,"<TNT>: use //CH <port/call> <text>");
  strcat(chat_text,rem_newlin_str);
  if((strlen(str) == 0) || (sscanf(str,"%8s %8s",&port,&port)!=2)) {
    rem_data_display(channel,chat_text);
    queue_cmd_data(channel,X_DATA,strlen(chat_text),0,chat_text);
    return;
  }
   
  strip_call_log(ans_str,channel);
  for(i=0; i<6; i++) {
    if(ans_str[i]=='-') { from_call[i]='\0'; break; }
    from_call[i]=toupper(ans_str[i]);
  }
  from_call[6]='\0';

  for(i=0; i<strlen(str); i++) {
    if(str[i]==' ') { rem_port_info[i]='\0'; break; }
    rem_port_info[i]=toupper(str[i]);
    if(i==6) {
      rem_data_display(channel,chat_text);
      queue_cmd_data(channel,X_DATA,strlen(chat_text),0,chat_text);
      return;
    }
  }

  i++;
  rem_port_info[i]='\0';
  for(j=0; i<strlen(str); i++) {
    chat_text[j]=str[i];
    j++;
  }
  chat_text[j]='\0';

  sscanf(rem_port_info, "%d", &port);

  if((rem_port_info[0] < '0') || (rem_port_info[0] > '9')) {
    port = -1;
    for(i=1; i<tnc_channels; i++) {
      strip_call_log(ans_str,i);
      for(j=0; j<6; j++) {
      if(ans_str[j]=='-') { to_call[j]='\0'; break; }
      to_call[j]=toupper(ans_str[j]);
      }
      to_call[6]='\0';
      if(strcmp(rem_port_info,to_call)==0) { port=i; break; }
    }
    if(port == -1) {
      sprintf(chat_text,"<TNT>:station %s not connected!",rem_port_info);
      strcat(chat_text,rem_newlin_str);
      rem_data_display(channel,chat_text);
      queue_cmd_data(channel,X_DATA,strlen(chat_text),0,chat_text);
      return;
    }
  }

  if((port<1) || (port>=tnc_channels)) {
    sprintf(chat_text,"<TNT>:illegal channel %d",port);
    strcat(chat_text,rem_newlin_str);
    rem_data_display(channel,chat_text);
    queue_cmd_data(channel,X_DATA,strlen(chat_text),0,chat_text);
    return;
  }

  if(ch_stat[port].conn_state != CS_CONN) {
    sprintf(chat_text,"<TNT>:channel %d not connected",port);
    strcat(chat_text,rem_newlin_str);
    rem_data_display(channel,chat_text);
    queue_cmd_data(channel,X_DATA,strlen(chat_text),0,chat_text);
    return;
  }

  if((tx_file[port].type != -1) || (rx_file[port].type != -1) ||
    (xconnect_active(port)) || (check_call_type(to_call))) {
    sprintf(chat_text,"<TNT>:chat on channel %d not allowed",port);
    strcat(chat_text,rem_newlin_str);
    rem_data_display(channel,chat_text);
    queue_cmd_data(channel,X_DATA,strlen(chat_text),0,chat_text);
    return;
  }

  sprintf(ans_str,"<Msg de %d:%s, reply with \"//CH %d ...\">",channel,
          from_call,channel);
  strcat(ans_str,rem_newlin_str);
  strcat(ans_str,chat_text);
  strcat(ans_str,rem_newlin_str);
  rem_data_display(port,ans_str);
  queue_cmd_data(port,X_DATA,strlen(ans_str),0,ans_str);
}

int check_call_type(char *call)
{
  char tmpstr[100];
  FILE *r_file;
  char i,found;

  if((r_file=fopen(route_file_name,"rb"))==NULL) return(0);
  while(fgets(tmpstr,99,r_file)!=NULL) {
    found=1;
    for(i=0; i<6; i++) {
      if(tmpstr[i+2]=='-') break;
      if(toupper(tmpstr[i+2])!=toupper(call[i])) { found=0; break; }
      }
    if(found==1) {
      fclose(r_file);
      if(toupper(tmpstr[0])=='T') return(0);
      return(1); }
    }
  fclose(r_file);
  return(0);
}

#ifdef TNT_SOUND
/* read soundfile, 0=ok, 1=error */
int read_sound(void)
{
  FILE *audio_file;
  struct stat st;
  char tmp_str[150];
  int i,j;

  if(stat(tnt_sound_file, &st) == -1) return(1);
  if(sound_mod == st.st_mtime)        return(0);

  sound_mod  = st.st_mtime;
  audio_file = fopen(tnt_sound_file, "rb");
  if(audio_file == NULL)              return(1);

  while((fgets(tmp_str, 148, audio_file)) != NULL) {
    if((tmp_str[0] != '\0') && (tmp_str[0] != '\n') && (tmp_str[0] != '#')) {
      i = sscanf(tmp_str, "%d %148s", &j, &tmp_str);
      if((i == 2) && (j >= 0) && (j <= SOUND_MAX)) {
        if(tmp_str[0] != '/') {
          strcpy(sound_data[j], sound_dir);
          strcat(sound_data[j], tmp_str);
        } else {
          strcpy(sound_data[j], tmp_str);
        }
      }
    }
  }
  fclose(audio_file);
  return(0);
}

/* play Sound */
int play_sound(int sound_nr)
{
  if(soundon_flag == 0)              return 1;  /* Sound disabled   */
  if(read_sound() == 1)              return 1;  /* Soundfile defect */
  if(sound_data[0][0] == 0)          return 1;  /* no soundhandler  */
  if(sound_data[sound_nr][0] == 0)   return 1;  /* no soundfile     */
  if(sound_data[sound_nr][0] == '0') return 1;  /* should not play  */

  if(access(sound_data[0],  X_OK) != 0)       return 1;
  if(access(sound_data[sound_nr], R_OK) != 0) return 1;
  if(fork() ==0) {
    execlp(sound_data[0], sound_data[0], sound_data[sound_nr], NULL);
    exit(1);
  }
  return 0;
}
#endif

void rem_heardlist(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  char tmpname[20];
  char tmpname2[20];
  int fd1;
  int x_par1;
  FILE *tmp_mh;
  char tmp_str[100];
  int i,j;
  int cou;

  strcpy(tmpname,"/tmp/tntmhlXXXXXX");
  mkstemp(tmpname);

  fd1 = open(tmpname,O_RDWR|O_CREAT,PMODE);
  if(fd1 < 0) return;

  /* sel_heardlist in xmon.c */
  cou = sel_remhlist(fd1, strlen(str), rem_newlin_str);
  close(fd1);

  if(strlen(str) > 0) {
    tmp_mh = fopen(tmpname, "rw");
    strcpy(tmpname2, "/tmp/tntmhxXXXXXXX");
    mkstemp(tmpname2);
    fd1 = open(tmpname2,O_RDWR|O_CREAT,PMODE);
    if( (fd1 < 0) || (tmp_mh == NULL) ) return;

    for(i=0; i<2; i++) {
      fgets(tmp_str, 98, tmp_mh);
      write(fd1, tmp_str, strlen(tmp_str));
    }

    i = sscanf(str, "%d", &j);
    if( (i==1) && (j>0) && (j<100) ) {
      for(i=0; i<j; i++) {
        if(fgets(tmp_str, 98, tmp_mh)==NULL) break;
        write(fd1, tmp_str, strlen(tmp_str));
      }
      sprintf(tmp_str,"[%d von %d Eintraegen gelistet]", i, cou);
      if(i==1) sprintf(tmp_str,"[1 Eintrag (von %d) gelistet]", cou);
      if(i == cou) strcpy(tmp_str, "[Alle Eintraege gelistet]");
      strcat(tmp_str, rem_newlin_str);
      write(fd1, tmp_str, strlen(tmp_str));
    } else {
      for(i=0;i<strlen(str);i++) str[i]=toupper(str[i]);

      i=0;
      while(fgets(tmp_str, 98, tmp_mh) != NULL) {
        if(strstr(tmp_str, str) != NULL) {
          write(fd1, tmp_str, strlen(tmp_str));
          i++;
        }
      }
      if(i==0) {
        sprintf(tmp_str, "[<%s> nicht gefunden]%s",str, rem_newlin_str);
        write(fd1, tmp_str, strlen(tmp_str));
      } else {
        sprintf(tmp_str, "[%d Eintraege mit <%s>]",i,str);
        if(i==1) sprintf(tmp_str, "[1 Eintrag mit <%s>]",str);
        strcat(tmp_str, rem_newlin_str);
        write(fd1, tmp_str, strlen(tmp_str));
      }
    }
    write(fd1, rem_newlin_str, strlen(rem_newlin_str));
    close(fd1);
    fclose(tmp_mh);
    unlink(tmpname);
    strcpy(tmpname, tmpname2);
  }

  x_par1 = TX_NORM;
  open_sendfile(x_par1,par2,channel,strlen(tmpname),M_REMOTE_TEMP,tmpname);
}
