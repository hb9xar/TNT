/*
   macro.c
   (C) Joerg Trieschmann (DD8FR @ DB0EAM) 1994
   
   updated: Mark Wahl DL4YBG 96/05/26

   25.03.04 hb9xar	include <time.h> for struct tm

*/

/* -------------------------------------------------- */

#define INFO_NONAME        "Pse enter your name with //name <your_name>"

/* -------------------------------------------------- */

extern char tnt_dir[];

char news_file_name[80];
char name_file_name[80];
char *info_noname_text = INFO_NONAME;

#define COOKIE_TMP         "/tmp/tntoriXXXXXX"

#include "tnt.h"
#include "macro.h"

#include <time.h>

#undef DEBUG

extern int gen_cookie(char *tmpname,int headfoot); /* from cookie.c */
extern char rem_ver_str[];                         /* from main.c */

void macro_gettime(char *);
void macro_getdate(char *);
int  macro_getname(char *,char *);
int  find_line_for_call(char *,char *);

int getbyte(int fd)
{
  char cs[1];
  if(read(fd,cs,1) <= 0) return EOF;
  return (int)cs[0];
}

int putbyte(int c,int fd)
{
  char cs[1];
  cs[0] = (char)c;
  return write(fd,cs,1);
}

void putstring(char *str,int fd)
{
  int i;
  for(i = 0;str[i] != 0;i++) putbyte((int)(str[i]),fd);
}

/*
   replace_macros() reads file 'in' and replaces all '%'-macros by the text they
   stand for. The new file is written to 'out'.
*/

int replace_macros(int in,int out,char *othercall,char *mycall,int channel)
{
  int c,c1;
  char number[4];
  char *tmp_name = strdup(COOKIE_TMP);
  int tmp_cookie;
  int news_file;
  char datetimestr[30];
  char namestr[28];
  char line[128];
  char tmpstr[160];

  while((c = getbyte(in)) != EOF) {

#ifdef DEBUG
    fprintf(stderr,"%c",c);
#endif

    if(c == '%') {
      if((c1 = getbyte(in)) != EOF) {

#ifdef DEBUG
	fprintf(stderr,"%c",c1);
#endif

	switch(c1) {
	case 'V':
	case 'v':
	  putstring(rem_ver_str,out);
	  break;
	case '%':   /* '%' itself */
	  putbyte(c,out);
	  break;
	case 'C':   /* call of other station */
	case 'c':
	  putstring(othercall,out);
	  break;
	case 'N':   /* name of other station */
	case 'n':
	  if(macro_getname(othercall,namestr) == TNT_OK)
	    putstring(namestr,out);
	  break;
	case 'Y':   /* call of this station */
	case 'y':
	  putstring(mycall,out);
	  break;
	case 'K':   /* channel number */
	case 'k':
	  sprintf(number,"%d",channel);
	  putstring(number,out);
	  break;
	case 'T':   /* time */
	case 't':
	  macro_gettime(datetimestr);
	  putstring(datetimestr,out);
	  break;
	case 'D':   /* date */
	case 'd':
	  macro_getdate(datetimestr);
	  putstring(datetimestr,out);
	  break;
	case 'B':   /* bell */
	case 'b':
	  putbyte(0x07,out);
	  break;
	case 'I':
	case 'i':
	  strcpy(tmpstr,news_file_name);
	  if((news_file = open(tmpstr,O_RDONLY))) {
	    while((c1 = getbyte(news_file)) != EOF) putbyte(c1,out);
	    close(news_file);
	  }
	  break;
	case 'Z':   /* timezone */
	case 'z':
	  /* a call to localtime() initializes tzname[] */
	  macro_gettime(datetimestr);
	  putstring(tzname[0],out);
	  break;
	case '_':   /* CR/LF */
	  putstring("\r\n",out);
	  break;
	case 'O':   /* print origin (cookie) */
	case 'o':
	  mkstemp(tmp_name);
	  if(!gen_cookie(tmp_name,0)) {
	    if((tmp_cookie = open(tmp_name,O_RDONLY))) {
	      while((c1 = getbyte(tmp_cookie)) != EOF) putbyte(c1,out);
	      close(tmp_cookie);
	      unlink(tmp_name);
	    }
	  } else putstring("sorry, no cookie",out);
	  break;
	case '?':   /* user is not in name-file ? -> tell it */
	  if(find_line_for_call(othercall,line) == LINE_NOTFOUND)
	    putstring(info_noname_text,out);
	  break;
	default:
	  putbyte(c,out);
	  putbyte(c1,out);
	}
      } else { putbyte(c,out); free(tmp_name); return TNT_OK; }
    } else putbyte(c,out);
  }

  free(tmp_name);
  return TNT_OK;
}

