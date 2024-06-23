/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for connect (connect.c)
   created: Mark Wahl DL4YBG 94/03/14
   updated: Mark Wahl DL4YBG 97/04/06
   updated: mayer hans oe1smc, 6.1.1999
*/

#include "tnt.h"
#ifdef USE_IFACE
#include "ifacedef.h"
#else
#define MAXQRGS 10
#endif
#include "connect.h"


extern void update_name(int channel);
extern void stat_display(int channel);
extern void cmd_display(int flag,int channel,char *buffer,int cr);
#ifdef USE_IFACE
extern void get_iface_name(int iface,char *str);
extern void cmd_actiface(int par1,int par2,int channel,
                         int len,int mode,char *str);
extern void cmd_actiface_ext(int par1,int par2,int par3,int channel,
                             int len,int mode,char *str);
extern void abort_routing(char *call);
extern void no_success_connect(int iface,int usernr);
#endif
#ifdef HAVE_SOCKET
extern void socksetup_to_conn(int channel);
extern void socksetup_timeout(int channel);
#endif
extern void statlin_update();
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void update_ssid(int channel,char *destcall);
extern void update_remcall(int channel,char *call);
extern void rem_data_display(int channel,char *buffer);
#ifdef HAVE_SOCKET
extern void connstr_to_socket(int channel,char *str);
#endif
extern void set_pwmode(int channel);
extern void set_remmode(int channel);
extern void strip_call(char *call,int channel);
#ifdef USE_IFACE
extern void send_qrginfo_all();
#endif


extern int tnc_channels;
extern struct channel_stat *ch_stat;
extern char route_file_name[];
extern char tnt_dir[];
extern int tnc_command;
extern int altstat;
extern int ax25k_active;

char connect_str[] = "onnected to ";
static char cmpstr[][20] = {
     "failure",
     "loop",
     "warning",
     "busy",
     "no path",
     "no route",
     "invalid",
     "connect twice"
};

#define NUM_CMPSTR 8

static char xc_active_text[] = "XCONNECT already active";
static char xc_notfound_text[] = "Routing file not found";
static char xc_corrupt_text[] = "Routing file corrupt";
static char xc_maxdigis_text[] = "Digipeater-table full";
static char xc_abort_text[] = "XCONNECT aborted";
static char xc_noxconn_text[] = "XCONNECT not active";
static char xc_inv_arg_text[] = "invalid arguments";
static char xc_inv_call_text[] = "invalid callsign";

static struct xconn_stat *xc_stat;

struct sqrg_info qrg_info[MAXQRGS+1];


static int test_connected(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char str_buf[257];
  char *str_ptr;
  int result;
  int i;
  
  
  /* copy buffer to string, \0 may occur in buffer, but if connected
     is assumed to be received, plain ASCII will be received */
  result = 0;
  memcpy(str_buf,buffer,len);
  str_buf[len] = '\0';
  /* try to find connect-string */
  if ((str_ptr = strstr(str_buf,connect_str)) == NULL) {
    /* check for other strings, if xconnect active */
    if (xc_stat[channel].active) {
      for (i=0;i<len;i++)
        str_buf[i] = tolower(str_buf[i]);
      i = 0;
      while (i < NUM_CMPSTR) {
        if (strstr(str_buf,cmpstr[i]) != NULL)
          return(T_RECON);
        i++;
      }
    }
    return(T_NOCON);
  }
  /* check if connected */
  if ((str_ptr - str_buf) == 1) {
    if (tolower(*(str_ptr - 1)) == 'c')
      result = T_CON;
  }
  if ((str_ptr - str_buf) >= 2) {
    if ((tolower(*(str_ptr - 1)) == 'c') && ((*(str_ptr - 2)) == ' '))
      result = T_CON;
  }
  /* else check if reconnected */
  if (!result && ((str_ptr - str_buf) >= 3)) {
    if (((*(str_ptr - 1)) == 'c') && ((*(str_ptr - 2)) == 'e') &&
        (tolower(*(str_ptr - 3)) == 'r'))
      result = T_RECON;
  }
  if (!result) return(result);
  str_ptr += strlen(connect_str);
  /* now get new callsign */
  if (sscanf(str_ptr,"%30s",ch_stat[channel].disp_call) != 1) return(0);
  if (altstat) {
    update_name(channel);
  }
  stat_display(channel);
  if (result == T_CON) /* if connect copy to call for logbook */
    strcpy(ch_stat[channel].log_call,ch_stat[channel].disp_call);
  return(result);
}

