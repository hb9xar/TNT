/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Procedures for monitor boxfill (monbox.c)
   -----------------------------------------------------------------------
   Most of the procedures in this file are translated by
   * p2c, the Pascal-to-C translator *
   from the original DigiPoint sources in Pascal.
   DigiPoint is Copyright (C) 1992,1994 by Joachim Schurig, DL8HBS.
   Code used by kind permission of the author.
   -----------------------------------------------------------------------
   created: Mark Wahl DL4YBG 94/07/22
   updated: Mark Wahl DL4YBG 96/12/14
*/

#include "tnt.h"
#ifdef USE_IFACE
#include "ifacedef.h"
#include "iface.h"
#include "monbox.h"


extern void strip_call_log(char *call,int channel);
extern int conv_name_to_iface(char *str);
extern void send_command_packet_if(short channel,short usernr,
                                   unsigned short len,char *buf,int iface);
extern void get_call_xc(int channel,char *call);
extern void close_monbox(int xmon_ch);
extern int open_monbox(int monbox_channel,int chksum);
extern void fill_moni_call(char *srccall,char *destcall);
extern int own_connection(char *call1,char *call2);
extern void cmd_display(int flag,int channel,char *buffer,int cr);
extern void fill_xmon_call(int channel,char *srccall,char *destcall);
extern int find_free_channel();
extern int queue_cmd_data(int channel,char code,int len,int flag,char *data);
extern void update_owncall(int channel,char *call);
extern void cmd_xconnect(int par1,int par2,int channel,
                         int len,int mode,char *str);


extern int tnc_channels;
extern struct channel_stat *ch_stat;
extern char newmaildir[];
extern char box_socket[];
extern char autobox_dir[];
extern char tnt_boxender[];
extern char f6fbb_box[];
extern char ok_text[];
extern int COLS;

#define SCANMBEA_TIMEOUT 300
int scanmbea_flag;
int scanmbea_valid;
int scanmbea_start;
static time_t scanmbea_time;
static char scan_srccall[10];
static int scan_srcsum;
static char scan_destcall[10];
static int scan_destsum;
static char scan_owncall[7];
static char scan_concall[10];
static int scan_timeout;

char monbox_not_act_txt[] = "No channel active";
static char dash_str[] =
"-------------------------------------------------------------------------------"
"-------------------------------------------------------------------------------";
static boolean user_moncut;
static boolean bulletin_moncut;
static char fbb_fromfield[21];
static char fbb_tofield[21];
static char fbb314_fromfield[21];
static char fbb314_tofield[21];
static char dpreadoutmagic[] = "#DPRDOUT66521A#";
static short erasedelay;
int monbox_flag;
int autobox_flag;
extern int box_active_flag;
extern int box_busy_flag;

static struct monbox_info monbox_info[MAX_MONBOX];

static struct multiboxlist multiboxlist[MAX_MULTIBOXLIST];
static int multiboxlist_entries;

static char otherbender1[4][81];
static char otherbender2[4][81];

static struct boxcut_info *bcut_info;

static cutboxtyp boxheader(Char *zeile);
void upper(Char *s);


static boolean badname(dummy)
char *dummy;
{
  return(false);
}

static boolean check_double(dummy)
char *dummy;
{
  return(true);
}

static void del_callextender(channel,call2)
int channel;
char *call2;
{
  char call[10];
  int i;
  int end;
  
  strip_call_log(call,channel);
  i = 0;
  end = 0;
  do {
    if ((call[i] == '-') || (call[i] == '\0')) {
      call2[i] = '\0';
      end = 1;
    }
    else {
      call2[i] = call[i];
    }
    i++;
  } while ((i <= 9) && !end);
}

/* send via interface to box */
void sort_new_mail(short unr, Char *pattern_, Char *rcall)
{
  int iface;
  int len;
  IFACE_CMDBUF command;
  
  iface = conv_name_to_iface(box_socket);
  if (iface == -1) return;
  /* fill command */
  command.command = CMD_SORT_NEW_MAIL;
  len = LEN_SIMPLE;
  /* fill pattern_ */
  strcpy(command.data.buffer,pattern_);
  len += strlen(pattern_);
  *(command.data.buffer + len - LEN_SIMPLE) = '\0';
  len++;
  /* fill rcall */
  strcpy(command.data.buffer + len - LEN_SIMPLE ,rcall);
  len += strlen(pattern_);
  *(command.data.buffer + len - LEN_SIMPLE) = '\0';
  len++;
  /* now send command */
  send_command_packet_if(NO_CHANNEL,unr,len,(char *)&command,iface);
}

static void sf_rx_emt(short unr, Char *eingabe_)
{
  int iface;
  int len;
  IFACE_CMDBUF command;

  iface = conv_name_to_iface(box_socket);
  if (iface == -1) return;
  /* fill command */
  command.command = CMD_SF_RX_EMT;
  len = LEN_SIMPLE;
  /* fill eingabe_ */
  strcpy(command.data.buffer,eingabe_);
  len += strlen(eingabe_);
  *(command.data.buffer + len - LEN_SIMPLE) = '\0';
  len++;
  /* now send command */
  send_command_packet_if(NO_CHANNEL,unr,len,(char *)&command,iface);
}

void abort_routing(char *call)
{
  int iface;
  int len;
  IFACE_CMDBUF command;
  
  iface = conv_name_to_iface(box_socket);
  if (iface == -1) return;
  /* fill command */
  command.command = CMD_ABORT_ROUTING;
  len = LEN_SIMPLE;
  /* fill call */
  strcpy(command.data.buffer,call);
  len += strlen(call);
  *(command.data.buffer + len - LEN_SIMPLE) = '\0';
  len++;
  /* now send command */
  send_command_packet_if(NO_CHANNEL,NO_USERNR,len,(char *)&command,iface);
}

void start_sf(channel,iface)
int channel;
int iface;
{
  int len;
  char call[257];
  char *ptr;
  IFACE_CMDBUF command;
  
  /* fill command */
  command.command = CMD_START_SF;
  len = LEN_SIMPLE;
  /* fill callsign */
  get_call_xc(channel,call);
  if ((ptr = strchr(call,'-')) != NULL)
    *ptr = '\0';
  strcpy(command.data.buffer,call);
  len += strlen(call);
  *(command.data.buffer + len - LEN_SIMPLE) = '\0';
  len++;
  send_command_packet_if((short)channel,NO_USERNR,len,(char *)&command,iface);
}

/****************
* from p2clib.c *
****************/

/* Return the index of the first occurrence of "pat" as a substring of "s",
   starting at index "pos" (1-based).  Result is 1-based, 0 if not found. */

int strpos2(s, pat, pos)
char *s;
register char *pat;
register int pos;
{
    register char *cp, ch;
    register int slen;

    if (--pos < 0)
        return 0;
    slen = strlen(s) - pos;
    cp = s + pos;
    if (!(ch = *pat++))
        return 0;
    pos = strlen(pat);
    slen -= pos;
    while (--slen >= 0) {
        if (*cp++ == ch && !strncmp(cp, pat, pos))
            return cp - s;
    }
    return 0;
}

/* Delete the substring of length "len" at index "pos" from "s".
   Delete less if out-of-range. */

void strdelete(s, pos, len)
register char *s;
register int pos, len;
{
    register int slen;

    if (--pos < 0)
        return;
    slen = strlen(s) - pos;
    if (slen <= 0)
        return;
    s += pos;
    if (slen <= len) {
        *s = 0;
        return;
    }
    while ((*s = s[len])) s++;
}

/* Store in "ret" the substring of length "len" starting from "pos" (1-based).
   Store a shorter or null string if out-of-range.  Return "ret". */

char *strsub(ret, s, pos, len)
register char *ret, *s;
register int pos, len;
{
    register char *s2;

    if (--pos < 0 || len <= 0) {
        *ret = 0;
        return ret;
    }
    while (pos > 0) {
        if (!*s++) {
            *ret = 0;
            return ret;
        }
        pos--;
    }
    s2 = ret;
    while (--len >= 0) {
        if (!(*s2++ = *s++))
            return ret;
    }
    *s2 = 0;
    return ret;
}

/* Insert string "src" at index "pos" of "dst". */

void strinsert(src, dst, pos)
register char *src, *dst;
register int pos;
{
    register int slen, dlen;

    if (--pos < 0)
        return;
    dlen = strlen(dst);
    dst += dlen;
    dlen -= pos;
    if (dlen <= 0) {
        strcpy(dst, src);
        return;
    }
    slen = strlen(src);
    do {
        dst[slen] = *dst;
        --dst;
    } while (--dlen >= 0);
    dst++;
    while (--slen >= 0)
        *dst++ = *src++;
}

/*************
* from tools *
*************/

Static void create_pattern(Char *txt, Char *ps)
{
  short x, i;
  Char c;

  i = strlen(txt);
  ps[i] = '\0';
/* p2c: tools.p, line 212:
 * Note: Modification of string length may translate incorrectly [146] */

  for (x = 0; x < i; x++) {
    c = txt[x];
    if (isupper(c))
      ps[x] = 'A';
    else if (isdigit(c))
      ps[x] = 'N';
    else if (islower(c))
      ps[x] = 'a';
    else if (c == '-')
      ps[x] = '-';
    else if (c == '/')
      ps[x] = '/';
    else
      ps[x] = '#';
  }

}


