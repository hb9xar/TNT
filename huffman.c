/**************************************************************
        lzhuf.c
        written by Haruyasu Yoshizaki 11/20/1988
        some minor changes 4/6/1989
        comments translated by Haruhiko Okumura 4/7/1989
**************************************************************/

/**************************************************************
** modified for use in DPBOX by Mark Wahl, DL4YBG            **
** last update 96/05/27                                      **
**************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static int in_memory, out_memory;
FILE *infile, *outfile;
static unsigned long int  textsize, codesize;
static char *srcbuf, *destbuf;
static char *srcbufptr, *destbufptr;
static long srclen, destlen;
static long srcbuflen, destbuflen;

/* read byte from input */
static int read_char()
{
  int chr;
  
  if (in_memory) {
    if (srclen < srcbuflen) {
      chr = *srcbufptr;
      srcbufptr++;
      srclen++;
      return(chr);
    }
    else {
      return(EOF);
    }
  }
  else {
    return(getc(infile));
  }
}

/* write byte to output */
static int wri_char(chr)
int chr;
{
  if (out_memory) {
    if (destlen < destbuflen) {
      *destbufptr = (char) chr;
      destbufptr++;
      destlen++;
      return((char) chr);
    }
    else {
      return(EOF);
    }
  }
  else {
    return (putc(chr,outfile)); 
  }
}

/********** LZSS compression **********/

#define N               2048    /* buffer size */
/* Attention: When using this file for f6fbb-type compressed data exchange,
   set N to 2048 ! */
#define F               60      /* lookahead buffer size */
#define THRESHOLD       2
#define NIL             N       /* leaf of tree */

static unsigned char
                text_buf[N + F - 1];
static short    match_position, match_length,
                lson[N + 1], rson[N + 257], dad[N + 1];

static void InitTree(void)  /* initialize trees */
{
        short  i;

        for (i = N + 1; i <= N + 256; i++)
                rson[i] = NIL;                  /* root */
        for (i = 0; i < N; i++)
                dad[i] = NIL;                   /* node */
}

static void InsertNode(short r)  /* insert to tree */
{
        short i, p, cmp;
        unsigned char  *key;
        unsigned short c;

        cmp = 1;
        key = &text_buf[r];
        p = N + 1 + key[0];
        rson[r] = lson[r] = NIL;
        match_length = 0;
        for ( ; ; ) {
                if (cmp >= 0) {
                        if (rson[p] != NIL)
                                p = rson[p];
                        else {
                                rson[p] = r;
                                dad[r] = p;
                                return;
                        }
                } else {
                        if (lson[p] != NIL)
                                p = lson[p];
                        else {
                                lson[p] = r;
                                dad[r] = p;
                                return;
                        }
                }
                for (i = 1; i < F; i++)
                        if ((cmp = key[i] - text_buf[p + i]) != 0)
                                break;
                if (i > THRESHOLD) {
                        if (i > match_length) {
                                match_position = ((r - p) & (N - 1)) - 1;
                                if ((match_length = i) >= F)
                                        break;
                        }
                        if (i == match_length) {
                                if ((c = ((r - p) & (N - 1)) - 1) < match_position) {
                                        match_position = c;
                                }
                        }
                }
        }
        dad[r] = dad[p];
        lson[r] = lson[p];
        rson[r] = rson[p];
        dad[lson[p]] = r;
        dad[rson[p]] = r;
        if (rson[dad[p]] == p)
                rson[dad[p]] = r;
        else
                lson[dad[p]] = r;
        dad[p] = NIL;  /* remove p */
}

static void DeleteNode(short p)  /* remove from tree */
{
        short q;

        if (dad[p] == NIL)
                return;                 /* not registered */
        if (rson[p] == NIL)
                q = lson[p];
        else
        if (lson[p] == NIL)
                q = rson[p];
        else {
                q = lson[p];
                if (rson[q] != NIL) {
                        do {
                                q = rson[q];
                        } while (rson[q] != NIL);
                        rson[dad[q]] = lson[q];
                        dad[lson[q]] = dad[q];
                        lson[q] = lson[p];
                        dad[lson[p]] = q;
                }
                rson[q] = rson[p];
                dad[rson[p]] = q;
        }
        dad[q] = dad[p];
        if (rson[dad[p]] == p)
                rson[dad[p]] = q;
        else
                lson[dad[p]] = q;
        dad[p] = NIL;
}