void get_qrgpos(qrgpos,chanstr)
int *qrgpos;
char *chanstr;
{
  int i;
  int j;
  char tmp1[11];
  char tmp2[11];
  
  if (*qrgpos != -1) return;
  i = 0;
  while (qrg_info[i].qrg[0] != '\0') {
    if ((qrg_info[i].port_str[0] != '\0') && (chanstr[0] != '\0')) {
      strcpy(tmp1,chanstr);
      for (j=0;j<strlen(tmp1);j++) tmp1[j] = toupper(tmp1[j]);
      strcpy(tmp2,qrg_info[i].port_str);
      for (j=0;j<strlen(tmp2);j++) tmp2[j] = toupper(tmp2[j]);
      if (strcmp(tmp1,tmp2) == 0) {
        *qrgpos = i;
        return;
      }
    }
    i++;
  }
  return;
}

static int check_qrg(qrg,qrgpos,chanstr)
char *qrg;
int *qrgpos;
char *chanstr;
{
  int i;
  int j;
  char tmp1[11];
  char tmp2[11];
  
  i = 0;
  while (qrg_info[i].qrg[0] != '\0') {
    if (strcmp(qrg,qrg_info[i].qrg) == 0) {
      if (chanstr[0] != '\0') {
        strcpy(tmp1,chanstr);
        for (j=0;j<strlen(tmp1);j++) tmp1[j] = toupper(tmp1[j]);
        strcpy(tmp2,qrg_info[i].port_str);
        for (j=0;j<strlen(tmp2);j++) tmp2[j] = toupper(tmp2[j]);
        if (strcmp(tmp1,tmp2) == 0) {
          *qrgpos = i;
          return(1);
        }
      }
      else {
        *qrgpos = i;
        return(1);
      }
    }
    i++;
  }
  return(0);
}

void get_chanstr(char *qrg, char *chanstr)
{
  int qrgpos;
  char dummystr;

  chanstr[0] = '\0';  
  dummystr = '\0';
  if (check_qrg(qrg,&qrgpos,&dummystr)) {
    strcpy(chanstr,qrg_info[qrgpos].port_str);
  }
}

/* fill qrg from channelstring */
void get_qrg_from_chanstr(char *qrg, char *chanstr)
{
  int i;
  int j;
  char tmp1[11];
  char tmp2[11];

  qrg[0] = '\0';
  if (chanstr[0] == '\0') {
    if (qrg_info[0].qrg[0] != '\0')
      strcpy(qrg, qrg_info[0].qrg);
    return;
  }
    
  i = 0;
  while (qrg_info[i].qrg[0] != '\0') {
    if (qrg_info[i].port_str[0] != '\0') {
      strcpy(tmp1,chanstr);
      for (j=0;j<strlen(tmp1);j++) tmp1[j] = toupper(tmp1[j]);
      strcpy(tmp2,qrg_info[i].port_str);
      for (j=0;j<strlen(tmp2);j++) tmp2[j] = toupper(tmp2[j]);
      if (strcmp(tmp1,tmp2) == 0) {
        strcpy(qrg, qrg_info[i].qrg);
        return;
      }
    }
    i++;
  }
  if (qrg_info[0].qrg[0] != '\0')
    strcpy(qrg, qrg_info[0].qrg);
}

#define LINELENGTH 127