Static boolean compare_pattern(Char *ps, Char *pattern)
{
  boolean Result;
  short x, l1, l2;
  Char c;

  Result = false;
  x = 1;
  l1 = strlen(pattern);
  l2 = strlen(ps);
  c = '*';

  while (x <= l1 && x <= l2) {
    if (pattern[x - 1] == c) {
      x = 1000;
      break;
    }

    if (pattern[x - 1] == ps[x - 1])
      x++;
    else
      x = 500;
  }


  if (x == 500)
    return false;
  else if (x == 1000)
    return true;
  else if (x > l1 && pattern[l1 - 1] != c)
    return false;
  else if (x > l2 && ((x == l1 && pattern[x - 1] != c) || x < l1))
    return false;
  else
    return true;

}


boolean callsign(Char *rubrik)
{
  Char ps[257];
  Char cs[7];
  boolean ret;

  ret = false;
  if ((unsigned long)strlen(rubrik) >= 32 ||
      ((1L << strlen(rubrik)) & 0x70) == 0)
    return ret;
/* p2c: tools.p, line 262: Note: Unusual use of STRLEN encountered [142] */
  strcpy(cs, rubrik);
/* p2c: tools.p, line 263:
 * Note: Possible string truncation in assignment [145] */
  upper(cs);
  create_pattern(cs, ps);
  if (compare_pattern(ps, "NANA*")) {
    ret = true;
    return ret;
  }
  if (compare_pattern(ps, "AANA*")) {
    ret = true;
    return ret;
  }
  if (compare_pattern(ps, "ANAA*")) {
    ret = true;
    return ret;
  }
  if (compare_pattern(ps, "ANNA*")) {
    ret = true;
    return ret;
  }
  if (strpos2(cs, "REQ", 1) == 1) {
    ret = true;   /*FÅr REQDIR usw.*/
    return ret;
  }
  if (!strcmp(cs, "WP")) {
    ret = true;
    return ret;
  }
  if (!strcmp(cs, "7PSERV"))
    ret = true;
  else if (!strcmp(cs, "AUTO7P"))
    ret = true;
  return ret;
}

/***************
* from pastrix *
***************/

boolean exist(Char *name)
{
/* if exist(Filename) then ... */
  struct stat buf;
  
  if (stat(name,&buf) != 0) {
    return 0;
  }
  return 1;
/*
  return (getmyfsfirst(name));
*/
}
                 
void cut(Char *s, short newlength)
{
  if (strlen(s) > newlength) {
    s[newlength] = '\0';
/* p2c: pastrix.p, line 904:
 * Note: Modification of string length may translate incorrectly [146] */
  }
}

Char upcase_(Char ch)
{
  /* ist zwar in TP schon drin, aber ohne Umlaute */
  Char Result;

  switch (ch) {

  case 132:
/* p2c: pastrix.p, line 922: Note: Character >= 128 encountered [281] */
    Result = 142;
/* p2c: pastrix.p, line 922: Note: Character >= 128 encountered [281] */
    break;

  case 148:
/* p2c: pastrix.p, line 923: Note: Character >= 128 encountered [281] */
    Result = 153;
/* p2c: pastrix.p, line 923: Note: Character >= 128 encountered [281] */
    break;

  case 129:
/* p2c: pastrix.p, line 924: Note: Character >= 128 encountered [281] */
    Result = 154;
/* p2c: pastrix.p, line 924: Note: Character >= 128 encountered [281] */
    break;

  default:
    if (islower(ch))
      Result = ch - 0x20;
    else
      Result = ch;
    break;
  }
  return Result;
}

void upper(Char *s)
{
  short x, FORLIM;

  FORLIM = strlen(s);
  for (x = 0; x < FORLIM; x++)
    s[x] = upcase_(s[x]);
}

/* lokal */
Static short makehexdigit(Char c)
{
  short Result;

  switch (c) {

  case 'A':
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'F':
    Result = c - 55;
    break;

  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
    Result = c - 87;
    break;

  default:
    if (isdigit(c))
      Result = c - '0';
    else
      Result = 0;
    break;
  }
  return Result;
}

long hstr2int(Char *s)
{
  /* "Hex-String to Integer", String wird unbedingt als Hex-Zahl interpretiert */
  long erg;
  short ct, start, FORLIM;

  erg = 0;
  if (s[0] == '$')
    start = 2;
  else
    start = 1;
  FORLIM = strlen(s);
  for (ct = start - 1; ct < FORLIM; ct++)
    erg = (erg << 4) + makehexdigit(s[ct]);
  return erg;
}


long bstr2int(Char *s)
{
  /* "Bin-String to Integer", String wird unbedingt als Bin-Zahl interpretiert */
  long erg;
  short ct, start, FORLIM;

  erg = 0;
  if (s[0] == '%')
    start = 2;
  else
    start = 1;
  FORLIM = strlen(s);
  for (ct = start - 1; ct < FORLIM; ct++) {
    erg <<= 1;
    if (s[ct] != '0')
      erg++;
  }
  return erg;
}


long Str2int(Char *s)
{
  /* "String to Integer", beginnt der String mit "$", -> Hex-Zahl, "%", -> Bin-Zahl */
  long erg;
  short d;

  if (s[0] == '%') {
    erg = bstr2int(s);
    return erg;
  }
  if (s[0] == '$') {
    erg = hstr2int(s);
    return erg;
  }
  d = (sscanf(s, "%ld", &erg) == 0);
  if (d == 1)
    erg = 0;
  return erg;
}

void int2str(long i, Char *s)
{
  /* "Integer to String"   */
  sprintf(s, "%ld", i);
}

/*fuegt eine Zeile an eine bereits geoeffnete Datei an*/

void string_to_file(short *handle, Char *line_, boolean crlf)
{
  Char line[257];

  static uchar crlfarr[1] = {
    10
  };

  bst err;

  strcpy(line, line_);
  if (*handle == -1)
    return;
  err = write(*handle, line, strlen(line));
  if (err != strlen(line)) {
    close(*handle);
    *handle = -1;
  }
  else if (crlf)
    write(*handle, crlfarr, 1);
}

/* Holt nÑchstes Wort aus "inp" und lîscht */
/* dieses dort, Leerzeichen werden Åberlesen  */

void get_word(Char *inp, Char *outp)
{
  short delct, start, len, outct;
  Char c;

  outct = 0;
  len = strlen(inp);
  if (len > 0) {
    start = 1;

    while (start <= len && (inp[start - 1] == '\b' || inp[start - 1] == ' '))
      start++;

    if (start <= len) {
      c = inp[start - 1];
      while (start <= len && c != '\b' && c != ' ') {
	outct++;
	outp[outct - 1] = c;
	start++;
	c = inp[start - 1];
      }

      if (start <= len) {
	while (start <= len &&
	       (inp[start - 1] == '\b' || inp[start - 1] == ' '))
	  start++;
      }
    }

    delct = start - 1;
    if (delct > 0) {
      if (delct >= len)
	inp[0] = '\0';
      else
	strdelete((void *)inp, 1, delct);
    }

  }

  outp[outct] = '\0';
/* p2c: pastrix.p, line 1278:
 * Note: Modification of string length may translate incorrectly [146] */
}

void del_leadblanks(Char *s)
{
  /*Lîscht fÅhrende Leerzeichen in "s"*/
  short x, k, i;
  Char c;

  c = ' ';
  k = strlen(s);
  x = 1;
  while (x <= k && s[x - 1] == c)
    x++;
  if (x <= 1)
    return;
  for (i = x; i <= k; i++)
    s[i - x] = s[i - 1];
  s[k - x + 1] = '\0';
/* p2c: pastrix.p, line 856:
 * Note: Modification of string length may translate incorrectly [146] */
}

void del_lastblanks(Char *s)
{
  /*Lîscht letzte Leerzeichen*/
  short k;
  Char c;

  c = ' ';
  k = strlen(s);
  while (k > 0 && s[k - 1] == c)
    k--;
  s[k] = '\0';
/* p2c: pastrix.p, line 867:
 * Note: Modification of string length may translate incorrectly [146] */
}

short count_words(Char *s)
{
  short erg, ct;
  boolean space;
  Char spc;
  short FORLIM;

  erg = 0;
  space = true;
  spc = ' ';

  FORLIM = strlen(s);
  for (ct = 0; ct < FORLIM; ct++) {
    if (s[ct] == spc)
      space = true;
    else {
      if (space)
	erg++;
      space = false;
    }
  }

  return erg;
}

long sfsize(Char *name)
{
struct stat buf;

  if (stat(name,&buf) != 0) {
    return 0;
  }
  return buf.st_size;
}


/* haengt ein File an ein bereits geoeffnetes an */

void app_file(Char *filea, short k2, boolean del_source)
{
  uchar *puffer;
  bst psize;
  long done, bc, cstep, hli, fsize;
  short k1;

  psize = 10000000; /* 10 Meg available :-) */
  fsize = sfsize(filea);
  if (psize > fsize)
    psize = fsize;
  puffer = (uchar *)malloc(psize);
  done = 0;
  hli = 0;
  k1 = open(filea, 0);
  if (k1 != -1) {
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
    }
    close(k1);
    if (hli == fsize && done == fsize && del_source)
      unlink(filea);
  }
  free(puffer);
}

/****************
* from packetl1 *
****************/

