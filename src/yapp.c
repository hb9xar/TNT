/* tnt: Hostmode Terminal for TNC
  Copyright (C) 1993-97 by Mark Wahl
  For license details see documentation
  procedures for YAPP file transmit and receive (yapp.c)
  created: Mark Wahl DL4YBG 96/12/15
  updated: Mark Wahl DL4YBG 97/01/21
  updated: Matthias Hensler WS1LS 99/03/09
*/

/* source based on work by several authors, original headers follow below    */
 
/*---------------------------------------------------------------------------*/
/* yapp.c
 *
 * Copyright (C) 1994 by Jonathan Naylor
 *
 * This module implements the YAPP file transfer protocol as defined by Jeff
 * Jacobsen WA7MBL in the files yappxfer.doc and yappxfer.pas.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the license, or (at your option) any later version.
 */

/*
 * Yapp C and Resume support added by S N Henson.
 */

/*
 * Adopted for the use
 * in DPBOX by Joachim Schurig, DL8HBS 10.05.96
 * 
 * Fixed two errors of the original source
 * inhibiting resume mode on upload 16.05.96
 */
/*---------------------------------------------------------------------------*/

               
#include "tnt.h"


#define NUL             0
#define SOH             0x1
#define STX             0x2
#define ETX             0x3
#define EOT             0x4
#define ENQ             0x5
#define ACK             0x6
#define DLE             0x10
#define NAK             0x15
#define CAN             0x18

#define FC_FILE 0600
#define FO_RW O_RDWR
#define SFSEEKEND SEEK_END
#define SFSEEKSET SEEK_SET
#define FO_CREATE O_CREAT|O_RDWR|O_TRUNC
#define nohandle        (-1)   /* ungueltiges Handle */

/* external variables */
extern int file_paclen;
extern struct tx_file *tx_file;
extern struct rx_file *rx_file;
extern int tnc_channels;
extern int pty_timeout;
extern char download_dir[];
extern char yapp_dir[];
extern char remote_dir[];
extern char rem_newlin_str[];

/* external procedures */
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void rem_data_display(int channel,char *buffer);
extern long hstr2int(char *s);
extern void statlin_update();
extern char *strsub(char *ret,char *s,int pos,int len);
extern void rem_stat_display(int channel,char *buffer);
extern void del_leadblanks(char *s);
extern void del_lastblanks(char *s);

static unsigned short Make_DTime(int Hour, int Min, int Sec)
{
  return (((Hour << 11) & 0xf800) | 
          ((Min << 5) & 0x07e0) | 
          ((Sec >> 1) & 0x001f));
}

/* originally in bcastadd.c (9.3.99) */
typedef char hexarr[16];

static hexarr hextab = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
  'F'
};

#define digits          8

static void del_lead0(char *s)
{
  /*Loescht fuehrende '00' in "s"*/
  short x, k, i;
  char c;

  k = strlen(s);
  if (k <= 0)
    return;
  c = '0';
  x = 1;
  while (x <= k && s[x - 1] == c)
    x++;
  if (x > 1) {
    for (i = x; i <= k; i++)
      s[i - x] = s[i - 1];
    s[k - x + 1] = '\0';
  }
  if (*s == '\0')
    strcpy(s, "0");
}

void del_path(char *s)
{
  char *ptr;
  char *ptr1;
  char *sptr;

  ptr = s;
  ptr1 = strrchr(s,'/');
  if (ptr1 != NULL) ptr = ptr1 + 1;
  ptr1 = strrchr(s,'\\');
  if (ptr1 != NULL) ptr = ptr1 + 1;
  ptr1 = strrchr(s,':');
  if (ptr1 != NULL) ptr = ptr1 + 1;

  if (ptr != NULL) {
    sptr = s;
    while (*ptr != '\0') {
      *sptr = *ptr;
      sptr++;
      ptr++;
    }
    *sptr = '\0';
  }
}