static int route_call(channel,len,mode,str,nextlink,callflag,qrgpos,chanstr)
int channel;
int len;
int mode;
char *str;
int nextlink;
int callflag;
int *qrgpos;
char *chanstr;
{  
  int found;
  int corrupt;
  int num;
  char *line_ptr;
  FILE *xc_fp;
  char *call_ptr;
  char *tmp_ptr;
  char tmpstr[160];
  int valid;

  strcpy(tmpstr,route_file_name);
  if ((xc_fp = fopen(tmpstr,"r")) == NULL) {
    cmd_display(mode,channel,xc_notfound_text,1);
    return(0);
  }
  found = 0;
  line_ptr = malloc(LINELENGTH + 1);
  valid = 1;
  while (!found) {
    corrupt = 1;
    if ((fgets(line_ptr,LINELENGTH,xc_fp)) != NULL) {
      /* test if line too long */
      if (*(line_ptr + strlen(line_ptr) -1) == '\n') {
        if ((line_ptr[0] == '#') || (line_ptr[0] == '\n')) {
          /* empty line or comment: ignore */
          corrupt = 0;
        }
        else if (line_ptr[0] == '<') {
          /* check for <IF qrg> and <END> */
          if ((strncmp(line_ptr,"<IF",3) == 0) ||
              (strncmp(line_ptr,"<if",3) == 0)) {
            if (strtok(line_ptr," \t") != NULL) {
              if ((tmp_ptr = strtok(NULL,">")) != NULL) {
                valid = check_qrg(tmp_ptr,qrgpos,chanstr);
                corrupt = 0;
              } 
            }
          }
          else if ((strncmp(line_ptr,"<END>",5) == 0) ||
                   (strncmp(line_ptr,"<end>",5) == 0)) {
            valid = 1;
            *qrgpos = -1;
            corrupt = 0;
          }
        }
        else if ((call_ptr = strtok(line_ptr," \t")) != NULL ) {
          corrupt = 0;
          if (valid) {
            tmp_ptr = callflag ? call_ptr : (call_ptr + 2);
            if (strcmp(tmp_ptr,str) == 0) {
              found = 1;
            }
          }
        }
      }
    }
    else {
      free(line_ptr);
      fclose(xc_fp);
      get_qrgpos(qrgpos,chanstr);
      return(C_NOTFOUND);
    }
    if (corrupt) {
      cmd_display(mode,channel,xc_corrupt_text,1);
      free(line_ptr);
      fclose(xc_fp);
      return(0);
    }
  }
  tmp_ptr = strtok(NULL,";"); 
  if ((tmp_ptr == NULL) || (*(tmp_ptr + strlen(tmp_ptr) - 1) == '\n')) {
    /* station can be reached directly */
    free(line_ptr);
    fclose(xc_fp);
    get_qrgpos(qrgpos,chanstr);
    return(C_DIRECT);  
  }
  /* station has routing data available */
  num = xc_stat[channel].num_digi;
  tmp_ptr = strtok(NULL," \t");
  if ((tmp_ptr[0] == '\n') && (strlen(tmp_ptr) == 1)) {
    /* station can be reached directly */
    free(line_ptr);
    fclose(xc_fp);
    get_qrgpos(qrgpos,chanstr);
    return(C_DIRECT);  
  }
  while (tmp_ptr != NULL) {
    if (!((tmp_ptr[0] == '\n') && (strlen(tmp_ptr) == 1))) {
      xc_stat[channel].digiinfo[num].nextdigi = tmp_ptr;
      if (*(tmp_ptr = tmp_ptr + strlen(tmp_ptr) - 1) == '\n')
        *tmp_ptr = '\0';
      num++;
      if (num >= MAXDIGIS) {
        cmd_display(mode,channel,xc_maxdigis_text,1);
        free(line_ptr);
        fclose(xc_fp);
        return(0);
      }
      xc_stat[channel].digiinfo[num - 1].nextlink = num;
    }
    tmp_ptr = strtok(NULL," \t");
  }
  if (num == xc_stat[channel].num_digi) {
    /* no routing data found after ; */
    cmd_display(mode,channel,xc_corrupt_text,1);
    free(line_ptr);
    fclose(xc_fp);
    return(0);
  }
  xc_stat[channel].digiinfo[num - 1].nextlink = nextlink;
  xc_stat[channel].num_digi = num;
  xc_stat[channel].alloc[xc_stat[channel].num_alloc] = line_ptr;
  xc_stat[channel].num_alloc++;
  fclose(xc_fp);
  get_qrgpos(qrgpos,chanstr);
  return(C_ROUTE);
}

