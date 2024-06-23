/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993,1994 by Mark Wahl
   For license details see documentation
   include file for keyboard codes (keys.h)
   created: Mark Wahl DL4YBG 94/02/05
   updated: Mark Wahl DL4YBG 94/12/19
   updated: mayer hans oe1smc, 6.1.1999 
   updated: Berndt Josef Wulf, 99/02/17
*/

#ifdef TNT_LINUX
/* keycodes used if TERM not xterm */
static struct func_keys special_keys[] = {
 {"\033m",0,C_MONITOR},		/* ALT-m */
 {"\033c",0,C_COMMAND},		/* ALT-c */
 {"\033q",0,C_CONNECT},		/* ALT-q */
 {"\033b",0,C_MAILBOX},		/* ALT-b */
 {"\033x",0,C_EXTMON},		/* ALT-x */
 {"\033l",0,C_BOXLIST},		/* ALT-l */
 {"\033s",0,C_MHEARD},		/* ALT-s */
 {"\033h",0,C_HELP},		/* ALT-h */
 {"\033p",0,C_PAUSE},		/* ALT-p */
 {"\033M",0,C_MONITOR},		/* ALT-M */
 {"\033C",0,C_COMMAND},		/* ALT-C */
 {"\033Q",0,C_CONNECT},		/* ALT-Q */
 {"\033B",0,C_MAILBOX},		/* ALT-B */
 {"\033X",0,C_EXTMON},		/* ALT-X */
 {"\033L",0,C_BOXLIST},		/* ALT-L */
 {"\033S",0,C_MHEARD},		/* ALT-S */
 {"\033H",0,C_HELP},		/* ALT-H */
 {"\033P",0,C_PAUSE},		/* ALT-P */
/*&&&not working */
// {"\033[[A",1,1},		/* F1 */
// {"\033[[B",1,2},		/* F2 */
// {"\033[[C",1,3},		/* F3 */
// {"\033[[D",1,4},		/* F4 */
// {"\033[[E",1,5},		/* F5 */
/*&&&new*/
 {"\033[OP",1,1},		/* F1 */
 {"\033[OQ",1,2},		/* F2 */
 {"\033[OR",1,3},		/* F3 */
 {"\033[OS",1,4},		/* F4 */
 {"\033[15~",1,5},		/* F5 */
/*&&&--*/
 {"\033[17~",1,6},		/* F6 */
 {"\033[18~",1,7},		/* F7 */
 {"\033[19~",1,8},		/* F8 */
 {"\033[20~",1,9},		/* F9 */
 {"\033[21~",1,0},		/* F10 */
/* {"\033[28~",0,C_MONITOR},	 * F11 old */
/* {"\033[29~",0,C_COMMAND},	 * F12 old */
 {"\033[23~",0,C_MONITOR},	/* F11 */
 {"\033[24~",0,C_COMMAND},	/* F12 */
 {"\033[5~",0,C_WINUP},		/* PAGEUP */
 {"\033[6~",0,C_WINDWN},	/* PAGEDWN */
 {"\033[A",0,C_CUUP},		/* CURSOR UP */
 {"\033[B",0,C_CUDWN},		/* CURSOR DOWN */
 {"\033[C",0,C_CURIGHT},	/* CURSOR RIGHT */
 {"\033[D",0,C_CULEFT},		/* CURSOR LEFT */
 {"\033[2~",0,C_INSERT},	/* INSERT */
 {"\033[3~",0,C_DELCHAR},	/* DELETE */
 {"\033[1~",0,C_CUTOP},		/* HOME */
 {"\033[4~",0,C_CUBOT},		/* END */
/* {"\033[23~",2,1},		 * SF1 */
/* {"\033[24~",2,2},		 * SF2 */
 {"\033[25~",2,3},		/* SF3 */
 {"\033[26~",2,4},		/* SF4 */
 {"\033[28~",2,5},		/* SF5 */
 {"\033[29~",2,6},		/* SF6 */
 {"\033[31~",2,7},		/* SF7 */
 {"\033[32~",2,8},		/* SF8 */
 {"\033[33~",2,9},		/* SF9 */
 {"\033[34~",2,10},		/* SF10 */
/* {"\033[23~",2,11},		 * SF11 */
/* {"\033[24~",2,12},		 * SF12 */
 {"\0331",2,1},			/* ALT-1 */
 {"\0332",2,2},			/* ALT-2 */
 {"\0333",2,3},			/* ALT-3 */
 {"\0334",2,4},			/* ALT-4 */
 {"\0335",2,5},			/* ALT-5 */
 {"\0336",2,6},			/* ALT-6 */
 {"\0337",2,7},			/* ALT-7 */
 {"\0338",2,8},			/* ALT-8 */
 {"\0339",2,9},			/* ALT-9 */
 {"\0330",2,10},		/* ALT-0 */
 {"\033-",2,11},		/* ALT-- */
 {"\033=",2,12}			/* ALT-= */
};

