/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Procedures for code conversion (codconv.c)
   created: Mark Wahl DL4YBG 93/12/19
   updated: Mark Wahl DL4YBG 96/03/21
*/

#undef TNT
#include "tnt.h"

struct codeconv_tab {
  char code;
  char conv_code;
  char conv_code1;
  char conv_code2;
};

/* flag if umlauts shall be displayed (!=0) or converted to 2 chars (==0) */
int umlaut;
extern int termcap;

static struct codeconv_tab codeconv_tab[] = {
  {0x8E,0xC4,'A','e'}, /* Ae */
  {0x99,0xD6,'O','e'}, /* Oe */
  {0x9A,0xDC,'U','e'}, /* Ue */
  {0x84,0xE4,'a','e'}, /* ae */
  {0x94,0xF6,'o','e'}, /* oe */
  {0x81,0xFC,'u','e'}, /* ue */
  {0xE1,0xDF,'s','s'}, /* ss */
  {0x9E,0xDF,'s','s'}, /* ss atari */
  {0,0,0,0}
};

void init_conv()
{
  umlaut = 1;
}

int conv_rx_to_local(code,newcode1,newcode2)
char code;
char *newcode1;
char *newcode2;
{
  int i;
  
  for (i = 0; (codeconv_tab[i].code != 0); i++) {
    if (code == codeconv_tab[i].code) {
      if (umlaut) {
        *newcode1 = codeconv_tab[i].conv_code;
        return(1);
      }
      else {
        *newcode1 = codeconv_tab[i].conv_code1;
        *newcode2 = codeconv_tab[i].conv_code2;
        return(2);
      }
    }
  }
  if ((code >= 0x80) && (code < 0xA0)) {
    *newcode1 = '^';
    *newcode2 = code - 0x20;
    return(2);
  }
  else return(0);
}

int conv_local_to_tx(code,newcode1,newcode2)
char code;
char *newcode1;
char *newcode2;
{
  int i;
  
  for (i = 0; (codeconv_tab[i].conv_code != 0); i++) {
    if (code == codeconv_tab[i].conv_code) {
      if (umlaut) {
        *newcode1 = codeconv_tab[i].code;
        return(1);
      }
      else {
        *newcode1 = codeconv_tab[i].conv_code1;
        *newcode2 = codeconv_tab[i].conv_code2;
        return(2);
      }
    }
  }
  return(0);
}

int conv_umlaut_to_tx(code,newcode)
char code;
char *newcode;
{
  int i;
  
  for (i = 0; (codeconv_tab[i].conv_code != 0); i++) {
    if (code == codeconv_tab[i].conv_code) {
      if (umlaut) {
        *newcode = codeconv_tab[i].code;
        return(1);
      }
    }
  }
  return(0);
}

int conv_local_to_umlaut(code,newcode1,newcode2)
char code;
char *newcode1;
char *newcode2;
{
  int i;
  
  for (i = 0; (codeconv_tab[i].conv_code != 0); i++) {
    if (code == codeconv_tab[i].conv_code) {
      if (!umlaut) {
        *newcode1 = codeconv_tab[i].conv_code1;
        *newcode2 = codeconv_tab[i].conv_code2;
        return(2);
      }
      else if (!termcap) {
        *newcode1 = codeconv_tab[i].code;
        return(1);
      }
    }
  }
  return(0);
}
