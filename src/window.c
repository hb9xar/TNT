/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for window handling (window.c)
   created: Mark Wahl DL4YBG 93/08/01
   updated: Mark Wahl DL4YBG 97/03/02
   updated: mayer hans oe1smc, 6.1.1999
   updated: Berndt Josef Wulf, 99/02/17
*/

#undef TNT
#include "tnt.h"

extern int conv_rx_to_local(char code,char *newcode1,char *newcode2);
extern void statlin_update();
extern int conv_local_to_umlaut(char code,char *newcode1,char *newcode2);

extern char attc_normal;
extern char attc_statline;
extern char attc_monitor;
extern char attc_cstatline;
extern char attc_controlchar;
extern char attc_remote;
extern char attc_special;
extern char attm_normal;
extern char attm_statline;
extern char attm_monitor;
extern char attm_cstatline;
extern char attm_controlchar;
extern char attm_remote;
extern char attm_special;
extern int color;
extern int color_save;
extern int termcap;
extern int termcap_save;
extern int wholeline_flag;
extern int bscrhold_flag;
extern int tabexp_flag;
extern int input_linelen;
extern char tnt_termcapfile[];

/* these values are stored in screen buffers */
#define ATT_NORMAL 0
#define ATT_STATLINE 1
#define ATT_MONITOR 2
#define ATT_CSTATLINE 3
#define ATT_CONTROLCHAR 4
#define ATT_REMOTE 5
#define ATT_SPECIAL 6

/* number of attributes */
#define ATT_NBR 7

char att_normal;
char att_statline;
char att_monitor;
char att_cstatline;
char att_controlchar;
char att_remote;
char att_special;

char att_array[7];

int LINES,COLS;
char *term;
char *tbuf;
char *cbuf;
char termsave[80];
int auto_newline;
int supp_hicntl;
int auto_newline_save;
int supp_hicntl_save;
int pagesize;
int frontend_active;
int frontend_fd;
int frontend_sockfd;
int tnt_daemon;
int insertmode;
int charconv;
int xtermkeys;

/* Terminal capabilities */
char *AL, *DL, *IC, *DC, *WR;
char *CL, *CM, *CS, *IS, *BC;
char *ND, *CE, *NL, *SO, *SE;
char *US, *UE, *VE, *VI, *RS;
char *MB, *MR, *MD, *MH, *ME;
char CRNL[10];
char RUBOUT[10];
static int scroll_reg;
static int ins_char;
static int del_char;
static int ins_line;
static int del_line;
static int cur_always_vis;
static int no_attr;
char *_tptr = NULL;

static char *strptr;

void frontend_exit();
void real_window_down();

/* frontend buffer */
#define FR_BUFSIZE 2048
struct fr_buffer {
  char *buffer;
  int buflen;
  int offset;
  struct fr_buffer *next;
};
static struct fr_buffer *fr_buffer_root;
static struct fr_buffer *fr_buffer_last;

#ifndef HAVE_MEMMOVE
void *memmove(dst,src,len)
void *dst;
const void *src;
unsigned int len;
{
  char buffer[256];
  
  if (len > 256) return;
  memcpy(buffer,src,len);
  memcpy(dst,buffer,len);
}
#endif

static struct win_info real_info[5]; /* real scr contains up to 5 windows */
static int get_line2();

struct termcaplist {
  char term[83];
  int termcap;
  int color;
  int charconv;
  int xtermkeys;
  int supp_hicntl;
  int auto_newline;
  struct termcaplist *next;
};

static struct termcaplist *termcaplist_root;


static void getrowcols(rows, cols)
int *rows;
int *cols;
{
#ifdef TIOCGWINSZ
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) < 0) {
		*rows = 0;
		*cols = 0;
	} else {
		*rows = ws.ws_row;
		*cols = ws.ws_col;
	}	
#else
#  ifdef TIOCGSIZE
	struct ttysize ws;

	if (ioctl(0, TIOCGSIZE, &ws) < 0) {
		*rows = 0;
		*cols = 0;
	} else {
		*rows = ws.ts_lines;
		*cols = ws.ts_cols;
	}
#  else 
	char *p, *getenv();

	if (p = getenv("LINES"))
		*rows = atoi(p);
	else
		*rows = 0;
	if (p = getenv("COLUMNS"))
		*cols = atoi(p);
	else
		*cols = 0;
#  endif
#endif	
}

/* handle one line of termcap file */
static int termcapline_analyse(str1,val1,val2,val3,val4,
                               val5,val6,termcaplist_ptr)
char *str1;
int val1;
int val2;
int val3;
int val4;
int val5;
int val6;
struct termcaplist **termcaplist_ptr;
{
  struct termcaplist *termcaplist_wrk;
  
  termcaplist_wrk = (struct termcaplist *)malloc(sizeof(struct termcaplist));
  if (termcaplist_wrk == NULL) return 1;
  strcpy(termcaplist_wrk->term,str1);
  termcaplist_wrk->termcap = val1;
  termcaplist_wrk->color = val2;
  termcaplist_wrk->charconv = val3;
  termcaplist_wrk->xtermkeys = val4;
  termcaplist_wrk->supp_hicntl = val5;
  termcaplist_wrk->auto_newline = val6;
  termcaplist_wrk->next = NULL;
  if (termcaplist_root == NULL) {
    termcaplist_root = termcaplist_wrk;
    *termcaplist_ptr = termcaplist_wrk;
  }
  else {
    (*termcaplist_ptr)->next = termcaplist_wrk;
    *termcaplist_ptr = termcaplist_wrk;
  }
  return(0);
} 

static void clear_termcapfile()
{
  struct termcaplist *termcaplist_wrk;
  struct termcaplist *termcaplist_tmp;
  
  termcaplist_wrk = termcaplist_root;
  while (termcaplist_wrk != NULL) {
    termcaplist_tmp = termcaplist_wrk;
    termcaplist_wrk = termcaplist_tmp->next;
    free(termcaplist_tmp);
  }
  termcaplist_root = NULL;
}

