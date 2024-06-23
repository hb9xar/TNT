/* TNT - RUN-PROGRAMM (logcall)
 *
 * Written by Matthias Hensler, 25.8.98
 * Copyright WSPse 1998
 * eMail: wsp@gmx.de
 *
 * Free software. Redistribution and modify under the terms of GNU Public
 * License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tntrun.h"

#define CUR_VER "1.0g"
#define CUR_DAT "9.3.99"

void print_info (char call[7]);

int main (int argc, char *argv[])
{
  int tmp;
  char call[7];

  if(argc > 2) {
    printf("maximal ein Argument (Rufzeichen) erlaubt\n");
    return 1;
  }

  if(argc == 1) print_info (GET_CALLSSID);
  else {
    if(strlen(argv[1]) > 6) {
      printf("ungueltiges Rufzeichen \"%s\".\n", argv[1]);
      return 1;
    }
  print_info (argv[1]);
  }

  printf("\nby WSPse, V" CUR_VER "/" CUR_DAT ".\n");
  return 0;
}

void print_info (char call[7])
{
  char tmp_str[256];
  FILE *log_file;
  int tmp;
  int i;
  unsigned char found;
  char *buffer;
  long int connect_zeit;
  int connect_nrs;
  long int diffmins;
  unsigned int offset;

  struct {
    unsigned int year_0, year_1;
    unsigned int month_0, month_1;
    unsigned int day_0, day_1;
    unsigned int hour_0, hour_1;
    unsigned int min_0, min_1;
    char call[7];
    } con;

  struct {
    unsigned char year, month, day, hour, min;
    } last_con;

  if(call == NULL) return;

  buffer = (char *) malloc(256);
  if(buffer == NULL) {
    perror("malloc");
    return;
  }

  for(i=0; i<strlen(call); i++) {
    call[i] = toupper(call[i]);
    if(call[i] == '-') call[i] = '\0';
  }
  printf("Informationen ueber %s\n\n", call);
  found = 0;

  strcpy(tmp_str, TNT_MAIN_PATH);
  strcat(tmp_str, "/names.tnt");
  log_file = fopen(tmp_str, "r");
  if(log_file != NULL) {
    while(1) {
      tmp = getc(log_file);
      tmp = getc(log_file);
      if( fgets(tmp_str, 250, log_file) != NULL) {
        if(strncasecmp(tmp_str, call, strlen(call)) == 0) {
          printf("Name:");
          tmp=0;
          for(i=0; i<strlen(tmp_str);i++) {
            if(tmp_str[i] == ' ') tmp=1;
            if(tmp == 1) putchar(tmp_str[i]);
          }
          found = 1;
          break;
        }
      } else break;
    }
    fclose(log_file);
    if(found == 0) printf("Name: unbekannt\n");
  }

  strcpy(tmp_str, TNT_MAIN_PATH);
  strcat(tmp_str, "/routes.tnt");
  log_file = fopen(tmp_str, "r");
  if(log_file != NULL) {
    while(1) {
      tmp = getc(log_file);
      tmp = getc(log_file);
      if( fgets(tmp_str, 158, log_file) != NULL) {
        if(strncasecmp(tmp_str, call, strlen(call)) == 0) {
          buffer = strtok(tmp_str, ";");
          buffer = strtok(NULL, ";");
          if(buffer != NULL) {
            printf("Route:%s", buffer);
            found = 2;
            break;
          }
        }
      } else break;
    }
    fclose(log_file);
  }
  if(found != 2) printf("Route: direkt oder unbekannt\n");

  strcpy(tmp_str, TNT_MAIN_PATH);
  strcat(tmp_str, "/log.tnt");
  log_file = fopen(tmp_str, "r");
  connect_zeit = 0;
  connect_nrs = 0;
  con.call[6] = '\0';
  if(log_file != NULL) {
    fgets(tmp_str, 250, log_file);
    fgets(tmp_str, 250, log_file);
    while( fgets(tmp_str, 250, log_file) != NULL) {
      offset = 0;
      if(tmp_str[5] != '.') {
        offset = strcspn(tmp_str + (char) 10, " ") +10;
        offset = (int) (strchr(tmp_str + (char) (offset), '.') - tmp_str) -2;
      }
      tmp = sscanf(tmp_str + (char) offset, "%d.%d.%d %d:%d | %d.%d.%d %d:%d",
                   &con.day_0, &con.month_0, &con.year_0,
                   &con.hour_0, &con.min_0,
                   &con.day_1, &con.month_1, &con.year_1,
                   &con.hour_1, &con.min_1);
      offset += 34;
      if(tmp == 10) {
        for(i=0; i<6; i++) {
          if((tmp_str[offset +i]=='-')||(tmp_str[offset +i]==' ')||
          (tmp_str[offset +i]==',')||(tmp_str[offset +i]==':')||
          (tmp_str[offset +i]=='\n')) con.call[i] = ' ';
          else con.call[i] = tmp_str[offset +i];
        }
        if(strncasecmp(call, con.call, strlen(call)) == 0) {
          connect_nrs++;
          last_con.year = con.year_1;
          last_con.month = con.month_1;
          last_con.day = con.day_1;
          last_con.hour = con.hour_1;
          last_con.min = con.min_1;

          diffmins = (((con.hour_1 - con.hour_0) *60) +
                      (con.min_1 - con.min_0)) +1;
          while( (con.day_1 > con.day_0) || (con.month_1 > con.month_0)
                                         || (con.year_1 > con.year_0)) {
            diffmins += 1440;
            con.day_0++;
            switch(con.month_0) {
              case 1:
              case 3:
              case 5:
              case 7:
              case 8:
              case 10:
              case 12: {
                if(con.day_0 > 31) {
                  con.day_0 = 1;
                  con.month_0++;
                }
                break;
              }
              case 2: {
                if(con.day_0 > 28) {
                  con.day_0 = 1;
                  con.month_0++;
                }
                break;
              }
              case 4:
              case 6:
              case 9:
              case 11: {
                if(con.day_0 > 30) {
                  con.day_0 = 1;
                  con.month_0++;
                }
              }
            }
            if(con.month_0 > 12) {
              con.month_0 = 1;
              con.year_0++;
            }
          }
          connect_zeit += diffmins;
        }
      }
    }
  fclose(log_file);
  }
  if(connect_nrs == 0) {
    printf("Bisher kein Connect mit %s.\n\n", call);
  } else {
    tmp = ' ';
    if(connect_nrs != 1) tmp = 's';
    printf("Insgesamt bereits %d Connect%c mit %s.\n", connect_nrs,
           tmp, call);
    tmp = connect_zeit / connect_nrs;
    i = (tmp/60);
    tmp %= 60;
    printf("Die durchschnittliche Connectzeit mit %s betraegt %02d:%02d.\n",
           call, i, tmp);
    tmp_str[0] = '\0';
    tmp = (connect_zeit / 1440);
    connect_zeit %= 1400;
    if(tmp > 0) {
      sprintf(buffer, "%d Tag", tmp);
      if(tmp != 1) strcat(buffer, "e");
      strcat(tmp_str, buffer);
      tmp = 0;
      strcat(tmp_str, " ");
    }
    tmp = (connect_zeit / 60);
    connect_zeit %= 60;
    sprintf(buffer, "%02d:%02d", tmp, connect_zeit);
    strcat(tmp_str, buffer);
    printf("Connectzeit mit %s insgesamt: %s\nDer letzte Connect mit %s"
           " war am %d.%d.%d um %02d:%02d.\n", call, tmp_str, call,
           last_con.day, last_con.month, last_con.year, last_con.hour,
           last_con.min);
  }
}