void int2hstr(long i, char *s)
{
  /* "Integer to Hex-String", Zahl als Hex-Zahl dargestellt,"$" am Anfang */
  short ct, c;
  long v;
  char *p;

  p = hextab;
  c = 0xf;
  v = i;
  s[digits] = '\0';
  for (ct = 1; ct <= digits; ct++) {
    s[digits - ct] = p[v & c];
    v = ((unsigned long)v) >> 4;
  }
  del_lead0(s);
}

short sfcreate(char *name, short mode)
{
  return((short)open(name, FO_CREATE, mode));
}

short sfopen(char *name, short mode)
{
  return((short)open(name, mode));
}

void sfclose(short *handle)
{
  if ((*handle) != nohandle)
    close(*handle);
  *handle = nohandle;
}

void del_blanks(char *s)
{
  del_leadblanks(s);
  del_lastblanks(s);
}

/*-----*/

static unsigned short Make_DDate(int Day, int Mon, int Yr)
{
  int yh;

  if (Yr >= 1980) yh = Yr - 1980;
  else if (Yr < 80) yh = Yr + 20;   /* 2000 - 2079 */
  else yh = Yr - 80;
  return (((yh << 9) & 0xfe00) | ((Mon << 5) & 0x01e0) | Day);
}

void Get_DDate(unsigned short ddate, int *Day, int *Mon, int *Yr)
{
  *Day = ddate & 31;
  *Mon = (ddate >> 5) & 15;
  *Yr = ((ddate >> 9) & 127) + 80;
}


void Get_DTime(unsigned short dtime, int *Hour, int *Min, int *Sec)
{
  *Sec = (dtime & 31) << 1;
  *Min = (dtime >> 5) & 63;
  *Hour = (dtime >> 11) & 31;
}         

static int sfgetdatime(char *name, unsigned short *ddate, unsigned short *dtime)
{
  struct tm *timebuf;

  struct stat buf;
  if (stat(name,&buf) != 0) {
    *ddate = 0;
    *dtime = 0;
    return -1;
  }   
  timebuf = localtime(&(buf.st_mtime));
  *ddate = Make_DDate(timebuf->tm_mday, timebuf->tm_mon, timebuf->tm_year);
  *dtime = Make_DTime(timebuf->tm_hour, timebuf->tm_min, timebuf->tm_sec);
  return 0; 
}           

static int sfsetdatime(char *name, unsigned short *ddate, unsigned short *dtime)
{
  time_t ixt;   
  struct utimbuf utim;
  struct tm timebuf;

  Get_DDate(*ddate,&timebuf.tm_mday,&timebuf.tm_mon,&timebuf.tm_year);
  Get_DTime(*dtime,&timebuf.tm_hour,&timebuf.tm_min,&timebuf.tm_sec);
  ixt = mktime(&timebuf);
  utim.modtime = ixt;
  utim.actime = ixt;
  if (utime (name,&utim) >= 0) return 0;
  return (-1);
}

void yapp_tx_flush(int channel)
{
  yapptype *yapp;
  int flag;

  if (tx_file[channel].type == TX_YAPP) {
    yapp = tx_file[channel].yapp;
    if (yapp->sendbuflen) {
      flag = 0;
      queue_cmd_data(yapp->channel,X_DATA,yapp->sendbuflen,
                     flag,yapp->sendbuffer);
      yapp->sendbuflen = 0;
    }
  }
}

void yapp_rx_flush(int channel)
{
  yapptype *yapp;
  int flag;

  if (rx_file[channel].type == RX_YAPP) {
    yapp = rx_file[channel].yapp;
    if (yapp->sendbuflen) {
      flag = 0;
      queue_cmd_data(yapp->channel,X_DATA,yapp->sendbuflen,
                     flag,yapp->sendbuffer);
      yapp->sendbuflen = 0;
    }
  }
}