/* load termcap file */
static void load_termcapfile()
{
  int file_end;
  int file_corrupt;
  char file_str[160];
  char line[83];
  char str1[83];
  int val1;
  int val2;
  int val3;
  int val4;
  int val5;
  int val6;
  int rslt;
  FILE *termcap_file_fp;
  struct termcaplist *termcaplist_cur;

  strcpy(file_str,tnt_termcapfile);
  if (!(termcap_file_fp = fopen(file_str,"r"))) {
    /* no file present, exit */
    return;
  }
  file_end = 0;
  file_corrupt = 0;
  termcaplist_cur = NULL;
  while(!file_end) {
    if (fgets(line,82,termcap_file_fp) == NULL) {
      file_end = 1;
    }
    else {
      if (strlen(line) == 82) {
        file_corrupt = 1;
        file_end = 1;
      }
      else {
        if (line[0] != '#') {
          rslt = sscanf(line,"%s %d %d %d %d %d %d",
                        str1,&val1,&val2,&val3,&val4,&val5,&val6);
          switch (rslt) {
          case EOF:
            break;
          case 7:
            if (termcapline_analyse(str1,val1,val2,val3,val4,val5,val6,
                                    &termcaplist_cur)) {
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
  fclose(termcap_file_fp);
  if (file_corrupt) {
    clear_termcapfile();
  }
}

static int find_termcap(term,termcap,color,charconv,
                        xtermkeys,supp_hicntl,auto_newline)
char *term;
int *termcap;
int *color;
int *charconv;
int *xtermkeys;
int *supp_hicntl;
int *auto_newline;
{
  struct termcaplist *termcaplist_wrk;
  int found;
  char *ptr;
  int len;
  
  termcaplist_wrk = termcaplist_root;
  found = 0;
  while ((termcaplist_wrk != NULL) && (!found)) {
    if (termcaplist_wrk->term[0] == '*') {
      found = 1;
    }
    else {
      ptr = strchr(termcaplist_wrk->term,'*');
      if (ptr != NULL) {
        len = ptr - termcaplist_wrk->term;
        if (strncmp(term,termcaplist_wrk->term,len) == 0) {
          found = 1;
        }
      }
      else {
        if (strlen(term) == strlen(termcaplist_wrk->term)) {
          if (strcmp(term,termcaplist_wrk->term) == 0) {
            found = 1;
          }
        }
      }
    }
    if (found) {
      *termcap = termcaplist_wrk->termcap;
      *color = termcaplist_wrk->color;
      *charconv = termcaplist_wrk->charconv;
      *xtermkeys = termcaplist_wrk->xtermkeys;
      *supp_hicntl = termcaplist_wrk->supp_hicntl;
      *auto_newline = termcaplist_wrk->auto_newline;
    }
    else {
      termcaplist_wrk = termcaplist_wrk->next;
    }
  }
  return found;
}

void init_fr_buffer()
{
  fr_buffer_root = NULL;
  fr_buffer_last = NULL;
}

static void del_fr_buffer()
{
  struct fr_buffer *fr_buffer_work;

  if (!frontend_active) return;
  while (fr_buffer_root != NULL) {
    fr_buffer_work = fr_buffer_root;
    fr_buffer_root = fr_buffer_work->next;
    free(fr_buffer_work->buffer);
    free(fr_buffer_work);
  }
  fr_buffer_last = NULL;
}

static int flush_fr_buffer()
{
  int res;
  int len;
  struct fr_buffer *fr_buffer_work;

  if (!frontend_active) return(0);  
  while (fr_buffer_root != NULL) {
    fr_buffer_work = fr_buffer_root;
    len = fr_buffer_work->buflen - fr_buffer_work->offset;
    res = write(frontend_fd,fr_buffer_work->buffer+fr_buffer_work->offset,len);
    if (res == fr_buffer_work->buflen) {
      fr_buffer_root = fr_buffer_work->next;
      free(fr_buffer_work->buffer);
      free(fr_buffer_work);
    }
    else {
      if ((res == -1) && (errno == EAGAIN)) {
        return(1);
      }
      else if ((res > 0) && (res < len)) {
        fr_buffer_work->offset += res;
        return(1);
      }
      else {
        return(2);
      }
    }
  }
  fr_buffer_last = NULL;
  return(0);
}

void flush_frontend()
{
  if (flush_fr_buffer() == 2)
    frontend_exit(0);
}

static struct fr_buffer *alloc_fr_buffer()
{
  struct fr_buffer *fr_buffer_work;

  fr_buffer_work = (struct fr_buffer *)malloc(sizeof(struct fr_buffer));
  if (fr_buffer_work == NULL) {
    return(NULL);
  }
  fr_buffer_work->buffer = (char *)malloc(FR_BUFSIZE);
  if (fr_buffer_work->buffer == NULL) {
    free(fr_buffer_work);
    return(NULL);
  }
  fr_buffer_work->buflen = 0;
  fr_buffer_work->offset = 0;
  fr_buffer_work->next = NULL;
  return(fr_buffer_work);
}

static void queue_fr_buffer(buf,len)
char *buf;
int len;
{
  int tmplen;
  struct fr_buffer *fr_buffer_work;

  if (len <= 0) return;
  if (fr_buffer_root == NULL) {
    if ((fr_buffer_work = alloc_fr_buffer()) == NULL) {
      frontend_exit(0);
      return;
    }
    fr_buffer_root = fr_buffer_work;
    fr_buffer_last = fr_buffer_work;
  }
  else {
    fr_buffer_work = fr_buffer_last;
  }
  if (fr_buffer_work->buflen + len < FR_BUFSIZE) {
    memcpy(fr_buffer_work->buffer + fr_buffer_work->buflen,buf,len);
    fr_buffer_work->buflen += len;
  }
  else {
    tmplen = FR_BUFSIZE - fr_buffer_work->buflen;
    memcpy(fr_buffer_work->buffer + fr_buffer_work->buflen,buf,tmplen);
    fr_buffer_work->buflen = FR_BUFSIZE;
    if ((fr_buffer_work = alloc_fr_buffer()) == NULL) {
      frontend_exit(0);
      return;
    }
    fr_buffer_last->next = fr_buffer_work;
    fr_buffer_last = fr_buffer_work;
    memcpy(fr_buffer_work->buffer,buf+tmplen,len-tmplen);
    fr_buffer_work->buflen = len-tmplen;
  }
}

static void write_frontend(buf,len)
char *buf;
int len;
{
  int res;
  
  if (!frontend_active) return;
  switch (flush_fr_buffer()) {
  case 0:
    res = write(frontend_fd,buf,len);
    if (res == len) return;
    if ((res == -1) && (errno == EAGAIN)) {
      queue_fr_buffer(buf,len);
    }
    else if ((res > 0) && (res < len)) {
      queue_fr_buffer(buf+res,len-res);
    }
    else {
      frontend_exit(0);
    }
    break;
  case 1:
    queue_fr_buffer(buf,len);
    break;
  case 2:
    frontend_exit(0);
    break;
  }
}

static void phy_strout(str)
char *str;
{
  if (!tnt_daemon) {
    write(1,str,strlen(str));
  }
  else {
    write_frontend(str,strlen(str));
  }
}

#ifdef TNT_NETBSD
void phy_charout(ch)
#else
static int phy_charout(ch)
#endif
char ch;
{
  if (!tnt_daemon) {
    write(1,&ch,1);
  }
  else {
    write_frontend(&ch,1);
  }
#ifndef TNT_NETBSD
  return(0);
#endif
}

void set_linescols()
{
   LINES = 25;
   COLS = input_linelen;
}

void term_exit()
{
  if (termcap) {
    free(tbuf);
    free(cbuf);
  }
  else {
     free(AL); free(DL); free(IC); free(DC);
     free(WR); free(CL);
     free(IS); free(BC); free(ND); free(CE);
     free(NL); free(SO); free(SE); free(US);
     free(UE); free(VE); free(VI);
     free(MB); free(MR); free(MD); free(MH);
     free(ME);
  }
  clear_termcapfile();
}

int term_init(extterm,extlines,extcols)
char *extterm;
int extlines;
int extcols;
{
 int term_found;

 termcaplist_root = NULL;
 load_termcapfile();
  
 color = color_save;
 termcap = termcap_save;
 supp_hicntl = supp_hicntl_save;
 auto_newline = auto_newline_save;
 
 if (tnt_daemon) {
   strcpy(termsave,extterm);
   term = termsave;
 }
 else {
   if ((term = getenv("TERM")) == NULL) {
     printf("Environment variable TERM not set\n");
     return(1);
   }
 }
 term_found = 0;
 if (termcaplist_root != NULL) {
   if (find_termcap(term,&termcap,&color,&charconv,
                    &xtermkeys,&supp_hicntl,&auto_newline))
     term_found = 1;
 }
 if (!term_found) {
   charconv = termcap;
   if ((strncmp(term,"con",3) != 0) &&
       (strncmp(term,"linux",5) != 0) &&
       (color == 1)) {
     termcap = 1;
     charconv = 1;
     color = 0;
   }
   if ((strncmp(term,"xterm",5) == 0) &&
       (color == 3)) {
     termcap = 1;
     charconv = 1;
   }
   xtermkeys = 0;
   if (strncmp(term,"xterm",5) == 0) {
     xtermkeys = 1;
   }
 }
 if (color) {
   att_array[0] = attc_normal;
   att_array[1] = attc_statline;
   att_array[2] = attc_monitor;
   att_array[3] = attc_cstatline;
   att_array[4] = attc_controlchar;
   att_array[5] = attc_remote;
   att_array[6] = attc_special;
 }
 else {
   att_array[0] = attm_normal;
   att_array[1] = attm_statline;
   att_array[2] = attm_monitor;
   att_array[3] = attm_cstatline;
   att_array[4] = attm_controlchar;
   att_array[5] = attm_remote;
   att_array[6] = attm_special;
 }
 if (termcap) {
  if ((tbuf = (char *)malloc(1024)) == NULL ||
  	(cbuf = (char *)malloc(1024)) == NULL) {
  	printf("Out of memory.\n");
  	return(1);
  }
  switch(tgetent(cbuf, term)) {
  	case 0:
  		printf("No termcap entry for %s\n", term);
  		return(1);
  	case -1:
  		printf("No /etc/termcap present!\n");
  		return(1);
  	default:
  		break;
  }
  _tptr = tbuf;

  if ((CM = (char*)tgetstr("cm", &_tptr)) == NULL) {
  	printf("No cursor motion capability (cm)\n");
  	return(1);
  }
  if (tnt_daemon) {
    LINES = extlines;
    /*COLS = extcols;*/
    COLS = input_linelen;
  }
  else {
    getrowcols(&LINES, &COLS);
  }
  if (LINES == 0)
    if ((LINES = tgetnum("li")) <= 0) {
      printf("Number of terminal lines unknown\n");
      return(1);
  }
  if (LINES < 16) {
    printf("Illegal number of lines on terminal: %d (x >= 16)\n",LINES);
    return(1);
  }
  if (COLS == 0)
    if ((COLS = tgetnum("co")) <= 0) {
      printf("Number of terminal columns unknown\n");
      return(1);
  }
  if ((COLS < MINCOLS) || (COLS > MAXCOLS)) {
    printf("Illegal number of columns on terminal: %d (80 <= x <= 160)\n",COLS);
    return(1);
  }

  /* Terminal Capabilities */
  if ((AL = (char*)tgetstr("al", &_tptr)) == NULL) {
    ins_line = 0;
  }
  else {
    ins_line = 1;
  }
  if ((DL = (char*)tgetstr("dl", &_tptr)) == NULL) {
    del_line = 0;
  }
  else {
    del_line = 1;
  }
  if ((IC = (char*)tgetstr("ic", &_tptr)) == NULL) {
    ins_char = 0;
  }
  else {
    ins_char = 1;
  }
  if ((DC = (char*)tgetstr("dc", &_tptr)) == NULL) {
    del_char = 0;
  }
  else {
    del_char = 1;
  }
  if ((CL = (char*)tgetstr("cl", &_tptr)) == NULL) {
  	printf("No clear screen capability (cl)\n");
  	return(1);
  }
  if ((IS = (char*)tgetstr("is", &_tptr)) == NULL)
    IS = "\033[r";
  RS = (char*)tgetstr("rs", &_tptr);
  if ((ND = (char*)tgetstr("nd", &_tptr)) == NULL) {
  	printf("No cursor right capability (nd)\n");
  	return(1);
  }
  if ((CE = (char*)tgetstr("ce", &_tptr)) == NULL) {
  	printf("No clear to end of line capability (ce)\n");
  	return(1);
  }
  if ((WR = (char*)tgetstr("cr", &_tptr)) == NULL)
    WR = "\015";
  if ((NL = (char*)tgetstr("nl", &_tptr)) == NULL)
    NL = "\012";
  if (tgetflag("bs") || ((BC = (char*)tgetstr("bc", &_tptr)) == NULL))
    BC = "\010";
  if ((CS = (char*)tgetstr("cs", &_tptr)) == NULL)
    scroll_reg = 0;
  else
    scroll_reg = 1;
  if ((!scroll_reg) && !(ins_line && del_line)) {
    printf
     ("Neither scroll region(cs), nor insert/delete line (al,dl) capability\n");
    return(1);
  }
  VE = (char*)tgetstr("ve", &_tptr);
  VI = (char*)tgetstr("vi", &_tptr);
  if ((VE == NULL) || (VI == NULL )) cur_always_vis = 1;

  SO = (char*)tgetstr("so", &_tptr);
  SE = (char*)tgetstr("se", &_tptr);
  US = (char*)tgetstr("us", &_tptr);
  UE = (char*)tgetstr("ue", &_tptr);
  MB = (char*)tgetstr("mb", &_tptr);
  MR = (char*)tgetstr("mr", &_tptr);
  MD = (char*)tgetstr("md", &_tptr);
  MH = (char*)tgetstr("mh", &_tptr);
  ME = (char*)tgetstr("me", &_tptr);
  if ((ME == NULL) && (SE == NULL) && (SO == NULL)) no_attr = 1; 
  
  strcpy(CRNL,WR);
  strcat(CRNL,NL);
  strcpy(RUBOUT,BC);
  strcat(RUBOUT," ");
  strcat(RUBOUT,BC);
  return(0);
 }
 else {
  if (tnt_daemon) {
    LINES = extlines;
    COLS = extcols;
  }
  else {
    getrowcols(&LINES, &COLS);
  }
  if (LINES == 0)
    if ((LINES = tgetnum("li")) <= 0) {
  	printf("Number of terminal lines unknown\n");
  	return(1);
  }
  if (COLS == 0)
    if ((COLS = tgetnum("co")) <= 0) {
  	printf("Number of terminal columns unknown\n");
  	return(1);
  }
  AL = strdup("\033[L");
  ins_line = 1;
  DL = strdup("\033[M");
  del_line = 1;
  IC = strdup("\033[@");
  ins_char = 1;
  DC = strdup("\033[P");
  del_char = 1;
  CL = strdup("\033[H\033[J");
  IS = strdup("\033[r");
  RS = NULL;
  ND = strdup("\033[a");
  CE = strdup("\033[0K");
  WR = strdup("\015");
  NL = strdup("\012");
  BC = strdup("\010");
  VE = strdup("\033[?25h");
  VI = strdup("\033[?25l");
  cur_always_vis = 0;
  scroll_reg = 1;
  SO = strdup("\033[1m");
  SE = strdup("\033[21m");
  US = strdup("\033[4m");
  UE = strdup("\033[24m");
  MB = strdup("\033[5m");
  MR = strdup("\033[7m");
  MD = strdup("\033[1m");
  MH = strdup("\033[2m");
  ME = strdup("\033[m");
  no_attr = 0; 
  strcpy(CRNL,WR);
  strcat(CRNL,NL);
  strcpy(RUBOUT,BC);
  strcat(RUBOUT," ");
  strcat(RUBOUT,BC);
  return(0);
 }
}

static void phy_treset() 
{
  if (termcap) {
    tputs(IS,1,phy_charout);
    tputs(CL,1,phy_charout);
  }
  else {
    if (!tnt_daemon) {
      write(1,"\033c",2);
    }
    else {
      write_frontend("\033c",2);
    }
  }
}

static void phy_newline() 
{
  if (termcap) {
    tputs(WR,1,phy_charout);
    tputs(NL,1,phy_charout);
  }
  else {
    phy_strout(CRNL);
  }
}

static void phy_insline()
{
  if (termcap) {
    tputs(AL,1,phy_charout);
  }
  else {
    phy_strout(AL);
  }
}

static void phy_delline()
{
  if (termcap) {
    tputs(DL,1,phy_charout);
  }
  else {
    phy_strout(DL);
  }
}

static void phy_charins()
{
  if (termcap) {
    tputs(IC,1,phy_charout);
  }
  else {
    phy_strout(IC);
  }
}

static void phy_chardel()
{
  if (termcap) {
    tputs(DC,1,phy_charout);
  }
  else {
    phy_strout(DC);
  }
}

static void phy_cr()
{
  if (termcap) {
    tputs(WR,1,phy_charout);
  }
  else {
    phy_strout(WR);
  }
}

static void phy_curvis()
{
  if (!cur_always_vis) {
    if (termcap) {
      tputs(VE,1,phy_charout);
    }
    else {
      phy_strout(VE);
    }
  }
}

static void phy_curinvis()
{
  if (!cur_always_vis) {
    if (termcap) {
      tputs(VI,1,phy_charout);
    }
    else {
      phy_strout(VI);
    }
  }
}

static void phy_clrscr()
{
  if (termcap) {
    tputs(CL,1,phy_charout);
  }
  else {
    phy_strout(CL);
  }
}

static void phy_gotoxy(col,lin)
int col;
int lin;
{
  char tmpstr[20];

  if (!termcap) {
    sprintf(tmpstr,"\033[%u;%uH",lin+1,col+1);
    phy_strout(tmpstr);
  }
  else tputs(tgoto(CM,col,lin),1,phy_charout);
}

static void phy_par_scrdwn(lin1,lin2)
int lin1;
int lin2;
{
  /* cursor to lin2 */
  phy_gotoxy(0,lin2);
  /* delete line */
  phy_delline();
  /* cursor to lin1 */
  phy_gotoxy(0,lin1);
  /* insert line */
  phy_insline();
}

static void phy_par_scrup(lin1,lin2)
int lin1;
int lin2;
{
  /* cursor to lin1 */
  phy_gotoxy(0,lin1);
  /* delete lin */
  phy_delline();
  /* cursor to lin2 */
  phy_gotoxy(0,lin2);
  /* insert lin */
  phy_insline();
}

static void phy_scrreg(lin1,lin2)
int lin1;
int lin2;
{
  char tmpstr[20];
 
  if (scroll_reg) {
    if (termcap) {
      tputs(tgoto(CS,lin2,lin1),1,phy_charout);
    }
    else {
      sprintf(tmpstr,"\033[%u;%ur",lin1+1,lin2+1);
      phy_strout(tmpstr);
    }
  }
}

static void phy_scrnor()
{
  if (scroll_reg) {
    if (termcap) {
      if (RS != NULL)
        tputs(RS,1,phy_charout);
      else
        tputs(IS,1,phy_charout);
    }
    else {
      if (!tnt_daemon) {
        write(1,"\033[r",3);
      }
      else {
        write_frontend("\033[r",3);
      }
    }
  }
}

static void phy_rubout()
{
  if (termcap) {
    tputs(BC,1,phy_charout);
    (void)phy_charout(' ');
    tputs(BC,1,phy_charout);
  }
  else {
    phy_strout(RUBOUT);
  }
}

static void phy_curleft()
{
  if (termcap) {
    tputs(BC,1,phy_charout);
  }
  else {
    phy_strout(BC);
  }
}

static void phy_curright()
{
  if (termcap) {
    tputs(ND,1,phy_charout);
  }
  else {
    phy_strout(ND);
  }
}

static void phy_clreol()
{
  if (termcap) {
    tputs(CE,1,phy_charout);
  }
  else {
    phy_strout(CE);
  }
}

#ifdef TNT_NETBSD
void addchar(ch)
#else
static int addchar(ch)
#endif
char ch;
{
  int pos;
  pos = strlen(strptr);
  strptr[pos] = ch;
  strptr[pos+1] = '\0';
#ifndef TNT_NETBSD
  return(0);
#endif
}

static void addstr(str,add)
char *str;
char *add;
{
  if (termcap) {
    strptr = str;
    tputs(add,1,addchar);
  }
  else {
    strcat(str,add);
  }
}

static void str_attrout(str,ch2)
char *str;
char ch2;
{
  char tch;
  char ch;

  if (ch2 < ATT_NBR) {
    ch = att_array[(int)ch2];
  }
  else return;

  if (color) {
    strcat (str,"\033[");
    if (ch & 0x80) strcat(str,"5;");
    else strcat(str,"25;");
    if (ch & 0x08) strcat(str,"1;");
//&&& no bold (21 does underline)
//&&&    else strcat(str,"21;");
    else strcat(str,"22;");
    strcat(str,"3");
    tch = (ch & 07)+'0';
    strncat(str,&tch,1);
    strcat(str,";4");
    tch = ((ch >> 4) & 07)+'0';
    strncat(str,&tch,1);
    strcat(str,"m");
  }
  else if (!no_attr) {
    /* reset attributes */
    if (ME != NULL) addstr(str,ME);
    else if (SE != NULL) addstr(str,SE);
    else if (UE != NULL) addstr(str,UE);
    if ((ch & 0x01) && (SO != NULL) && !((SE == NULL) && (ME == NULL)))
      addstr(str,SO);
    if ((ch & 0x02) && (US != NULL) && !((UE == NULL) && (ME == NULL)))
      addstr(str,US);
    if ((ch & 0x04) && (MB != NULL) && (ME != NULL)) addstr(str,MB);
    if ((ch & 0x08) && (MR != NULL) && (ME != NULL)) addstr(str,MR);
    if ((ch & 0x10) && (MD != NULL) && (ME != NULL)) addstr(str,MD);
    if ((ch & 0x20) && (MH != NULL) && (ME != NULL)) addstr(str,MH);
  }
}

static void phy_attrout(ch)
char ch;
{
  char str[20];
  int i;

  if (!no_attr) {
    strcpy (str,"");
    str_attrout(str,ch);
    if ((i = strlen(str))) {
      if (!tnt_daemon) {
        write(1,str,i);
      }
      else {
        write_frontend(str,i);
      }
    }
  }
}

static void phy_no_transl()
{
  if (!tnt_daemon) {
    write(1,"\033(U",3);
  }
  else {
    write_frontend("\033(U",3);
  }
}

static void phy_normal_transl()
{
  if (!tnt_daemon) {
    write(1,"\033(B",3);
  }
  else {
    write_frontend("\033(B",3);
  }
}

void beep()
{
  if (!tnt_daemon) {
    write(1,"\007",1);
  }
  else {
    write(frontend_fd,"\007",1);
  }
}

void frontend_exit(int deact)
{
  int i;
  
  for (i = 0; i < 5; i++) {
    if (real_info[i].win != NULL)
      real_info[i].win->real = -1;
    real_info[i].win = NULL;
  }
  if (deact) {
    phy_clrscr();
    phy_scrnor();
    phy_treset();
    phy_strout("\033AA");
  }
  del_fr_buffer();
  frontend_active = 0;
  close(frontend_fd);
  term_exit();
}

void window_init()
{
  int i;

  termcaplist_root = NULL;
  load_termcapfile(1);

  att_normal = ATT_NORMAL;
  att_statline = ATT_STATLINE;
  att_monitor = ATT_MONITOR;
  att_cstatline = ATT_CSTATLINE;
  att_controlchar = ATT_CONTROLCHAR;
  att_remote = ATT_REMOTE;
  att_special = ATT_SPECIAL;

  for (i = 0; i < 5; i++) {
    real_info[i].win = NULL;
  }
  if (!tnt_daemon)
    phy_clrscr();
}

void window_exit()
{
  if (!tnt_daemon) {
    phy_clrscr();
    phy_scrnor();
    phy_treset();
  }
}

/* test, if insert mode active */
int insert_active()
{
  if (tnt_daemon && !frontend_active) return(0);
  if (real_info[0].win == NULL) return(0);
  return (real_info[0].win->insertflag);
}

/* open new window, returns 0 on success, 1 if no memory available */
int open_window(lines,win,bot)
int lines;
struct window *win;
int bot;
{
  int buffer_len;
  char *add;
  
  win->num_lines = lines;
  if (bot)
    win->line = lines - 1;
  else
    win->line = 0;
  win->column = 0;
  win->line_offset = 0;
  win->phys_line = win->line; /* equal because line_offset == 0 */
  win->real = -1;
  win->attribut = att_normal;
  win->insertflag = insertmode;
  win->holdflag = 0;
  buffer_len = lines * 2 * COLS;
  win->window_buffer = (char *)malloc(buffer_len);
  if (win->window_buffer == NULL) return(1);
  add = win->window_buffer;
  while (buffer_len > 0) {
    *add++ = att_normal;
    *add++ = SPACE;
    buffer_len -= 2 ;
  }
  return(0);
}

/* clears window (clear screen) */
void clear_window(win)
struct window *win;
{
  int buffer_len;
  char *add;
  int lines;
  
  lines = win->num_lines;
  win->line = lines - 1;
  win->column = 0;
  win->line_offset = 0;
  win->phys_line = win->line; /* equal because line_offset == 0 */
  win->real = -1;
  win->attribut = att_normal;
  win->insertflag = insertmode;
  win->holdflag = 0;
  buffer_len = lines * 2 * COLS;
  add = win->window_buffer;
  while (buffer_len > 0) {
    *add++ = att_normal;
    *add++ = SPACE;
    buffer_len -= 2 ;
  }
}

/* closes window and frees memory, returns 1 if window already closed */
int close_window(win)
struct window *win;
{
  if (win->window_buffer == NULL) return(1);
  if (win->real != -1) {
    real_info[win->real].win = NULL;
  }
  free(win->window_buffer);
  win->window_buffer = NULL;
  return(0);
}

/* increment window-line-offset with boundary-check */
static void inc_line_offset(win)
struct window *win;
{
  if (++win->line_offset == win->num_lines) win->line_offset = 0;
}

/* increment window-physical-line with boundary-check */
static void inc_phys_line(win)
struct window *win;
{
  if (++win->phys_line == win->num_lines) win->phys_line = 0;
}

/* decrement window-physical-line with boundary-check */
static void dec_phys_line(win)
struct window *win;
{
  if (win->phys_line == 0) win->phys_line = win->num_lines - 1;
  else win->phys_line--;
}

/* clear to end of line from col(included) in physical line  */
static void cl_eoline(win)
struct window *win;
{
  char *add;
  int i;
  
  add = win->window_buffer + 2 * (COLS * win->phys_line + win->column);
  i = COLS - win->column;
  while (i > 0) {
    *add++ = att_normal;
    *add++ = SPACE;
    i--;    
  }
}

/* function returns 1 if line is the last line of the physical screen */
int last_phy_line(win,line)
struct window *win;
int line;
{
  int last_line;
  
  if (real_info[win->real].last_real_line == (LINES - 1)) {
    last_line = real_info[win->real].last_log_line;
    if ((last_line += win->line_offset) >= win->num_lines)
      last_line -= win->num_lines;
    if (last_line == line) return(1);
  }
  return(0);
}

/* copy contents of one windowline to real screen
   assumes real cursor is already positioned at proper position */
static void update_line(win,phy_line)
struct window *win;
int phy_line;
{
  char att;
  char *addr;
  int i;
  int max_col;
/*  char *tmpstr; */
  char tmpstr[15*160+1];

/*  tmpstr = (char *)malloc(15*COLS+1); */
  max_col = COLS;
  if (auto_newline) {
    if (last_phy_line(win,phy_line)) max_col = COLS - 1;
  }
  addr = win->window_buffer + phy_line * 2 * COLS;
  phy_attrout(att = *addr);
  strcpy(tmpstr,"");
  for (i = 0; i < max_col; i++) {
    /* if attribute not current, change it */
    if (*addr != att) {
      att = *addr;
      str_attrout(tmpstr,att);
    }
    addr++;
    strncat(tmpstr,addr,1);
    addr++;
  }
  if (!termcap) phy_no_transl();
  phy_strout(tmpstr);
/*  free(tmpstr); */
  if (!termcap) phy_normal_transl();
}

/* physical window will be scrolled up and last line will be updated */
static void win_scrollup(part)
int part;
{
  struct window *win;
  int line;
  int phy_line;

  win = real_info[part].win;
  line = real_info[part].last_real_line;
  if (!scroll_reg)
    phy_par_scrup(real_info[part].first_real_line,line);
  else {  
    phy_gotoxy(0,line);
    phy_newline(part);
  }
  line = real_info[part].last_log_line;
  if ((phy_line = line + win->line_offset) >= win->num_lines)
    phy_line -= win->num_lines; 
  update_line(real_info[part].win,phy_line);
  if (auto_newline) {
    phy_gotoxy(0,real_info[part].last_real_line);
  }
  else {
    phy_cr();
  }
}

/* put cursor in window to new line and scroll if needed */
static void newline(win)
struct window *win;
{
  /* new column is 0 */
  win->column = 0;
  /* next line */
  if (win->line == (win->num_lines - 1)) {
    /* in last line, scroll buffer */
    inc_line_offset(win);
    inc_phys_line(win);
    if ((win->holdflag) && (win->first_win_line > 0))
      win->first_win_line--;
    cl_eoline(win);
    /* check if window in real screen */
    if (win->real != -1) {
      if (bscrhold_flag &&
          (real_info[win->real].last_log_line != 
          (real_info[win->real].win->num_lines - 1)) &&
          (real_info[win->real].first_log_line != 0)) {
        real_info[win->real].last_log_line--;
        real_info[win->real].first_log_line--;
      }
      else {
        if (!scroll_reg) {
          /* scroll physical window 1 line up and update line */
          win_scrollup(win->real);
        }
        else {
          if (real_info[win->real].last_log_line == (win->num_lines - 1)) {
            /* newline does the scroll */
            phy_newline(win->real);
          }
          else {
            /* scroll physical window 1 line up and update line */
            win_scrollup(win->real);
          }
        }
      }
    }
  }
  else {
    /* not in last line, no scroll needed */
    win->line++;
    inc_phys_line(win);
    /* check if window in real screen */
    if (win->real != -1) {
      /* if new line in physical window (except first line) do
         physical new line */
      if ((win->line > (real_info[win->real].first_log_line)) &&
          (win->line <= (real_info[win->real].last_log_line))) {
        phy_newline(win->real);
      }
      /* if newline first in physical window, cursor must be positioned */
      else if (win->line == (real_info[win->real].first_log_line)) {
        phy_gotoxy(0,real_info[win->real].first_real_line);
      }
      else if (win->line > (real_info[win->real].last_log_line)) {
        if (win->real == 0) {
          real_window_down(win->real,0);
        }
      }
    }
  }
}

/* put cursor to start of line (for statuslines) */
static void crstatline(win)
struct window *win;
{
  win->column = 0;
  if (win->real != -1) {
    phy_cr();
  }
}

/* copy contents of window to real screen */
static void update_window(part)
int part;
{
  struct window *win;
  int line;
  int phy_line;

  phy_gotoxy(0,real_info[part].first_real_line);  
  win = real_info[part].win;
  line = real_info[part].first_log_line;
  if ((phy_line = line + win->line_offset) >= win->num_lines)
    phy_line -= win->num_lines;
  while (line <= real_info[part].last_log_line) {
    update_line(win,phy_line);
    /* newline not after last line or if auto_newline */
    if (!auto_newline) {
      if (line != real_info[part].last_log_line) phy_newline(part);
    }
    line++;
    if ((phy_line = line + win->line_offset) >= win->num_lines)
      phy_line -= win->num_lines;
  }
}

static void set_cursor(part)
int part;
{
  struct window *win;
  int line;
  
  win = real_info[part].win;
  if (scroll_reg)
    phy_scrreg(real_info[part].first_real_line,real_info[part].last_real_line);
  if ((win->line >= real_info[part].first_log_line) &&
      (win->line <= real_info[part].last_log_line)) {
    line = win->line - real_info[part].first_log_line
                     + real_info[part].first_real_line;
    phy_gotoxy(win->column,line);
    phy_attrout(win->attribut);
    if (part != 0) phy_curinvis(); 
    else phy_curvis();
  }
  else {
    phy_curinvis();
  }
}

static void real_screen_gen(layout,hold,part)
struct real_layout *layout;
int hold;
int part;
{
  int i;

  if (hold) {
    real_info[part].win->holdflag = 1;
    real_info[part].win->first_win_line = real_info[part].first_log_line;
  }  
  for (i = 0; ((real_info[i].win != NULL) && (i < 5)); i++) {
    real_info[i].win->real = -1;
  }
  if (tnt_daemon && !frontend_active) return;
  phy_clrscr();
  phy_curinvis();
  phy_scrnor();
  for (i = 0; ((layout[i].win != NULL) && (i < 5)); i++) {
    real_info[i].win = layout[i].win;
    real_info[i].win->real = i;
    real_info[i].first_real_line = layout[i].first_real_line;
    real_info[i].pagesize = layout[i].pagesize;
    
    if ((real_info[i].win_num_lines = layout[i].win_num_lines) >
         real_info[i].win->num_lines)
      real_info[i].win_num_lines = real_info[i].win->num_lines;
    real_info[i].last_real_line =
      layout[i].first_real_line + real_info[i].win_num_lines - 1;
    if (real_info[i].win->holdflag) {
      real_info[i].first_log_line = real_info[i].win->first_win_line;
      real_info[i].last_log_line = real_info[i].first_log_line
                                 + real_info[i].win_num_lines - 1;
      real_info[i].win->holdflag = 0;
    }
    else {
      real_info[i].last_log_line = real_info[i].win->num_lines - 1;
      real_info[i].first_log_line =
        real_info[i].last_log_line - (real_info[i].win_num_lines - 1);
      if (real_info[i].win->line < real_info[i].first_log_line) {
        real_info[i].first_log_line = real_info[i].win->line;
        real_info[i].last_log_line = real_info[i].first_log_line
                                   + real_info[i].win_num_lines - 1;
      }
      else if (real_info[i].win->line > real_info[i].last_log_line) {
        real_info[i].last_log_line = real_info[i].win->line;
        real_info[i].first_log_line = real_info[i].last_log_line
                                   - (real_info[i].win_num_lines - 1);
      }
    }
    update_window(i);
  }
  set_cursor(0);
}

void real_screen(layout)
struct real_layout *layout;
{
  real_screen_gen(layout,0,0);
}

void real_screen_hold(layout,part)
struct real_layout *layout;
int part;
{
  real_screen_gen(layout,1,part);
}

/* scroll real windowpart line or page up */
void real_window_up(part,page)
int part;
int page;
{
  int lines;
  int line;
  int phy_line;
  struct window *win;
  
  if (real_info[part].first_log_line == 0) return;
  if (page)
    lines = real_info[part].pagesize;
  else
    lines = 1;
  if ((lines == 1) && ins_line) {
    real_info[part].first_log_line--;
    real_info[part].last_log_line--;
    set_cursor(part);
    phy_curinvis();
    line = real_info[part].first_real_line;
    if (!scroll_reg) {
      phy_par_scrdwn(real_info[part].first_real_line,
                     real_info[part].last_real_line);
    }
    else {
      phy_gotoxy(0,line);
      phy_insline();
    }
    line = real_info[part].first_log_line;
    win = real_info[part].win;
    if ((phy_line = line + win->line_offset) >= win->num_lines)
      phy_line -= win->num_lines;
    update_line(win,phy_line);
    set_cursor(0);
  }
  else {
    if (real_info[part].first_log_line < lines) {
      lines = real_info[part].first_log_line;
    }  
    real_info[part].first_log_line -= lines;
    real_info[part].last_log_line -= lines;
    set_cursor(part);
    phy_curinvis();
    update_window(part);
    set_cursor(0);
  }
}

/* scroll real windowpart to top */
void real_window_top(part)
int part;
{
  if (real_info[part].first_log_line == 0)
    return;
  real_info[part].first_log_line = 0;
  real_info[part].last_log_line = real_info[part].win_num_lines - 1;
  set_cursor(part);
  phy_curinvis();
  update_window(part);
  set_cursor(0);
}


/* scroll real windowpart line or page down */
void real_window_down(part,page)
int part;
int page;
{
  int lines;
  int line;
  int phy_line;
  struct window *win;
  
  if (real_info[part].last_log_line == (real_info[part].win->num_lines - 1))
    return;
  if (page)
    lines = real_info[part].pagesize;
  else
    lines = 1;
  if ((lines == 1) && del_line) {
    real_info[part].first_log_line++;
    real_info[part].last_log_line++;
    set_cursor(part);
    phy_curinvis();
    line = real_info[part].first_real_line;
    if (!scroll_reg) {
      phy_par_scrup(real_info[part].first_real_line,
                    real_info[part].last_real_line);
    }
    else {
      phy_gotoxy(0,line);
      phy_delline();
    }
    line = real_info[part].last_real_line;
    phy_gotoxy(0,line);
    line = real_info[part].last_log_line;
    win = real_info[part].win;
    if ((phy_line = line + win->line_offset) >= win->num_lines)
      phy_line -= win->num_lines;
    update_line(win,phy_line);
    set_cursor(0);
  }
  else {
    if ((real_info[part].last_log_line + lines) >
        (real_info[part].win->num_lines - 1)) {
      lines = real_info[part].win->num_lines - 1 - 
              real_info[part].last_log_line;
    }     
    real_info[part].first_log_line += lines;
    real_info[part].last_log_line += lines;
    set_cursor(part);
    phy_curinvis();
    update_window(part);
    set_cursor(0);
  }
}

/* scroll real windowpart to bottom */
void real_window_bot(part)
int part;
{
  if (real_info[part].last_log_line == (real_info[part].win->num_lines - 1))
    return;
  real_info[part].first_log_line = 
    real_info[part].win->num_lines - 1 - real_info[part].win_num_lines + 1;
  real_info[part].last_log_line = real_info[part].win->num_lines - 1;
  set_cursor(part);
  phy_curinvis();
  update_window(part);
  set_cursor(0);
}

/* execute cursor backspace */
static void backspace(win)
struct window *win;
{
  char *pos;
  int line;
  
  if (win->column == 0) return;
  win->column--;
  /* calculate character position and store in buffer */
  pos = win->window_buffer + win->phys_line*2*COLS + 2*win->column;
  if (win->insertflag) {
    memmove(pos,pos+2,2*(COLS - 1 - win->column));
    pos = win->window_buffer + (win->phys_line+1)*2*COLS - 2;
  } 
  *pos++ = win->attribut;
  *pos = SPACE;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      if (win->insertflag) {
        phy_curleft();
        if (del_char)
          phy_chardel();
        else {
          phy_cr();
          update_line(win,win->phys_line);
          line = win->line - real_info[win->real].first_log_line
                           + real_info[win->real].first_real_line;
          phy_gotoxy(win->column,line);
        }       
      }
      else {
        phy_rubout();
      }
    }
  }
}

/* execute delete character */
static void cur_delchar(win)
struct window *win;
{
  char *pos;
  int line;
  
  if (win->column >= COLS - 1) return;
  /* calculate character position and store in buffer */
  pos = win->window_buffer + win->phys_line*2*COLS + 2*win->column;
  memmove(pos,pos+2,2*(COLS - 1 - win->column));
  pos = win->window_buffer + (win->phys_line+1)*2*COLS - 2;
  *pos++ = win->attribut;
  *pos = SPACE;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      if (del_char)
        phy_chardel();
      else {
        phy_cr();
        update_line(win,win->phys_line);
        line = win->line - real_info[win->real].first_log_line
                         + real_info[win->real].first_real_line;
        phy_gotoxy(win->column,line);
      }       
    }
  }
}

/* execute cursor left */
static void cur_left(win)
struct window *win;
{
  if (win->column == 0) return;
  win->column--;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      phy_curleft();
    }
  }
}

