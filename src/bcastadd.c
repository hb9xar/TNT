/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Additional procedures for broadcast (bcastadd.c)
   -----------------------------------------------------------------------
   Some of the procedures in this file are translated by
   * p2c, the Pascal-to-C translator *
   from the original DigiPoint sources in Pascal.
   DigiPoint is Copyright (C) 1992,1994 by Joachim Schurig, DL8HBS.
   Code used by kind permission of the author.
   -----------------------------------------------------------------------
   created: Mark Wahl DL4YBG 95/11/08
   updated: Mark Wahl DL4YBG 96/05/27
   updated: Matthias Hensler WS1LS 99/03/09
*/

#include "tnt.h"
#ifdef BCAST
#include "ifacedef.h"
#include "bcastadd.h"
#include "p2cdef.h"
#include "bcast.h"


extern int strpos2(char *s,char *pat,int pos);
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern char *strsub(char *ret,char *s,int pos,int len);
extern void sort_new_mail(short unr, Char *pattern_, Char *rcall);
extern int enchuf(char *inputfile,char *outputfile,int crlfconv);
extern int dechuf(char *inputfile,char *outputfile,int crlfconv);
extern void _bcast_init();
extern void moni_display2(char *buffer,int attrib);
extern void moni_display_len2(char *buffer,int len);
extern void cmd_display(int flag,int channel,char *buffer,int cr);
extern int conv_name_to_iface(char *str);
extern void send_command_packet_if(short channel,short usernr,
                                   unsigned short len,char *buf,int iface);
extern void del_path (char *s);
extern void int2hstr(long i, char *s);
extern short sfopen(char *name, short mode);
extern void sfclose(short *handle);
extern short sfcreate(char *name, short mode);
extern void del_blanks(char *s);


extern int free_buffers;
extern struct channel_stat *ch_stat;
extern int send_queue_type;
extern char box_socket[];

/* variables containing info of frameheader */
extern char srccall[];
extern char destcall[];
extern char vialist[];
extern int pid;
extern int frame_ui;
extern int head_valid;

char tnt_bctempdir[80];
char tnt_bcsavedir[80];
char tnt_bcnewmaildir[80];
extern char tnt_dir[];

int shpacsat_flag;
int bcrequest_flag;
int decbcast_flag;
extern char bcast_log_file[80];
bccbtype func_callback;

static struct timeval tv;
static struct timezone tz;
static struct timeval tv_old;
static struct timezone tz_old;

/* crc-table */
typedef unsigned short crctabtype[256];