static int gen_con_string(channel,str,ctype,qrgpos)
int channel;
char *str;
int ctype;
int qrgpos;
{
  char tmp_str[256];
  char *tmp_ptr;
  int nodetype;
  int digi;
  
  strcpy(tmp_str,"");
  digi = xc_stat[channel].cur_digi;
  while (1) {
    tmp_ptr = xc_stat[channel].digiinfo[digi].nextdigi;
    digi = xc_stat[channel].digiinfo[digi].nextlink;
    xc_stat[channel].cur_digi = digi;
    if (digi == D_EOL) {
      nodetype = CT_END;
      break;
    }
    if (*tmp_ptr == 'N') {
      nodetype = CT_NODE;
      break;
    }
    if (*tmp_ptr == 'T') {
      nodetype = CT_USER;
      break;
    }
    strcat(tmp_str," "); 
    strcat(tmp_str,tmp_ptr+2);
  }
  switch (ctype) {
  case CT_CONNECT:
    strcpy(str,"C");
    if (qrgpos != -1) {
      strcat(str,qrg_info[qrgpos].port_str);
    }
    break;
  case CT_NODE:
    strcpy(str,"C ");
    break;
  case CT_USER:
    strcpy(str,"//C ");
    break;
  }
  strcat(str,tmp_ptr+2);
  strcat(str,tmp_str);
  if ((ctype == CT_NODE) || (ctype == CT_USER)) {
    strcat(str,"\r");
  }
  return(nodetype);
}

static void free_alloc(channel)
int channel;
{
  int i;
  
  if (xc_stat[channel].num_alloc) {
    for (i = 0; i < xc_stat[channel].num_alloc; i++) {
      free(xc_stat[channel].alloc[i]);
    }
  }
}

void close_xconnect_2(channel,success,noaction)
int channel;
int success;
int noaction;
{
#ifdef USE_IFACE
  char socket_str[256];
#endif

  if (xc_stat[channel].active) {
    if (!noaction) {
      if (success) {
#ifdef USE_IFACE
        if (xc_stat[channel].inform_box) {
          get_iface_name(xc_stat[channel].iface,socket_str);
          if (socket_str[0] != '\0') {
            switch (xc_stat[channel].inform_box) {
            case 1:
              cmd_actiface(1,1,channel,strlen(socket_str),
                           M_CMDSCRIPT,socket_str);
              break;
            case 2:
              cmd_actiface_ext(1,2,xc_stat[channel].usernr,channel,
                               strlen(socket_str),M_CMDSCRIPT,socket_str);
              break;
            }
          }
        }
#endif
#ifdef HAVE_SOCKET
        if (xc_stat[channel].inform_sock) {
          socksetup_to_conn(channel);
        }
#endif
      }
      else {
#ifdef USE_IFACE
        if (xc_stat[channel].inform_box) {
          switch (xc_stat[channel].inform_box) {
          case 1:
            abort_routing(xc_stat[channel].callsign);
            break;
          case 2:
            no_success_connect(xc_stat[channel].iface,xc_stat[channel].usernr);
            break;
          }
        }
#endif
#ifdef HAVE_SOCKET
        if (xc_stat[channel].inform_sock) {
          socksetup_timeout(channel);
        }
#endif
      }
    }
    xc_stat[channel].active = 0;
    xc_stat[channel].firstcon = 0;
    statlin_update();
    free_alloc(channel);
  }
}