void yapp_timeout()
{
  int channel;
  yapptype *yapp;
  int flag;
  
  for (channel=0;channel<tnc_channels;channel++) {
    if (tx_file[channel].type == TX_YAPP) {
      yapp = tx_file[channel].yapp;
      if (yapp->sendbuflen) {
        if ((time(NULL) - yapp->bufupdate) >= pty_timeout) {
          flag = 0;
          queue_cmd_data(yapp->channel,X_DATA,yapp->sendbuflen,
                         flag,yapp->sendbuffer);
          yapp->sendbuflen = 0;
        }
      }
    }
    if (rx_file[channel].type == RX_YAPP) {
      yapp = rx_file[channel].yapp;
      if (yapp->sendbuflen) {
        if ((time(NULL) - yapp->bufupdate) >= pty_timeout) {
          flag = 0;
          queue_cmd_data(yapp->channel,X_DATA,yapp->sendbuflen,
                         flag,yapp->sendbuffer);
          yapp->sendbuflen = 0;
        }
      }
    }
  }
}

static void yapp_chout(yapptype *yapp, char ch)
{
  int flag;
  
  yapp->sendbuffer[yapp->sendbuflen] = ch;
  yapp->sendbuflen++;
  yapp->bufupdate = time(NULL);
  if (yapp->sendbuflen == file_paclen) {
    flag = 0;
    queue_cmd_data(yapp->channel,X_DATA,yapp->sendbuflen,
                   flag,yapp->sendbuffer);
    yapp->sendbuflen = 0;
  }
}

static void yapp_lineout(yapptype *yapp, char *string)
{
  char *ptr;
  
  ptr = string;
  while (*ptr != '\0') {
    yapp_chout(yapp,*ptr);
    ptr++;
  }
}

static void yapp_buffout(yapptype *yapp, char *buffer, int size)
{
  char *ptr;
  int count;
  
  ptr = buffer;
  count = size;
  while (size) {
    yapp_chout(yapp,*ptr);
    ptr++;
    size--;
  }
}

static void Write_Status(yapptype *yapp, char *s)
{
  char *ptr;
  
  ptr = (char *)malloc(strlen(s)+2*strlen(rem_newlin_str)+3);
  if (yapp->progress) {
    strcpy(ptr,rem_newlin_str);
    yapp->progress = 0;
  }
  else {
    (*ptr) = '\0';
  }
  strcat(ptr,s);
  strcat(ptr,rem_newlin_str);
  rem_data_display(yapp->channel, ptr);
  free(ptr);
}


static void Send_RR(yapptype *yapp)
{
  yapp_chout(yapp, ACK);
  yapp_chout(yapp, 1);
}


static void Send_RF(yapptype *yapp)
{
  yapp_chout(yapp, ACK);
  yapp_chout(yapp, 2);
}


static void Send_RT(yapptype *yapp)
{
  yapp_chout(yapp, ACK);
  yapp_chout(yapp, ACK);
}


static void Send_AF(yapptype *yapp)
{
  yapp_chout(yapp, ACK);
  yapp_chout(yapp, 3);
}


static void Send_AT(yapptype *yapp)
{
  yapp_chout(yapp, ACK);
  yapp_chout(yapp, 4);
}


static void Send_NR(yapptype *yapp, char *reason)
{
  yapp_chout(yapp, NAK);
  yapp_chout(yapp, strlen(reason));
  yapp_lineout(yapp, reason);
}


/* Send a Resume Sequence */

static void Send_RS(yapptype *yapp, int laenge)
{
  char buff[20];

  yapp_chout(yapp, NAK);
  sprintf(buff, "%ld", (long)laenge);
  yapp_chout(yapp, strlen(buff) + 5);
  yapp_chout(yapp, 'R');
  yapp_chout(yapp, 0);
  yapp_lineout(yapp, buff);
  yapp_chout(yapp, 0);
  yapp_chout(yapp, 'C');
  yapp_chout(yapp, 0);
}


static void Send_SI(yapptype *yapp)
{
  yapp_chout(yapp, ENQ);
  yapp_chout(yapp, 1);
}


static void Send_CN(yapptype *yapp, char *reason)
{
  yapp_chout(yapp, CAN);
  yapp_chout(yapp, strlen(reason));
  yapp_lineout(yapp, reason);
}