/* execute cursor right */
static void cur_right(win)
struct window *win;
{
  if (win->column == (COLS - 1)) return;
  win->column++;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      phy_curright();
    }
  }
}

/* execute cursor start of line */
static void cur_stline(win)
struct window *win;
{
  if (win->column == 0) return;
  win->column = 0;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      phy_cr();
    }
  }
}

/* execute cursor end of line */
static void cur_eoline(win)
struct window *win;
{
  char *pos;
  int column;
  int line;
  
  column = COLS-1;
  pos = win->window_buffer + win->phys_line*2*COLS + 2*column + 1;
  /* find first position not SPACE */
  while ((*pos == SPACE) && (column > 0)) {
    pos -= 2;
    column--;
  }
  if (column < COLS-1 &&
      *pos != SPACE) /* DH3MB */
    column++; /* position at space after char */
  win->column = column;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      line = win->line - real_info[win->real].first_log_line
                       + real_info[win->real].first_real_line;
      phy_gotoxy(win->column,line);
    }
  }
}

/* execute cursor erase to end of line */
static void cur_eroline(win)
struct window *win;
{
  /* output on real screen ? */
  cl_eoline(win);
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      phy_clreol();
    }
  }
}

/* do wordwrap at end of line */
int wordwrap(str,win)
char *str;
struct window *win;
{
  int line;
  int column;
  char *pos;
  char *linesave;
  int len;
  