void close_xconnect(channel,success)
int channel;
int success;
{
  close_xconnect_2(channel,success,0);
}

#ifdef USE_IFACE
void end_ifacexconnect(int iface,int usernr)
{
  int i;
  
  i = 0;
  while (i < tnc_channels) {
    if (xc_stat[i].active) {
      if (xc_stat[i].inform_box == 2) {
        if ((xc_stat[i].iface == iface) && (xc_stat[i].usernr == usernr)) {
          queue_cmd_data(i,X_COMM,1,M_CMDSCRIPT,"D");
          close_xconnect_2(i,0,1);
        }
      }
    }
    i++;
  }
}
#endif

void cmd_xconnect_ext(par1,par2,par3,channel,len,mode,str)
int par1;
int par2;
int par3;
int channel;
int len;
int mode;
char *str;
{
  int result;
  char tnc_string[256];
  int i;
  int first_digi;
  int second_digi;
  int expand_end;
  char callbuf[256];
  char *call;
  char chanstr[11];
  int timeout;
  int numpar;
  char *remcall;
  int qrgpos;
  char *ptr;
  
  numpar = sscanf(str,"%s %d",callbuf,&timeout);
  switch (numpar) {
  case 1:
    timeout = 0;
    break;
  case 2:
    break;
  default:
    cmd_display(mode,channel,xc_inv_arg_text,1);
    return;
    break;
  }
  
  call = callbuf;
  chanstr[0] = '\0';
  ptr = strchr(callbuf,':');
  if (ptr != NULL) {
    call = ptr+1;
    strncpy(chanstr,callbuf,ptr+1-callbuf);
    chanstr[ptr+1-callbuf] = '\0';
  }
  
  if (strlen(call) > 9) {
    cmd_display(mode,channel,xc_inv_call_text,1);
    return;
  }
  for (i = 0; i < strlen(call); i++) {
    call[i] = toupper(call[i]);
  }
  if (xc_stat[channel].active) {
    if (strcmp(call,"OFF") == 0) {
      close_xconnect(channel,0);
#ifdef USE_IFACE
      if (xc_stat[channel].inform_box) {
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
        switch (xc_stat[channel].inform_box) {
        case 1:
          abort_routing(xc_stat[channel].callsign);
          break;
        case 2:
          no_success_connect(xc_stat[channel].iface,xc_stat[channel].usernr);
          break;
        }
      }
#endif
      cmd_display(mode,channel,xc_abort_text,1);
      return;
    }
    else {
      cmd_display(mode,channel,xc_active_text,1);
      return;
    }
  }
  if (strcmp(call,"OFF") == 0) {
    cmd_display(mode,channel,xc_noxconn_text,1);
    return;
  }
  qrgpos = -1;
  xc_stat[channel].num_digi = 0;
  xc_stat[channel].num_alloc = 0;
  xc_stat[channel].first_digi = 0;

  result = route_call(channel,len,mode,call,D_EOL,0,&qrgpos,chanstr);
  switch (result) {
  case 0:
    /* somethings wrong with routing file */
#ifdef USE_IFACE  
    abort_routing(call);
#endif
    free_alloc(channel);
    return;
    break;
  case C_NOTFOUND:
  case C_DIRECT:
    xc_stat[channel].ctype = CT_END;
    xc_stat[channel].active = 1;
    statlin_update();
    xc_stat[channel].timeout = timeout;
    xc_stat[channel].starttime = time(NULL);
#ifdef USE_IFACE
    xc_stat[channel].inform_box = 0;
    if ((par1 == 1) || (par1 == 2)) {
      xc_stat[channel].inform_box = par1;
      strcpy(xc_stat[channel].callsign,call);
      xc_stat[channel].iface = par2;
      if (par1 == 2) xc_stat[channel].usernr = par3;
    }
#endif
#ifdef HAVE_SOCKET
    xc_stat[channel].inform_sock = 0;
    if (par1 == 3) {
      xc_stat[channel].inform_sock = 1;
    }
#endif
    update_ssid(channel,call);
    tnc_command = 1;
    strcpy(tnc_string,"C");
    if (qrgpos != -1) {
      strcat(tnc_string,qrg_info[qrgpos].port_str);
    }
    strcat(tnc_string,call);
    queue_cmd_data(channel,X_COMM,strlen(tnc_string),mode,tnc_string);
    update_remcall(channel,call);
    if (altstat) {
      update_name(channel);
    }
    ch_stat[channel].conn_state = CS_SETUP;
    xc_stat[channel].firstcon = 1;
    break;
  case C_ROUTE:
    expand_end = 0;
    while (!expand_end) {
      first_digi = xc_stat[channel].first_digi;
      second_digi = xc_stat[channel].digiinfo[first_digi].nextlink;
      xc_stat[channel].first_digi = xc_stat[channel].num_digi;
      result = route_call(channel,len,mode,
               xc_stat[channel].digiinfo[first_digi].nextdigi,
               second_digi,1,&qrgpos,chanstr);
      switch (result) {
      case 0:
#ifdef USE_IFACE  
        abort_routing(call);
#endif
        free_alloc(channel);
        return;
        break;
      case C_NOTFOUND:
      case C_DIRECT:
        xc_stat[channel].first_digi = first_digi;
        expand_end = 1;
        break;
      case C_ROUTE:
        break;
      }
    }
    xc_stat[channel].cur_digi = xc_stat[channel].first_digi;
    xc_stat[channel].ctype = gen_con_string(channel,tnc_string,
                                            CT_CONNECT,qrgpos);
    xc_stat[channel].active = 1;
    statlin_update();
    xc_stat[channel].timeout = timeout;
    xc_stat[channel].starttime = time(NULL);
#ifdef USE_IFACE
    xc_stat[channel].inform_box = 0;
    if ((par1 == 1) || (par1 == 2)) {
      xc_stat[channel].inform_box = par1;
      strcpy(xc_stat[channel].callsign,call);
      xc_stat[channel].iface = par2;
      if (par1 == 2) xc_stat[channel].usernr = par3;
    }
#endif
#ifdef HAVE_SOCKET
    xc_stat[channel].inform_sock = 0;
    if (par1 == 3) {
      xc_stat[channel].inform_sock = 1;
    }
#endif
    remcall = &tnc_string[1];
    ptr = strchr(tnc_string,':');
    if (ptr != NULL) {
      remcall = ptr+1;
    }
    update_ssid(channel,remcall);
    tnc_command = 1;
    queue_cmd_data(channel,X_COMM,strlen(tnc_string),mode,tnc_string);
    update_remcall(channel,&tnc_string[1]);
    if (altstat) {
      update_name(channel);
    }
    ch_stat[channel].conn_state = CS_SETUP;
    xc_stat[channel].firstcon = 1;
    break;
  }
}

