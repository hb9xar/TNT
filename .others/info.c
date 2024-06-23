/* TNT - RUN-PROGRAMM (info)
 *
 * Written by Matthias Hensler, 25.8.98
 * Copyright WSPse 1998
 * eMail: wsp@gmx.de
 *
 * Free software. Redistribution and modify under the terms of GNU Public
 * License.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define CUR_VER "1.0d"
#define CUR_DAT "17.9.98"

int main (void)
{
  const char delimiters[] = " \n";
  struct stat st;
  char *tmp_str;
  char *buffer;
  char *token;
  int tmp;
  char fields[3][10];
  int bfields[4] = { -1, -1, -1, -1 };
  int i;
  FILE *procstream;

  tmp = stat("/proc/", &st);
  if(tmp != 0) {
    perror("stat");
    return 1;
  }

  tmp = S_ISDIR(st.st_mode);
  if(tmp == 0) {
    fprintf(stderr, "/proc-fs not found\n");
    return 1;
  }

  printf("\n(TNT-Run/INFO) Copyright WSPse 1998\n\n"
         "Hardwareinformation ueber diesen Rechner:\n\n");

  tmp_str = (char *) malloc(80);
  buffer = (char *) malloc(160);
  if((tmp_str == NULL) || (buffer == NULL)) {
    perror("malloc");
    return 1;
  }

  for(i=0; i<3; i++) strcpy(fields[i], "unknown");

  procstream = fopen("/proc/cpuinfo", "r");
  if(procstream != NULL) {
    while((fgets(buffer,158,procstream))!=NULL) {
      if(strncmp(buffer, "cpu", 3) == 0) {
        if(buffer[3] == 0x09) {
          token = strtok (buffer, ":");
          token = strtok (NULL, delimiters);
          if(token != NULL) strcpy(fields[0], token);
        }
      }
      else if(strncmp(buffer, "model", 5) == 0) {
        token = strtok (buffer, ":");
        token = strtok (NULL, delimiters);
        if(token != NULL) strcpy(fields[1], token);
      }
      else if(strncmp(buffer, "bogomips", 8) == 0) {
        token = strtok (buffer, ":");
        token = strtok (NULL, delimiters);
        if(token != NULL) strcpy(fields[2], token);
      }
    }
    fclose(procstream);
  }

  printf("Prozessor: %s, Modell: %s (%s MIPS)\n", fields[0], fields[1],
         fields[2]);

  procstream = fopen("/proc/meminfo", "r");
  if(procstream != NULL) {
    while((fgets(buffer,158,procstream))!=NULL) {
      if(strncmp(buffer, "MemTotal", 8) == 0) {
        token = strtok (buffer, ":");
        token = strtok (NULL, delimiters);
        if(token != NULL) sscanf(token, "%d", &bfields[0]);
      }
      else if(strncmp(buffer, "MemFree", 7) == 0) {
        token = strtok (buffer, ":");
        token = strtok (NULL, delimiters);
        if(token != NULL) sscanf(token, "%d", &bfields[1]);
      }
      else if(strncmp(buffer, "SwapTotal", 9) == 0) {
        token = strtok (buffer, ":");
        token = strtok (NULL, delimiters);
        if(token != NULL) sscanf(token, "%d", &bfields[2]);
      }
      else if(strncmp(buffer, "SwapFree", 8) == 0) {
        token = strtok (buffer, ":");
        token = strtok (NULL, delimiters);
        if(token != NULL) sscanf(token, "%d", &bfields[3]);
      }
    }
    fclose(procstream);
  }
  printf("Speicher: %d KB (frei: %d KB)\n Auslagerungsspeicher: %d KB"
         " (frei: %d KB)\n Gesamt: %d KB, davon %d KB frei\n\n"
         ,bfields[0], bfields[1], bfields[2], bfields[3]
         ,bfields[0] + bfields[2], bfields[1] + bfields[3]);

  procstream = fopen("/proc/version", "r");
  if(procstream != NULL) {
    fgets(buffer, 158, procstream);
    token = strtok (buffer, " ");
    sprintf(tmp_str, "Betriebssystem: %s",token);
    while(1) {
      token = strtok (NULL, " ");
      if(token == NULL) break;
      if((strlen(tmp_str) + strlen(token)) > 78) {
        printf("%s\n", tmp_str);
        strcpy(tmp_str, "                ");
        strcat(tmp_str, token);
      }
      else {
        strcat(tmp_str," ");
        strcat(tmp_str, token);
      }
    }
    if(tmp_str[16] != '\0') printf("%s\n", tmp_str);
    fclose(procstream);
  }

  procstream = fopen("/proc/uptime", "r");
  if(procstream != NULL) {
    fgets(buffer, 100, procstream);
    token = strtok (buffer, " ");
    i = sscanf(buffer, "%d", &tmp);
    if((i=1) && (tmp>0)) {
      bfields[0] = 0; /* Tage */
      bfields[1] = 0; /* Stunden */
      bfields[2] = 0; /* Minuten */
      while(tmp >= 86400) { bfields[0]++; tmp -= 86400; }
      while(tmp >= 3600) { bfields[1]++; tmp -= 3600; }
      while(tmp >= 60) { bfields[2]++; tmp -= 60; }
      strcpy(fields[0], "Tag");
      if(bfields[0] != 1) strcat(fields[0], "en");
      printf("Das System laeuft seit %d %s, %02d:%02d\n",
             bfields[0], fields[0], bfields[1], bfields[2]);
    }
    fclose(procstream);
  }

  procstream = fopen("/proc/loadavg", "r");
  if(procstream != NULL) {
    fgets(buffer, 100, procstream);
    token = strtok(buffer, " ");
    i = sscanf(token, "0.%d", &bfields[0]);
    token = strtok(NULL, " ");
    i += sscanf(token, "0.%d", &bfields[1]);
    token = strtok(NULL, " ");
    i += sscanf(token, "0.%d", &bfields[2]);
    if(i == 3) {
      printf("Systembelastung: JETZT:%d%%, VOR 5-Min:%d%%,"
             " VOR 15-Min:%d%%\n",
             bfields[0], bfields[1], bfields[2]);
    }
    fclose(procstream);
  }

  procstream = fopen("/proc/interrupts", "r");
  if(procstream != NULL) {
    strcpy(tmp_str, "belegte Interrupts: ");
    while((fgets(buffer, 100, procstream)) != NULL) {
      token = strtok(buffer, " :");
      if(token != NULL) {
        strcat(tmp_str, token);
        if(strlen(tmp_str) > 75) {
          printf("%s\n", tmp_str);
          strcpy(tmp_str, "                    ");
        }
        else strcat(tmp_str, ",");
      }
    }
    if(tmp_str[(strlen(tmp_str)-1)] == ',') tmp_str[strlen(tmp_str)-1] = '\0';
    if(tmp_str[(strlen(tmp_str)-1)] != ' ') printf("%s\n", tmp_str);
    fclose(procstream);
  }

  procstream = fopen("/proc/filesystems", "r");
  if(procstream != NULL) {
    printf("\nVon diesem System unterstuetzte Dateisysteme:\n");
    tmp_str[0] = '\0';
    while((fgets(buffer, 158, procstream)) != NULL) {
      token = strtok(buffer, " \t\n");
      if( strncmp(token, "nodev", 5) == 0) token = strtok(NULL, " \t\n");
      if(token != NULL) {
        if((strlen(tmp_str) + strlen(token)) > 77) {
          printf("%s\n", tmp_str);
          strcpy(tmp_str, token);
        }
        else {
          strcat(tmp_str, token);
          strcat(tmp_str, " ");
        }
      }
    }
    if(tmp_str[0] != '\0') {
      tmp_str[(strlen(tmp_str)-1)] = '\0';
      printf("%s\n", tmp_str);
    }
    fclose(procstream);
  }

  procstream = popen("df", "r");
  if(procstream != NULL) {
    printf("\nPartitions-/Filesystemdaten:\n");
    while( (tmp = fgetc(procstream)) != EOF) putchar(tmp);
    fclose(procstream);
  }

  procstream = popen("w", "r");
  if(procstream != NULL) {
    printf("\nEingeloggte User auf diesem System:\n");
    while( ((tmp = fgetc(procstream)) !=EOF) && (tmp != '\n'));
    while( (tmp = fgetc(procstream)) != EOF) putchar(tmp);
    fclose(procstream);
  }

  printf("\nby WSPse, V" CUR_VER "/" CUR_DAT "\n\n");
  return 0;
}