  /* save line */
  linesave = malloc(2*COLS);
  if (linesave == NULL) return(0);
  memcpy(linesave,win->window_buffer + win->phys_line*2*COLS,2*COLS);
  
  /* search backwards for SPACE */
  column = input_linelen-2;
  pos = linesave + 2*column + 1;
  while ((*pos != SPACE) && (column > 0)) {
    pos -= 2;
    column--;
  }
  if (*pos != SPACE) {
    free(linesave);
    return(0);
  }

  /* delete incomplete word and position at end of resulting line */
  win->column = column;
  cl_eoline(win);
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      line = win->line - real_info[win->real].first_log_line
                       + real_info[win->real].first_real_line;
      phy_gotoxy(win->column,line);
      phy_clreol();
    }
  }
  
  /* now copy line */
  len = get_line2(str,win);
  
  /* go to next line */
  newline(win);
  
  /* copy incomplete word to line and position cursor */
  if (column < input_linelen-2) {
    memcpy(win->window_buffer + win->phys_line*2*COLS,
           pos+1,2*(input_linelen-2-column));
    win->column = input_linelen-2-column;
    if (win->real != -1) {
      update_line(win,win->phys_line);
      line = win->line - real_info[win->real].first_log_line
             + real_info[win->real].first_real_line;
      phy_gotoxy(win->column,line);
    }
  }
  free(linesave);
  return(len);
}