void cmd_xconnect(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  cmd_xconnect_ext(par1,par2,0,channel,len,mode,str);
}

void next_connect(channel)
int channel;
{
  char tnc_string[256];
  int flag;

  if (xc_stat[channel].active) {
    if (xc_stat[channel].ctype == CT_END) {
      close_xconnect(channel,1);
      return;
    }
    xc_stat[channel].ctype = gen_con_string(channel,tnc_string,
                                            xc_stat[channel].ctype,-1);
    flag = 0;
    rem_data_display(channel,tnc_string);
#ifdef HAVE_SOCKET
    if (xc_stat[channel].inform_sock) {
      connstr_to_socket(channel,tnc_string);
    }
#endif
    queue_cmd_data(channel,X_DATA,strlen(tnc_string),flag,tnc_string);
  }
}

void connect_update(int channel,char *buffer, int len)
{
  int was_active;
  
  switch (test_connected(channel,buffer,len)) {
  case T_CON:
    next_connect(channel);
    set_pwmode(channel);
    set_remmode(channel);
    break;
  case T_RECON:
    set_pwmode(channel);
    set_remmode(channel);
    was_active = xc_stat[channel].active;
    close_xconnect(channel,0);
    if (was_active) {
#ifdef USE_IFACE
      if (xc_stat[channel].inform_box)
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
#endif
#ifdef HAVE_SOCKET
      if (xc_stat[channel].inform_sock)
        queue_cmd_data(channel,X_COMM,1,M_CMDSCRIPT,"D");
#endif
      }
    break;
  default:
    break;
  }
}

