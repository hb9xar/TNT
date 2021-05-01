/* Header for module dpglobal, generated by p2c */
#ifndef DPGLOBAL_H
#define DPGLOBAL_H


#ifndef PASTRIX_H
#include "pastrix.h"
#endif


#ifdef DPGLOBAL_G
# define vextern
#else
# define vextern extern
#endif


/* ************************* Das Hauptprogramm ************************** */


#define maxbackcommand  1   /* DOSkey f�r die Box-Console        */


#define mindiskdefault  300000L

#define maxstrsize      120

#define dpreadoutmagic  "#DPRDOUT66521A#"
#define txt203          "*** out of disk-memory"
#define txt18           "disk read error"
#define txt19           "disk write error"
#define txt21           "#*DPLZHDP*#"
#define memfailtxt      "*** out of memory"




typedef enum {
  nix, diebox, baycom, f6fbb, wampes, diebox_bid
} boxtype;

typedef Char str2[3];

typedef Char str3[4];

typedef Char str4[5];

typedef Char str5[6];

typedef Char str6[7];

typedef Char str8[9];

typedef Char str9[10];

typedef Char str10[11];

typedef Char str11[12];

typedef Char str12[13];

typedef Char str15[16];

typedef Char str18[19];

typedef Char str20[21];

typedef Char str25[26];

typedef Char str30[31];

typedef Char str40[41];

typedef Char str50[51];

typedef Char str60[61];

typedef Char str65[66];

typedef Char str80[81];

typedef Char str120[121];

typedef Char str160[161];

typedef Char str255[256];

typedef Char maxstr[maxstrsize + 1];

typedef Char pathstr[256];



typedef uchar hostinfotype[300];


typedef enum {
  NOP, THEBOX_USER, W0RLI_SF, F6FBB_SF, F6FBB_USER, BAYCOM_USER, WAMPES_USER,
  W0RLI_USER, AA4RE_USER, F6FBB_USER_314, RAW_IMPORT
} cutboxtyp;



vextern str4 dp_vnr;
vextern str3 dp_vnr_sub;
vextern str8 dp_date;

vextern str6 myqthwwloc, Console_call;

vextern pathstr savedir, boxdir, tempdir, editprg, impmaildir, spooldir,
		whotalks_lan;

vextern Char lastproc[256];

vextern double mylaenge, mybreite;

vextern unsigned short wd_while_ext_prg;


vextern boolean disk_full;
vextern long mindiskavail;
vextern Char ramdisk;

vextern uchar *whichlangmem;
vextern long whichlangsize;

vextern boolean wd_active;

vextern boolean ende;
vextern Char laufwerk;


#undef vextern

#endif /*DPGLOBAL_H*/

/* End. */
