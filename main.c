/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Main procedure (main.c)
   created: Mark Wahl DL4YBG 93/08/07
   updated: Mark Wahl DL4YBG 97/02/02

   08.01.00 hb9xar	struct fd_set changed to fd_set (is a typedef'd struct)

*/
#include "tnt.h"
#define TNT_DATE "97/02/02"
#ifdef BCAST
#define TNT_VERSION "V1.0a1(BC)"
#else
#define TNT_VERSION "V1.0a1"
#endif

#ifdef USE_IFACE
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#endif

extern int serial;
extern char device[];
extern int soft_tnc;
extern unsigned int speed;
extern int speedflag;
extern char tnt_logfile[];
extern char tnt_dir[];
extern char frontend_socket[];
extern int frontend_active;
extern int frontend_fd;
extern int frontend_sockfd;
extern int tnt_daemon;
extern int use_select;
extern int softtnc_error;
extern struct staterr staterr;

extern void free_boxlist();
extern void free_connect();
extern void free_display();
extern void free_file();
extern void free_iface();
extern void free_monbox();
extern void free_priv();
extern void free_serial();
extern void free_shell();
extern int alloc_boxlist();
extern int alloc_connect();
extern int alloc_display();
extern int alloc_file();
extern int alloc_iface();
extern int alloc_monbox();
extern int alloc_priv();
extern int alloc_serial();
extern int alloc_shell();
extern int read_init_file(int argc,char *argv[],int *unlock);
extern int term_init(char *extterm,int extlines,int excols);
extern void set_linescols();
extern int init_serial(char *serstr,unsigned int speed,
                       int speedflag,int unlock);
extern int init_proc();
extern void term_exit();
extern int exit_serial();
extern void init_file();
extern int init_screen();
extern void init_keyboard();
extern void init_help();
extern void init_priv();
extern void init_xconnect();
extern void init_mheard();
extern void init_xmon();
extern void init_conv();
extern void init_remote();
extern void init_cookie();
extern void init_crc();
extern void init_shell();
#ifdef USE_IFACE
extern void init_iface();
extern void init_mb();
#endif
extern void init_blist();
extern void init_log();
#ifdef BCAST
extern void init_bcast();
#endif
extern void open_logfile(int type,int flag,int channel,
                         int len,int mode,char *str);
extern int serial_server(int *state,char *buffer,int len);
extern void exit_tnt(int par1,int par2,int channel,
                     int len,int mode, char *str);
extern void iface_trying();
extern void flush_frontend();
extern void check_xmon_timeout();
extern void check_xconnect_timeout();
extern void yapp_timeout();
#ifdef BCAST
extern void bc_timing();
#endif
extern void shell_fdset(int *maxfd, fd_set *fdmask);
extern int shell_receive(fd_set *fdmask);
#ifdef USE_IFACE
extern void iface_fdset(int *maxfd, fd_set *fdmask);
extern int iface_receive(fd_set *fdmask);
#endif
extern void keyboard_server(int *state,char *ch);
extern void resync_tnc(int *state);
#ifdef BCAST
extern void exit_bcast();
#endif
extern void exit_log();
extern void exit_blist();
#ifdef USE_IFACE
extern void exit_mb();
extern void exit_iface();
#endif
extern void exit_shell();
extern void exit_file();
extern void exit_priv();
extern void exit_remote();
extern void exit_xmon();
extern void exit_mheard();
extern void exit_xconnect();
extern void exit_help();
extern void exit_keyboard();
extern void exit_screen();
extern void exit_proc();
extern void frontend_exit(int deact);
extern void uwait(unsigned long time);
#ifdef USE_IFACE
struct sockaddr *build_sockaddr(const char *name, int *addrlen);
extern void init_fr_buffer();
extern void reinit_screen();
#endif

int act_channel;
int act_mode;
int act_state;
int is_root;
int terminated;

char frontend_para[80];

char signon[] =
"TNT "TNT_VERSION" Packet Radio Hostmode Terminal\015"
#ifdef TNT_LINUX
"LINUX"
#endif
#ifdef TNT_ISC
"ISC"
#endif
"-Version, Created: "TNT_DATE"\015"
"Copyright (C) 1993-97 by Mark Wahl, DL4YBG\015"
"For license details see documentation.\015"
"To get help: <ALT> H or <Esc> H\015";

char add_signon[] =
"No root permissions: SHELL command disabled\015";

char rem_ver_str[] =
"TNT/"
#ifdef TNT_LINUX
"LINUX"
#endif
#ifdef TNT_ISC
"ISC"
#endif
" "TNT_VERSION" ("TNT_DATE", DL4YBG)";

void free_all()
{
  free_boxlist();
  free_connect();
  free_display();
  free_file();
  free_iface();
  free_monbox();
  free_priv();
  free_serial();
  free_shell();
}

int alloc_all()
{
  int i;

  i = alloc_boxlist();
  i += alloc_connect();
  i += alloc_display();
  i += alloc_file();
  i += alloc_iface();
  i += alloc_monbox();
  i += alloc_priv();
  i += alloc_serial();
  i += alloc_shell();
  return (i>0);
}

static void sigterm()
{
  terminated = 1;
  signal(SIGTERM, SIG_IGN);
}

static void sighup()
{
  terminated = 1;
  signal(SIGHUP, SIG_IGN);
}

int 
main(argc,argv)
int argc;
char *argv[];
{
  int len;
  char rdbuffer[259];
  char ch;
  int unlock;
  int result;
  fd_set fdmask;
  struct timeval timevalue;
  fd_set dummy_mask;
  int served;
  int max_fd;
  int count;
  int kbd_res;
  int ser_res;
  time_t ser_resp_time;
  int shell_res;
  int empty_rounds;
  int ser_rounds;
  int resync_needed;
  int iface_res;
  int fr_lines;
  int fr_cols;
  int fr_error;
  int parm;
  char hlpstr[10];
  char fr_con[80];
#ifdef USE_IFACE
  int servlen;
  struct sockaddr *saddr;
  int arg;
#endif

  umask(0); /* don't filter file-permissions */

  is_root = (geteuid() == 0);
  
  tnt_daemon = 0;
  frontend_active = 0;
  terminated = 0;
  
  
  if (read_init_file(argc,argv,&unlock))
    exit(1);
  if (!tnt_daemon) {
    if (term_init("",0,0))
      exit(1);
  }
  else
    set_linescols();
    
  if (alloc_all())
    exit(1);
  
  if (init_serial(device,speed,speedflag,unlock)) {
    free_all();
    exit(1);
  }
  if (!tnt_daemon) {
    if (init_proc()) {
      term_exit();
      exit_serial();
      free_all();
      exit(1);
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, sighup);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTERM, sigterm);
  }
  else {
#ifdef USE_IFACE
    saddr = build_sockaddr(frontend_socket,&servlen);
    if (!saddr) {
      printf("ERROR: invalid definition of frontend socket\n");
      exit(1);
    }
    if ((frontend_sockfd = socket(saddr->sa_family,SOCK_STREAM,0)) < 0) {
      printf("ERROR: Can't open stream socket\n");
      exit(1);
    }

    switch (saddr->sa_family) {
    case AF_UNIX:
      unlink(saddr->sa_data);
      break;     
    case AF_INET:
      arg = 1;
      setsockopt(frontend_sockfd,SOL_SOCKET,
                 SO_REUSEADDR,(char *) &arg,sizeof(arg));
      break;     
    }              
                                             
    if (bind(frontend_sockfd,saddr,servlen) < 0) {
      printf("ERROR: Can't bind socket\n");
      exit(1);
    }
  
    listen(frontend_sockfd,5);

    if (!use_select)
      fcntl(frontend_sockfd,F_SETFL,O_NONBLOCK);
  
    printf("%s successfully started\n",
            rem_ver_str);
    if (fork() != 0)
      exit(0);
    if (init_proc()) {
      close(frontend_sockfd);
      exit_serial();
      free_all();
      exit(1);
    }
    close(0);
    close(1);
    close(2);
    chdir("/");
    setsid();
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, sighup);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTERM, sigterm);
#endif
  }

  init_file();
  init_screen();  
  init_keyboard();
  init_help();
  init_priv();
  init_xconnect();
  init_mheard();
  init_xmon();
  init_conv();
  init_remote();
  init_cookie();
  init_crc();
  init_shell();