#define NUMFUNC 61 /* number of function-keys */
#endif

#ifdef TNT_SOLARIS
/* keycodes used if TERM not xterm */
static struct func_keys special_keys[] = {
 {"\033m",0,C_MONITOR},		/* ALT-m */
 {"\033c",0,C_COMMAND},		/* ALT-c */
 {"\033[26~",0,C_COMMAND},		/* zuruecknehmen */
 {"\033q",0,C_CONNECT},		/* ALT-q */
 {"\033b",0,C_MAILBOX},		/* ALT-b */
 {"\033x",0,C_EXTMON},		/* ALT-x */
 {"\033l",0,C_BOXLIST},		/* ALT-l */
 {"\033s",0,C_MHEARD},		/* ALT-s */
 {"\033h",0,C_HELP},		/* ALT-h */
 {"\033p",0,C_PAUSE},		/* ALT-p */
 {"\033M",0,C_MONITOR},		/* ALT-M */
 {"\033C",0,C_COMMAND},		/* ALT-C */
 {"\033Q",0,C_CONNECT},		/* ALT-Q */
 {"\033B",0,C_MAILBOX},		/* ALT-B */
 {"\033X",0,C_EXTMON},		/* ALT-X */
 {"\033L",0,C_BOXLIST},		/* ALT-L */
 {"\033S",0,C_MHEARD},		/* ALT-S */
 {"\033H",0,C_HELP},		/* ALT-H */
 {"\033P",0,C_PAUSE},		/* ALT-P */
 {"\033[224z",1,1},		/* F1 */
 {"\033[225z",1,2},		/* F2 */
 {"\033[226z",1,3},		/* F3 */
 {"\033[227z",1,4},		/* F4 */
 {"\033[228z",1,5},		/* F5 */
 {"\033[229z",1,6},		/* F6 */
 {"\033[230z",1,7},		/* F7 */
 {"\033[231z",1,8},		/* F8 */
 {"\033[232z",1,9},		/* F9 */
 {"\033[233z",1,0},		/* F10 */
 {"\033[5~",0,C_WINUP},		/* PAGEUP */
 {"\033[6~",0,C_WINDWN},	/* PAGEDWN */
 {"\033[A",0,C_CUUP},		/* CURSOR UP */
 {"\033[B",0,C_CUDWN},		/* CURSOR DOWN */
 {"\033[C",0,C_CURIGHT},	/* CURSOR RIGHT */
 {"\033[D",0,C_CULEFT},		/* CURSOR LEFT */
 {"\033[2~",0,C_INSERT},	/* INSERT */
 {"\033[3~",0,C_DELCHAR},	/* DELETE */
 {"\033[1~",0,C_CUTOP},		/* HOME */
 {"\033[4~",0,C_CUBOT},		/* END */
 {"\033[q",2,1},		 /* * SF1 */
 {"\033[w",2,2},		 /* * SF2 */
 {"\033[e",2,3},		/* SF3 */
 {"\033[r",2,4},		/* SF4 */
 {"\033[t",2,5},		/* SF5 */
 {"\033[z",2,6},		/* SF6 */
 {"\033[u",2,7},		/* SF7 */
 {"\033[i",2,8},		/* SF8 */
 {"\033[o",2,9},		/* SF9 */
 {"\033[p",2,10},		/* SF10 */
 {"\0331",2,1},			/* ALT-1 */
 {"\0332",2,2},			/* ALT-2 */
 {"\0333",2,3},			/* ALT-3 */
 {"\0334",2,4},			/* ALT-4 */
 {"\0335",2,5},			/* ALT-5 */
 {"\0336",2,6},			/* ALT-6 */
 {"\0337",2,7},			/* ALT-7 */
 {"\0338",2,8},			/* ALT-8 */
 {"\0339",2,9},			/* ALT-9 */
 {"\0330",2,10},		/* ALT-0 */
 {"\033-",2,11},		/* ALT-- */
 {"\033=",2,12}			/* ALT-= */
};