static void Send_HD(yapptype *yapp, char *filename, int laenge)
{
  unsigned short date, time;
  short len;
  char hs[20], w[256];
  char STR1[256];

  sprintf(hs, "%ld", (long)laenge);
  yapp_chout(yapp, SOH);
  sfgetdatime(filename, &date, &time);
  strcpy(w, filename);
  del_path(w);
  len = strlen(hs) + strlen(w) + 2;   /* include the NULs */
  if (date != 0)
    len += 9;
  yapp_chout(yapp, len);
  yapp_lineout(yapp, w);
  yapp_chout(yapp, 0);
  yapp_lineout(yapp, hs);

  yapp_chout(yapp, 0);
  if (date == 0)
    return;
  int2hstr(date, hs);
  while (strlen(hs) < 4)
    sprintf(hs, "0%s", strcpy(STR1, hs));
  yapp_lineout(yapp, hs);
  int2hstr(time, hs);
  while (strlen(hs) < 4)
    sprintf(hs, "0%s", strcpy(STR1, hs));
  yapp_lineout(yapp, hs);
  yapp_chout(yapp, 0);
}



static void Send_ET(yapptype *yapp)
{
  yapp_chout(yapp, EOT);
  yapp_chout(yapp, 1);
}


static void Send_DT(yapptype *yapp, short len)
{
  yapp_chout(yapp, STX);
  if (len > 255)
    len = 0;
  yapp_chout(yapp, len);
}


static void Send_EF(yapptype *yapp)
{
  yapp_chout(yapp, ETX);
  yapp_chout(yapp, 1);
}


static char checksum_(char *buf, short len)
{
  short i;
  char sum;

  sum = 0;
  for (i = 0; i < len; i++)
    sum += buf[i];
  return sum;
}