static crctabtype ccitt_table = {
  0, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108L, 0x9129L,
  0xa14aL, 0xb16bL, 0xc18cL, 0xd1adL, 0xe1ceL, 0xf1efL, 0x1231, 0x210, 0x3273,
  0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339L, 0x8318L, 0xb37bL, 0xa35aL,
  0xd3bdL, 0xc39cL, 0xf3ffL, 0xe3deL, 0x2462, 0x3443, 0x420, 0x1401, 0x64e6,
  0x74c7, 0x44a4, 0x5485, 0xa56aL, 0xb54bL, 0x8528L, 0x9509L, 0xe5eeL,
  0xf5cfL, 0xc5acL, 0xd58dL, 0x3653, 0x2672, 0x1611, 0x630, 0x76d7, 0x66f6,
  0x5695, 0x46b4, 0xb75bL, 0xa77aL, 0x9719L, 0x8738L, 0xf7dfL, 0xe7feL,
  0xd79dL, 0xc7bcL, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x840, 0x1861, 0x2802,
  0x3823, 0xc9ccL, 0xd9edL, 0xe98eL, 0xf9afL, 0x8948L, 0x9969L, 0xa90aL,
  0xb92bL, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0xa50, 0x3a33, 0x2a12,
  0xdbfdL, 0xcbdcL, 0xfbbfL, 0xeb9eL, 0x9b79L, 0x8b58L, 0xbb3bL, 0xab1aL,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0xc60, 0x1c41, 0xedaeL,
  0xfd8fL, 0xcdecL, 0xddcdL, 0xad2aL, 0xbd0bL, 0x8d68L, 0x9d49L, 0x7e97,
  0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0xe70, 0xff9fL, 0xefbeL,
  0xdfddL, 0xcffcL, 0xbf1bL, 0xaf3aL, 0x9f59L, 0x8f78L, 0x9188L, 0x81a9L,
  0xb1caL, 0xa1ebL, 0xd10cL, 0xc12dL, 0xf14eL, 0xe16fL, 0x1080, 0xa1, 0x30c2,
  0x20e3, 0x5004, 0x4025, 0x7046, 0x6067, 0x83b9L, 0x9398L, 0xa3fbL, 0xb3daL,
  0xc33dL, 0xd31cL, 0xe37fL, 0xf35eL, 0x2b1, 0x1290, 0x22f3, 0x32d2, 0x4235,
  0x5214, 0x6277, 0x7256, 0xb5eaL, 0xa5cbL, 0x95a8L, 0x8589L, 0xf56eL,
  0xe54fL, 0xd52cL, 0xc50dL, 0x34e2, 0x24c3, 0x14a0, 0x481, 0x7466, 0x6447,
  0x5424, 0x4405, 0xa7dbL, 0xb7faL, 0x8799L, 0x97b8L, 0xe75fL, 0xf77eL,
  0xc71dL, 0xd73cL, 0x26d3, 0x36f2, 0x691, 0x16b0, 0x6657, 0x7676, 0x4615,
  0x5634, 0xd94cL, 0xc96dL, 0xf90eL, 0xe92fL, 0x99c8L, 0x89e9L, 0xb98aL,
  0xa9abL, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x8e1, 0x3882, 0x28a3,
  0xcb7dL, 0xdb5cL, 0xeb3fL, 0xfb1eL, 0x8bf9L, 0x9bd8L, 0xabbbL, 0xbb9aL,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0xaf1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2eL,
  0xed0fL, 0xdd6cL, 0xcd4dL, 0xbdaaL, 0xad8bL, 0x9de8L, 0x8dc9L, 0x7c26,
  0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0xcc1, 0xef1fL, 0xff3eL,
  0xcf5dL, 0xdf7cL, 0xaf9bL, 0xbfbaL, 0x8fd9L, 0x9ff8L, 0x6e17, 0x7e36,
  0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0xed1, 0x1ef0
};

/* functions needed for p2c */

/* out of memory */
int _OutMem()
{
  bcastprotokollcall("Out of memory!");
  return(0);
}


/* functions from pastrix.c, tools.c and crc.c */

void sfdelfile(Char *name)
{
  unlink(name);
}

void get_ext(Char *s, Char *sext)
{
  char *ptr;

  sext[0] = '\0';
  ptr = strrchr(s,'.');
  if (ptr == NULL) return;
  strcpy(sext,ptr+1);
}  

/* updating name until no file is existing */
void validate(Char *name)
{
  int fd;
  char tmpname[256];
  char *ptr;
  int ok;
  int count;
  
  fd = open(name,O_RDONLY);
  if (fd == -1) return;
  close(fd);
  ptr = strrchr(name,'.');
  if (ptr != NULL) {
    *ptr = '\0';
  }
  ok = 0;
  count = 1;
  while (!ok && (count < 999)) {
    sprintf(tmpname,"%s.%03.3d",name,count);
    fd = open(tmpname,O_RDONLY);
    if (fd == -1) {
      ok = 1;
    }
    else {
      close(fd);
      count++;
    }
  }
  strcpy(name,tmpname);
}

           
/* "Integer to String"  */
void lint2str(long i, Char *s)
{
  sprintf(s, "%ld", i);
}

void rspacing(Char *txt, short l)
{
  short x, k, i;
  Char c;
    
  i = strlen(txt);   /* aktuelle Laenge               */
  x = l - i;   /* soviele Zeichen fehlen noch   */
  if (x <= 0)
    return;
  c = ' ';
  for (k = 0; k < x; k++)
    txt[k + i] = c;
  txt[l] = '\0';   /* neue Laenge                   */
}
                                  
typedef Char hexarr[16];

static hexarr hextab = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
  'F'
};

#define digits          8

#undef digits

long sys_ixtime()
{
  time_t timeval;
  
  timeval = time(&timeval);
  return((long)timeval);
}

