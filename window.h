/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1995 by Mark Wahl
   For license details see documentation
   include file for window handling (window.h)
   created: Mark Wahl DL4YBG 93/07/30
   updated: Mark Wahl DL4YBG 95/05/19
*/

/* minimum and maximum columns per line, don't change limits
   unless you know what you are doing... */
#define MINCOLS 80
#define MAXCOLS 160

#define BELL	  '\007'
#define BS        '\010'
#define TAB       '\011'
#define LF	  '\012'
#define CR	  '\015'
#define ESC	  '\033'
#define SPACE	  ' '
#define DEL	  '\177'
#define ESC2	  '\233'

struct window {
  char *window_buffer;	/* pointer to buffer for window data */
  int num_lines;	/* number of lines in this window */
  int line;		/* current line (counting 0 to num_lines-1) */
  int column;		/* current column (counting 0 to NUM_COLUMNS-1) */
  int line_offset;	/* physical line which is first logical line */
  int phys_line;	/* physical line in buffer,
  			   (line+line_offset) modulo line_offset */
  int real;		/* flag if part of the window is on real screen
  			   0 : not on real screen, else : number of part */
  char attribut;	/* current attribute of charaters */
  int statusflag;	/* statusflag for escape/control sequences */
  int insertflag;	/* flag if insertmode active */
  int holdflag;         /* flag if windowposition shall be holden */
  int first_win_line;   /* first line of display-window */

};

struct win_info {
  struct window *win;	/* pointer to window struct */
  int first_real_line;	/* first real line of window */
  int last_real_line;	/* last real line of window */
  int first_log_line;	/* first logical line of window */
  int last_log_line;	/* last logical line of window 
  			   == first_log_line + win_num_lines - 1 */
  int win_num_lines;	/* number of displayed lines */
  int pagesize;         /* number of lines for page-scroll */
};

struct real_layout {
  struct window *win;	/* window to use */
  int first_real_line;	/* first line of window on screen */
  int win_num_lines;	/* number of displayed lines */
  int pagesize;         /* number of lines for page-scroll */
};

#define LAYOUTPARTS 5
