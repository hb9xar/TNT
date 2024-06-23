/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Procedures for cookies (cookie.c)
   created: Mark Wahl DL4YBG 94/01/16
   updated: Mark Wahl DL4YBG 96/03/21
*/

#include "tnt.h"

extern char tnt_cookiefile[];

char cook_head_str[80];
char cook_foot_str[80];
char cook_dash_str[80];

void init_cookie()
{
  int t;

  strcpy(cook_head_str,
   ">> TNT/Linux (Hostmode-Terminal-Program) << ---> One cookie for you:\012");
  strcpy(cook_foot_str,
   "------ //INFO: Station-Description, //HELP: Possible Commands ------\012");
  strcpy(cook_dash_str,
   "--------------------------------------------------------------------\012");
   t = (int)time(NULL);
#ifdef HAVE_SRANDOM
   srandom(t);
#else
   srand(t);
#endif
}

int x_random(my_range)
int my_range;
{
  int i,j;
  
  i = RAND_MAX / my_range;
  i *= my_range;
  while ((j = rand()) >= i) continue;
  return (j % i) % my_range;
}

static int write_cookie(org,dest)
int org;
int dest;
{
   char c;
   
   do {
      do {
         if (read(org,&c,1) < 1) return(1);
      } while (c!=0x0a);
      if (read(org,&c,1) < 1) return(1);
   } while ((c!='-') && (c!='%'));
   do {
      if (read(org,&c,1) < 1) return(1);
   } while (c!=0x0a);
   do {
      do {
         if (read(org,&c,1) < 1) return(1);
         if (c==9) c=' ';
         if (c != 0x0d) write(dest,&c,1);
      } while (c!=0x0a);
      if (read(org,&c,1) < 1) return(1);
      if (c==9) c=' ';
      if ((c=='-') || (c=='%')) break;
      else if (c != 0x0d) write(dest,&c,1);
   } while (1);
   c = 0x0a;
   write(dest,&c,1);
   return(0);
}

int gen_cookie(tmpname,headfoot)
char *tmpname;
int headfoot;
{
   int i,v;
   long pos;
   int org;
   int dest;
   struct stat file_stat;

   org=open(tnt_cookiefile,O_RDONLY);
   if (org<0) {
      return(1);
   }
   if (stat(tnt_cookiefile,&file_stat) == -1) {
     close(org);
     return(1);
   }
   /* minimal size ok cookie-file is 2048 byte */
   if (file_stat.st_size < 2048) return(1);
   dest=open(tmpname,O_RDWR|O_CREAT|O_APPEND,PMODE);
   if (dest<0) {
      close(org);
      return(2);
   }
   
   if (headfoot) {
     write(dest,cook_head_str,strlen(cook_head_str));
     write(dest,cook_dash_str,strlen(cook_head_str));
   }
   
   i=x_random(file_stat.st_size/256);
   v=x_random(256);
   pos=(long)i*256L+(long)v;
   lseek(org,pos,SEEK_SET);

   if (write_cookie(org,dest)) {
     close(org);
     close(dest);
     unlink(tmpname);
     return(3);
   }

   if (headfoot) {
     write(dest,cook_dash_str,strlen(cook_head_str));
     write(dest,cook_foot_str,strlen(cook_head_str));
   }
   close(org);
   close(dest);
   return(0);
}