static int yapp_download_data(yapptype *yapp)
{
  int Result;
  char c;
  char *hptr;
  int i, x;
  char checksum;
  int seekh, len;
  char hfield[3][256];
  char hs[256];
  char STR1[256];
  int time_trans;
  int baud_trans;
  char ans_str[200];
  char reason[300];
  int reaslen;
  int abolen;
  int remain_time;
  unsigned char trans_per;

  Result = 0;
  if (yapp == NULL)
    return Result;

  if (yapp->buffer[0] == (char)CAN) {
    strcpy(reason,"RcvABORT");
    abolen = strlen(reason);
    if (yapp->buflen > 1) {
      reaslen = (int)yapp->buffer[1];
      if ((reaslen > 0) && (reaslen <= yapp->buflen - 2)) {
        strcat(reason,", Reason: ");
        abolen = strlen(reason);
        strncat(reason,&yapp->buffer[2],reaslen);
        reason[abolen+reaslen] = '\0';
      }
    } 
    Write_Status(yapp,reason);
    return Result;
  }
  if (yapp->buffer[0] == (char)NAK) {
    Write_Status(yapp, "RcvABORT");
    return Result;
  }

  switch (yapp->state) {

  case YAPPSTATE_R:
    if (yapp->buffer[0] != (char)ENQ || yapp->buffer[1] != '\001') {
      Send_CN(yapp, "Unknown code");
      Write_Status(yapp, "SndABORT");
      return Result;
    }
    Send_RR(yapp);
    Write_Status(yapp, "YAPP reception started");
    yapp->state = YAPPSTATE_RH;
    break;

  case YAPPSTATE_RH:
    if (yapp->buffer[0] == (char)SOH) {

      /* Parse header: 3 fields == YAPP C */

      len = yapp->buffer[1];
      if (len == 0)
	len = 256;

      hptr = &yapp->buffer[2];

      for (x = 0; x <= 2; x++)
	*hfield[x] = '\0';
      yapp->fdate = 0;
      yapp->ftime = 0;

      while (len > 0 && yapp->yappc < 3) {
	c = hptr[0];
	if (c == '\0')
	  yapp->yappc++;
	else
	  sprintf(hfield[yapp->yappc] + strlen(hfield[yapp->yappc]), "%c", c);
	len--;
	hptr = (char *)(&hptr[1]);
      }

      if (rx_file[yapp->channel].fd == -1 && 
          rx_file[yapp->channel].name[0] == '\0') {
	del_path(hfield[0]);
	switch (rx_file[yapp->channel].mode) {
	case M_REMOTE:
	  sprintf(STR1, "%s%s", remote_dir, hfield[0]);
	  break;
	case M_CMDSCRIPT:
	  sprintf(STR1, "%s%s", yapp_dir, hfield[0]);
	  break;
	default:
	  sprintf(STR1, "%s%s", download_dir, hfield[0]);
	  break;
	}
	strcpy(rx_file[yapp->channel].name, STR1);
	statlin_update();
      }

      yapp->filelength = atol(hfield[1]);

      if (yapp->yappc < 3)
	yapp->yappc = 0;
      else {
	yapp->yappc = 1;
	if (strlen(hfield[2]) == 8) {
	  sprintf(STR1, "%.4s", hfield[2]);
	  yapp->fdate = hstr2int(STR1);
	  yapp->ftime = hstr2int(strsub(STR1, hfield[2], 4, 4));
	}
      }

      if (rx_file[yapp->channel].fd == -1) {
	 rx_file[yapp->channel].fd =
	                    sfopen(rx_file[yapp->channel].name, FO_RW);
	if (rx_file[yapp->channel].fd == -1)
	  rx_file[yapp->channel].fd = 
	                    sfcreate(rx_file[yapp->channel].name, FC_FILE);
	if (rx_file[yapp->channel].fd == -1) {
	  sprintf(STR1, "Unable to open %s", rx_file[yapp->channel].name);
	  Write_Status(yapp, STR1);
	  Send_NR(yapp, "Invalid filename");
	  return Result;
	}
      }

      sprintf(hs, "Receiving %s", hfield[0]);
      if (yapp->filelength > 0)
	sprintf(hs + strlen(hs), " (%s bytes)", hfield[1]);

      strcat(hs, ", mode = YAPP");

      if (yapp->yappc > 0)
	strcat(hs, "C");

      del_blanks(hs);
      Write_Status(yapp, hs);

      if (yapp->yappc > 0) {
	seekh = lseek(rx_file[yapp->channel].fd, 0, SFSEEKEND);
	if (seekh > 0) {
	  Send_RS(yapp, seekh);
	  yapp->startval = seekh;
	  yapp->curval = seekh;
	}
	else
	  Send_RT(yapp);
      } else
	Send_RF(yapp);

      yapp->state = YAPPSTATE_RD;
      rx_file[yapp->channel].start_time = time(NULL);
    } else {
      if (yapp->buffer[0] != (char)ENQ || yapp->buffer[1] != '\001') {
	if (yapp->buffer[0] == (char)EOT && yapp->buffer[1] == '\001') {
	  Send_AT(yapp);
	  Write_Status(yapp, "YAPP reception ended");
	  return Result;
	}

	Send_CN(yapp, "Unknown code");
	Write_Status(yapp, "SndABORT");
	return Result;
      }
    }

    break;

  case YAPPSTATE_RD:
    if (yapp->buffer[0] == (char)STX) {
      len = yapp->buffer[1];
      if (len == 0)
	len = 256;
      yapp->total += len;
      if (yapp->yappc != 0) {
	checksum = 0;
	for (i = 2; i <= len + 1; i++)
	  checksum += yapp->buffer[i];
	if (checksum != yapp->buffer[len + 2]) {
	  Send_CN(yapp, "Bad Checksum");
	  Write_Status(yapp, "SndABORT: Bad Checksum");
	  return Result;
	}
      }

      write(rx_file[yapp->channel].fd, &yapp->buffer[2], len);
      yapp->seekpos += len;
      yapp->curval += len;

      yapp->progress = 1;
      time_trans = (int) (time(NULL) - rx_file[yapp->channel].start_time);
      if (time_trans < 1) time_trans = 1;
      baud_trans = (yapp->curval - yapp->startval) * 8 /time_trans;
      if (yapp->filelength) {
        if(baud_trans < 1) baud_trans=1;
        remain_time = (yapp->filelength - yapp->curval) /
                      (baud_trans * 7.5 );
        trans_per = (( yapp->curval * 100) / yapp->filelength);
        sprintf(ans_str,"RX: %d/%d [%d%%], %d Baud, remaining: %d minute",
                yapp->curval, yapp->filelength,
                trans_per, baud_trans, remain_time);
        if(remain_time!=1) strcat(ans_str,"s");
      }
      else {
        sprintf(ans_str,"RX: %d, %d Baud",yapp->curval,baud_trans);
      }
      rem_stat_display(yapp->channel,ans_str);

    } else if (yapp->buffer[0] == (char)ETX && yapp->buffer[1] == '\001') {
      Send_AF(yapp);
      Write_Status(yapp, "RcvEof");
      yapp->state = YAPPSTATE_RH;
      sfclose((short *)&rx_file[yapp->channel].fd);
      if (yapp->fdate != 0)
	sfsetdatime(rx_file[yapp->channel].name, &yapp->fdate, &yapp->ftime);
      yapp->delete = 0;
    } else {
      Send_CN(yapp, "Unknown code");
      Write_Status(yapp, "SndABORT");
      return Result;
    }

    break;

  default:
    Send_CN(yapp, "Unknown state");
    Write_Status(yapp, "SndABORT");
    return Result;
    break;
  }
  return 1;
}


