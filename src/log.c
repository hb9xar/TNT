/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1997 by Mark Wahl
   For license details see documentation
   Procedures for logbook (log.c)
   created: Mark Wahl DL4YBG 94/03/12
   updated: Mark Wahl DL4YBG 97/01/26
   updated: mayer hans oe1smc, 6.1.1999
   updated: Matthias Hensler WS1LS, 12.2.1999
*/


#define MYQRG "438.600 " 

#include "tnt.h"
#include "connect.h"
#include "ifacedef.h"

extern void cmd_display(int flag,int channel,char *buffer,int cr);

extern int tnc_channels;
extern struct channel_stat *ch_stat;
extern char tnt_dir[];

extern struct sqrg_info qrg_info[] ; 

char tnt_logbookfile[80];
int logbook_active;
int logbook_flag;

static char log_error_text[] =
     "Warning: Can't write to Logbook, function now disabled!";
     
static char loghead[] =
     "CALL      QRG        Starttime   |    Endtime     |       "
     "   Callsign       \n";
     
static char logdelim[] =
     "----------------------------------------"
     "---------------------------------------\n";


void disable_logging()
{
  logbook_active = 0;
  cmd_display(M_COMMAND,0,log_error_text,1);
}

#ifdef DEBUGIO
extern int fddebugio ; 
extern char debugiof[] ; 

void init_debugio()
{
  if (( fddebugio = open ( debugiof, O_RDWR|O_CREAT , PMODE )) == -1 ) 
    {
      close ( fddebugio ) ; 
    } ;
  return ;
}

void exit_debugio()
{ 
      close ( fddebugio ) ;
} 
#endif 

void init_log()
{
  int fd;
  char tmpname[160];

  logbook_flag = 1;
  logbook_active = 1;  
  strcpy(tmpname,tnt_logbookfile);
  /* test if file already existing */
  if ((fd = open(tmpname,O_RDONLY)) != -1) {
    close(fd);
    return;
  }  
  /* file, not existing, create it and write header */
  if ((fd = open(tmpname,O_RDWR|O_CREAT|O_EXCL,PMODE)) == -1) {
    /* can't create file */
    disable_logging();
    return;
  }
  if (write(fd,loghead,strlen(loghead)) < strlen(loghead)) {
    disable_logging();
    close(fd);
    return;
  }
  if (write(fd,logdelim,strlen(logdelim)) < strlen(logdelim)) {
    disable_logging();
    close(fd);
    return;
  }
  close(fd);
}


#define LOG_LINELEN 120

void write_log(channel)
int channel;
{
  char logstr[LOG_LINELEN];
  char tmpstr[LOG_LINELEN];
  char qualstr[10];
  struct tm cvtime;
  int fd;
  char tmpname[2*LOG_LINELEN];
  
  if (!logbook_flag) return;
  if (!logbook_active) return;
  strcpy(tmpname,tnt_logbookfile);
  if ((fd = open(tmpname,O_RDWR|O_APPEND)) == -1) {
    /* can't open file */
    disable_logging();
    return;
  }
  /* generate the logline */
  strcpy(logstr,"");
  strcpy ( tmpstr, ch_stat[channel].mycall ) ; 
  strcat ( tmpstr, "     " ) ; 
  strncat ( logstr, tmpstr, (size_t)10 ) ;
/*
  strcat(logstr, MYQRG ); 
*/
  strncat ( logstr, qrg_info[(int)0].qrg , (size_t)7 );
  if(strlen(qrg_info[(int)0].qrg) < 7)
    strncat( logstr, "       ", 7-strlen(qrg_info[(int)0].qrg) );
  strcat ( logstr, " ") ; 
  cvtime = *localtime(&ch_stat[channel].start_time);
  sprintf(tmpstr,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u | ",
          cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
          cvtime.tm_hour,cvtime.tm_min);
  strcat(logstr,tmpstr);
  cvtime = *localtime(&ch_stat[channel].end_time);
  sprintf(tmpstr,"%2.2u.%2.2u.%2.2u %2.2u:%2.2u | ",
          cvtime.tm_mday,cvtime.tm_mon+1,cvtime.tm_year,
          cvtime.tm_hour,cvtime.tm_min);
  strcat(logstr,tmpstr);
  if (ch_stat[channel].log_call[0] != '\0') {
    strcat(logstr,ch_stat[channel].log_call);
    strcat(logstr,", Uplink: ");
  }
  sprintf(qualstr,"%%.%us\n",LOG_LINELEN - strlen(logstr) - 2);
  sprintf(tmpstr,qualstr,ch_stat[channel].call);
  strcat(logstr,tmpstr);
  /* logline available in logstr */
  if (write(fd,logstr,strlen(logstr)) < strlen(logstr)) {
    disable_logging();
    close(fd);
    return;
  }
  close(fd);
}


void exit_log()
{
  int i;
  
  for (i = 1;i < tnc_channels;i++) {
    if (ch_stat[i].state >= 4) {
      ch_stat[i].end_time = time(NULL);
      write_log(i);
    }
  }
}