#define NUMFUNC 61 /* number of function-keys */
#endif
/* end SUN */

#ifdef TNT_ISC
/* keycodes used if TERM not xterm */
static struct func_keys special_keys[] = {
 {"\033m",0,C_MONITOR},		/* ALT-m */
 {"\033c",0,C_COMMAND},		/* ALT-c */
 {"\033q",0,C_CONNECT},		/* ALT-q */
 {"\033b",0,C_MAILBOX},		/* ALT-b */
 {"\033x",0,C_EXTMON},		/* ALT-x */
 {"\033l",0,C_BOXLIST},		/* ALT-l */
 {"\033s",0,C_MHEARD},		/* ALT-s */
 {"\033h",0,C_HELP},		/* ALT-h */
 {"\033p",0,C_PAUSE},		/* ALT-p */
 {"\033M",0,C_MONITOR},		/* ALT-M */
 {"\033C",0,C_COMMAND},		/* ALT-C */
 {"\033Q",0,C_CONNECT},		/* ALT-Q */
 {"\033B",0,C_MAILBOX},		/* ALT-B */
 {"\033X",0,C_EXTMON},		/* ALT-X */
 {"\033L",0,C_BOXLIST},		/* ALT-L */
 {"\033S",0,C_MHEARD},		/* ALT-S */
 {"\033H",0,C_HELP},		/* ALT-H */
 {"\033P",0,C_PAUSE},		/* ALT-P */
 {"\033OP",1,1},		/* F1 */
 {"\033OQ",1,2},		/* F2 */
 {"\033OR",1,3},		/* F3 */
 {"\033OS",1,4},		/* F4 */
 {"\033OT",1,5},		/* F5 */
 {"\033OU",1,6},		/* F6 */
 {"\033OV",1,7},		/* F7 */
 {"\033OW",1,8},		/* F8 */
 {"\033OX",1,9},		/* F9 */
 #ifdef TNT_CHAMBER
 {"\033OY",1,10},		/* F10 */
 #else
 {"\033OY",1,0},		/* F10 */
 #endif
 {"\033OZ",0,C_MONITOR},	/* F11 */
 {"\033OA",0,C_COMMAND},	/* F12 */
 {"\033[23~",0,C_MONITOR},	/* F11 */
 {"\033[24~",0,C_COMMAND},	/* F12 xterm */
 {"\033[@",0,C_INSERT},		/* INSERT */
 {"\033[V",0,C_WINUP},	        /* PAGEUP */
 {"\033[U",0,C_WINDWN},		/* PAGEDWN */
 {"\033[A",0,C_CUUP},		/* CURSOR UP */
 {"\033[B",0,C_CUDWN},	        /* CURSOR DOWN */
 {"\033[C",0,C_CURIGHT},	/* CURSOR RIGHT */
 {"\033[D",0,C_CULEFT},		/* CURSOR LEFT */
 {"\033Op",2,1},		/* SF1 */
 {"\033Oq",2,2},		/* SF2 */
 {"\033Or",2,3},		/* SF3 */
 {"\033Os",2,4},		/* SF4 */
 {"\033Ot",2,5},		/* SF5 */
 {"\033Ou",2,6},		/* SF6 */
 {"\033Ov",2,7},		/* SF7 */
 {"\033Ow",2,8},		/* SF8 */
 {"\033Ox",2,9},		/* SF9 */
 {"\033Oy",2,10},		/* SF10 */
 {"\033Oz",2,11},		/* SF11 */
 {"\033Oa",2,12}		/* SF12 */
};


#define NUMFUNC 51 /* number of function-keys */
#endif