Static boolean sfboxheader(Char *zeile)
{
  boolean Result;
  short p1, p2, p3, p4;
  Char w1[21], w2[21];
  Char hzeile[257];
  short TEMP;
  Char STR5[257];

  Result = false;
  if (strlen(zeile) <= 6)
    return Result;
  if (zeile[0] != 'S')
    return Result;
  if (zeile[1] != ' ' && zeile[2] != ' ')
    return Result;
  if (strpos2(zeile, "<", 1) <= 3)
    return Result;
  TEMP = count_words(zeile);
  if (!((unsigned)TEMP < 32 && ((1L << TEMP) & 0x7f0) != 0))
    return Result;
  p1 = strpos2(zeile, "@", 1);
  p2 = strpos2(zeile, "<", 1);
  p3 = strpos2(zeile, "$", 1);
  p4 = strpos2(zeile, "#", 1);
  if (p3 == 0)
    p3 = p2 + 1;
  if (p4 == 0)
    p4 = p3 + 1;
  if (p1 >= p2 || p2 >= p3 || p3 >= p4)
    return Result;
  strcpy(hzeile, zeile);
  upper(hzeile);
  get_word(hzeile, w1);   /*S*/
  get_word(hzeile, w2);   /*Rubrik*/
  if (strlen(w2) != 1)
    return true;
  if (strcmp(w2, "M") && strcmp(w2, "E"))
    return true;

  /* S M @ THEBOX < DH4FBC $BID MYBBS TIME CTRL-Z  */
  /* S E @ THEBOX < DB0HAG $BID ERASEBID CTRL-Z    */

  if (count_words(hzeile) <= 5)
    return true;
  if (bulletin_moncut) {
    sprintf(hzeile, "%s %s", w2, strcpy(STR5, hzeile));
    sf_rx_emt(0, hzeile);
  }
  return Result;
}

/***************
* from box_sub *
***************/

void separate_status(Char *status_, Char *ziel, Char *absender, Char *mbx,
		     Char *datum, Char *zeit, Char *laenge, Char *lifetime)
{
  Char status[257];
  Char hs[257];
  cutboxtyp typ;
  short x, TEMP;
  Char STR5[257];

  strcpy(status, status_);
  typ = boxheader(status);
  *ziel = '\0';
  *absender = '\0';
  *mbx = '\0';
  *datum = '\0';
  *zeit = '\0';
  *laenge = '\0';
  *lifetime = '\0';

  if (((1L << ((long)typ)) &
       ((1L << ((long)THEBOX_USER)) | (1L << ((long)NOP)))) != 0) {
    if (count_words(status) != 8)
      return;
    get_word(status, hs);
    cut(hs, 8);
    strcpy(ziel, hs);
/* p2c: box_sub.p, line 491:
 * Note: Possible string truncation in assignment [145] */
    upper(ziel);
    get_word(status, hs);
    if (hs[0] == '@')
      strdelete((void *)hs, 1, 1);
    cut(hs, 40);
    strcpy(mbx, hs);
/* p2c: box_sub.p, line 496:
 * Note: Possible string truncation in assignment [145] */
    upper(mbx);
    get_word(status, hs);
    if (strpos2(hs, "de:", 1) == 1)
      strdelete((void *)hs, 1, 3);
    cut(hs, 6);
    strcpy(absender, hs);
/* p2c: box_sub.p, line 501:
 * Note: Possible string truncation in assignment [145] */
    upper(absender);
    get_word(status, hs);
    cut(hs, 8);
    strcpy(datum, hs);
    get_word(status, hs);
    cut(hs, 5);
    strcpy(zeit, hs);
    get_word(status, hs);
    cut(hs, 3);
    strcpy(lifetime, hs);
    get_word(status, hs);
    cut(hs, 8);
    strcpy(laenge, hs);
    get_word(status, hs);
    if (strpos2(hs, "Byte", 1) != 1) {
      *ziel = '\0';
      *absender = '\0';
    }
    return;
  }

  if (typ == BAYCOM_USER) {
    TEMP = count_words(status);
    if (!((unsigned)TEMP < 32 && ((1L << TEMP) & 0xc00) != 0))
      return;
    get_word(status, absender);
    get_word(status, hs);
    get_word(status, ziel);
    get_word(status, datum);
    get_word(status, zeit);
    get_word(status, hs);
    get_word(status, hs);
    get_word(status, laenge);
    get_word(status, hs);
    if (strcmp(hs, "Bytes")) {
      *ziel = '\0';
      *absender = '\0';
      return;
    }
    get_word(status, hs);
    if (*hs == '\0')
      return;
    if (hs[0] == '#') {
      strcpy(lifetime, hs);
      strdelete((void *)lifetime, 1, 1);
      get_word(status, hs);
    }
    if (*hs == '\0')
      return;
    x = strpos2(hs, "@", 1);
    if (x > 0) {
      strdelete((void *)hs, 1, x);
      strcpy(mbx, hs);
/* p2c: box_sub.p, line 546:
 * Note: Possible string truncation in assignment [145] */
    }
    return;
  }

  if (typ != WAMPES_USER)
    return;
  if (strpos2(status, "bbs>", 1) == 1)
    strdelete((void *)status, 1, 4);
  if (count_words(status) != 9)
    return;
  get_word(status, hs);
  get_word(status, hs);
  get_word(status, hs);
  if (strcmp(hs, "To:"))
    return;
  get_word(status, ziel);
  get_word(status, mbx);
  strdelete((void *)mbx, 1, 1);
  get_word(status, hs);
  get_word(status, absender);
  get_word(status, hs);
  get_word(status, hs);
  strsub(zeit, hs, 9, 4);
  cut(hs, 7);
  if (strlen(zeit) == 4)
    strinsert(":", (void *)zeit, 3);
  strcpy(datum, hs);
  strdelete((void *)datum, 3, 3);
  strdelete((void *)hs, 1, 2);
  cut(hs, 3);
  if (!strcmp(hs, "Jan"))
    strcpy(hs, "01");
  else if (!strcmp(hs, "Feb"))
    strcpy(hs, "02");
  else if (!strcmp(hs, "Mar"))
    strcpy(hs, "03");
  else if (!strcmp(hs, "Apr"))
    strcpy(hs, "04");
  else if (!strcmp(hs, "May"))
    strcpy(hs, "05");
  else if (!strcmp(hs, "Jun"))
    strcpy(hs, "06");
  else if (!strcmp(hs, "Jul"))
    strcpy(hs, "07");
  else if (!strcmp(hs, "Aug"))
    strcpy(hs, "08");
  else if (!strcmp(hs, "Sep"))
    strcpy(hs, "09");
  else if (!strcmp(hs, "Oct"))
    strcpy(hs, "10");
  else if (!strcmp(hs, "Nov"))
    strcpy(hs, "11");
  else if (!strcmp(hs, "Dec"))
    strcpy(hs, "12");
  else
    strcpy(hs, "01");
  sprintf(hs, ".%s.", strcpy(STR5, hs));
  strinsert(hs, (void *)datum, 3);
  strcpy(laenge, "999999");
}

/*
=====================Header DB0PIC (Baycom)==========================

DG9FDA > ATARI    18.03.93 16:55 29 Zeilen 1502 Bytes #90 @ALLE
BID : 0833DB0SIFH7
{Read: Callsigns...}
Subj: DP = DAS PR-Programm fÅr ST
Path: !DB0PIC!DB0RBS!DB0LX!DB0AAA!DB0MWE!DB0KCP!OE9XPI!DB0CZ!DB0FRB!DB0GE!
      !DB0LJ!DB0SGL!DB0EAM!DB0SIF!
From: DG9FDA @ DB0SIF.DEU.EU
To  : ATARI @ ALLE

DK5SG:

Msg# 302784   To: HP @ALLE   From: DG1IY   Date: 14Sep92/1518
Subject: PACKET MIT HP48SX ???
Bulletin ID: 139258DB0GV
Path: DB0CZ!OE9XPI!HB9EAS!DB0GE!DB0IZ!DK0MWX!DB0EAM!DB0SIF!DB0GV
de DG1IY @ DB0GV

F6FBB:

Van : GB7BBS voor TPK   @EU
Type/Status : B$
Datum/tijd  : 21-Mrt 13:54
Bericht #   : 72617
Titel       : PERTON:G1DKI} Connected to BNOR71-1

Path: !PI8ZAA!PI8HWB!PI8GWO!PI8VNW!PI8MID!ON1CED!GB7TLH!GB7RUT!GB7BAD!GB7NOT!
      !GB7WRG!GB7YAX!GB7CHS!GB7SAM!GB7MAX!
HOW TO CONNECT USING A NODE


Von        : DK0NET
Nach       : DL8HBS@DB0HB.DEU.EU
Typ/Status : PF
Datum      : 17-Jun 08:23
BID/MID    : 166311DK0NET
Meldung #  : 85079
Titel      : Rubriken?! <dl5hbf

Path: !DK0NET!
 de DK0NET   @ DK0NET.DB0HBS.DEU.EU
 to DL8HBS   @ DL8HBS.DB0GR

 Moin Joachim,

Von        : DG8NBR
Nach       : YAESU @EU
Typ/Status : B$
Datum      : 18-Jun 06:44
BID/MID    : 17630BDB0BOX
Meldung #  : 85385
Titel      : info > FT 530

Path: !DB0HB!DB0HBS!DB0EAM!DB0SIF!DB0HOM!DB0GE!HB9EAS!HB9OS!DB0KCP!DB0AAB!
      !DB0FSG!DB0LNA!DB0RGB!DB0BOX!
de DG8NBR @ DB0BOX

hallo ft 530 user,                      tnx fÅrs lesen.ich beabsichtige den kauf

Von        : DG8NBR
Nach       : YAESU @EU
Typ/Status : B$
Datum      : 18-Jun 06:44
BID/MID    : 17630BDB0BOX
Meldung #  : 85385
Titel      : info > FT 530

R:930618/0427z @DB0HB  [NORD><LINK HAMBURG, JO43XP, OP:DF4HR/DL6HAZ]
R:930618/0535l @DB0HBS.#HH.DEU.EU [€≤±∞BBS-Hamburg,JO43TN,DB-ST,OP:DL8XAW∞±≤€]
R:930617/2247z @DB0EAM.DEU.EU [Kassel JO41PI TheBox 1˙9 OP:DB8AS]
R:930617/2244z @:DB0SIF.DEU.EU
R:930617/2244z @:DB0HOM.#SAR.DEU.EU
R:930617/2243z @DB0GE.#SL.DEU.EU [BBS Saarbruecken, DieBox 1.9]
R:930617/2017z @HB9EAS.CHE.EU [The Basel Area BBS]
R:930617/2015z @HB9OS  [Digital Radio Club Ost-Schweiz (HB9OS)  op:HB9CXN]
R:930617/2012z @:DB0KCP.#BAY.DEU.EU
R:930617/2012z @:DB0AAB.#BAY.DEU.EU
R:930617/2011z @:DB0FSG.#BAY.DEU.EU
R:930617/2011z @:DB0LNA.#BAY.DEU.EU
R:930617/2041z @:DB0RGB.#BAY.DEU.EU
R:930617/1841z @DB0BOX [DIE BOX in NUERNBERG JN59NJ, OP: DC3YC]
de DG8NBR @ DB0BOX

hallo ft 530 user,                      tnx fÅrs lesen.ich beabsichtige den kauf

*/