/* execute cursor up */
static void cur_up(win)
struct window *win;
{
  int line;
  
  if (win->line == 0) return;
  win->line--;
  dec_phys_line(win);
  if (win->real != -1) {
    if (win->line < real_info[win->real].first_log_line) {
      real_window_up(0,0);
    }
    line = win->line - real_info[win->real].first_log_line
                     + real_info[win->real].first_real_line;
    phy_gotoxy(win->column,line);
  }
}

/* execute cursor page up */
static void cur_up_page(win)
struct window *win;
{
  if (win->line == 0) return;
  if ((win->line -= real_info[0].pagesize) < 0) win->line = 0;
  if ((win->phys_line = win->line + win->line_offset) >= win->num_lines)
    win->phys_line -= win->num_lines;
  if (win->real != -1) { 
    real_window_up(0,1);
    set_cursor(0);
  }
}

/* execute cursor top of window */
static void cur_top(win)
struct window *win;
{
  if (win->line == 0) return;
  win->line = 0;
  if ((win->phys_line = win->line + win->line_offset) >= win->num_lines)
    win->phys_line -= win->num_lines;
  if (win->real != -1) { 
    real_window_top(0);
    set_cursor(0);
  }
}

/* execute cursor down */
static void cur_down(win)
struct window *win;
{
  int line;
  