#ifdef USE_IFACE
  init_iface();
  init_mb();
#endif
  init_blist();
  init_log();
#ifdef BCAST
  init_bcast();
#endif
  
  if (strcmp(tnt_logfile,"") != 0) {
    open_logfile(RX_NORM,RX_RCV,1,strlen(tnt_logfile),
                 M_CMDSCRIPT,tnt_logfile);
  }
  
  if (!use_select) {
    ser_resp_time = time(NULL);
    ser_rounds = 0;
    empty_rounds = 0;
  }
  act_state = S_INIT;
  len = 1;
  serial_server(&act_state,rdbuffer,len);
  
  while (act_state) {
    if (terminated) {
      exit_tnt(0,0,0,0,M_REMOTE,NULL);
      terminated = 0;
    }
#ifdef USE_IFACE
    iface_trying();
#endif
    flush_frontend();
    check_xmon_timeout();
    check_xconnect_timeout();
    yapp_timeout();
#ifdef BCAST
    bc_timing();
#endif
    if (use_select) {
      FD_ZERO(&fdmask);
      if (!tnt_daemon) {
        FD_SET(0,&fdmask);
        max_fd = 1;
      }
      else {
        if (frontend_active) {
          FD_SET(frontend_fd,&fdmask);
          max_fd = frontend_fd + 1;
        }
      }
      FD_SET(serial,&fdmask);
      if (serial > max_fd - 1)
        max_fd = serial + 1;
      shell_fdset(&max_fd,&fdmask);
#ifdef USE_IFACE
      iface_fdset(&max_fd,&fdmask);
#endif
      timevalue.tv_usec = 0;
      timevalue.tv_sec = RESY_TIME;
      count = select(max_fd,&fdmask,
                    (fd_set *) 0,(fd_set *) 0,&timevalue);
      if (count == -1) {
        continue;
      }
      served = 0;
      resync_needed = 0;
      if (!tnt_daemon) {
        if (FD_ISSET(0,&fdmask)) {
          if (read(0,&ch,1) == 1) {
            keyboard_server(&act_state,&ch);
          } 
          served = 1;
        }
      }
      else {
        if (frontend_active) {
          if (FD_ISSET(frontend_fd,&fdmask)) {
            result = read(frontend_fd,&ch,1);
            if (result == 1) {
              if (frontend_active == 2) {
                fr_error = 1;
                if (ch != '\n') {
                  if (strlen(frontend_para) < 79) {
                    hlpstr[0] = ch;
                    hlpstr[1] = '\0';
                    strcat(frontend_para,hlpstr);
                    fr_error = 0;
                  }
                }
                else {
                  parm = sscanf(frontend_para,"%s %d %d",
                                fr_con,&fr_lines,&fr_cols);
                  if (parm == 3) {
                    if (!term_init(fr_con,fr_lines,fr_cols)) {
                      frontend_active = 1;
                      init_fr_buffer();
                      reinit_screen();
                      fr_error = 0;
                    }
                  }
                }
                if (fr_error) {
                  frontend_active = 0;
                  close(frontend_fd);
                }
              }
              else {
                keyboard_server(&act_state,&ch);
              }
            }
            served = 1;
          }
        }
      }
      if (FD_ISSET(serial,&fdmask)) {
        if ((len = read(serial,rdbuffer,259)) > 0) {
          resync_needed = serial_server(&act_state,rdbuffer,len);
        }
        else {
          if ((soft_tnc) && (len == 0)) {
            act_state = S_END;
            softtnc_error = 1;
          }
        }
        served = 1;
      }
      if (!shell_receive(&fdmask)) {
        served = 1;
      }
#ifdef USE_IFACE
      if (!iface_receive(&fdmask)) {
        served = 1;
      }
#endif 
      if ((!served) || (resync_needed)) {
        resync_tnc(&act_state);
      }
    }
    else {
      ser_res = 1;
      resync_needed = 0;
      if ((len = read(serial,rdbuffer,259))) {
        if (len > 0) {
          resync_needed = serial_server(&act_state,rdbuffer,len);
          ser_res = 0;
          ser_resp_time = time(NULL);
          ser_rounds = 0;
        }
      }
      else {
        if ((soft_tnc) && (len == 0)) {
          act_state = S_END;
          softtnc_error = 1;
        }
      }
      kbd_res = 1;
      if (!tnt_daemon) {
        if (read(0,&ch,1)) {
          keyboard_server(&act_state,&ch);
          kbd_res = 0;
        }
      }
      else {
        if (frontend_active) {
          result = read(frontend_fd,&ch,1);
          if (result == 1) {
            if (frontend_active == 2) {
              fr_error = 1;
              if (ch != '\n') {
                if (strlen(frontend_para) < 79) {
                  hlpstr[0] = ch;
                  hlpstr[1] = '\0';
                  strcat(frontend_para,hlpstr);
                  fr_error = 0;
                }
              }
              else {
                parm = sscanf(frontend_para,"%s %d %d",
                              fr_con,&fr_lines,&fr_cols);
                if (parm == 3) {
                  if (!term_init(fr_con,fr_lines,fr_cols)) {
                    frontend_active = 1;
                    init_fr_buffer();
                    reinit_screen();
                    fr_error = 0;
                  }
                }
              }
              if (fr_error) {
                frontend_active = 0;
                close(frontend_fd);
              }
            }
            else {
              keyboard_server(&act_state,&ch);
            }
            kbd_res = 0;
          }
        }
      }
      shell_res = shell_receive(&dummy_mask);
#ifdef USE_IFACE
      iface_res = iface_receive(&dummy_mask);
#else
      iface_res = 1;
#endif
      if (resync_needed) {
        resync_tnc(&act_state);
        ser_resp_time = time(NULL);
        ser_rounds = 0;
        empty_rounds = 0;
      }
      else {
      /* if no serial processing and no key pressed: sleep */
        if (ser_res && kbd_res && shell_res && iface_res) {
          if (empty_rounds > 10) {
            empty_rounds = 0;
            uwait(10000); /* sleep for 10ms */
          }
          else {
            empty_rounds++;
          }
          ser_rounds++;
          if (ser_rounds > 100*RESY_TIME) {
            if ((time(NULL) - ser_resp_time) >= RESY_TIME) {
              resync_tnc(&act_state);
              ser_resp_time = time(NULL);
              ser_rounds = 0;
              empty_rounds = 0;
            }
          }
        }
      }
    }
  }

  exit_file();
#ifdef BCAST
  exit_bcast();
#endif
  exit_log();
  exit_blist();
#ifdef USE_IFACE
  exit_mb();
  exit_iface();
#endif
  exit_shell();
  exit_priv();
  exit_remote();
  exit_xmon();
  exit_mheard();
  exit_xconnect();
  exit_help();
  exit_keyboard();
  exit_screen();
  exit_proc();
  if (tnt_daemon) {
#ifdef USE_IFACE
    if (frontend_active) {
      frontend_exit(1);
    }
    close(frontend_sockfd);
/*
    strcpy(serv_addr.sun_path,frontend_socket);
    unlink(serv_addr.sun_path);
*/
#endif
  }
  else {
    term_exit();
  }
  if (exit_serial()) {
    free_all();
    exit(1);
  }
  free_all();
  exit(0);
}