void crcfcs(uchar Data, unsigned short *crc)
{
  *crc = ((*crc) >> 8) ^ ccitt_table[((*crc) ^ Data) & 0xff];
}
   
void crcfbb_buf(uchar *adr, long size, unsigned short *crc)
{
  long ct;
  
  if (size > 0) {
    for (ct = 0; ct < size; ct++)
      *crc = ((*crc) << 8) ^ ccitt_table[((*crc) >> 8) ^ adr[ct]];
  }
}

void checksum16_buf(uchar *adr, long size, unsigned short *crc)
{
  long ct;
  
  if (size > 0) {
    for (ct = 0; ct < size; ct++)
      *crc += adr[ct];
  }
}

#define blocksize_      32768L


/* Errechnet eine CRC ueber ein File                 */

unsigned short file_crc(short methode, Char *name, unsigned short preload,
			long start, long size)
{
  unsigned short crc;
  short k;
  long fsize, done, cstep, psize;
  uchar *buf;

  crc = preload;
  fsize = sfsize(name);

  if (size > 0)
    fsize = size;
  if (start > 0) {
    if (size == 0)
      fsize -= start;
  }

  if (fsize <= 0)
    return crc;
  psize = blocksize_;
  if (psize > fsize)
    psize = fsize;
  buf = (uchar *)malloc(psize);
  if (buf == NULL)
    return crc;
  k = sfopen(name, FO_READ);
  if (k != -1) {
    if (start > 0)
      lseek(k, start, 0);
    done = 0;
    cstep = 1;
    while (done < fsize && cstep > 0) {
      cstep = read(k, buf, psize);
      if (cstep <= 0)
	break;
      done += cstep;
      switch (methode) {

      case 5:
	checksum16_buf(buf, cstep, &crc);
	break;
      }
    }
    sfclose(&k);
  }
  free(buf);
  return crc;
}


void filecut(Char *filea, Char *fileb, long start, long size)
{
  long err;
  uchar *puffer;
  bst psize;
  long done, bc, cstep, hli, fsize;
  short k1, k2;


  fsize = sfsize(filea);
  if (fsize <= 0)
    return;

  if (size > 0) {
    if (fsize > size - start)
      fsize = size - start;
  } else if (start > 0) {
    if (start <= fsize)
      fsize -= start;
  }

  if (fsize <= 0)
    return;
  psize = blocksize_;
  if (psize > fsize)
    psize = fsize;
  puffer = (uchar *)malloc(psize);
  if (puffer == NULL)
    return;
  done = 0;
  hli = 0;
  k1 = sfopen(filea, FO_READ);
  if (k1 != -1) {
    if (start > 0)
      err = lseek(k1, start, 0);
    else
      err = 0;
    if (err == start) {
      k2 = sfcreate(fileb, FC_FILE);
      if (k2 != -1) {
	bc = 1;
	while (done < fsize && bc > 0) {
	  cstep = fsize - done;
	  if (cstep > psize)
	    cstep = psize;
	  bc = read(k1, puffer, cstep);
	  if (bc <= 0)
	    break;
	  done += bc;
	  bc = write(k2, puffer, cstep);
	  if (bc > 0)
	    hli += bc;
	}
	sfclose(&k2);
      }
    }
    sfclose(&k1);
    if (hli != fsize || done != fsize)
      sfdelfile(fileb);
  }
  free(puffer);
}

#undef blocksize_

void unhpath(Char *ins, Char *outs)
{
  short k, i;

  k = strpos2(ins, ".", 1);
  i = strpos2(ins, "-", 1);
  if (k > 0)
    k--;
  else
    k = strlen(ins);
  if (k > 6)
    k = 6;
  if (i > 0 && i <= k)
    k = i - 1;
  sprintf(outs, "%.*s", k, ins);
}

/*Loescht Spaces an Anfang und Ende des Strings*/

void lspacing(Char *txt, short l)
{
  short x, k, i;
  Char c;
    
  i = strlen(txt);   /* aktuelle Laenge               */
  x = l - i;   /* soviele Zeichen fehlen noch   */
  if (x <= 0)
    return;
  for (k = i - 1; k >= 0; k--)
    txt[k + x] = txt[k];
  c = ' ';
  for (k = 0; k < x; k++)
    txt[k] = c;
  txt[l] = '\0';   /* neue Laenge                   */
}