  if (win->line == (win->num_lines - 1)) return;
  win->line++;
  inc_phys_line(win);
  if (win->real != -1) {
    if (win->line > real_info[win->real].last_log_line) {
      real_window_down(0,0);
    }
    line = win->line - real_info[win->real].first_log_line
                     + real_info[win->real].first_real_line;
    phy_gotoxy(win->column,line);
  }
}

/* execute cursor page down */
static void cur_down_page(win)
struct window *win;
{
  if (win->line == (win->num_lines - 1)) return;
  if ((win->line += real_info[0].pagesize) > (win->num_lines - 1))
    win->line = win->num_lines - 1;
  if ((win->phys_line = win->line + win->line_offset) >= win->num_lines)
    win->phys_line -= win->num_lines;
  if (win->real != -1) { 
    real_window_down(0,1);
    set_cursor(0);
  }
}

/* execute cursor bottom of window */
static void cur_bot(win)
struct window *win;
{
  if (win->line == (win->num_lines - 1)) return;
  win->line = win->num_lines - 1;
  if ((win->phys_line = win->line + win->line_offset) >= win->num_lines)
    win->phys_line -= win->num_lines;
  if (win->real != -1) { 
    real_window_bot(0);
    set_cursor(0);
  }
}

/* output of character, no control-chars allowed */
static void win_charout_noctl(ch,win,ins)
char ch;
struct window *win;
int ins;
{
  char *pos;
  int line;

  if (auto_newline) {
    /* don't print last character of screen */
    if ((win->column == (COLS - 1)) &&
        last_phy_line(win,win->phys_line)) {
      return;
    }
  }
  if (supp_hicntl) {
    if ((ch >= 128) && (ch <= 160)) ch = '.'; 
  }
  /* no scroll for statuslines, ignore characters at end of line */
  if ((win->column == COLS) && (win->num_lines == 1)) return;
  /* calculate character position and store in buffer */
  pos = win->window_buffer + win->phys_line*2*COLS + 2*win->column;
  if (ins && (win->insertflag) && (win->column < COLS - 2)) {
    memmove(pos+2,pos,2*(COLS - 1 - win->column));
  }
  *pos++ = win->attribut;
  *pos = ch;
  /* output on real screen ? */
  if (win->real != -1) {
    if ((win->line >= real_info[win->real].first_log_line) &&
        (win->line <= real_info[win->real].last_log_line)) {
      if (ins && (win->insertflag) && (win->column < COLS - 2)) {
        if (ins_char) {
          phy_charins();
          (void)phy_charout(ch);
        }
        else {
          phy_cr();
          update_line(win,win->phys_line);
          line = win->line - real_info[win->real].first_log_line
                           + real_info[win->real].first_real_line;
          phy_gotoxy(win->column,line);
          phy_curright();
        }       
      }
      else (void)phy_charout(ch);
    }
  }
  /* advance cursor in buffer */
  if (++win->column == COLS) { /* was in last column ? */
    if (win->num_lines != 1) newline(win);
  }
}