/* Huffman coding */

#define N_CHAR          (256 - THRESHOLD + F)
                                /* kinds of characters (character code = 0..N_CHAR-1) */
#define T               (N_CHAR * 2 - 1)        /* size of table */
#define R               (T - 1)                 /* position of root */
#define MAX_FREQ        0x8000          /* updates tree when the */
                                        /* root frequency comes to this value. */
typedef unsigned char uchar;


/* table for encoding and decoding the upper 6 bits of position */

/* for encoding */
static uchar p_len[64] = {
        0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

static uchar p_code[64] = {
        0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
        0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
        0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
        0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
        0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
        0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
        0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* for decoding */
static uchar d_code[256] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
        0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
        0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
        0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
        0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
        0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
        0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
        0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
        0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
        0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
        0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
        0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
        0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
        0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

static uchar d_len[256] = {
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

static unsigned short freq[T + 1];   /* frequency table */

static short prnt[T + N_CHAR];   /* pointers to parent nodes, except for the */
                        /* elements [T..T + N_CHAR - 1] which are used to get */
                        /* the positions of leaves corresponding to the codes. */

static short son[T];      /* pointers to child nodes (son[], son[] + 1) */

static unsigned short getbuf;
static uchar getlen;

static short GetBit(void)        /* get one bit */
{
        short i;

        while (getlen <= 8) {
                if ((i = read_char()) < 0) i = 0;
                getbuf |= i << (8 - getlen);
                getlen += 8;
        }
        i = getbuf;
        getbuf <<= 1;
        getlen--;
        return (i < 0);
}

static short GetByte(void)       /* get one byte */
{
        unsigned short i;
        short temp;

        while (getlen <= 8) {
                if ((temp = read_char()) < 0) i = 0;
                else i = temp;
                getbuf |= i << (8 - getlen);
                getlen += 8;
        }
        i = getbuf;
        getbuf <<= 8;
        getlen -= 8;
        return i >> 8;
}

static unsigned putbuf;
static uchar putlen;

static int Putcode(short l, unsigned short c)   /* output c bits of code */
{
        putbuf |= c >> putlen;
        if ((putlen += l) >= 8) {
                if (wri_char(putbuf >> 8) == EOF) return(1);
                if ((putlen -= 8) >= 8) {
                        if (wri_char(putbuf) == EOF) return(1);
                        codesize += 2;
                        putlen -= 8;
                        putbuf = c << (l - putlen);
                } else {
                        putbuf <<= 8;
                        codesize++;
                }
        }
        return(0);
}


/* initialization of tree */

static void StartHuff(void)
{
        short i, j;

        for (i = 0; i < N_CHAR; i++) {
                freq[i] = 1;
                son[i] = i + T;
                prnt[i + T] = i;
        }
        i = 0; j = N_CHAR;
        while (j <= R) {
                freq[j] = freq[i] + freq[i + 1];
                son[j] = i;
                prnt[i] = prnt[i + 1] = j;
                i += 2; j++;
        }
        freq[T] = 0xffff;
        prnt[R] = 0;
}


/* reconstruction of tree */

static void reconst(void)
{
        short i, j, k;
        unsigned short f, l;

        /* collect leaf nodes in the first half of the table */
        /* and replace the freq by (freq + 1) / 2. */
        j = 0;
        for (i = 0; i < T; i++) {
                if (son[i] >= T) {
                        freq[j] = (freq[i] + 1) / 2;
                        son[j] = son[i];
                        j++;
                }
        }
        /* begin constructing tree by connecting sons */
        for (i = 0, j = N_CHAR; j < T; i += 2, j++) {
                k = i + 1;
                f = freq[j] = freq[i] + freq[k];
                for (k = j - 1; f < freq[k]; k--);
                k++;
                l = (j - k) * 2;
                memmove(&freq[k + 1], &freq[k], l);
                freq[k] = f;
                memmove(&son[k + 1], &son[k], l);
                son[k] = i;
        }
        /* connect prnt */
        for (i = 0; i < T; i++) {
                if ((k = son[i]) >= T) {
                        prnt[k] = i;
                } else {
                        prnt[k] = prnt[k + 1] = i;
                }
        }
}


/* increment frequency of given code by one, and update tree */

static void update(short c)
{
        short i, j, k, l;

        if (freq[R] == MAX_FREQ) {
                reconst();
        }
        c = prnt[c + T];
        do {
                k = ++freq[c];

                /* if the order is disturbed, exchange nodes */
                if (k > freq[l = c + 1]) {
                        while (k > freq[++l]);
                        l--;
                        freq[c] = freq[l];
                        freq[l] = k;

                        i = son[c];
                        prnt[i] = l;
                        if (i < T) prnt[i + 1] = l;

                        j = son[l];
                        son[l] = i;

                        prnt[j] = c;
                        if (j < T) prnt[j + 1] = c;
                        son[c] = j;

                        c = l;
                }
        } while ((c = prnt[c]) != 0);   /* repeat up to root */
}

static unsigned code, len;

static int EncodeChar(unsigned short c)
{
        unsigned short i;
        short j, k;

        i = 0;
        j = 0;
        k = prnt[c + T];

        /* travel from leaf to root */
        do {
                i >>= 1;

                /* if node's address is odd-numbered, choose bigger brother node */
                if (k & 1) i += 0x8000;

                j++;
        } while ((k = prnt[k]) != R);
        if (Putcode(j, i)) return(1);
        code = i;
        len = j;
        update(c);
        return(0);
}

static int EncodePosition(unsigned short c)
{
        unsigned short i;

        /* output upper 6 bits by table lookup */
        i = c >> 6;
        if (Putcode(p_len[i], (unsigned)p_code[i] << 8)) return(1);

        /* output lower 6 bits verbatim */
        if (Putcode(6, (c & 0x3f) << 10)) return(1);
        return(0);
}

static int EncodeEnd(void)
{
        if (putlen) {
                if (wri_char(putbuf >> 8) == EOF) return(1);
                codesize++;
        }
        return(0);
}

static short DecodeChar(void)
{
        unsigned short c;

        c = son[R];

        /* travel from root to leaf, */
        /* choosing the smaller child node (son[]) if the read bit is 0, */
        /* the bigger (son[]+1} if 1 */
        while (c < T) {
                c += GetBit();
                c = son[c];
        }
        c -= T;
        update(c);
        return c;
}

static short DecodePosition(void)
{
        unsigned short i, j, c;

        /* recover upper 6 bits from table */
        i = GetByte();
        c = (unsigned)d_code[i] << 6;
        j = d_len[i];

        /* read lower 6 bits verbatim */
        j -= 2;
        while (j--) {
                i = (i << 1) + GetBit();
        }
        return c | (i & 0x3f);
}

/* compression */

static int Encode(void)  /* compression */
{
        short  i, c, len, r, s, last_match_length;

        textsize = 0;                   /* rewind and re-read */
        StartHuff();
        InitTree();
        s = 0;
        r = N - F;
        for (i = s; i < r; i++)
                text_buf[i] = ' ';
        for (len = 0; len < F && (c = read_char()) != EOF; len++)
                text_buf[r + len] = c;
        textsize = len;
        for (i = 1; i <= F; i++)
                InsertNode(r - i);
        InsertNode(r);
        do {
                if (match_length > len)
                        match_length = len;
                if (match_length <= THRESHOLD) {
                        match_length = 1;
                        if (EncodeChar(text_buf[r])) return(1);
                } else {
                        if (EncodeChar(255 - THRESHOLD + match_length))
                          return(1);
                        if (EncodePosition(match_position)) return(1);
                }
                last_match_length = match_length;
                for (i = 0; i < last_match_length &&
                                (c = read_char()) != EOF; i++) {
                        DeleteNode(s);
                        text_buf[s] = c;
                        if (s < F - 1)
                                text_buf[s + N] = c;
                        s = (s + 1) & (N - 1);
                        r = (r + 1) & (N - 1);
                        InsertNode(r);
                }
                textsize += i;
                while (i++ < last_match_length) {
                        DeleteNode(s);
                        s = (s + 1) & (N - 1);
                        r = (r + 1) & (N - 1);
                        if (--len) InsertNode(r);
                }
        } while (len > 0);
        if (EncodeEnd()) return(1);
        return(0);
}

static int Decode(textsize)  /* recover */
unsigned long int textsize;
{
        short  i, j, k, r, c;
        unsigned long int  count;

        StartHuff();
        for (i = 0; i < N - F; i++)
                text_buf[i] = ' ';
        r = N - F;
        for (count = 0; count < textsize; ) {
                c = DecodeChar();
                if (c < 256) {
                        if (wri_char(c) == EOF) return(1);
                        text_buf[r++] = c;
                        r &= (N - 1);
                        count++;
                } else {
                        i = (r - DecodePosition() - 1) & (N - 1);
                        j = c - 255 + THRESHOLD;
                        for (k = 0; k < j; k++) {
                                c = text_buf[(i + k) & (N - 1)];
                                if (wri_char(c) == EOF) return(1);
                                text_buf[r++] = c;
                                r &= (N - 1);
                                count++;
                        }
                }
        }
        return(0);
}

static void init_huf()
{
  textsize = 0;
  codesize = 0;
  getbuf = 0;
  getlen = 0;
  putbuf = 0;
  putlen = 0;
}

int enchuf(inputfile,outputfile,crlfconv)
char *inputfile;
char *outputfile;
int crlfconv;
{
  int error;
  char tempname[80];
  char *inputfptr;
  int conv;
  int bin;
  int prebin;
  int bintest;
  int c;

  conv = 0;
  if (crlfconv) {
    bin = 0;
    prebin = 0;
    bintest = 0;
    strcpy(tempname,"/tmp/hufXXXXXX");
    mkstemp(tempname);
    if ((infile = fopen(inputfile,"rb")) == NULL) {
      return(1);
    }
    if ((outfile = fopen(tempname,"wb")) == NULL) {
      fclose(infile);
      return(1);
    }
    while ((c = getc(infile)) != EOF) {
      if (!bin) {
        if ((char)c == '\n') {
          putc('\r',outfile);
          if (prebin) bin = 1;
        }
      }
      putc(c,outfile);
      if (!prebin) {
        switch (bintest) {
        case 0:
          if ((char)c == '\n') bintest++;
          break;
        case 1:
          if ((char)c == '#') bintest++;
          else if ((char)c != '\n') bintest = 0;
          break;
        case 2:
          if ((char)c == 'B') bintest++;
          else bintest = 0;
          break;
        case 3:
          if ((char)c == 'I') bintest++;
          else bintest = 0;
          break;
        case 4:
          if ((char)c == 'N') bintest++;
          else bintest = 0;
          break;
        case 5:  
          if ((char)c == '#') bintest++;
          else bintest = 0;
          break;
        case 6:  
          if (isdigit(c)) prebin = 1;
          else bintest = 0;
          break;
        default:
          bintest = 0;
          break;
        }
      }
    }
    fclose(infile);
    fclose(outfile);
    inputfptr = tempname;
    conv = 1;
  }
  else {
    inputfptr = inputfile;
  }
  in_memory = 0;
  out_memory = 0;
  init_huf();
  if ((infile = fopen(inputfptr,"rb")) == NULL) {
    if (conv) {
      unlink(tempname);
    }
    return(1);
  }
  if ((outfile = fopen(outputfile,"wb")) == NULL) {
    fclose(infile);
    if (conv) {
      unlink(tempname);
    }
    return(1);
  } 
  error = 0;
  fseek(infile, 0L, 2);
  textsize = ftell(infile);
  if (fwrite(&textsize, sizeof textsize, 1, outfile) < 1) {
    error = 1;
  }
  if (!error) {
    if (textsize == 0)
      error = 1;
    if (!error) {
      rewind(infile);
      if (Encode())
        error = 1;
    }
  }
  fclose(infile);
  fclose(outfile);
  if (conv) {
    unlink(tempname);
  }
  return(error);
}

int dechuf(inputfile,outputfile,crlfconv)
char *inputfile;
char *outputfile;
int crlfconv;
{
  int error;
  char tempname[80];
  char *outputfptr;
  int conv;
  int bin;
  int prebin;
  int bintest;
  int c;
  int only_cr;

  conv = 0;
  outputfptr = outputfile;
  if (crlfconv) {
    strcpy(tempname,"/tmp/hufXXXXXX");
    mkstemp(tempname);
    conv = 1;
    outputfptr = tempname;
  }
  in_memory = 0;
  out_memory = 0;
  init_huf();
  if ((infile = fopen(inputfile,"rb")) == NULL) {
    return(1);
  }
  if ((outfile = fopen(outputfptr,"wb")) == NULL) {
    fclose(infile);
    return(1);
  } 
  error = 0;
  if (fread(&textsize, sizeof textsize, 1, infile) < 1)
    error = 1;
  if (!error) {
    if (textsize == 0)
      error = 1;
    if (!error) {
      if (Decode(textsize))
        error = 1;
    }
  }
  fclose(infile);
  fclose(outfile);
  if ((conv) && (!error)) {
    bin = 0;
    prebin = 0;
    bintest = 0;
    if ((infile = fopen(tempname,"rb")) == NULL) {
      return(1);
    }
    if ((outfile = fopen(outputfile,"wb")) == NULL) {
      fclose(infile);
      unlink(tempname);
      return(1);
    }
    only_cr = 0;
    while ((c = getc(infile)) != EOF) {
      if (!bin) {
        if (only_cr && ((char)c != '\n')) putc('\n',outfile);
        only_cr = 0;
      }
      if (bin || ((char)c != '\r')) putc(c,outfile);
      if (!bin) {
        if ((char)c == '\r') only_cr = 1;
        if (prebin && ((char)c == '\n')) bin = 1;
      }
      if (!prebin) {
        switch (bintest) {
        case 0:
          if ((char)c == '\n') bintest++;
          break;
        case 1:
          if ((char)c == '#') bintest++;
          else if ((char)c != '\n') bintest = 0;
          break;
        case 2:
          if ((char)c == 'B') bintest++;
          else bintest = 0;
          break;
        case 3:
          if ((char)c == 'I') bintest++;
          else bintest = 0;
          break;
        case 4:
          if ((char)c == 'N') bintest++;
          else bintest = 0;
          break;
        case 5:  
          if ((char)c == '#') bintest++;
          else bintest = 0;
          break;
        case 6:  
          if (isdigit(c)) prebin = 1;
          else bintest = 0;
          break;
        default:
          bintest = 0;
          break;
        }
      }
    }
    fclose(infile);
    fclose(outfile);
  }
  if (conv) {
    unlink(tempname);
  }
  return(error);
}

int enchufmem(membase,osize,outbase,outsize,outputfile,crlfconv)
char *membase;
long osize;
char **outbase;
long *outsize;
char *outputfile;
int crlfconv;
{
  int error;
  char *ptr;
  int i;
  long size;
  int conv;
  char *tmpbuf;
  long tmplen;
  int bin;
  int prebin;
  int bintest;
  char c;
  char *tmpptr;
  char *srcptr;

  if (osize == 0) return(1);
  srcbuf = membase;
  size = osize;
  conv = 0;

  if (crlfconv) {
    tmpbuf = malloc(osize*2);
    if (tmpbuf == NULL) return(1);
    tmpptr = tmpbuf;
    tmplen = 0;
    srcbuf = membase;
    srcptr = srcbuf;
    srclen = 0;
    bin = 0;
    prebin = 0;
    bintest = 0;
    
    while (srclen < osize) {
      if (!bin) {
        if (*(srcptr) == '\n') {
          *(tmpptr) = '\r';
          tmpptr++;
          tmplen++;
          if (prebin) bin = 1;
        }
      }
      *(tmpptr) = *(srcptr);
      if (!prebin) {
        c = *(srcptr);
        switch (bintest) {
        case 0:
          if (c == '\n') bintest++;
          break;
        case 1:
          if (c == '#') bintest++;
          else if (c != '\n') bintest = 0;
          break;
        case 2:
          if (c == 'B') bintest++;
          else bintest = 0;
          break;
        case 3:
          if (c == 'I') bintest++;
          else bintest = 0;
          break;
        case 4:
          if (c == 'N') bintest++;
          else bintest = 0;
          break;
        case 5:  
          if (c == '#') bintest++;
          else bintest = 0;
          break;
        case 6:  
          if (isdigit((int)c)) prebin = 1;
          else bintest = 0;
          break;
        default:
          bintest = 0;
          break;
        }
      }
      tmplen++;
      srclen++;
      tmpptr++;
      srcptr++;
    }
    size = tmplen;
    srcbuf = tmpbuf;
    conv = 1;
  }

  in_memory = 1;
  out_memory = 1;
  init_huf();

  srcbufptr = srcbuf;
  srclen = 0;
  srcbuflen = size;

  /* first try memory */
  error = 0;
  destlen = 0;
  destbuflen = size + size/6;
  destbuf = malloc(destbuflen);   /* allocate buffer with security margin */
  if (destbuf != NULL) {
    destbufptr = destbuf;
    ptr = (char *)&size;
    /* send length */
    for (i=0;i<sizeof(size);i++) {
      if (wri_char(*ptr++) == EOF) {
        error = 1;
        break;
      }
    }
    if (!error) {
      if (!Encode()) {
        *outbase = destbuf;
        *outsize = destlen;
        if (conv)
          free(tmpbuf);
        return(0);
      }
    }
  }
  free(destbuf);

  /* output to memory failed, write to file */
  out_memory = 0;
  init_huf();

  srcbufptr = srcbuf;
  srclen = 0;
  *outbase = NULL;
  *outsize = 0;

  if ((outfile = fopen(outputfile,"wb")) == NULL) {
    if (conv)
      free(tmpbuf);
    return(1);
  } 
  error = 0;
  if (fwrite(&size, sizeof size, 1, outfile) < 1) {
    error = 1;
  }
  if (!error) {
    if (Encode())
      error = 1;
  }
  fclose(outfile);
  if (conv)
    free(tmpbuf);
  return(error);
}

int dechufmem(membase,size,outbase,outsize,outputfile,crlfconv)
char *membase;
long size;
char **outbase;
long *outsize;
char *outputfile;
int crlfconv;
{
  int error;
  char *ptr;
  int i;
  long packsize;
  char tempname[80];
  char *outputfptr;
  int conv;
  long worlen;
  long tmplen;
  char *tmpbuf;
  int bin;
  int prebin;
  int bintest;
  int c;
  int only_cr;

  conv = 0;
  outputfptr = outputfile;
  if (crlfconv) {
    strcpy(tempname,"/tmp/hufXXXXXX");
    mkstemp(tempname);
    conv = 1;
    outputfptr = tempname;
  }

  in_memory = 1;
  out_memory = 1;
  init_huf();

  srcbuf = membase;
  srcbufptr = srcbuf;
  srclen = 0;
  srcbuflen = size;
  if (srcbuflen < sizeof(packsize)) return(1);
  /* get length */
  ptr = (char *)&packsize;
  for (i=0;i<sizeof(packsize);i++) {
    *ptr++ = read_char();
  }
  if (packsize == 0) return(1);

  /* first try memory */
  error = 0;
  destlen = 0;
  destbuflen = packsize;
  destbuf = malloc(destbuflen);   /* allocate buffer */
  if (destbuf != NULL) {
    destbufptr = destbuf;
    if (!Decode(packsize)) {
      if (conv) {
        tmpbuf = malloc(packsize);
        if (tmpbuf != NULL) {
          tmplen = 0;
          worlen = 0;
          bin = 0;
          prebin = 0;
          bintest = 0;
          only_cr = 0;
          while (worlen < packsize) {
            if (!bin) {
              if (only_cr && (*(destbuf + worlen) != '\n')) {
                *(tmpbuf + tmplen) = '\n';
                tmplen++;
              }
              only_cr = 0;
            }
            if (bin || (*(destbuf + worlen) != '\r')) {
              *(tmpbuf + tmplen) = *(destbuf + worlen);
              tmplen++;
            }
            if (!bin && (*(destbuf + worlen) == '\r')) only_cr = 1;
            if (prebin && (*(destbuf + worlen) == '\n')) bin = 1;
            if (!prebin) {
              c = (int)(*(destbuf + worlen));
              switch (bintest) {
              case 0:
                if ((char)c == '\n') bintest++;
                break;
              case 1:
                if ((char)c == '#') bintest++;
                else if ((char)c != '\n') bintest = 0;
                break;
              case 2:
                if ((char)c == 'B') bintest++;
                else bintest = 0;
                break;
              case 3:
                if ((char)c == 'I') bintest++;
                else bintest = 0;
                break;
              case 4:
                if ((char)c == 'N') bintest++;
                else bintest = 0;
                break;
              case 5:  
                if ((char)c == '#') bintest++;
                else bintest = 0;
                break;
              case 6:  
                if (isdigit(c)) prebin = 1;
                else bintest = 0;
                break;
              default:
                bintest = 0;
                break;
              }
            }
            worlen++;
          }
          *outbase = tmpbuf;
          *outsize = tmplen;
          free(destbuf);
          return(0);
        }
      }
      else {
        *outbase = destbuf;
        *outsize = packsize;
        return(0);
      }
    }
  }
  free(destbuf);

  /* output to memory failed, write to file */
  out_memory = 0;
  init_huf();

  srcbuf = membase;
  srcbufptr = srcbuf + sizeof(packsize);
  srclen = sizeof(packsize);
  srcbuflen = size;
  *outbase = NULL;
  *outsize = 0;

  if ((outfile = fopen(outputfptr,"wb")) == NULL) {
    return(1);
  } 
  if (Decode(packsize))
    error = 1;
  fclose(outfile);
  if ((conv) && (!error)) {
    bin = 0;
    bintest = 0;
    prebin = 0;
    if ((infile = fopen(tempname,"rb")) == NULL) {
      return(1);
    }
    if ((outfile = fopen(outputfile,"wb")) == NULL) {
      fclose(infile);
      unlink(tempname);
      return(1);
    }
    only_cr = 0;
    while ((c = getc(infile)) != EOF) {
      if (!bin) {
        if (only_cr && ((char)c != '\n')) {
          putc('\n',outfile);
        }
        only_cr = 0;
      }
      if (bin || ((char)c != '\r')) putc(c,outfile);
      if (!bin) {
        if ((char)c == '\r') only_cr = 1;
        if (prebin && ((char)c == '\n')) bin = 1;
      }
      if (!prebin) {
        switch (bintest) {
        case 0:
          if ((char)c == '\n') bintest++;
          break;
        case 1:
          if ((char)c == '#') bintest++;
          else if ((char)c != '\n') bintest = 0;
          break;
        case 2:
          if ((char)c == 'B') bintest++;
          else bintest = 0;
          break;
        case 3:
          if ((char)c == 'I') bintest++;
          else bintest = 0;
          break;
        case 4:
          if ((char)c == 'N') bintest++;
          else bintest = 0;
          break;
        case 5:  
          if ((char)c == '#') bintest++;
          else bintest = 0;
          break;
        case 6:  
          if (isdigit(c)) prebin = 1;
          else bintest = 0;
          break;
        default:
          bintest = 0;
          break;
        }
      }
    }
    fclose(infile);
    fclose(outfile);
  }
  if (conv) {
    unlink(tempname);
  }
  return(error);
}