static cutboxtyp boxheader(Char *zeile)
{
  short zoffs, x, zl;
  cutboxtyp typ;
  short dpp;
  Char hs[257], w[257];
  short TEMP;

  typ = NOP;
  zl = strlen(zeile);

  if (zl <= 18)
    return typ;


  dpp = strpos2(zeile, ":", 1);
  if (dpp <= 4)
    return typ;

  if ((zeile[0] == '>') || (zeile[0] == '|'))  /* Kommentarzeichen */
    return typ;

  /*
TEST @DL8HBS.DB0GR.DEU.EU de:DL8HBS 04.05.93 23:20   0 100008 Bytes$#NONE#
SP @DB0GR        de:DL7WA  15.09.91 13:37  70    814 Bytes
*/
  if (zl >= 58) {  /* DieBox */
    zoffs = zl - 58;
    if (zeile[zoffs + 19] == ':') {
      if (strpos2(zeile, "de:", 1) == zoffs + 18) {
	if (strpos2(zeile, "Bytes", 1) == zoffs + 54)
	  typ = THEBOX_USER;
      }
    }
  }

  /*
DD6OQ  > MEINUN   04.05.93 13:38 39 Zeilen 1805 Bytes #30 (DB0GV)@DL
BID : 045307DB0GR
Subj: Gebuehrenerhoehung ok?
Path: !DL0AGU!DB0BRB!DB0GR!
Sent: 930504/1151z @DB0GR.DEU.EU [BERLIN-BBS, TheBox 1.9a, OP: DL7QG]
de DD6OQ @ DB0GR.DEU.EU
to MEINUNG @ DL

DG9NFF > DG9NFF   06.06.94 03:10 11 Lines 631 Bytes #120
DG9NFF > DG9NFF   06.06.94 03:10 11 Lines 80 Bytes #120


*/
  /*
TEST @DL8HBS.DB0GR.DEU.EU de:DL8HBS 04.05.93 23:20   0 100008 Bytes$#NONE#
*/
  if (typ == NOP) {
    if (zl >= 50) {  /* Baycom */
      zoffs = dpp;
      if (zoffs >= 19) {
	if (strpos2(zeile, "Bytes", 1) >= zoffs + 15) {
	  x = strpos2(zeile, ".", 1);
	  if (x > 9 && x < zoffs - 7) {
	    if (strpos2(zeile, "de:", 1) == zoffs - 2)
	      typ = THEBOX_USER;
	    else
	      typ = BAYCOM_USER;
	  }
	}
      }
    }
  }

  /*
Msg# 19522   To: ALL @WW   From: N0NJY   Date: 04May93/1559
Subject: DONT READ MSG FRM YV5KWS!!!
Bulletin ID: 28099_KA0WIN
Path: DB0BBX!DB0ERF!DB0EAM!DB0GV!DK0MTV!DB0GE!DB0IZ!PI8HRL!ON1KGX!ON1KPU!ON4TOR!
7KLY!GB7YAX!GB7WRG!GB7NOT!GB7
BAD!GB7RUT!GB7PET!GB7MHD!GB7KHW!GB7ZPU!GB7SPV!GB7DAA!GB7HSN!WA2NDV!N2BQF!KA0WIN!
KA0WIN

From: N0NJY@KA0WIN.#SECO.CO.USA.NA
To  : ALL@WW
*/
  if (typ == NOP) {  /* WAMPES */
    if (zl >= 57) {
      zoffs = strpos2(zeile, "Msg#", 1);
      if (zoffs > 0) {
	if (strpos2(zeile, "To:", 1) >= zoffs + 13) {
	  if (strpos2(zeile, "From:", 1) > zoffs + 26) {
	    if (strpos2(zeile, "Date:", 1) > zoffs + 40)
	      typ = WAMPES_USER;
	  }
	}
      }
    }
  }

  /*
Date: 28 Oct 91 13:32
Message-ID: <0@EA2RCG>
From: EB2DJM@EA2RCG
To: ALL@EU
Subject: I need the schemas of FT-73R(yaesu)
*/
  /*
                  if typ = NOP then if dpp > 4 then begin { W0RLI }
                      if pos('Date:',zeile) = dpp-4 then begin
                          if zeile[14+dpp] = ':' then begin
                              if count_words(zeile) = 5 then typ := W0RLI_USER;
                          end;
                      end;
                  end;
  */
  /*
From    : PI8TMA @ PI8TMA.#GLD.NLD.EU
To      : SCIENC @ NLDNET
Date    : 911127/1645
Msgid   : B+ 2252@PI8TMA, 31885@PI8DAZ $SCIENC.220
Subject : Bewoners Biosphere bestrijden kooldioxyde.
Path    : PI8UTR!PI8TMA
*/
  /*
                  if typ = NOP then if dpp > 8 then begin { AA4RE }
                      if zl > 15 then begin
                          if zeile[dpp-5] = 'm' then begin
                              if pos('From    : ',zeile) = dpp-8 then begin
                                  if count_words(zeile) = 5 then begin
                                      if pos('@',zeile) > dpp+2 then typ := AA4RE_USER;
                                  end;
                              end;
                          end;
                      end;
                  end;
  */
  /*
Van : DC6OQ  voor IBM   @DL
Type/Status : B$
Datum/tijd  : 21-Mrt 13:55
Bericht #   : 72618
Titel       : hilfe aastor
*/
  if (typ == NOP) {  /* F6FBB */
    if (*fbb_fromfield != '\0') {
      if (fbb_fromfield[0] == zeile[0]) {
	if (fbb_fromfield[1] == zeile[1]) {
	  if (strpos2(zeile, fbb_fromfield, 1) == 1) {
	    x = strpos2(zeile, fbb_tofield, 1);
	    if (x > strlen(fbb_fromfield) + 4) {
	      strcpy(hs, zeile);
	      strdelete((void *)hs, x, strlen(fbb_tofield));
	      strdelete((void *)hs, 1, strlen(fbb_fromfield));
	      del_leadblanks(hs);
	      TEMP = count_words(hs);
	      if ((unsigned)TEMP < 32 && ((1L << TEMP) & 0xc) != 0) {
		get_word(hs, w);
		if (callsign(w))
		  typ = F6FBB_USER;
	      }
	    }
	  }
	}
      }
    }
  }

  /*
Von        : DG8NBR
Nach       : YAESU @EU
Typ/Status : B$
Datum      : 18-Jun 06:44
BID/MID    : 17630BDB0BOX
Meldung #  : 85385
Titel      : info > FT 530
*/
  if (typ != NOP)  /* F6FBB314 */
    return typ;

  if (*fbb314_fromfield == '\0')
    return typ;
  if (fbb314_fromfield[0] != zeile[0])
    return typ;
  if (fbb314_fromfield[1] != zeile[1])
    return typ;
  if (strpos2(zeile, fbb314_fromfield, 1) != 1)
    return typ;
  strcpy(hs, zeile);
  strdelete((void *)hs, 1, strlen(fbb314_fromfield));
  del_leadblanks(hs);
  if (count_words(hs) != 1)
    return typ;
  get_word(hs, w);
  if (callsign(w))
    typ = F6FBB_USER_314;
  return typ;
}

/****************
* from packetl1 *
****************/

/* Diese Funktion vergleicht die weiteren vom Benutzer angegebenen   */
/* Box-End / Abbruch - Indikatoren mit der aktuellen Zeile           */

Static short other_bender(Char *zeile)
{
  short x;

  for (x = 0; x <= 3; x++) {
    if (strlen(zeile) >= strlen(otherbender1[x])) {
      if (strlen(otherbender1[x]) > 1) {
	if (otherbender1[x][0] == zeile[0]) {
	  if (otherbender1[x][1] == zeile[1]) {
	    if (strpos2(zeile, otherbender1[x], 1) == 1)
	      return 1;
	  }
	}
      }
    }
  }

  for (x = 0; x <= 3; x++) {
    if (strlen(zeile) >= strlen(otherbender2[x])) {
      if (strlen(otherbender2[x]) > 1) {
	if (otherbender2[x][0] == zeile[0]) {
	  if (otherbender2[x][1] == zeile[1]) {
	    if (strpos2(zeile, otherbender2[x], 1) == 1)
	      return 2;
	  }
	}
      }
    }
  }

  return 0;
}