void ix2string(long zeit, Char *timestring)
{
  struct tm cvtime;
  
  cvtime = *gmtime(&zeit);
  sprintf(timestring,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u.%2.2u",
          cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
          cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec);
}

     
/* interface functions to bcast.c */

/* switch between normal and broadcast sendqueue */
void ibcastswitchtobc(short tnc, boolean y)
{
  if (y)
    send_queue_type = SQ_BCAST;
  else
    send_queue_type = SQ_NORMAL;
}

/* restore old pid */
void ibcastpoppid(short tnc)
{
  char tmp_str[257];
  
  sprintf(tmp_str,"@Ppush");
  queue_cmd_data(0,X_COMM,strlen(tmp_str),M_PUSHPOP,tmp_str);
}

/* save pid for later use */
void ibcastpushpid(short tnc)
{
  queue_cmd_data(0,X_COMM,2,M_PUSHPOP,"@P");
}

/* restore old monitor state */
void ibcastpopmonstat(short tnc)
{
  char tmp_str[257];
  
  sprintf(tmp_str,"Mpush");
  queue_cmd_data(0,X_COMM,strlen(tmp_str),M_PUSHPOP,tmp_str);
}

/* save monitor state for later use */
void ibcastpushmonstat(short tnc)
{
  queue_cmd_data(0,X_COMM,1,M_PUSHPOP,"M");
}

/* restore old unproto call */
void ibcastpopunproto(short tnc)
{
  char tmp_str[257];
  
  sprintf(tmp_str,"Cpush");
  queue_cmd_data(0,X_COMM,strlen(tmp_str),M_PUSHPOP,tmp_str);
}

/* save old unproto call for later use */
void ibcastpushunproto(short tnc)
{
  queue_cmd_data(0,X_COMM,1,M_PUSHPOP,"C");
}

/* ??? */
boolean ibcastusereingabe()
{
  return(false);
}

/* error logging */
void ibcastprotokollcall(char *string)
{
  FILE *fp;
  struct tm cvtime;
  time_t ctime;
  
  fp = NULL;
  if (bcast_log_file[0] != '\0') {
    fp = fopen(bcast_log_file,"a");
    if (fp != NULL) {
      ctime = time(NULL);
      cvtime = *localtime(&ctime);
      fprintf(fp,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u.%2.2u: ",
              cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
              cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec);
      fprintf(fp,"%s\n",string);
      fclose(fp);
    }
  }
}

/* set new pid */
void ibcastsetpid(short tnc, short pid)
{
  char tmp_str[257];
  
  sprintf(tmp_str,"@P%d",pid);
  queue_cmd_data(0,X_COMM,strlen(tmp_str),M_PUSHPOP,tmp_str);
}

/* set new monitor state */
void ibcastsetmonstat(short tnc, char *monstr)
{
  char tmp_str[257];
  
  sprintf(tmp_str,"M%s",monstr);
  queue_cmd_data(0,X_COMM,strlen(tmp_str),M_PUSHPOP,tmp_str);
}

/* set new unproto call */
void ibcastsetunproto(short tnc, char *unprstr)
{
  char tmp_str[257];
  
  sprintf(tmp_str,"C%s",unprstr);
  queue_cmd_data(0,X_COMM,strlen(tmp_str),M_PUSHPOP,tmp_str);
}

/* send buffer */
void ibcastsend(short channel, short len, uchar *buffer)
{
  int flag;
  
  flag = 0;
  queue_cmd_data(0,X_DATA,len,flag,(char *)buffer);
}

/* get number of free buffers and continue transmission */
void ibcasttncbuffer(bccbtype callback, short tnc)
{
  short dummychan;
  
  dummychan = 0;
  func_callback = callback;
  queue_cmd_data(0,X_COMM,2,M_PUSHPOP,"@B");
}

/* return number of unproto channel */
short ibcastmonchan(short tnc)
{
  return(0);
}

/* return own callsign */
char *ibcastgetmycall(char *result, short chan)
{
  result = ch_stat[chan].mycall;
  return(result);
}