/*
  Get the time in a human readable form. The file /usr/lib/zoneinfo/localtime
  (the location might be different in your configuration)
  has to point to the right file representing your timezone.
  The environment variable "TZ" will be used to detect the correct timezone, too
  (I think, hi).
*/

void macro_gettime(char *datetimestr)
{
  struct tm *timestr;
  time_t timeval;

  timeval = time(&timeval);
  timestr = localtime(&timeval);

  sprintf(datetimestr,"%2.2u:%2.2u:%2.2u",
	  timestr->tm_hour,timestr->tm_min,timestr->tm_sec);
}

void macro_getdate(char *datetimestr)
{
  struct tm *timestr;
  time_t timeval;

  timeval = time(&timeval);
  timestr = localtime(&timeval);

  sprintf(datetimestr,"%2.2u/%2.2u/%2.2u",
	  timestr->tm_year,timestr->tm_mon+1,timestr->tm_mday);
}

int macro_getname(char *call,char *name)
{
  int i;
  char line[128];
  char *cptr,*cptr1;

#ifdef DEBUG
  fprintf(stderr,"in macro_getname()\n");
#endif

  if(find_line_for_call(call,line) == LINE_FOUND) {
    cptr = line;
    while((*cptr != ' ') && (*cptr != '\t') && (*cptr != 0)) cptr++;
    while((*cptr == ' ') || (*cptr == '\t')) cptr++;
    i = 0;
    cptr1 = cptr;
    while((*cptr1 != ';') && (*cptr1 != 0) && (i < 27)) {
      cptr1++; i++;
    }
    *cptr1=0;
    if(i) {
      strncpy(name,cptr,i+1);
      name[i+1] = '\0';
    }
    else  strcpy(name,call);
    return TNT_OK;
  }

  strcpy(name,call);
  return TNT_OK;
}