Static short boxender(Char *zeile)
{
  Char hs[257];
  Char w[257];
  short be;

  be = 0;
  if (strlen(zeile) > '\b') {
    if (zeile[0] == '(') {
      if (strpos2(zeile, ")", 1) > 0 && strpos2(zeile, " de ", 1) > 0) {
	strcpy(hs, zeile);
	get_word(hs, w);
	if (strpos2(w, "(", 1) == 1 && strpos2(w, ")", 1) > 1) {
	  get_word(hs, w);
	  if (callsign(w)) {
	    get_word(hs, w);
	    if (!strcmp(w, "de")) {
	      get_word(hs, w);
	      if (strpos2(w, ">", 1) > 5)
		be = 1;
	    }
	  }
	}
      }
    }
  }
  if (be == 0) {
    if (zeile[0] == 'b') {
      if (strpos2(zeile, "bbs>", 1) == 1)   /*WAMPES*/
	be = 1;
    }
  }
  if (be == 0) {
    if ((zeile[0] == '(') && (strlen(zeile) > 3)) {
      if (strpos2(zeile, "->", 1) == strlen(zeile) - 1)   /*Baycom*/
	be = 1;
    }
  }
  if (be == 0)   /*Benutzerdefinierte Abbruchkriterien*/
    be = other_bender(zeile);
  return be;
}

Static void boxcutstart(short chan, Char *zeile)
{
  Char hs[257];
  Char hs2[257];
  short zoffs;
  cutboxtyp typ;
  struct boxcut_info *WITH;

  typ = boxheader(zeile);
  if (typ == NOP) return;
  WITH = &bcut_info[chan];
  /* kein selbstconnect
  if (strpos2(WITH->zielcall, Console_call, 1) != 0)
	  return;
  */	  
  WITH->bcutct++;
  int2str(WITH->bcutct, hs);
  int2str(chan,hs2);
  sprintf(WITH->bcutname, "%sBOXCUT%s.%s", newmaildir, hs2, hs);
  WITH->bcutchan = creat(WITH->bcutname, FC_FILE);
  if (WITH->bcutchan == -1)
    return;
  WITH->boxcut = true;
  del_callextender(chan, WITH->bcutcall);
  sprintf(hs, "%s %s", dpreadoutmagic, WITH->bcutcall);
  string_to_file(&WITH->bcutchan, hs, true);
  strcpy(hs, zeile);
  switch (typ) {

  case THEBOX_USER:
    zoffs = strlen(zeile) - 58;
    if (zoffs > 0)
      strdelete((void *)hs, 1, zoffs);
    break;

  case WAMPES_USER:
    zoffs = strpos2(zeile, "Msg#", 1);
    if (zoffs > 1)
      strdelete((void *)hs, 1, zoffs - 1);
    break;

  case F6FBB_USER_314:
    zoffs = strpos2(zeile, fbb314_fromfield, 1);
    if (zoffs > 1)
      strdelete((void *)hs, 1, zoffs - 1);
    break;
  default:
    break;
  }
  string_to_file(&WITH->bcutchan, hs, true);
}


Static void take_it(short chan)
{
  sort_new_mail(-1, bcut_info[chan].bcutname, bcut_info[chan].bcutcall);
}

Static void delete_it(short chan)
{
  unlink(bcut_info[chan].bcutname);
}


void boxcutwork(short chan, Char *zeile, boolean return_)
{
  struct boxcut_info *WITH;

  WITH = &bcut_info[chan];
  if (boxheader(zeile) != NOP) {
    if (WITH->bcutchan != -1)
      close(WITH->bcutchan);
    WITH->boxcut = false;
    take_it(chan);
    boxcutstart(chan, zeile);
    return;
  }
  if (boxender(zeile) == 1) {
    if (WITH->bcutchan != -1)
      close(WITH->bcutchan);
    WITH->boxcut = false;
    take_it(chan);
    return;
  }
  if (boxender(zeile) != 2) {
    if (WITH->boxcut)
      string_to_file(&WITH->bcutchan, zeile, return_);
    return;
  }
  if (WITH->bcutchan != -1)
    close(WITH->bcutchan);
  WITH->boxcut = false;
  delete_it(chan);
}


static void abort_multibox(short monbox_ch, short err)
{
  Char hs[257];
  struct monbox_info *WITH;
  Char STR5[257];

  WITH = &monbox_info[monbox_ch];
  if (WITH->fd != -1)
    close(WITH->fd);
  int2str(monbox_ch, hs);
  close_monbox(WITH->xmon_ch);
  WITH->active = 0;
  WITH->fd = -1;
  WITH->fractline[0] = '\0';
  sprintf(hs, "%sMONBOX.%s", newmaildir, strcpy(STR5, hs));
/*
  switch (WITH->cuttype) {
  case W0RLI_SF:
    sprintf(hs, "%sSFBOX.%s", newmaildir, strcpy(STR5, hs));
    break;

  default:
    sprintf(hs, "%sAUTOBOX.%s", newmaildir, strcpy(STR5, hs));
    break;
  }
*/
  unlink(hs);
}

Static int multiboxcutstart(short vorgabe, short typ,
			     Char *daten, int chksum)
{
  Char w[257], w1[257];
  boolean ok;
  unsigned short ct;
  short x;
  boolean callmoni;
  Char ziel[9];
  Char absender[7];
  Char mbx[41];
  Char datum[9], zeit[9], laenge[9], lifetime[9];
  short lt;
  cutboxtyp typ2;
  struct monbox_info *WITH;
  Char STR1[257];
  int monbox_channel;
  int xmon_channel;
  boolean ispriv;

  ok = false;
  callmoni = false;
  typ2 = NOP;
  lt = 0;
  if (typ == 0) {
    typ2 = boxheader(daten);
    if (typ2 == THEBOX_USER) {
      x = strlen(daten);
      if (x > 58)   /* wenn da noch ein Prompt vor dem Header steht */
	strdelete((void *)daten, 1, x - 58);
    } else if (typ2 == WAMPES_USER) {
      x = strpos2(daten, "Msg#", 1);
      if (x > 1)
	strdelete((void *)daten, 1, x - 1);
    }

    if (((1L << ((long)typ2)) & ((1L << ((long)THEBOX_USER)) |
	   (1L << ((long)BAYCOM_USER)) | (1L << ((long)WAMPES_USER)))) != 0) {
      separate_status(daten, ziel, absender, mbx, datum, zeit, laenge,
		      lifetime);

      ispriv = callsign(ziel);
      for (ct=0;ct < multiboxlist_entries;ct++) {
        if ((ok = (strcmp(multiboxlist[ct].rubrik,ziel) == 0)))
          break;
        if (ispriv)
          if ((ok = (strcmp(multiboxlist[ct].rubrik,absender) == 0)))
          break;
      }

      if (ok)
	lt = multiboxlist[ct].dayct;
      else {
	if (ispriv && user_moncut) {
	  ok = true;
	  lt = Str2int(lifetime);
	  if (lt == 0)
	    lt = erasedelay;
	  else if (lt > erasedelay)
	    lt = erasedelay;
	  callmoni = true;
	} else if (!ispriv && bulletin_moncut) {
	  ok = !badname(ziel);
	  lt = 0;
	  callmoni = false;
	}
	/*   !!!!!Kein wahlloser Mitschnitt von Usermails!!!!! */
      }

/*
      if (ok) {
	if (lt > 0) {
	  date1 = str2datum(datum);
	  time1 = str2zeit(zeit);
	  ddiff = daydiff(date1, time1, clock.dosdate, clock.dostime);
	  ok = ((unsigned long)ddiff <= lt * 86400L);
	  if (ddiff < 0)
	    prprotokoll("Clock not set?");
	}
      }
*/

    } else if (typ2 == F6FBB_USER) {
      x = strpos2(daten, fbb_tofield, 1);
      strcpy(w, daten);
      strdelete((void *)w, 1, x + strlen(fbb_tofield) - 1);
      get_word(w, w1);
      x = strpos2(w1, "@", 1);
      if (x > 0)
	cut(w1, x - 1);
      callmoni = callsign(w1);
      ok = !callmoni;
    } else if (typ2 != NOP) {
      callmoni = false;
      ok = true;
    }
  } else if (typ == 1) {
    strcpy(w, daten);
    get_word(w, w1);   /*S ALLE @ DL < DL7AAA $12635DB0GR # 1*/
    get_word(w, w1);
    ok = !callsign(w1);
    if (ok) {
      ok = !badname(w1);
      if (ok) {
	x = strpos2(w, "$", 1);
	if (x > 0) {
	  strdelete((void *)w, 1, x);
	  get_word(w, w1);
	  ok = check_double(w1);
	}
      }
    }
  }

  if (ok) {
    if (vorgabe != -1)
      monbox_channel = vorgabe;
    else {
      ok = false;
      for (monbox_channel=0;monbox_channel < MAX_MONBOX;monbox_channel++) {
        if (!monbox_info[monbox_channel].active) {
          ok = ((xmon_channel = open_monbox(monbox_channel,chksum)) != -1);
          if (ok) {
            monbox_info[monbox_channel].active = 1;
            monbox_info[monbox_channel].xmon_ch = xmon_channel;
            monbox_info[monbox_channel].fd = -1;
          }
          break;
        }
      }
    }
  }

  if (!ok)
    return(-1);

  WITH = &monbox_info[monbox_channel];
  WITH->linect = 1;
  if (typ == 0) {
    if (callmoni && typ2 == THEBOX_USER) {
      strcpy(w, daten);   /*'UTC' gegen LT #0 tauschen*/
      if (strlen(w) > 50) {
	strdelete((void *)w, 43, 3);
	int2str(lt, w1);
	while (strlen(w1) < 3)
	  sprintf(w1, " %s", strcpy(STR1, w1));
	strinsert(w1, (void *)w, 43);
      }
      strcpy(WITH->kopf, w);
    } else {
      strcpy(WITH->kopf, daten);
    }
    WITH->fractline[0] = '\0';
    WITH->betreff[0] = '\0';
    WITH->cuttype = typ2;
    WITH->msgid[0] = '\0';
  } else if (typ == 1) {
    strcpy(WITH->kopf, daten);
    WITH->fractline[0] = '\0';
    WITH->betreff[0] = '\0';
    WITH->cuttype = W0RLI_SF;
    WITH->msgid[0] = '\0';
  }
  return(monbox_channel);
}