/* returns if pacsat-frames are displayed */
boolean ibcastshpacsat()
{
  if (shpacsat_flag)
    return(true);
  else
    return(false);
}

/* returns if requests shall be generated */
boolean ibcastcreaterequests()
{
  if (bcrequest_flag)
    return(true);
  else
    return(false);
}

/* the following procedure is 'create_status2' from box_sub.c */

/* Dies ist speziell fr den Broadcast - Empfang */

void ibcastcreatestatus(boolean hierarchicals, Char *dest, Char *absender,
		    long rxdate, long expire_time, long size, Char msgtype,
		    Char *status)
{
  short k, lifetime;
  long hl;
  Char hs[256], w[256];

/*  debug(4, 0, 112, dest); */
  *w = '\0';
  strcpy(hs, dest);
  k = strpos2(hs, "@", 1);
  if (k > 0) {
    strsub(w, hs, k + 1, strlen(hs) - k);
    cut(hs, k - 1);
  }
  del_blanks(hs);
  cut(hs, 8);
  del_blanks(w);
  cut(w, 40);
  if (*w == '\0')
    strcpy(w, ch_stat[0].mycall);

  if (!hierarchicals) {
    unhpath(w, w);
    sprintf(hs + strlen(hs), " @%s", w);
    rspacing(hs, 17);
  } else
    sprintf(hs + strlen(hs), " @%s ", w);

  strcpy(w, absender);
  k = strpos2(w, "@", 1);
  if (k > 0)   /* bei Pacsats sind auch Absender mit @bbs blich */
    cut(w, k - 1);
  del_blanks(w);
  cut(w, 6);
  sprintf(hs + strlen(hs), "de:%s ", w);
  rspacing(hs, 27);

  strcpy(status, hs);
  ix2string(rxdate, hs);
  cut(hs, 14);
  sprintf(status + strlen(status), "%s ", hs);

  lifetime = 0;
  if (expire_time > 0) {
    hl = expire_time - sys_ixtime();
    if (hl >= 86400L)
      lifetime = (hl + 86399L) / 86400L;
    else
      lifetime = 1;
  }
  if (lifetime > 999)
    lifetime = 999;
  int2str(lifetime, hs);
  lspacing(hs, 3);

  lint2str(size + 180, w);   /* 180 Bytes fr Header... */
  lspacing(w, 6);
  sprintf(hs + strlen(hs), " %s Bytes", w);

  if (hierarchicals && msgtype != '\0')
    sprintf(hs + strlen(hs), "~%c", msgtype);

  strcat(status, hs);
}

void ibcastsortnewmail(short unr,Char *pattern,Char *rcall)
{
  sort_new_mail(unr,pattern,rcall);
}

short ibcasthuffpacker(boolean encode,Char *s1,Char *s2, boolean crlfconv)
{
  short ret;
  
  if (encode)
    ret = enchuf(s1, s2, crlfconv);
  else
    ret = dechuf(s1, s2, crlfconv);
  return ret;
}

boolean ibcastcheckbid(Char *bid_string)
{
  return(true); /* BID always ok */
}

Char *ibcasttempdir(Char *result)
{
  strcpy(result,tnt_bctempdir);
  return(result);
}

Char *ibcastsavedir(Char *result)
{
  strcpy(result,tnt_bcsavedir);
  return(result);
}

Char *ibcastnewmaildir(Char *result)
{
  strcpy(result,tnt_bcnewmaildir);
  return(result);
}