/* output of character */
void win_charout_2(ch,win,conv)
char ch;
struct window *win; 
int conv;
{
  char newch1,newch2;
  int cnt;
  
  if ((ch < SPACE) || (ch == DEL) || (ch == ESC2)) {
    switch (ch) {
    case CR:
      /* no newline on statuslines ! */
      if (win->num_lines != 1) newline(win);
      else crstatline(win);
      break;
    case TAB:
      if (tabexp_flag) {
        win_charout_noctl(' ',win,0);
        while ((win->column % 8) != 0) {
          win_charout_noctl(' ',win,0);
        }
      }
      else {
        win_charout_noctl('^',win,0);
        win_charout_noctl('I',win,0);
      }
      break;
    case DEL:
      win_charout_noctl('^',win,0);
      win_charout_noctl('0',win,0);
      break;
    case ESC2:
      if (charconv) { 
        win_charout_noctl('^',win,0);
        win_charout_noctl('1',win,0);
      }
      else win_charout_noctl('*',win,0);
      break;
    default:
      win_charout_noctl('^',win,0);
      win_charout_noctl(ch + '@',win,0);
      break;
    }
  }
  else {
    if (charconv && conv && (ch >= 0x80)) {
      cnt = conv_rx_to_local(ch,&newch1,&newch2);
      if (cnt) { 
        win_charout_noctl(newch1,win,0);
        if (cnt == 2)
          win_charout_noctl(newch2,win,0);
      }
      else win_charout_noctl(ch,win,0);
    }
    else win_charout_noctl(ch,win,0);
  }
}