int yapp_download(int init, int abort, yapptype *yapp,
		      char *buffp, int blen)
{
  int Result;
  int len, bminus;
  int used;

  Result = 0;
  if (yapp == NULL)
    return Result;

  bminus = 0;

  if (init) {
    yapp->state = YAPPSTATE_R;
  } else if (abort) {
    Send_CN(yapp, "Cancelled");
    Write_Status(yapp, "SndABORT");
    return Result;
  }


  if (blen <= 0)
    return (bminus == 0);


  if (blen + yapp->buflen > 1024)
    bminus = 1024 - yapp->buflen;

  memcpy(&yapp->buffer[yapp->buflen], buffp, blen - bminus);
  yapp->buflen += blen - bminus;

  do {
    used = 0;

    switch (yapp->buffer[0]) {

    case ACK:
    case ENQ:
    case ETX:
    case EOT:
      if (yapp->buflen >= 2) {
	if (!yapp_download_data(yapp))
	  return Result;
	yapp->buflen -= 2;
	memcpy(yapp->buffer, &yapp->buffer[2], yapp->buflen);
	used = 1;
      }
      break;

    default:
      len = yapp->buffer[1];
      if (len == 0)
	len = 256;
      if (yapp->buffer[0] == (char)STX)
	len += yapp->yappc;
      if (yapp->buflen >= len + 2) {
	if (!yapp_download_data(yapp))
	  return Result;
	yapp->buflen += -len - 2;
	memcpy(yapp->buffer, &yapp->buffer[len + 2], yapp->buflen);
	used = 1;
      }
      break;
    }
  } while (used);

  return (bminus == 0);
}