void init_bcast()
{
  shpacsat_flag = 1;
  bcrequest_flag = 0;
  decbcast_flag = 1;
  gettimeofday(&tv_old,&tz_old);

  bcastswitchtobc = ibcastswitchtobc;
  bcastpoppid = ibcastpoppid;
  bcastpushpid = ibcastpushpid;
  bcastpopmonstat = ibcastpopmonstat;
  bcastpushmonstat = ibcastpushmonstat;
  bcastpopunproto = ibcastpopunproto;
  bcastpushunproto = ibcastpushunproto;
  bcastusereingabe = ibcastusereingabe;
  bcastprotokollcall = ibcastprotokollcall;
  bcastsetpid = ibcastsetpid;
  bcastsetmonstat = ibcastsetmonstat;
  bcastsetunproto = ibcastsetunproto;
  bcastsend = ibcastsend;
  bcasttncbuffer = ibcasttncbuffer;
  bcastmonchan = ibcastmonchan;
  bcastgetmycall = ibcastgetmycall;
  bcastshpacsat = ibcastshpacsat;
  bcastcreaterequests = ibcastcreaterequests;
  bcastcreatestatus = ibcastcreatestatus;
  bcastsortnewmail = ibcastsortnewmail;
  bcasthuffpacker = ibcasthuffpacker;
  bcastcheckbid = ibcastcheckbid;
  BcastTempDir = ibcasttempdir;
  BcastSaveDir = ibcastsavedir;
  BcastNewMailDir = ibcastnewmaildir;

  _bcast_init();
}

void exit_bcast()
{
  bcast_exit();
}

int analyse_bcast_body(char *buffer)
{
  int is_pacsat;
  uchar tnc;
  char bcastdec[257];
  unsigned short infosize;
  unsigned short infstart;
  
  if (!shpacsat_flag && !decbcast_flag) return(0);
  if (!head_valid) return(0);
  if (!frame_ui) return(0);
  if ((pid != 0xf0) && (pid != 0xbb) && (pid != 0xbd)) return(0);
  if ((strcmp(destcall,"QST-1") != 0) &&
      (strcmp(destcall,ch_stat[0].mycall) != 0)) return(0);
  is_pacsat = 1;
  infosize = (unsigned short)(*buffer + 1);
  if ((decbcast_flag) && (strcmp(srccall,ch_stat[0].mycall) != 0)) {
    tnc = 0;
    is_pacsat = dec_bcast((uchar)pid,tnc,srccall,destcall,vialist,
                          infosize,(uchar *)(buffer+1));
  }
  if (!is_pacsat) return(0);
  if (!shpacsat_flag) return(1); /* no display */
  infstart = decode_frameheader((uchar *)(buffer+1),infosize,bcastdec);
  if (infstart > 8) infosize -= 2; /* not for REQ-frames */
  moni_display2(bcastdec,1);
  if (infosize > infstart) {
    moni_display_len2(buffer+1+(infstart-1),infosize-(infstart-1));
  }
  return(1);
}

void bc_timing()
{
  long bcticks;
  
  gettimeofday(&tv,&tz);
  bcticks = (((tv.tv_sec - tv_old.tv_sec) * 100) +
             ((tv.tv_usec - tv_old.tv_usec) / 10000));
  if ((bcticks > 25) || (bcticks < 0)) { /* 250ms time between each call */                  
    bcast_timing();
    tv_old = tv;
  }
}

/* callback, if normal file transmitted */
void cbsf(long file_id)
{
  cmd_display(M_COMMAND,0,"Broadcast-transmission finished",1);
}

void send_bcfile(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  unsigned short file_id;
  char dstr[10];
  char BcastViaPath[256];
  int i;
  void (*cbsf_ptr)(long l);
  
  if (len == 0) {
    cmd_display(mode,channel,"Illegal filename",1);
    return;
  }
  BcastViaPath[0] = '\0';
  file_id = 0;
  for (i=0;i<strlen(str);i++) {
    crcfcs((uchar)str[i],&file_id);
  }
  dstr[0] = '\0';
  cbsf_ptr = cbsf;
  if (!bcast_file(0,0,dstr,file_id,0,str,BcastViaPath,
                  dstr,dstr,dstr,0,0,0,dstr,0,dstr,dstr,0,false,&cbsf_ptr)) {
    cmd_display(mode,channel,"Unable to broadcast",1);
    return;
  }
  cmd_display(mode,channel,OK_TEXT,1);
}

/* callback, if box file transmitted */
void cbbox(long file_id)
{
  IFACE_CMDBUF command;
  int iface;
  
  iface = conv_name_to_iface(box_socket);
  if (iface == -1) return;
  command.command = CMD_BCCALLBACK;
  command.data.file_id = file_id;
  send_command_packet_if(NO_CHANNEL,NO_USERNR,LEN_BCCALLBACK,
                         (char *)&command,iface);
                                  
}