#ifdef TNT_NETBSD
/* keycodes used if TERM not xterm */
static struct func_keys special_keys[] = {
 {"\033m",0,C_MONITOR},                /* ALT-m */
 {"\033c",0,C_COMMAND},                /* ALT-c */
 {"\033q",0,C_CONNECT},                /* ALT-q */
 {"\033b",0,C_MAILBOX},                /* ALT-b */
 {"\033x",0,C_EXTMON},         /* ALT-x */
 {"\033l",0,C_BOXLIST},                /* ALT-l */
 {"\033s",0,C_MHEARD},         /* ALT-s */
 {"\033h",0,C_HELP},           /* ALT-h */
 {"\033p",0,C_PAUSE},          /* ALT-p */
 {"\033M",0,C_MONITOR},                /* ALT-M */
 {"\033C",0,C_COMMAND},                /* ALT-C */
 {"\033Q",0,C_CONNECT},                /* ALT-Q */
 {"\033B",0,C_MAILBOX},                /* ALT-B */
 {"\033X",0,C_EXTMON},         /* ALT-X */
 {"\033L",0,C_BOXLIST},                /* ALT-L */
 {"\033S",0,C_MHEARD},         /* ALT-S */
 {"\033H",0,C_HELP},           /* ALT-H */
 {"\033P",0,C_PAUSE},          /* ALT-P */
 {"\033[[A",1,1},              /* F1 */
 {"\033[[B",1,2},              /* F2 */
 {"\033[[C",1,3},              /* F3 */
 {"\033[[D",1,4},              /* F4 */
 {"\033[[E",1,5},              /* F5 */
 {"\033[17~",1,6},             /* F6 */
 {"\033[18~",1,7},             /* F7 */
 {"\033[19~",1,8},             /* F8 */
 {"\033[20~",1,9},             /* F9 */
 {"\033[21~",1,0},             /* F10 */
/* {"\033[28~",0,C_MONITOR},    * F11 old */
/* {"\033[29~",0,C_COMMAND},    * F12 old */
 {"\033[23~",0,C_MONITOR},     /* F11 */
 {"\033[24~",0,C_COMMAND},     /* F12 */
 {"\033[5~",0,C_WINUP},                /* PAGEUP */
 {"\033[6~",0,C_WINDWN},       /* PAGEDWN */
 {"\033[A",0,C_CUUP},          /* CURSOR UP */
 {"\033[B",0,C_CUDWN},         /* CURSOR DOWN */
 {"\033[C",0,C_CURIGHT},       /* CURSOR RIGHT */
 {"\033[D",0,C_CULEFT},                /* CURSOR LEFT */
 {"\033[2~",0,C_INSERT},       /* INSERT */
 {"\033[3~",0,C_DELCHAR},      /* DELETE */
 {"\033[1~",0,C_CUTOP},                /* HOME */
 {"\033[4~",0,C_CUBOT},                /* END */
/* {"\033[23~",2,1},            * SF1 */
/* {"\033[24~",2,2},            * SF2 */
 {"\033[25~",2,3},             /* SF3 */
 {"\033[26~",2,4},             /* SF4 */
 {"\033[28~",2,5},             /* SF5 */
 {"\033[29~",2,6},             /* SF6 */
 {"\033[31~",2,7},             /* SF7 */
 {"\033[32~",2,8},             /* SF8 */
 {"\033[33~",2,9},             /* SF9 */
 {"\033[34~",2,10},            /* SF10 */
/* {"\033[23~",2,11},           * SF11 */
/* {"\033[24~",2,12},           * SF12 */
 {"\0331",2,1},                        /* ALT-1 */
 {"\0332",2,2},                        /* ALT-2 */
 {"\0333",2,3},                        /* ALT-3 */
 {"\0334",2,4},                        /* ALT-4 */
 {"\0335",2,5},                        /* ALT-5 */
 {"\0336",2,6},                        /* ALT-6 */
 {"\0337",2,7},                        /* ALT-7 */
 {"\0338",2,8},                        /* ALT-8 */
 {"\0339",2,9},                        /* ALT-9 */
 {"\0330",2,10},               /* ALT-0 */
 {"\033-",2,11},               /* ALT-- */
 {"\033=",2,12}                        /* ALT-= */
};

#define NUMFUNC 61 /* number of function-keys */
#endif
 