static int yapp_upload_data(yapptype *yapp)
{
  int Result;
  int len, x;
  char w[256];
  char reason[300];
  int reaslen;
  int abolen;

  Result = 0;

  if (yapp == NULL)
    return Result;


  if (yapp->buffer[0] == (char)CAN) {
    strcpy(reason,"RcvABORT");
    abolen = strlen(reason);
    if (yapp->buflen > 1) {
      reaslen = (int)yapp->buffer[1];
      if ((reaslen > 0) && (reaslen <= yapp->buflen - 2)) {
        strcat(reason,", Reason: ");
        abolen = strlen(reason);
        strncat(reason,&yapp->buffer[2],reaslen);
        reason[abolen+reaslen] = '\0';
      }
    } 
    Write_Status(yapp,reason);
    return Result;
  }
  if (yapp->buffer[0] == (char)NAK &&
      (yapp->buffer[1] <= '\003' || yapp->buffer[2] != 'R' ||
       yapp->buffer[3] != '\0')) {
    Write_Status(yapp, "RcvABORT");
    return Result;
  }

  switch (yapp->state) {

  case YAPPSTATE_S:
    if (yapp->buffer[0] == (char)ACK && yapp->buffer[1] == '\001') {
      Write_Status(yapp, "SendHeader");
      Send_HD(yapp, tx_file[yapp->channel].name, yapp->filelength);
      yapp->state = YAPPSTATE_SH;
    } else if (yapp->buffer[0] == (char)ACK && yapp->buffer[1] == '\002') {
      yapp->outlen = read(tx_file[yapp->channel].fd, yapp->outbuffer, 255);
      yapp->curval += yapp->outlen;
      yapp->outbufptr = 0;
      if (yapp->outlen > 0)
	Send_DT(yapp, yapp->outlen);
      if (yapp->yappc != 0) {
	yapp->outbuffer[yapp->outlen] = checksum_(yapp->outbuffer, yapp->outlen);
	yapp->outlen++;
      }
      yapp->state = YAPPSTATE_SD;
      tx_file[yapp->channel].start_time = time(NULL);
    } else {
      Send_CN(yapp, "Unknown code");
      Write_Status(yapp, "SndABORT");
      return Result;
    }

    break;

  case YAPPSTATE_SH:
    /* Could get three replies here:
     * ACK 02 : normal acknowledge.
     * ACK ACK: yappc acknowledge.
     * NAK ...: resume request.
     */
    if (yapp->buffer[0] == (char)NAK && yapp->buffer[2] == 'R') {
      len = yapp->buffer[1];
      if (yapp->buffer[len] == 'C')
	yapp->yappc = 1;

      w[0] = '\0';
      x = 4;
      while (x <= len && isdigit(yapp->buffer[x])) {
	sprintf(w + strlen(w), "%c", yapp->buffer[x]);
	x++;
      }

      yapp->seekpos = atol(w);

      if (lseek(tx_file[yapp->channel].fd, yapp->seekpos, SFSEEKSET) != yapp->seekpos) {
	Send_CN(yapp, "Invalid resume position");
	Write_Status(yapp, "Invalid resume position");
	Write_Status(yapp, "SndABORT");
	return Result;
      }
      
      yapp->startval = yapp->seekpos;
      yapp->curval = yapp->seekpos;

      yapp->buffer[0] = (char)ACK;

      if (yapp->yappc != 0)
	yapp->buffer[1] = (char)ACK;
      else
	yapp->buffer[1] = '\002';
    }

    if (yapp->buffer[0] != (char)ACK ||
	(yapp->buffer[1] != '\002' && yapp->buffer[1] != (char)ACK)) {
      Send_CN(yapp, "Unknown code");
      Write_Status(yapp, "SndABORT");
      return Result;
    }

    if (yapp->buffer[1] == (char)ACK)
      yapp->yappc = 1;

    if (yapp->yappc == 1)
      Write_Status(yapp, "mode = YAPPC");
    else
      Write_Status(yapp, "mode = YAPP");

    yapp->outlen = read(tx_file[yapp->channel].fd, yapp->outbuffer, 255);
    yapp->curval += yapp->outlen;

    yapp->outbufptr = 0;
    if (yapp->outlen > 0) {
      yapp->seekpos += yapp->outlen;
      Send_DT(yapp, yapp->outlen);
      if (yapp->yappc != 0) {
	yapp->outbuffer[yapp->outlen] = checksum_(yapp->outbuffer, yapp->outlen);
	yapp->outlen++;
      }
    }
    yapp->state = YAPPSTATE_SD;
    tx_file[yapp->channel].start_time = time(NULL);
    break;

  case YAPPSTATE_SD:
    Send_CN(yapp, "Unknown code");
    Write_Status(yapp, "SndABORT");
    return Result;
    break;

  case YAPPSTATE_SE:
    if (yapp->buffer[0] != (char)ACK || yapp->buffer[1] != '\003') {
      Send_CN(yapp, "Unknown code");
      Write_Status(yapp, "SndABORT");
      return Result;
    }

    Write_Status(yapp, "YAPP transmission ended");
    Send_ET(yapp);
    yapp->state = YAPPSTATE_ST;
    break;

  case YAPPSTATE_ST:
    if (yapp->buffer[0] == (char)ACK && yapp->buffer[1] == '\004')
      return Result;
    else {
      Send_CN(yapp, "Unknown code");
      Write_Status(yapp, "SndABORT");
      return Result;
    }
    break;
  }
  return 1;
}