void start_boxbcast(char *filename)
{
  BCAST_HEADINFO bh;
  int fd;
  void (*cbbox_ptr)(long l);
  int result;
  
  fd = open(filename,O_RDONLY);
  if (fd == -1) return;
  result = read(fd,&bh,sizeof(bh));
  if (result == sizeof(bh)) {
    if (bh.magic == BHMAGIC) {
      cbbox_ptr = cbbox;
      bcast_file(0,0,bh.qrg,bh.file_id,bh.file_type,bh.filename,
                 bh.address,bh.bbs_source,bh.bbs_destination,
                 bh.bbs_ax25uploader,bh.bbs_upload_time,bh.bbs_expire_time,
                 bh.bbs_compression,bh.bbs_bid,bh.bbs_msgtype,bh.bbs_title,
                 bh.bbs_fheader,bh.bodychecksum,bh.delete_after_tx,
                 &cbbox_ptr);
    }
  }
  close(fd);
  unlink(filename);
}

#define BUFSIZE 16384
int del_cr(char *filename)
{
  char *srcbuffer;
  char *destbuffer;
  char *destptr;
  int srclen;
  int destlen;
  char tmpname[80];
  int srcfd;
  int destfd;
  int result;
  char ch;
  int binpos;
  int prebin;
  int bin;
  int end;
  int error;
  int i;
  int cr;
  
  srcbuffer = (char *)malloc(BUFSIZE);
  destbuffer = (char *)malloc(BUFSIZE);
  if ((srcbuffer == NULL) || (destbuffer == NULL)) return(0);

  strcpy(tmpname,filename);
  strcat(tmpname,"XXXXXX");
  mkstemp(tmpname);
  
  srcfd = open(filename,O_RDONLY);
  if (srcfd == -1) {
    free(srcbuffer);
    free(destbuffer);
    return(0);
  }
  destfd = open(tmpname,O_RDWR|O_CREAT|O_TRUNC,0666);
  if (destfd == -1) {
    free(srcbuffer);
    free(destbuffer);
    close(srcfd);
    return(0);
  }
  end = 0;
  error = 0;
  binpos = 0;
  prebin = 0;
  bin = 0;
  cr = 0;
  while ((!end) && (!error)) {
    srclen = read(srcfd,srcbuffer,BUFSIZE);
    if (srclen == -1) error = 1;
    if (!error) {
      destlen = 0;
      destptr = destbuffer;
      if (srclen < BUFSIZE) end = 1;
      for (i=0;i<srclen;i++) {
        ch = srcbuffer[i];
        if (!bin) {
          switch (ch) {
          case '\r':
            cr = 1;
            break;
          case '\n':
            cr = 0;
            if (prebin) bin = 1;
            else binpos = 1;
            break;
          default:
            if (cr) {
              cr = 0;
              *destptr = '\n';
              destlen++;
              destptr++;
              if (prebin) bin = 1;
              else binpos = 1;
            }
            switch (binpos) {
            case 0:
              break;
            case 1:
              if (ch == '#') binpos = 2;
              else binpos = 0;
              break;
            case 2:
              if (ch == 'B') binpos = 3;
              else binpos = 0;
              break;
            case 3:
              if (ch == 'I') binpos = 4;
              else binpos = 0;
              break;
            case 4:
              if (ch == 'N') binpos = 5;
              else binpos = 0;
              break;
            case 5:
              if (ch == '#') {
                prebin = 1;
                binpos = 0;
              }
              else binpos = 0;
              break;
            default:
              binpos = 0;
              break;
            }
            break;
          }
        }
        if (!cr) {
          *destptr = ch;
          destlen++;
          destptr++;
        }
      }
      if ((cr) && (end)) {
        cr = 0;
        *destptr = '\n';
        destlen++;
        destptr++;
      }
      result = write(destfd,destbuffer,destlen);
      if ((result == -1) || (result < destlen)) error = 1;
    }
  }
  free(srcbuffer);
  free(destbuffer);
  close(srcfd);
  close(destfd);
  if (error) {
    unlink(tmpname);
    return(0);
  }
  else {
    unlink(filename);
    rename(tmpname,filename);
    return(1);
  }
}

 
#endif