/* output of character */
void win_charout(ch,win,conv)
char ch;
struct window *win;
int conv; 
{
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(win->real);
  }
  win_charout_2(ch,win,conv);
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(0);
  }
}

/* output of 0-terminated string */
void win_stringout(str,win,conv)
char *str;
struct window *win;
int conv;
{
  char ch;
  
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(win->real);
  }
  if (!termcap && (win->real != -1)) {
    phy_no_transl();
  }
  while ((ch = *str++) != 0) win_charout_2(ch,win,conv);
  if (!termcap && (win->real != -1)) {
    phy_normal_transl();
  }
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(0);
  }
}

/* output of string, length in 1.byte (0 = 256 byte) */
void win_textout(str,win,conv)
char *str;
struct window *win;
int conv;
{
  int len;
  
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(win->real);
  }
  if (!termcap && (win->real != -1)) {
    phy_no_transl();
  }
  len = (*str++) + 1;
  while (len--) win_charout_2(*str++,win,conv);
  if (!termcap && (win->real != -1)) {
    phy_normal_transl();
  }
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(0);
  }
}

/* output of string, length in len */
void win_textout_len(str,len,win,conv)
char *str;
int len;
struct window *win;
int conv;
{
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(win->real);
  }
  if (!termcap && (win->real != -1)) {
    phy_no_transl();
  }
  while (len--) win_charout_2(*str++,win,conv);
  if (!termcap && (win->real != -1)) {
    phy_normal_transl();
  }
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(0);
  }
}

/* set new attribute */
void win_attrib(ch,win)
char ch;
struct window *win;
{
  win->attribut = ch;
  if (win->real == 0)
    phy_attrout(ch);
}

/* get line window */
static int get_line2(str,win)
char *str;
struct window *win;
{
  int i;
  char *ptr;
  char attr;
  char newchr;
  
  ptr = win->window_buffer + win->phys_line * 2 * COLS;
  for (i = 0; i < win->column; i++) {
    attr = *ptr++;
    str[i] = *ptr++;
    if (attr != att_normal) {
      str[i] -= '@';
    }
  }
  str[i++] = CR;
  return(i);
}

int get_line(str,win)
char *str;
struct window *win;
{
  if (wholeline_flag)
    cur_eoline(win);
  return(get_line2(str,win));
}

/* get line window without conversion */
int get_line_noconv(str,win)
char *str;
struct window *win;
{
  int i;
  char *ptr;
  
  ptr = win->window_buffer + win->phys_line * 2 * COLS;
  for (i = 0; i < win->column; i++) {
    ptr++;
    str[i] = *ptr++;
  }
  str[i++] = CR;
  return(i);
}

/* output of character and controlcodes */
void win_charout_cntl(ch,win)
char ch;
struct window *win; 
{
  char newch1,newch2;
  int cnt;

  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(win->real);
  }
  if ((ch < SPACE) || (ch == DEL) || (ch == ESC2)) {
    switch (ch) {
    case CR:
      /* no newline on statuslines ! */
      if (win->num_lines != 1) newline(win);
      else crstatline(win);
      break;
    case DEL:
    case BS:
      backspace(win);
      break;
    case C_CULEFT:
      cur_left(win);
      break;
    case C_CURIGHT:
      cur_right(win);
      break;
    case C_CUUP:
      cur_up(win);
      break;
    case C_CUDWN:
      cur_down(win);
      break;
    case C_WINUP:
      cur_up_page(win);
      break;
    case C_WINDWN:
      cur_down_page(win);
      break;
    case C_CUTOP:
      cur_top(win);
      break;
    case C_CUBOT:
      cur_bot(win);
      break;
    case C_STLINE:
      cur_stline(win);
      break;
    case C_EOLINE:
      cur_eoline(win);
      break;
    case C_INSERT:
      if (win->insertflag)
        win->insertflag = 0;
      else
        win->insertflag = 1;
      statlin_update();
      break;
    case C_DELLINE:
      cur_eroline(win);
      break;
    case C_DELCHAR:
      cur_delchar(win);
      break;
    case ESC2:
      win_charout_noctl('^',win,1);
      win_charout_noctl('1',win,1);
      break;
    default:
      win_charout_noctl('^',win,1);
      win_charout_noctl(ch + '@',win,1);
      break;
    }
  }
  else {
    if (!termcap && (win->real != -1)) {
      phy_no_transl();
    }
    if (ch >= 0x80) {
      cnt = conv_local_to_umlaut(ch,&newch1,&newch2);
      if (cnt) {
        win_charout_noctl(newch1,win,1);
        if (cnt == 2)
          win_charout_noctl(newch2,win,1);
      }
      else win_charout_noctl(ch,win,1);
    }
    else win_charout_noctl(ch,win,1);
    if (!termcap && (win->real != -1)) {
      phy_normal_transl();
    }
  }
  if ((win->real != -1) && (win->real != 0)) {
    set_cursor(0);
  }
}