int xconnect_active(channel)
int channel;
{
  return(xc_stat[channel].active > 0);
}

int xconnect_first(channel)
int channel;
{
  if (xc_stat[channel].active)
    return(xc_stat[channel].firstcon > 0);
  else
    return(0);
}

void xcon_reset_first(channel)
int channel;
{
  xc_stat[channel].firstcon = 0;
}

void init_xconnect()
{
  int i;
  
  for (i=0;i<=MAXQRGS;i++) {
    qrg_info[i].qrg[0] = '\0';
    qrg_info[i].port_str[0] = '\0';
  }
  for (i=0;i<tnc_channels;i++) {
    xc_stat[i].active = 0;
    xc_stat[i].firstcon = 0;
  }
}

void exit_xconnect()
{
  int i;
  
  for (i=0;i<tnc_channels;i++) {
    close_xconnect(i,0);
#ifdef USE_IFACE
    if (xc_stat[i].inform_box)
      queue_cmd_data(i,X_COMM,1,M_CMDSCRIPT,"D");
#endif
  }
}

void check_xconnect_timeout()
{
  int i;
  
  for (i=0;i<tnc_channels;i++) {
    if (xc_stat[i].active) {
      if (xc_stat[i].timeout != 0) {
        if ((time(NULL) - xc_stat[i].starttime) > xc_stat[i].timeout) {
          close_xconnect(i,0);
          queue_cmd_data(i,X_COMM,1,M_CMDSCRIPT,"D");
        }
      }
    }
  }
}

#ifdef USE_IFACE
void get_call_xc(channel,call)
int channel;
char *call;
{
  if (xc_stat[channel].active)
    strcpy(call,xc_stat[channel].callsign);
  else
    strcpy(call,"");
}
#endif

int own_connection(call1,call2)
char *call1;
char *call2;
{
  char destcall[10];
  int result;
  int i;
  
  result = 0;
  for (i=1;i<tnc_channels;i++) {
    if (ch_stat[i].conn_state == CS_CONN) {
      strip_call(destcall,i);
      if (((strcmp(destcall,call1) == 0) && 
           (strcmp(ch_stat[i].curcall,call2) == 0)) ||
          ((strcmp(destcall,call2) == 0) &&
           (strcmp(ch_stat[i].curcall,call1) == 0))) {
        result = 1;
        break;
      } 
    }
  }
  return(result);
}

void cmd_concall(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int i;
  
  if (ch_stat[channel].conn_state != CS_CONN) {
    cmd_display(mode,channel,"Only while connected",1);
    return;
  }
  if (sscanf(str,"%30s",ch_stat[channel].disp_call) != 1) {
    cmd_display(mode,channel,"Invalid callsign",1);
    return;
  }
  for (i=0;i<strlen(ch_stat[channel].disp_call);i++)
    ch_stat[channel].disp_call[i] = toupper(ch_stat[channel].disp_call[i]);
  strcpy(ch_stat[channel].log_call,ch_stat[channel].disp_call);
  if (altstat) {
    update_name(channel);
  }
  stat_display(channel);
  set_pwmode(channel);
  set_remmode(channel);
  cmd_display(mode,channel,OK_TEXT,1);
}