Static void multiboxcutwork(short monbox_ch, Char *daten, boolean return_)
{
  Char hs[257], w[257], hs2[257];
  Char monbox_str[7];
  Char id[13];
  boolean ok, ended;
  struct monbox_info *WITH;
  Char STR1[132];
  Char STR7[134];
  Char hs1[257];

  int2str(monbox_ch, monbox_str);
  WITH = &monbox_info[monbox_ch];
  ended = false;
  if (strlen(WITH->fractline) + strlen(daten) <= 160) {
    strcat(WITH->fractline, daten);
    if (((boxender(WITH->fractline) == 1) && (WITH->cuttype != W0RLI_SF)) ||
	((strpos2(daten, "\032", 1) > 0) && (WITH->cuttype == W0RLI_SF))) {
      ended = true;
      if (WITH->fd != -1)
	close(WITH->fd);
      close_monbox(WITH->xmon_ch);
      WITH->active = 0;
      WITH->fd = -1;
      WITH->fractline[0] = '\0';
      sprintf(hs1, "%sMONBOX.%s", newmaildir, monbox_str);
      if (WITH->cuttype == W0RLI_SF)
	sprintf(hs, "%sSFBOX.%s", newmaildir, monbox_str);
      else
	sprintf(hs, "%sAUTOBOX.%s", newmaildir, monbox_str);
      rename(hs1,hs);
      sort_new_mail(-1, hs, "");
    } else if ((boxender(WITH->fractline) == 2) &&
               (WITH->cuttype != W0RLI_SF)) {
      ended = true;
      abort_multibox(monbox_ch, 0);
    }
  } else {
    WITH->fractline[0] = '\0';
    return_ = false;
    abort_multibox(monbox_ch, 7);
  }
  if (!return_ || ended)
    return;
  if ((boxheader(WITH->fractline) != NOP) && (WITH->cuttype != W0RLI_SF)) {
    if (WITH->fd != -1)
      close(WITH->fd);
    WITH->fd = -1;
    sprintf(hs1, "%sMONBOX.%s", newmaildir, monbox_str);
    sprintf(hs, "%sAUTOBOX.%s", newmaildir, monbox_str);
    rename(hs1,hs);
    sort_new_mail(-1, hs, "");
    multiboxcutstart(monbox_ch, 0, WITH->fractline,0);
  } else if (WITH->linect == 1) {
    WITH->linect++;
    if (WITH->cuttype == W0RLI_SF) {
      if (sfboxheader(WITH->fractline))
	multiboxcutstart(monbox_ch, 1, WITH->fractline,0);
      else if (!strcmp(WITH->fractline, "F>")) {
        close_monbox(WITH->xmon_ch);
        WITH->active = 0;
	WITH->fd = -1;
      } else {
	/* sprintf(STR1, "%sSFBOX.%s", newmaildir, monbox_str); */
	sprintf(STR1, "%sMONBOX.%s", newmaildir, monbox_str);
	WITH->fd = creat(STR1, FC_FILE);
	string_to_file(&WITH->fd, WITH->kopf, true);
	string_to_file(&WITH->fd, WITH->fractline, true);
      }
    } else if (WITH->cuttype == THEBOX_USER) {
      cut(WITH->fractline, 80);
      if (strlen(WITH->fractline) >= 1 && strlen(WITH->fractline) <= 80) {
	strcpy(WITH->betreff, WITH->fractline);
      } else
	abort_multibox(monbox_ch, 6);
    } else if (WITH->cuttype == WAMPES_USER) {
      cut(WITH->fractline, 80);
      if (strlen(WITH->fractline) >= 10 && strlen(WITH->fractline) <= 80) {
	strcpy(WITH->betreff, WITH->fractline);
      } else
	abort_multibox(monbox_ch, 6);
    } else if (WITH->cuttype == BAYCOM_USER) {
      if (strpos2(WITH->fractline, "BID :", 1) == 1) {
	strcpy(WITH->msgid, WITH->fractline);
	strcpy(hs, WITH->fractline);
	strdelete((void *)hs, 1, 6);
	get_word(hs, hs2);
	cut(hs2, 12);
	strcpy(id, hs2);
	if (check_double(id) == false)
	  abort_multibox(monbox_ch, 4);
	else {
	  /* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
	  sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
	  WITH->fd = creat(STR7, FC_FILE);
	  sprintf(w, "%s -moni-", dpreadoutmagic);
	  string_to_file(&WITH->fd, w, true);
	  string_to_file(&WITH->fd, WITH->kopf, true);
	  string_to_file(&WITH->fd, WITH->msgid, true);
	}
      } else if (strlen(WITH->fractline) >= 1 && strlen(WITH->fractline) <= 80) {
	strcpy(WITH->betreff, WITH->fractline);
	/* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
        sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
	WITH->fd = creat(STR7, FC_FILE);
	sprintf(w, "%s -moni-", dpreadoutmagic);
	string_to_file(&WITH->fd, w, true);
	string_to_file(&WITH->fd, WITH->kopf, true);
	string_to_file(&WITH->fd, WITH->betreff, true);
      } else
	abort_multibox(monbox_ch, 6);
    } else if (WITH->cuttype == W0RLI_USER) {
      ok = false;
      if (strpos2(WITH->fractline, "Message-ID:", 1) == 1) {
	strcpy(hs, WITH->fractline);
	strdelete((void *)hs, 1, 11);
	get_word(hs, hs2);
	cut(hs2, 12);
	strcpy(id, hs2);
	if (id[0] == '<' && id[strlen(id) - 1] == '>') {
	  strdelete((void *)id, 1, 1);
	  strdelete((void *)id, strlen(id), 1);
	  ok = check_double(id);
	}
      }
      if (ok) {
	/* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
        sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
	WITH->fd = creat(STR7, FC_FILE);
	sprintf(w, "%s -moni-", dpreadoutmagic);
	string_to_file(&WITH->fd, w, true);
	string_to_file(&WITH->fd, WITH->kopf, true);
	string_to_file(&WITH->fd, WITH->fractline, true);
      } else
	abort_multibox(monbox_ch, 6);
    } else if (WITH->cuttype == AA4RE_USER) {
      ok = false;
      if (strpos2(WITH->fractline, "To      :", 1) == 1) {
	strcpy(hs, WITH->fractline);
	strdelete((void *)hs, 1, 9);
	get_word(hs, hs2);
	ok = !callsign(hs2);   /*or callsign in def.rubriken...*/
      }
      if (ok) {
	/* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
        sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
	WITH->fd = creat(STR7, FC_FILE);
	sprintf(w, "%s -moni-", dpreadoutmagic);
	string_to_file(&WITH->fd, w, true);
	string_to_file(&WITH->fd, WITH->kopf, true);
	string_to_file(&WITH->fd, WITH->fractline, true);
      } else
	abort_multibox(monbox_ch, 6);
    } else if (WITH->cuttype == F6FBB_USER) {
      /* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
      sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
      WITH->fd = creat(STR7, FC_FILE);
      sprintf(w, "%s -moni-", dpreadoutmagic);
      string_to_file(&WITH->fd, w, true);
      string_to_file(&WITH->fd, WITH->kopf, true);
      string_to_file(&WITH->fd, WITH->fractline, true);
    } else if (WITH->cuttype == F6FBB_USER_314) {
      ok = false;
      if (strpos2(WITH->fractline, fbb314_tofield, 1) == 1) {
	strcpy(hs, WITH->fractline);
	strdelete((void *)hs, 1, strlen(fbb314_tofield));
	get_word(hs, hs2);
	if (strpos2(hs2, "@", 1) > 0)
	  cut(hs2, strpos2(hs2, "@", 1) - 1);
	del_lastblanks(hs2);
	ok = !callsign(hs2);   /*or callsign in def.rubriken...*/
      }
      if (ok) {
	/* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
        sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
	WITH->fd = creat(STR7, FC_FILE);
	sprintf(w, "%s -moni-", dpreadoutmagic);
	string_to_file(&WITH->fd, w, true);
	string_to_file(&WITH->fd, WITH->kopf, true);
	string_to_file(&WITH->fd, WITH->fractline, true);
      } else
	abort_multibox(monbox_ch, 6);
    }
  } else if (WITH->linect == 2 &&
	     ((1L << ((long)WITH->cuttype)) &
	      ((1L << ((long)THEBOX_USER)) | (1L << ((long)WAMPES_USER)))) != 0) {
    id[0] = '\0';
    WITH->linect++;
    if ((unsigned long)strlen(WITH->fractline) <= 40) {
      strcpy(WITH->msgid, WITH->fractline);
      ok = true;
      if (WITH->cuttype == WAMPES_USER) {
	strcpy(hs, WITH->fractline);
	if (count_words(hs) == 3) {
	  get_word(hs, w);
	  get_word(hs, w);
	  if (!strcmp(w, "ID:"))
	    get_word(hs, id);
	  ok = check_double(id);
	}
      } else {
	if (count_words(WITH->fractline) == 4) {
	  strcpy(hs, WITH->fractline);
	  get_word(hs, w);
	  get_word(hs, w);
	  if (strpos2(w, "ID", 1) > 0)
	    get_word(hs, id);
	  else
	    id[0] = '\0';
	  ok = check_double(id);
	}
      }
      if (ok) {
	/* sprintf(STR7, "%sAUTOBOX.%s", newmaildir, monbox_str); */
        sprintf(STR7, "%sMONBOX.%s", newmaildir, monbox_str);
	WITH->fd = creat(STR7, FC_FILE);
	sprintf(w, "%s -moni-", dpreadoutmagic);
	string_to_file(&WITH->fd, w, true);
	string_to_file(&WITH->fd, WITH->kopf, true);
	string_to_file(&WITH->fd, WITH->betreff, true);
	string_to_file(&WITH->fd, WITH->msgid, true);
      } else
	abort_multibox(monbox_ch, 4);
    } else
      abort_multibox(monbox_ch, 6);
  } else {
    if (WITH->fd != -1) {
      WITH->linect++;
      string_to_file(&WITH->fd, WITH->fractline, return_);
    }
  }
  WITH->fractline[0] = '\0';
}

/*****************
* new procedures *
*****************/

static boolean fill_line(maximum,info,size,rpos,zeile,return_)
short maximum;
char *info;
short size;
short *rpos;
char *zeile;
boolean *return_;
{
  short zeile_len;
  
  if ((maximum > 120) || (maximum < 0) || (size < 0)) return(false);
  if (*rpos < 1) *rpos = 1;
  zeile_len = 0;
  *return_ = false;
  if (*rpos > size) return(false);
  while (((*rpos) <= size) && (!(*return_)) && (zeile_len < maximum)) {
    if (info[(*rpos)-1] != '\015') {
      zeile[zeile_len] = info[(*rpos)-1];
      (*rpos)++;
      zeile_len++;
    }
    else {
      (*rpos)++;
      *return_ = true;
    }
  }
  zeile[zeile_len] = '\0';
  return(true);
}

/* check buffer for mailbox-headers and open xmon if one found */
int check_monbox(buffer)
char *buffer;
{
  unsigned short x;
  char zeile[257];
  char *info;
  short size;
  boolean return_;
  int monbox_channel;
  int chksum;
  int i;
  char srccall[10];
  char destcall[10];

  if (!monbox_flag || !box_active_flag) return(-1);
  if (box_busy_flag) return(-1);
  
  fill_moni_call(srccall,destcall);
  if (own_connection(srccall,destcall)) return(-1);
  
  size = (short)buffer[0] + 1;
  info = buffer + 1;
  
  /* calculate checksum */
  chksum = 0;
  for (i=0;i<size;i++) {
    chksum += info[i];
  }
  
  monbox_channel = -1;

  x = 1;
  while (fill_line(115, info, size, &x, zeile, &return_)) {
    if (boxheader(zeile) != NOP) {
      monbox_channel = multiboxcutstart(-1, 0, zeile,chksum);
      if (monbox_channel != -1) {
	while (fill_line(115, info, size, &x, zeile, &return_)) {
	  multiboxcutwork(monbox_channel, zeile, return_);
	}
      }
      continue;
    }
    if (!sfboxheader(zeile))
      continue;
    monbox_channel = multiboxcutstart(-1, 1, zeile,chksum);
    if (monbox_channel != -1) {
      while (fill_line(115, info, size, &x, zeile, &return_)) {
	multiboxcutwork(monbox_channel, zeile, return_);
      }
    }
  }
  if (monbox_channel != -1) {
    if (monbox_info[monbox_channel].active) {
      return(monbox_info[monbox_channel].xmon_ch);
    }
  }
  return(-1);
}

/* cancel xmon because of error */
void cancel_monbox(xmon_ch,monbox_ch)
int xmon_ch;
int monbox_ch;
{
  abort_multibox(monbox_ch,255);
}

/* put buffer to xmon */
void write_monbox(int monbox_ch, char *buffer)
{
  unsigned short x;
  short size;
  char *info;
  char zeile[257];
  boolean return_;
  
  size = (short)buffer[0] + 1;
  info = buffer + 1;

  x = 1;
  while (fill_line(115, info, size, &x, zeile, &return_)) {
    multiboxcutwork(monbox_ch, zeile, return_);
  }
  return;
}

void init_monbox()
{
  short x;
  int monbox_channel;
  FILE *inf;
  char hs[257];
  char w[257];
  char dcw[257];
  char *TEMP;
  char *k;
  char inf_NAME[_FNSIZE];
  short ct;
  long dc;
  int error;
  char errstr[257];
  int zct;
  
  monbox_flag = 1;
  user_moncut = false;
  bulletin_moncut = true;
  *fbb_fromfield = '\0';
  *fbb_tofield = '\0';
  *fbb314_fromfield = '\0';
  *fbb314_tofield = '\0';
  if (exist(f6fbb_box)) {
    zct = 0;
    strcpy(inf_NAME,f6fbb_box);
    inf = fopen(inf_NAME,"r");
    while (fgets(hs,256,inf) != NULL) {
      del_leadblanks(hs);
      if (*hs == '\0')
        continue;
      if (hs[0] == '#')
        continue;
      if ((TEMP = strchr(hs,'\n')) != NULL) *TEMP = '\0';
      if ((TEMP = strchr(hs,'\r')) != NULL) *TEMP = '\0';
      zct++;
      switch (zct) {
      case 1:
        cut(hs, 20);
        strcpy(fbb_fromfield, hs);
        break;
      case 2:
        cut(hs, 20);
        strcpy(fbb_tofield, hs);
        break;
      case 3:
        cut(hs, 20);
        strcpy(fbb314_fromfield, hs);
        break;
      case 4:
        cut(hs, 20);
        strcpy(fbb314_tofield, hs);
        break;
      default:
        break;
      }
    }
    fclose(inf);
  }
  multiboxlist_entries = 0;
  if (exist(autobox_dir)) {
    ct = 0;
    strcpy(inf_NAME,autobox_dir);
    inf = fopen(inf_NAME,"r");
    while ((fgets(hs,256,inf) != NULL) && (ct < MAX_MULTIBOXLIST)) {
      TEMP = strchr(hs,'\n');
      if (TEMP != NULL)
        *TEMP = 0;
      if (strpos2(hs,"#",1) == 1)
        continue;
      do {
        get_word(hs,w);
        dc = 0;
        k = strchr(w,':');
        if (k != NULL) {
          strcpy(dcw,k);
          *k = '\0';
          dc = Str2int(dcw);
        }
        if ((strlen(w) > 1) && (strlen(w) <= 8)) {
          upper(w);
          ct++;
          strcpy(multiboxlist[ct].rubrik,w);
          multiboxlist[ct].dayct = dc;
        }
      } while (strlen(w) > 0);
    }
    if (ct >= MAX_MULTIBOXLIST) {
      sprintf(errstr,"Too much folders in %s, rest ignored!",autobox_dir);
      cmd_display(M_COMMAND,0,errstr,1);
    }
    multiboxlist_entries = ct;
    fclose(inf);
  }
  erasedelay = 3;
  for (monbox_channel=0;monbox_channel < MAX_MONBOX;monbox_channel++) {
    monbox_info[monbox_channel].active = 0;
    monbox_info[monbox_channel].fd = -1;
    monbox_info[monbox_channel].fractline[0] = '\0';
  }
  for (x = 0; x <= 3; x++) {
    otherbender1[x][0] = '\0';
    otherbender2[x][0] = '\0';
  }
  if (exist(tnt_boxender)) {
    error = 0;
    strcpy(inf_NAME,tnt_boxender);
    inf = fopen(inf_NAME,"r");
    while ((fgets(hs,256,inf) != NULL) && (!error)) {
      TEMP = strchr(hs,'\n');
      if (TEMP == hs)
        continue;
      if (TEMP != NULL)
        *TEMP = 0;
      if (strpos2(hs,"#",1) == 1)
        continue;
      if ((strlen(hs) > 83) || (strlen(hs) <= 3)) {
        error = 1;
        continue;
      }
      switch (hs[0]) {
      case '1':
        if ((hs[1] > '0') && (hs[1] < '5')) {
          strcpy(otherbender1[hs[1]-'1'],&hs[3]);
        }
        else error = 1;
        break;
      case '2':
        if ((hs[1] > '0') && (hs[1] < '5')) {
          strcpy(otherbender2[hs[1]-'1'],&hs[3]);
        }
        else error = 1;
        break;
      default:
        error = 1;
        break;
      }
    }
    if (error) {
      sprintf(errstr,"%s corrupt, contents ignored",tnt_boxender);
      cmd_display(M_COMMAND,0,errstr,1);
      for (x = 0; x <= 3; x++) {
        otherbender1[x][0] = '\0';
        otherbender2[x][0] = '\0';
      }
    }
    fclose(inf);
  }
}

void exit_monbox()
{
  int monbox_channel;
  
  for (monbox_channel=0;monbox_channel < MAX_MONBOX;monbox_channel++) {
    if (monbox_info[monbox_channel].active) {
      abort_multibox(monbox_channel,254);
    }
  }
}

void cmd_ldboxfil(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int monbox_flag_save;
  
  monbox_flag_save = monbox_flag;
  exit_monbox();
  init_monbox();
  monbox_flag = monbox_flag_save;
  cmd_display(mode,channel,ok_text,1);
}

void cmd_monboxlist(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int monbox_channel;
  char call_str[MAXCOLS+1];
  char srccall[80];
  char destcall[80];
  struct monbox_info *moni;
  int active;
  
  active = 0;
  for (monbox_channel=0;monbox_channel < MAX_MONBOX;monbox_channel++) {
    moni = &monbox_info[monbox_channel];
    if (moni->active) {
      active = 1;
      fill_xmon_call(moni->xmon_ch,srccall,destcall);
      sprintf(call_str,"<%d>: %s > %s ",monbox_channel,srccall,destcall);
      strncat(call_str,dash_str,COLS - 1 - strlen(call_str));
      cmd_display(mode,channel,call_str,1);
      cmd_display(mode,channel,moni->kopf,1);
      if (moni->betreff[0] != '\0')
        cmd_display(mode,channel,moni->betreff,1);
      if (moni->msgid[0] != '\0')
        cmd_display(mode,channel,moni->msgid,1);
    }
  }
  if (!active)
    cmd_display(mode,channel,monbox_not_act_txt,1);
  else {
    call_str[0] = '\0';
    strncat(call_str,dash_str,COLS - 1);
    cmd_display(mode,channel,call_str,1);
  }
}

void init_boxcut()
{
  int i;
  
  for (i=0;i<tnc_channels;i++) {
    bcut_info[i].boxcut = false;
    bcut_info[i].bcutct = 0;
    bcut_info[i].bcutchan = -1;
  }
  autobox_flag = 1;
}

void exit_boxcut()
{
  int i;
  
  for (i=0;i<tnc_channels;i++) {
    if (bcut_info[i].boxcut == true) {
      delete_it((short)i);
      bcut_info[i].boxcut = false;
    }
  }
}

void conn_boxcut(channel)
int channel;
{
  bcut_info[channel].boxcut = false;
  bcut_info[channel].bcutct = 0;
}

void disc_boxcut(channel)
int channel;
{
  char buffer[257];
  int len;
  char *zeroptr;
  
  if (bcut_info[channel].boxcut == true) {
    if (ch_stat[channel].oldbuflen) {
      len = ch_stat[channel].oldbuflen;
      memcpy(buffer,ch_stat[channel].oldbuf,len);
      ch_stat[channel].oldbuflen = 0;
      zeroptr = memchr(buffer,(int)'\0',len);
      if (zeroptr == NULL) {
        buffer[len] = '\0';
        boxcutwork(channel,buffer,true);
      }
    }
  }
  if (bcut_info[channel].boxcut == true) {
    delete_it((short)channel);
    bcut_info[channel].boxcut = false;
  }
}

int boxcut_active(channel)
int channel;
{
  return(bcut_info[channel].boxcut == true);
}

void abinfile_to_boxcut(channel,file,erase,error)
int channel;
char *file;
int erase;
int error;
{
  if (error) {
    bcut_info[channel].boxcut = false;
    if (erase)
      delete_it((short)channel);
  }
  else {
    app_file(file,bcut_info[channel].bcutchan,(boolean)erase);
    bcut_info[channel].boxcut = false;
    take_it((short)channel);
  }
}

void boxcut(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char buffer1[257];
  char *zeroptr;

  if ((!autobox_flag || !box_active_flag) ||
      (box_active_flag && box_busy_flag)) {
    if (bcut_info[channel].boxcut == true) {
      delete_it((short)channel);
      bcut_info[channel].boxcut = false;
    }
    return;
  }
  if (len == 0) return;
  if (len > 256) return;
  
  zeroptr = memchr(buffer,(int)'\0',len);
  if (zeroptr == NULL) {
    memcpy(buffer1,buffer,len);
    buffer1[len - 1] = '\0'; /* remove CR at end of string */
    if (bcut_info[channel].boxcut == false)
      boxcutstart(channel,buffer1);
    else
      boxcutwork(channel,buffer1,true);
  }
  else { /* \0 found in string */
    if (bcut_info[channel].boxcut == true) {
      delete_it((short)channel);
      bcut_info[channel].boxcut = false;
    }
  }
}

void boxcut_rest(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char buffer1[257];
  char *zeroptr;

  if ((!autobox_flag || !box_active_flag) ||
      (box_active_flag && box_busy_flag)) {
    if (bcut_info[channel].boxcut == true) {
      delete_it((short)channel);
      bcut_info[channel].boxcut = false;
    }
    return;
  }
  if (len == 0) return;
  if (len > 256) return;
  
  zeroptr = memchr(buffer,(int)'\0',len);
  if (zeroptr == NULL) {
    if (bcut_info[channel].boxcut == true) {
      memcpy(buffer1,buffer,len);
      buffer1[len - 1] = '\0'; /* remove CR at end of string */
      boxcutwork(channel,buffer1,true);
    }
  }
  else { /* \0 found in string */
    if (bcut_info[channel].boxcut == true) {
      delete_it((short)channel);
      bcut_info[channel].boxcut = false;
    }
  }
}

void boxcut_nocr(channel,buffer,len)
int channel;
char *buffer;
int len;
{
  char buffer1[257];
  char *zeroptr;

  if ((!autobox_flag || !box_active_flag) ||
      (box_active_flag && box_busy_flag)) {
    if (bcut_info[channel].boxcut == true) {
      delete_it((short)channel);
      bcut_info[channel].boxcut = false;
    }
    return;
  }
  if (len == 0) return;
  if (len > 256) return;
  
  if (bcut_info[channel].boxcut == true) {
    zeroptr = memchr(buffer,(int)'\0',len);
    if (zeroptr == NULL) {
      memcpy(buffer1,buffer,len);
      buffer1[len] = '\0';
      boxcutwork(channel,buffer1,false);
    }
    else { /* \0 found in string */
      delete_it((short)channel);
      bcut_info[channel].boxcut = false;
    }
  }
}

void free_monbox()
{
  free(bcut_info);
}

int alloc_monbox()
{
  bcut_info = 
    (struct boxcut_info *)malloc(tnc_channels * sizeof(struct boxcut_info));
  return(bcut_info == NULL);
}

void check_mbeacon_head(int srcsum,int destsum,char *srccall,char *destcall)
{
  if (scanmbea_start) {
    if ((time(NULL) - scanmbea_time) > SCANMBEA_TIMEOUT) {
      scanmbea_start = 0;
    }
    else {
      return;
    }
  }
  if ((srcsum == scan_srcsum) &&
      (destsum == scan_destsum)) {
    if ((strcmp(scan_srccall,srccall) == 0) &&
        (strcmp(scan_destcall,destcall) == 0)) {
      scanmbea_valid = 1;
    }
  }
}

void check_mbeacon(char *buffer)
{
  int con_channel;
  char hlpstr[257];
  int iface;
  
  memcpy(hlpstr,buffer+1,(*buffer + 1));
  hlpstr[*buffer + 1] = '\0';
  
  if (strstr(hlpstr,scan_owncall) == NULL) return;
  
  if (!box_active_flag) return;
  
  con_channel = find_free_channel();
  if (con_channel == -1) return;

  scanmbea_start = 1;
  scanmbea_time = time(NULL);
  
  strcpy(hlpstr,"I");
  strcat(hlpstr,scan_owncall);
  queue_cmd_data(con_channel,X_COMM,strlen(hlpstr),M_CMDSCRIPT,hlpstr);
  update_owncall(con_channel,scan_owncall);
  sprintf(hlpstr,"%s %d",scan_concall,scan_timeout);
  iface = conv_name_to_iface(box_socket);
  cmd_xconnect(1,iface,
               con_channel,strlen(hlpstr),M_CONNECT,hlpstr);
}

void cmd_scanmbeacon(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  int error;
  int numpar;
  char srccall[257];
  char destcall[257];
  char owncall[257];
  char concall[257];
  int timeout;
  char hs[257];
  int i;
  char *ptr;

  error = 1;
  numpar = sscanf(str,"%s %s %s %s %d",srccall,destcall,
                  owncall,concall,&timeout);
  switch (numpar) {
  case EOF:
    if (scanmbea_flag == 0)
      sprintf(hs,"scanning of mailbeacon not active");
    else
      sprintf(hs,"(%s>%s); own call:%s, xconnect-parms:%s %d",
      scan_srccall,scan_destcall,scan_owncall,scan_concall,scan_timeout);
    cmd_display(mode,channel,hs,1);
    return;
    break;
  case 1:
    if ((strlen(srccall) == 1) && (srccall[0] == '$')) {
      scanmbea_flag = 0;
      error = 0;
    }
    break;
  case 4:
    timeout = 120;
  case 5:
    if ((strlen(srccall) <= 9) && (strlen(destcall) <= 9) && 
        (strlen(concall) <= 9) && (strlen(owncall) <= 6)) {

      /* generate source sum */
      ptr = srccall;
      i = 0;
      scan_srcsum = 0;
      while (*ptr != '\0') {
        scan_srccall[i] = toupper(*ptr);
        scan_srcsum += scan_srccall[i];
        ptr++;
        i++;
      }
      scan_srccall[i] = '\0';

      /* generate destination sum */
      ptr = destcall;
      i = 0;
      scan_destsum = 0;
      while (*ptr != '\0') {
        scan_destcall[i] = toupper(*ptr);
        scan_destsum += scan_destcall[i];
        ptr++;
        i++;
      }
      scan_destcall[i] = '\0';
      
      /* copy own call */
      ptr = owncall;
      i = 0;
      while (*ptr != '\0') {
        scan_owncall[i] = toupper(*ptr);
        ptr++;
        i++;
      }
      scan_owncall[i] = '\0';
      
      /* copy connect call */
      ptr = concall;
      i = 0;
      while (*ptr != '\0') {
        scan_concall[i] = toupper(*ptr);
        ptr++;
        i++;
      }
      scan_concall[i] = '\0';
      
      /* copy connect timeout */
      scan_timeout = timeout;
       
      /* activate scanning */
      scanmbea_flag = 1;
       
      error = 0;
    }
    break;
  default:
    break;
  }
  if (error) {
    sprintf(hs,"SCANMBEA [$] [<source> <destination> <own_call>"
               " <connectcall> [<timeout>] ]");
    cmd_display(mode,channel,hs,1);
  }
  else {
    cmd_display(mode,channel,ok_text,1);
  }
}

#endif
