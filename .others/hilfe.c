/* TNT - RUN-PROGRAMM (hilfe)
 *
 * Written by Matthias Hensler, 25.8.98
 * Copyright WSPse 1998
 * eMail: wsp@gmx.de
 *
 * Free software. Redistribution and modify under the terms of GNU Public
 * License.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "tntrun.h"

#define CUR_VER "1.0a"
#define CUR_DAT "25.8.98"

int main (void)
{
  struct stat st;
  struct dirent *de;
  DIR *run_dir;
  int tmp;
  char tmp_str[200];
  FILE *help_file;

  run_dir = opendir( TNT_RUN_PATH ); /* defined in tntrun.h */
  if(run_dir == NULL) {
    perror("diropen");
    return 1;
  }

  printf("Uebersicht der ausfuehrbaren Programme\n"
         "Starten mit \"//run <programm>\".\n\n");

  while(1) {
    de = readdir(run_dir);
    if(de == NULL) break;
    if(de->d_name[0] != '.') {
      strcpy(tmp_str, TNT_RUN_PATH);
      strcat(tmp_str, "/");
      strcat(tmp_str, de->d_name);
      tmp = stat(tmp_str, &st);
      if(tmp == 0) {
        tmp = S_ISREG(st.st_mode);
        if(tmp != 0) {
          strcpy(tmp_str, TNT_RUN_PATH);
          strcat(tmp_str, "/.");
          strcat(tmp_str, de->d_name);
          help_file = fopen(tmp_str, "r");
          if(help_file == NULL) {
            printf("<%s> (keine Hilfe verfuegbar)\n\n", de->d_name);
          } else {
            printf("<%s>\n", de->d_name);
            while( (tmp=fgetc(help_file)) != EOF) putchar(tmp);
            fclose(help_file);
            printf("\n");
          }
        }
      }
    }
  }
  closedir(run_dir);
  printf("\nby WSPse, V" CUR_VER "/" CUR_DAT ".\n");
  return 0;
}