void cmd_qrg(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int i,j;
  char ans_str[80];
  char qrg[19];
  char port_str[11];
  int res;
  int port;
  
  if (len == 0) {
    if (qrg_info[0].qrg[0] == '\0')
      cmd_display(mode,channel,"empty",1);
    else {
      i = 0;
      while ((qrg_info[i].qrg[0] != '\0') && (i < MAXQRGS)) {
        sprintf(ans_str,"<%d> %s (%s)",i,qrg_info[i].qrg,qrg_info[i].port_str);
        cmd_display(mode,channel,ans_str,1);
        i++;
      }
    }
    return;
  }

  if (ax25k_active) {
    cmd_display(mode,channel,
                "Error: To change QRGs, do an update of 'axports'",1);
    return;
  }

  res = sscanf(str,"%d %18s %10s",&port,qrg,port_str);
  if ((res >= 2) && (res <= 3) && (port < MAXQRGS)) {
    i = 0;
    while (i < MAXQRGS) {
      if ((port != i) && (qrg_info[i].qrg[0] == '\0')) {
        cmd_display(mode,channel,"invalid portnumber",1);
        return;
      }
      if (port == i) {
        if (strcmp(qrg,"$") == 0) {
          if (qrg_info[i+1].qrg[0] != '\0') {
            cmd_display(mode,channel,"not allowed to delete",1);
            return;
          }
          qrg_info[i].qrg[0] = '\0';
          qrg_info[i].port_str[0] = '\0';
#ifdef USE_IFACE
          send_qrginfo_all();
#endif
          cmd_display(mode,channel,OK_TEXT,1);
          return;
        }
        strcpy(qrg_info[i].qrg,qrg);
        for (j=0;j<strlen(qrg_info[i].qrg);j++)
          qrg_info[i].qrg[j] = toupper(qrg_info[i].qrg[j]);
        if (res == 3)
          strcpy(qrg_info[i].port_str,port_str);
        else
          qrg_info[i].port_str[0] = '\0';
#ifdef USE_IFACE
        send_qrginfo_all();
#endif
        cmd_display(mode,channel,OK_TEXT,1);
        return;
      }
      i++;
    }
  }
  cmd_display(mode,channel,"Invalid syntax",1);
}

#ifdef USE_AX25K
int add_qrg(qrg,port)
char *qrg;
char *port;
{
  int i;
  char *ptr;

  /* search for next free entry */
  i = 0;
  while ((qrg_info[i].qrg[0] != '\0') && (i < MAXQRGS)) {
    i++;
  }
  if (i == MAXQRGS) return 1;

  /* copy data */
  strncpy(qrg_info[i].qrg,qrg,19);
  qrg_info[i].qrg[19] = '\0';
  if ((ptr = strchr(qrg_info[i].qrg,' ')) != NULL) *ptr = '\0';
  if ((ptr = strchr(qrg_info[i].qrg,0x09)) != NULL) *ptr = '\0';
  strncpy(qrg_info[i].port_str,port,9);
  qrg_info[i].port_str[9] = '\0';
  strcat(qrg_info[i].port_str,":");
  return 0;
}

int next_qrg(qrg,port,value)
char *qrg;
char *port;
int value;
{
  char *ptr;
  char port_str[11];
  
  if ((value < 0) || (value >= MAXQRGS)) {
    return -1;
  }
  if (qrg_info[value].qrg[0] == '\0') return -1;

  strcpy(qrg,qrg_info[value].qrg);
  strcpy(port_str,qrg_info[value].port_str);
  ptr = strchr(port_str,':');
  if (ptr != NULL) *ptr = '\0';
  strcpy(port,port_str);
  return(value+1);
}
#endif

void copy_qrg(qrg)
char qrg[MAXQRGS-1][20];
{
  int i;
  
  for (i=0;i<=MAXQRGS;i++) {
    strcpy(qrg[i],qrg_info[i].qrg);
  }
}

int alloc_connect()
{
  xc_stat = (struct xconn_stat *)
    malloc(tnc_channels * sizeof(struct xconn_stat));
  return(xc_stat == NULL);
}

void free_connect()
{
  free(xc_stat);
}