int yapp_upload(int init, int abort, yapptype *yapp, char *buffp,
		    int blen)
{
  int Result;
  int bminus, len, fill;
  int used;
  int time_trans;
  int baud_trans;
  char ans_str[200];
  int remain_time;
  unsigned char trans_per;

  Result = 0;
  if (yapp == NULL)
    return Result;

  bminus = 0;

  if (init) {
    Write_Status(yapp, "YAPP transmission started");
    yapp->state = YAPPSTATE_S;
    Send_SI(yapp);
    return 1;
  }

  if (abort) {
    Write_Status(yapp, "SndABORT");
    Send_CN(yapp, "Cancelled by user");
    return Result;
  }

  if (blen > 0) {
    if (blen + yapp->buflen > 1024)
      bminus = 1024 - yapp->buflen;
    else
      bminus = 0;

    memcpy(&yapp->buffer[yapp->buflen], buffp, blen - bminus);
    yapp->buflen += blen - bminus;

    do {
      used = 0;

      switch (yapp->buffer[0]) {

      case ACK:
      case ENQ:
      case ETX:
      case EOT:
	if (yapp->buflen >= 2) {
	  if (!yapp_upload_data(yapp))
	    return Result;

	  yapp->buflen -= 2;
	  memcpy(yapp->buffer, &yapp->buffer[2], yapp->buflen);
	  used = 1;
	}
	break;

      default:
	len = yapp->buffer[1];
	if (len == 0)
	  len = 256;

	if (yapp->buflen >= len + 2) {
	  if (!yapp_upload_data(yapp))
	    return Result;

	  yapp->buflen += -len - 2;
	  memcpy(yapp->buffer, &yapp->buffer[len + 2], yapp->buflen);
	  used = 1;
	}

	break;
      }

    } while (used);
  }


  fill = 0;
  while (fill < yapp->maxfill && yapp->state == YAPPSTATE_SD) {
    if (fill < yapp->maxfill && yapp->outlen == 0) {
      yapp->total -= yapp->yappc;

      yapp->outlen = read(tx_file[yapp->channel].fd, yapp->outbuffer, 255);
      yapp->curval += yapp->outlen;

      if (yapp->outlen > 0) {
	yapp->seekpos += yapp->outlen;
	yapp->outbufptr = 0;
	Send_DT(yapp, yapp->outlen);
	if (yapp->yappc != 0) {
	  yapp->outbuffer[yapp->outlen] = checksum_(yapp->outbuffer,
						    yapp->outlen);
	  yapp->outlen++;
	}
      } else {
	Write_Status(yapp, "SendEof");
	yapp->state = YAPPSTATE_SE;
	Send_EF(yapp);
      }
    }

    if (yapp->outlen <= 0)
      continue;
    yapp_buffout(yapp, &yapp->outbuffer[yapp->outbufptr], yapp->outlen);
    yapp->outbufptr += yapp->outlen;
    yapp->total += yapp->outlen;
    fill += yapp->outlen;
    yapp->outlen = 0;
    
    yapp->progress = 1;
    time_trans = (int) (time(NULL) - tx_file[yapp->channel].start_time);
    if (time_trans < 1) time_trans = 1;
    baud_trans = (yapp->curval - yapp->startval) * 8 /time_trans;
    if(baud_trans < 1) baud_trans=1;
    remain_time = (yapp->filelength - yapp->curval) / ( baud_trans * 7.5);
    trans_per = (( yapp->curval * 100 ) / yapp->filelength );
    sprintf(ans_str,"TX: %d/%d [%d%%], %d Baud, remaining: %d minute",
            yapp->curval, yapp->filelength,
            trans_per, baud_trans, remain_time);
    if(remain_time!=1) strcat(ans_str,"s");
    rem_stat_display(yapp->channel,ans_str);
  }
  
  return (bminus == 0);
}