int find_line_for_call(char *call,char *line)
{
  char callline[80];
  char remline[80] = "";
  int c = 0;
  int i;
  char mcall[10];
  long namelen;
  int fd;
  char namesfile[160];

  /* this can't be a callsign */
  if(strlen(call) > 9) return LINE_NOTFOUND;
  
  strcpy(namesfile,name_file_name);

  fd = open(namesfile,O_RDONLY);
  if(fd<0) return LINE_NOTFOUND;

  strcpy(mcall,call);

  i = 0;
  while(mcall[i] != 0) { mcall[i] = toupper(mcall[i]); i++; }

#ifdef DEBUG
  fprintf(stderr,"find_line_for_call(): searching for call %s\n",call);
#endif

  while(c != EOF) {

    /* get a line from file */
    c = getbyte(fd);
    i = 0;
    while((i < 80) && (c != '\n') && (c != EOF)) {
      callline[i++] = c;
      c = getbyte(fd);
    }
    callline[i] = 0;

#ifdef DEBUG
    fprintf(stderr,"find_line_for_call(): got line : %s\n",callline);
#endif

    if(i > 2) {
      if(callline[1] == '>') {
#ifdef HAS_INDEX      
	if(!index(mcall,'-')) {
#else
	if(!strchr(mcall,'-')) {
#endif	
	                       /* call has no ssid */
	  if(!strncmp(&(callline[2]),mcall,strlen(mcall))) {
	    if(callline[2+strlen(mcall)] != '-') {
	      strcpy(line,callline);               /* found exact match */
	      close(fd);
	      return LINE_FOUND;
	    } else
	      /* if we'll do not find a better matching call, we'll take this */
	      if(!strlen(remline)) strcpy(remline,callline);
	  }
	} else {
	                        /* call has ssid */
	  if(!strncmp(&(callline[2]),mcall,strlen(call))) {
	    strcpy(line,callline);                 /* found exact match */
	    close(fd);
	    return LINE_FOUND;
	  } else {
	    namelen = 0;
	    while(mcall[namelen] != '-') namelen++;
	    if(!strncmp(&(callline[2]),mcall,namelen)) strcpy(remline,callline);
	  }
	}
      }
    }
  }

  if(strlen(remline)) {
    strcpy(line,remline);
    close(fd);
    return LINE_FOUND;
  }

  close(fd);
  return LINE_NOTFOUND;
}

int delete_line_for_call(char *call)
{
  char callline[81];
  int c = 0;
  int i;
  char mcall[10];
  long namelen;
  int fd;
  int fd2;
  char namesfile[160];
  char newnamesfile[160];
  int found;

  /* this can't be a callsign */
  if(strlen(call) > 9) return(1);
 
  strcpy(namesfile,name_file_name);

  strcpy(newnamesfile,namesfile);
  strcat(newnamesfile,"~");
  
  fd = open(namesfile,O_RDONLY);
  if(fd<0) return(1);
  
  fd2 = open(newnamesfile,O_RDWR|O_CREAT|O_TRUNC,PMODE);
  if (fd2 < 0) {
    close(fd);
    return(1);
  }

  strcpy(mcall,call);

  i = 0;
  while(mcall[i] != 0) { mcall[i] = toupper(mcall[i]); i++; }

#ifdef DEBUG
  fprintf(stderr,"delete_line_for_call(): searching for call %s\n",call);
#endif

  while(c != EOF) {

    /* get a line from file */
    c = getbyte(fd);
    i = 0;
    while((i < 80) && (c != '\n') && (c != EOF)) {
      callline[i++] = c;
      c = getbyte(fd);
    }
    callline[i] = 0;

    found = 0;
    
    if(i > 2) {
      if(callline[1] == '>') {
#ifdef HAS_INDEX      
	if(!index(mcall,'-')) {
#else
	if(!strchr(mcall,'-')) {
#endif	
	                       /* call has no ssid */
	  if(!strncmp(&(callline[2]),mcall,strlen(mcall))) {
	    found = 1;                /* found match */
	  }
	} else {
	                        /* call has ssid */
	  if(!strncmp(&(callline[2]),mcall,strlen(call))) {
	    found = 1;                 /* found exact match */
	  } else {
	    namelen = 0;
	    while(mcall[namelen] != '-') namelen++;
	    if(!strncmp(&(callline[2]),mcall,namelen)) found = 1;
	  }
	}
      }
    }
    if ((!found) && (c != EOF)) {
      strcat(callline,"\n");
      if (write(fd2,callline,strlen(callline)) < strlen(callline)) {
        close(fd);
        close(fd2);
        unlink(newnamesfile);
        return(1);
      }
    }
  }
  close(fd);
  close(fd2);
  unlink(namesfile);
  rename(newnamesfile,namesfile);
  return(0);
}

int add_line_for_call(char *call,char *name)
{
  char namesfile[160];
  int fd;
  char tempstr[250];
  
  /* this can't be a callsign */
  if(strlen(call) > 9) return(1);
 
  strcpy(namesfile,name_file_name);

  fd = open(namesfile,O_RDWR|O_APPEND|O_CREAT,PMODE);
  if (fd < 0) return(1);

  sprintf(tempstr,"T>%s %.50s\n",call,name);
  if (write(fd,tempstr,strlen(tempstr)) < strlen(tempstr)) {
    close(fd);
    return(1);
  }
  
  close(fd);
  return(0);
}