/* keycodes used if TERM=xterm */
static struct func_keys special_keys_xterm[] = {
 {"\033m",0,C_MONITOR},		/* ALT-m */
 {"\033c",0,C_COMMAND},		/* ALT-c */
 {"\033q",0,C_CONNECT},		/* ALT-q */
 {"\033b",0,C_MAILBOX},		/* ALT-b */
 {"\033x",0,C_EXTMON},		/* ALT-x */
 {"\033l",0,C_BOXLIST},		/* ALT-l */
 {"\033s",0,C_MHEARD},		/* ALT-s */
 {"\033h",0,C_HELP},		/* ALT-h */
 {"\033p",0,C_PAUSE},		/* ALT-p */
 {"\033M",0,C_MONITOR},		/* ALT-M */
 {"\033C",0,C_COMMAND},		/* ALT-C */
 {"\033Q",0,C_CONNECT},		/* ALT-Q */
 {"\033B",0,C_MAILBOX},		/* ALT-B */
 {"\033X",0,C_EXTMON},		/* ALT-X */
 {"\033L",0,C_BOXLIST},		/* ALT-L */
 {"\033S",0,C_MHEARD},		/* ALT-S */
 {"\033H",0,C_HELP},		/* ALT-H */
 {"\033P",0,C_PAUSE},		/* ALT-P */
 {"\033[11~",1,1},		/* F1 xterm */
 {"\033[12~",1,2},		/* F2 xterm */
 {"\033[13~",1,3},		/* F3 xterm */
 {"\033[14~",1,4},		/* F4 xterm */
 {"\033[15~",1,5},		/* F5 xterm */
 {"\033[28~",1,1},		/* F1 xterm2 */
 {"\033[33~",1,2},		/* F2 xterm2 */
 {"\033[34~",1,3},		/* F3 xterm2 */
 {"\033[29~",1,4},		/* F4 xterm2 */
 {"\033[32~",1,5},		/* F5 xterm2 */
/*&&&+++*/
 {"\033OP",1,1},		/* F1 xterm3 */
 {"\033OQ",1,2},		/* F2 xterm3 */
 {"\033OR",1,3},		/* F3 xterm3 */
 {"\033OS",1,4},		/* F4 xterm3 */
 {"\03315~",1,5},		/* F5 xterm3 */
/*&&&---*/
 {"\033[17~",1,6},		/* F6 */
 {"\033[18~",1,7},		/* F7 */
 {"\033[19~",1,8},		/* F8 */
 {"\033[20~",1,9},		/* F9 */
 {"\033[21~",1,0},		/* F10 */
 {"\033[23~",0,C_MONITOR},	/* F11 xterm */
 {"\033[24~",0,C_COMMAND},	/* F12 xterm */
 {"\033[2~",0,C_INSERT},	/* INSERT */
 {"\033[5~",0,C_WINUP},		/* PAGEUP */
 {"\033[6~",0,C_WINDWN},	/* PAGEDWN */
 {"\033[A",0,C_CUUP},		/* CURSOR UP */
 {"\033[B",0,C_CUDWN},		/* CURSOR DOWN */
 {"\033[C",0,C_CURIGHT},	/* CURSOR RIGHT */
 {"\033[D",0,C_CULEFT},		/* CURSOR LEFT */
 {"\033[1~",0,C_CUTOP},		/* HOME */
 {"\033[4~",0,C_CUBOT},		/* END */
 {"\033[7~",0,C_CUTOP},		/* HOME */
 {"\033[8~",0,C_CUBOT},		/* END */
 {"\033[H",0,C_CUTOP},		/* HOME */
 {"\033Ow",0,C_CUBOT},		/* END */
/*&&&*/
 {"\033[F",0,C_CUBOT},		/* END */
 {"\0331",2,1},			/* ALT-1 */
 {"\0332",2,2},			/* ALT-2 */
 {"\0333",2,3},			/* ALT-3 */
 {"\0334",2,4},			/* ALT-4 */
 {"\0335",2,5},			/* ALT-5 */
 {"\0336",2,6},			/* ALT-6 */
 {"\0337",2,7},			/* ALT-7 */
 {"\0338",2,8},			/* ALT-8 */
 {"\0339",2,9},			/* ALT-9 */
 {"\0330",2,10},		/* ALT-0 */
 {"\033-",2,11},		/* ALT-- */
 {"\033=",2,12}			/* ALT-= */
};

#define NUMFUNC_XTERM 60 /* number of function-keys */
