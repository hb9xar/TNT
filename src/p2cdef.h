/* additional defines for p2c */

#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <assert.h>

#define FO_READ O_RDONLY
#define FO_WRITE O_WRONLY
#define FO_RW O_RDWR
#define FO_CREATE O_CREAT|O_RDWR|O_TRUNC

typedef struct DTA {
  char d_reserved[21];
  char d_attrib;
  int d_time;
  int d_date;
  int d_length;
  char d_fname[256];
} DTA;

#define Signed    signed
#define Void       void      /* Void f() = procedure */
#define Const     const
#define Volatile   volatile

#define Register    register  /* Register variables */
#define Char        char      /* Characters (not bytes) */

#ifndef Static
# define Static     static    /* Private global funcs and vars */
#endif

#ifndef Local
# define Local      static    /* Nested functions */
#endif


typedef unsigned char uchar;
typedef unsigned char boolean;

#ifndef true
# define true    1
# define false   0
#endif

#ifndef TRUE
# define TRUE    1
# define FALSE   0
#endif


