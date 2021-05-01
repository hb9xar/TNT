/* Output from p2c, the Pascal-to-C translator */
/* From input file "bcast.p" */


/*

This file is copyrighted by Joachim Schurig, DL8HBS, Berlin. You may use
it freely for your own non-profit products. For further details, refer to
the GNU free software licence and the ALAS licence regulations proposed
by NORD><LINK. Explicitely, you may not use this code or fragments of it
for any kind of commercial products. Please respect these fair licence
conditions. Code using this work has to be published at the same conditions.
Code using this work has to mention its roots.

Thank you.

*/
/* Some changes and additions by Mark Wahl, DL4YBG
   updated: 96/05/27

   25.03.04 hb9xar	include <time.h> for struct tm

*/

/* $UNDEF BCASTDEBUG*/
/* extended debug informations whilst runtime */
/* $UNDEF USE_LFIELD*/
/* Use the L-field in broadcast ? */
/* $UNDEF USE_EFLAG*/
/* Use the E-Flag in File Transmission ? */

#include "tnt.h"
#ifdef BCAST



#include "p2cdef.h"


#define BCAST_G
#include "bcast.h"

#include <time.h>


/*
#ifndef CRC_H
#include "crc.h"
#endif
*/

/*
#ifndef TOOLS_H
#include "tools.h"
#endif
*/


/* Mask bits for the FLAG-field in FRAME_HEADER or REQUEST_HEADER */

#define PF_L            1
#define PF_O            2
#define PF_VV           12
#define PF_E            32
#define PF_CC           3
#define PF_REQ          16   /* is it a request or a broadcast? */
#define PF_DIR          3

#define PD_E            32   /* For DIR-Header */

/* Resulting numbers when masking the FLAG-field with the mask bits */
#define PF_START        0
#define PF_STOP         1
#define PF_HOLE         2
#define PF_LPRESENT     1
#define PF_BLOCKMODE    0
#define PF_OFFSMODE     2
#define PF_ISREQUEST    16
#define PF_ISBCAST      0
#define PF_ISLAST       32
#define PF_VNUMBER      0
#define PF_ISDIR        0

/* Header ID's of mandatory headers  */
#define PH_HEADER_END   0
#define PH_FILE_NUMBER  1
#define PH_FILE_NAME    2
#define PH_FILE_EXT     3
#define PH_FILE_SIZE    4
#define PH_CREATE_TIME  5
#define PH_LAST_MODIFIED_TIME  6
#define PH_SEU_FLAG     7
#define PH_FILE_TYPE    8
#define PH_BODY_CHECKSUM  9
#define PH_HEADER_CHECKSUM  10
#define PH_BODY_OFFSET  11

/* Header ID's of extended headers   */
#define PH_SOURCE       16
#define PH_AX25_UPLOADER  17
#define PH_UPLOAD_TIME  18
#define PH_DOWNLOAD_COUNT  19
#define PH_DESTINATION  20
#define PH_AX25_DOWNLOADER  21
#define PH_DOWNLOAD_TIME  22
#define PH_EXPIRE_TIME  23
#define PH_PRIORITY     24

/* Header ID's of optional headers   */
#define PH_COMPRESSION_TYPE  25
#define PH_BBS_MESSAGE_TYPE  32
#define PH_BULLETIN_ID_NUMBER  33
#define PH_TITLE        34
#define PH_KEYWORDS     35
#define PH_FILE_DESCRIPTION  36
#define PH_COMPRESSION_DESCRIPTION  37
#define PH_USER_FILE_NAME  38

/* Header ID's of experimental headers */
#define EH_FHEADER      36332


/* unfortunately, the damned compiler on the Atari ST is unable to       */
/* generate a correct PACKED RECORD... it aligns... So the FRAME_HEADER  */
/* is defined as an array of bytes, below you find the indicies to access*/

#define fh_flags        1
#define fh_file_id_b1   2
#define fh_file_id_b2   3
#define fh_file_id_b3   4
#define fh_file_id_b4   5
#define fh_file_type    6
#define fh_offset_b1    7
#define fh_offset_b2    8
#define fh_offset_b3    9
#define fh_length_b1    10   /* only present if L bit set */
#define fh_length_b2    11   /* only present if L bit set */
#define fh_data_1_lset  12
#define fh_data_1_lunset  10
#define max_blocksize_lset  243
#define max_blocksize_lunset  245

#define fh_data_1       10
#define max_blocksize   245
#define max_blocksize_dir   237

#define fh_data_256     256

#define fh_headersize   (fh_data_1 - 1)

#define fh_headersize_lset  12
#define fh_headersize_lunset  10

#define rh_flags        1
#define rh_file_id_b1   2
#define rh_file_id_b2   3
#define rh_file_id_b3   4
#define rh_file_id_b4   5
#define rh_block_b1     6
#define rh_block_b2     7

#define dh_flags        1
#define dh_file_id_b1   2
#define dh_file_id_b2   3
#define dh_file_id_b3   4
#define dh_file_id_b4   5
#define dh_offset_b1    6
#define dh_offset_b2    7
#define dh_offset_b3    8
#define dh_offset_b4    9
#define dh_t_old_b1     10
#define dh_t_old_b2     11
#define dh_t_old_b3     12
#define dh_t_old_b4     13
#define dh_t_new_b1     14
#define dh_t_new_b2     15
#define dh_t_new_b3     16
#define dh_t_new_b4     17
#define dh_data_1       18

#define dh_headersize   (dh_data_1 - 1)

#define max_bcastfilesize  16777216L
#define over_bcastfilesize  17000000L

#define rx_ok_timeout   900   /* 15 minutes    */
#define rx_timeout      7200   /* 2 hours       */
#define rx_req_delay    15   /* 15 sec.       */
#define rx_req_if_mul   45   /* 45 sec.       */
#define rx_closefile    30   /* 30 sec.       */

#define tx_maxframes    50   /* max. 50 frames in one loop */
#define tx_timeout      180   /* 3 minutes */
#define tx_closefile    10   /* 10 sec.       */


typedef uchar frametyp[256];
typedef frametyp FRAME_HEADER;

typedef frametyp FRAME_HEADER_EXT;

typedef struct REQUEST_HEADER {
  uchar flags, file_id_b1, file_id_b2, file_id_b3, file_id_b4, block_size_b1,
	block_size_b2;
} REQUEST_HEADER;

typedef struct PAIR {
  uchar offset_b1, offset_b2, offset_b3, length_b1, length_b2;
} PAIR;

typedef struct DIR_HEADER {
  uchar flags, file_id_b1, file_id_b2, file_id_b3, file_id_b4, offset_b1,
	offset_b2, offset_b3, offset_b4, t_old_b1, t_old_b2, t_old_b3,
	t_old_b4, t_new_b1, t_new_b2, t_new_b3, t_new_b4;
} DIR_HEADER;

typedef uchar fileheadertype[512];

typedef struct PACSAT_FILEINFO {
  long file_number;
  Char file_name[9];
  Char file_ext[4];
  long file_size, create_time, last_modified_time;
  uchar seu_flag, file_type;
  unsigned short body_checksum, header_checksum, body_offset;

  Char *source;
  Char ax25uploader[7];
  long upload_time;
  uchar download_count;
  Char *destination;
  Char ax25downloader[7];
  long download_time, expire_time;
  uchar priority;

  uchar compression_type;
  Char bbs_message_type;
  Char bulletin_id[13];
  Char *title, *keywords, *file_description, *compression_description,
       *user_file_name;

  Char *fheader;

  uchar *fileheader;
  Char body_filename[120];
} PACSAT_FILEINFO;

typedef struct sendlisttype {
  struct sendlisttype *next;
  long offset, len;
} sendlisttype;

typedef struct holelisttype {
  struct holelisttype *next, *prev;
  long offset, len;
} holelisttype;

typedef struct pactxdesc {
  long FileID;
  uchar FileType;
  short FileHandle;
  long FileSize;
  uchar tnc, port;   /* for TNC3 */
  unsigned short blocksize;
  boolean delposttx, not_called;
  bcastcallbacktype callback;
  PACSAT_FILEINFO *fileinfo;
  sendlisttype *sendlist;
  struct pactxdesc *next;
  long lasttx, upload_time;
  Char UnprotoAddr[121];
} pactxdesc;

typedef struct pacrxdesc {
  long FileID;
  short FileHandle;
  Char FileExt[4];
  long tempSize;
  uchar tnc;
  boolean fheader_ok;
  unsigned short blocksize;
  long tempSeek;
  struct pacrxdesc *next;
  holelisttype *holelist;
  boolean receive_ready;
  long lastrx, lastrxother;
  Char fromAddr[12];
  Char viaDigi[121];
  boolean is_bbs_file;
  Char msgid[13];
} pacrxdesc;


extern void checksum16_buf(uchar *adr, long size, unsigned short *crc);
extern long sys_ixtime();
extern unsigned short file_crc(short methode, Char *name,
                               unsigned short preload, long start, long size);
extern int del_cr(char *filename);
extern void get_chanstr(char *qrg, char *chanstr);
extern void crcfbb_buf(uchar *adr, long size, unsigned short *crc);
extern int strpos2(char *s,char *pat,int pos);
extern void strdelete(char *s,int pos,int len);
extern void cmd_display(int flag,int channel,char *buffer,int cr);


Static pacrxdesc *pacrxroot;
Static pactxdesc *pactxroot;


/* Transfer routines for LSB-MSB reversal etc. */

Static long dp32long(uchar b1, uchar b2, uchar b3, uchar b4)
{
  long l;

  l = b4;
  l <<= 8;
  l += b3;
  l <<= 8;
  l += b2;
  l <<= 8;
  l += b1;
  return l;
}


Static long dp24long(uchar b1, uchar b2, uchar b3)
{
  long l;

  l = b3;
  l <<= 8;
  l += b2;
  l <<= 8;
  l += b1;
  return l;
}


Static unsigned short dp16integer(uchar b1, uchar b2)
{
  short l;

  l = b2;
  l <<= 8;
  l += b1;
  return l;
}

/*
Static long dpl32long(long l)
{
  return l;
}


Static unsigned short dpi16integer(unsigned short l)
{
  return l;
}
*/

Static void long32dp(long l, uchar *b1, uchar *b2, uchar *b3, uchar *b4)
{
  *b1 = l & 0xff;
  *b2 = (((unsigned long)l) >> 8) & 0xff;
  *b3 = (((unsigned long)l) >> 16) & 0xff;
  *b4 = (((unsigned long)l) >> 24) & 0xff;
}


Static void long24dp(long l, uchar *b1, uchar *b2, uchar *b3)
{
  *b1 = l & 0xff;
  *b2 = (((unsigned long)l) >> 8) & 0xff;
  *b3 = (((unsigned long)l) >> 16) & 0xff;
}


Static void integer16dp(unsigned short i, uchar *b1, uchar *b2)
{
  *b1 = i & 0xff;
  *b2 = (i >> 8) & 0xff;
}


/* ********************************************************************* */

Static void BcastProtokoll(Char *txt)
{
  Char TEMP[256];

  sprintf(TEMP, "PACSAT: %s", txt);
  bcastprotokollcall(TEMP);
}


Static void free_fileinfo(PACSAT_FILEINFO *fi)
{
  if (fi == NULL)
    return;
  if (fi->source != NULL)
    free(fi->source);
  if (fi->destination != NULL)
    free(fi->destination);
  if (fi->title != NULL)
    free(fi->title);
  if (fi->keywords != NULL)
    free(fi->keywords);
  if (fi->file_description != NULL)
    free(fi->file_description);
  if (fi->compression_description != NULL)
    free(fi->compression_description);
  if (fi->user_file_name != NULL)
    free(fi->user_file_name);
  if (fi->fheader != NULL)
    free(fi->fheader);
  if (fi->fileheader != NULL)
    free(fi->fileheader);
  free(fi);
}


Static void clear_fileinfo(PACSAT_FILEINFO *fi)
{
  if (fi == NULL)
    return;
  fi->file_number = 0;
  strcpy(fi->file_name, "        ");
  strcpy(fi->file_ext, "   ");
  fi->file_size = 0;
  fi->create_time = 0;
  fi->last_modified_time = 0;
  fi->seu_flag = 0;
  fi->file_type = 0;
  fi->body_checksum = 0;
  fi->header_checksum = 0;
  fi->body_offset = 0;

  fi->source = NULL;
  strcpy(fi->ax25uploader, "      ");
  fi->upload_time = 0;
  fi->download_count = 0;
  fi->destination = NULL;
  strcpy(fi->ax25downloader, "      ");
  fi->download_time = 0;
  fi->expire_time = 0;
  fi->priority = 0;

  fi->compression_type = 0;
  fi->bbs_message_type = '\0';
  *fi->bulletin_id = '\0';
  fi->title = NULL;
  fi->keywords = NULL;
  fi->file_description = NULL;
  fi->compression_description = NULL;
  fi->user_file_name = NULL;

  fi->fheader = NULL;

  fi->fileheader = NULL;
  *fi->body_filename = '\0';
}


Static boolean get_strmem(Char **s)
{
  boolean Result;
  Char TEMP[256];

  *s = malloc(256);
  if (*s != NULL)
    return true;
  Result = false;
  strcpy(TEMP, "no mem for extended header informations");
  BcastProtokoll(TEMP);
  return Result;
}


Static void get_hstr(uchar *h, unsigned short idx, unsigned short len,
		     Char **s)
{
  unsigned short y;

  if (!get_strmem(s))
    return;
  for (y = 1; y <= len; y++)
    (*s)[y - 1] = h[idx + y - 2];
  (*s)[len] = '\0';
}


Static boolean valid_for_bbs(PACSAT_FILEINFO *fi)
{
  boolean Result;

  Result = false;
  if (fi == NULL)
    return Result;

  if (fi->file_type >= 32 || ((1L << fi->file_type) & 0x3) == 0)
    return Result;
  if (fi->destination == NULL)
    return Result;
  if (*fi->destination == '\0')
    return Result;
  if (fi->source == NULL)
    return Result;
  if (*fi->source == '\0')
    return Result;
  if (fi->title != NULL) {
    if (*fi->title != '\0')
      return true;
  }
  return Result;
}


/* local fuer gen_bcastfile ********************************** */

Static void set_header(unsigned short header_id, unsigned short header_length,
		       uchar *header, unsigned short *at)
{
  integer16dp(header_id, &header[*at - 1], &header[*at]);
  header[*at + 1] = header_length;
  *at += 3;
}


Static unsigned short calc_header_checksum(uchar *fh, unsigned short headersize)
{
  unsigned short crc;

  crc = 0;
  checksum16_buf(fh, headersize, &crc);
  return crc;
}


Static boolean gen_bcstfile(long fid, unsigned short ftype, Char *name1,
  Char *bbs_source, Char *bbs_destination, Char *bbs_ax25uploader,
  long bbs_upload_time, long bbs_expire_time, uchar bbs_compression,
  Char *bbs_bid, Char bbs_msgtype, Char *bbs_title, Char *bbs_fheader,
  unsigned short bodychecksum, PACSAT_FILEINFO **fi)
{
  boolean Result;
  Char hs[256];
  long size;
  unsigned short x, idx, hp, fsb, csb, bob;
  boolean is_bbs;
  PACSAT_FILEINFO *WITH;

  Result = false;
  is_bbs = (*bbs_destination != '\0');
  (*fi)->fileheader = malloc(sizeof(fileheadertype));
  if ((*fi)->fileheader == NULL)
    return Result;
  size = sfsize(name1);

  WITH = *fi;
  WITH->file_number = fid;
  strcpy(WITH->file_ext, "BCT");
  rspacing(WITH->file_ext, 3);
  int2hstr(WITH->file_number, WITH->file_name);
  rspacing(WITH->file_name, 8);
  if (bbs_upload_time == 0)
    WITH->create_time = sys_ixtime();
  else
    WITH->create_time = bbs_upload_time;
  WITH->last_modified_time = WITH->create_time;
  WITH->seu_flag = 0;
  WITH->file_type = ftype;
  if (bodychecksum == 0)
    WITH->body_checksum = file_crc(5, name1, 0, 0, 0);
	/* Checksum 16 BIT, no CRC ! */
  else
    WITH->body_checksum = bodychecksum;
  WITH->header_checksum = 0;
  strcpy(WITH->body_filename, name1);
  strcpy(WITH->bulletin_id,bbs_bid);

  WITH->fileheader[0] = 0xaa;
  WITH->fileheader[1] = 0x55;

  hp = 3;
  set_header(PH_FILE_NUMBER, 4, WITH->fileheader, &hp);
  long32dp(WITH->file_number, &WITH->fileheader[hp - 1],
	   &WITH->fileheader[hp], &WITH->fileheader[hp + 1],
	   &WITH->fileheader[hp + 2]);
  hp += 4;

  set_header(PH_FILE_NAME, 8, WITH->fileheader, &hp);
  for (idx = hp; idx <= hp + 8; idx++)
    WITH->fileheader[idx - 1] = WITH->file_name[idx - hp];
  hp += 8;

  set_header(PH_FILE_EXT, 3, WITH->fileheader, &hp);
  for (idx = hp; idx <= hp + 3; idx++)
    WITH->fileheader[idx - 1] = WITH->file_ext[idx - hp];
  hp += 3;

  set_header(PH_FILE_SIZE, 4, WITH->fileheader, &hp);
  fsb = hp;   /* filled up at the end */
  hp += 4;

  set_header(PH_CREATE_TIME, 4, WITH->fileheader, &hp);
  long32dp(WITH->create_time, &WITH->fileheader[hp - 1],
	   &WITH->fileheader[hp], &WITH->fileheader[hp + 1],
	   &WITH->fileheader[hp + 2]);
  hp += 4;

  set_header(PH_LAST_MODIFIED_TIME, 4, WITH->fileheader, &hp);
  long32dp(WITH->last_modified_time, &WITH->fileheader[hp - 1],
	   &WITH->fileheader[hp], &WITH->fileheader[hp + 1],
	   &WITH->fileheader[hp + 2]);
  hp += 4;

  set_header(PH_SEU_FLAG, 1, WITH->fileheader, &hp);
  WITH->fileheader[hp - 1] = WITH->seu_flag;
  hp++;

  set_header(PH_FILE_TYPE, 1, WITH->fileheader, &hp);
  WITH->fileheader[hp - 1] = WITH->file_type;
  hp++;

  set_header(PH_BODY_CHECKSUM, 2, WITH->fileheader, &hp);
  integer16dp(WITH->body_checksum, &WITH->fileheader[hp - 1],
	      &WITH->fileheader[hp]);
  hp += 2;

  set_header(PH_HEADER_CHECKSUM, 2, WITH->fileheader, &hp);
  csb = hp;
  WITH->fileheader[hp - 1] = 0;   /* for calculation set to 0 */
  hp++;
  WITH->fileheader[hp - 1] = 0;
  hp++;

  set_header(PH_BODY_OFFSET, 2, WITH->fileheader, &hp);
  bob = hp;   /* filled up at the end */
  hp += 2;

  if (is_bbs) {
    x = strlen(bbs_source);
    set_header(PH_SOURCE, x, WITH->fileheader, &hp);
    for (idx = hp; idx <= hp + x; idx++)
      WITH->fileheader[idx - 1] = bbs_source[idx - hp];
    hp += x;

    strcpy(hs, bbs_ax25uploader);
    cut(hs, 6);
    rspacing(hs, 6);
    set_header(PH_AX25_UPLOADER, 6, WITH->fileheader, &hp);
    for (idx = hp; idx <= hp + 6; idx++)   /* must be 6 chars */
      WITH->fileheader[idx - 1] = hs[idx - hp];
    hp += 6;

    set_header(PH_UPLOAD_TIME, 4, WITH->fileheader, &hp);
    long32dp(bbs_upload_time, &WITH->fileheader[hp - 1],
	     &WITH->fileheader[hp], &WITH->fileheader[hp + 1],
	     &WITH->fileheader[hp + 2]);
    hp += 4;

    set_header(PH_DOWNLOAD_COUNT, 1, WITH->fileheader, &hp);
    WITH->fileheader[hp - 1] = 0;
    hp++;

    x = strlen(bbs_destination);
    set_header(PH_DESTINATION, x, WITH->fileheader, &hp);
    for (idx = hp; idx <= hp + x; idx++)
      WITH->fileheader[idx - 1] = bbs_destination[idx - hp];
    hp += x;

    set_header(PH_AX25_DOWNLOADER, 6, WITH->fileheader, &hp);
    for (idx = hp - 1; idx <= hp + 5; idx++)
      WITH->fileheader[idx] = 32;
    hp += 6;

    set_header(PH_DOWNLOAD_TIME, 4, WITH->fileheader, &hp);
    long32dp(0, &WITH->fileheader[hp - 1], &WITH->fileheader[hp],
	     &WITH->fileheader[hp + 1], &WITH->fileheader[hp + 2]);
    hp += 4;

    set_header(PH_EXPIRE_TIME, 4, WITH->fileheader, &hp);
    long32dp(bbs_expire_time, &WITH->fileheader[hp - 1],
	     &WITH->fileheader[hp], &WITH->fileheader[hp + 1],
	     &WITH->fileheader[hp + 2]);
    hp += 4;

    set_header(PH_PRIORITY, 1, WITH->fileheader, &hp);
    WITH->fileheader[hp - 1] = 0;
    hp++;

  }

  set_header(PH_COMPRESSION_TYPE, 1, WITH->fileheader, &hp);
  WITH->fileheader[hp - 1] = bbs_compression;
  hp++;

  if (WITH->bbs_message_type != '\0') {
    set_header(PH_BBS_MESSAGE_TYPE, 1, WITH->fileheader, &hp);
    WITH->fileheader[hp - 1] = WITH->bbs_message_type;
    hp++;
  }

  x = strlen(bbs_bid);
  if (x > 0) {
    set_header(PH_BULLETIN_ID_NUMBER, x, WITH->fileheader, &hp);
    for (idx = hp; idx <= hp + x; idx++)
      WITH->fileheader[idx - 1] = bbs_bid[idx - hp];
    hp += x;
  }

  x = strlen(bbs_title);
  if (x > 0) {
    set_header(PH_TITLE, x, WITH->fileheader, &hp);
    for (idx = hp; idx <= hp + x; idx++)
      WITH->fileheader[idx - 1] = bbs_title[idx - hp];
    hp += x;
  }

  strcpy(hs, "dpbox");
  if (*bbs_bid != '\0')
    sprintf(hs + strlen(hs), " BID:%s", bbs_bid);
  x = strlen(hs);
  set_header(PH_KEYWORDS, x, WITH->fileheader, &hp);
  for (idx = hp; idx <= hp + x; idx++)
    WITH->fileheader[idx - 1] = hs[idx - hp];
  hp += x;

  if (!is_bbs) {
    strcpy(hs, name1);
    del_path(hs);
    x = strlen(hs);
    if (x > 0) {
      set_header(PH_USER_FILE_NAME, x, WITH->fileheader, &hp);
      for (idx = hp; idx <= hp + x; idx++)
	WITH->fileheader[idx - 1] = hs[idx - hp];
      hp += x;
    }
  }

  x = strlen(bbs_fheader);
  if (x > 0) {
    set_header(EH_FHEADER, x, WITH->fileheader, &hp);
    for (idx = hp; idx <= hp + x; idx++)
      WITH->fileheader[idx - 1] = bbs_fheader[idx - hp];
    hp += x;
  }

  set_header(PH_HEADER_END, 0, WITH->fileheader, &hp);

  WITH->body_offset = hp - 1;   /* size of the header */
  integer16dp(WITH->body_offset, &WITH->fileheader[bob - 1],
	      &WITH->fileheader[bob]);

  WITH->file_size = size + WITH->body_offset;   /* resulting file size */
  long32dp(WITH->file_size, &WITH->fileheader[fsb - 1],
	   &WITH->fileheader[fsb], &WITH->fileheader[fsb + 1],
	   &WITH->fileheader[fsb + 2]);

  /* Now insert the header's checksum */

  WITH->header_checksum = calc_header_checksum(WITH->fileheader,
					       WITH->body_offset);
  integer16dp(WITH->header_checksum, &WITH->fileheader[csb - 1],
	      &WITH->fileheader[csb]);

  return true;

}


/* local fuer dec_bcastfile *********************************************** */

Static void fill_fileheader(PACSAT_FILEINFO *fi, uchar *header)
{
  unsigned short y, z, idx, hlen;
  boolean stop;
  unsigned short header_id;

  stop = false;
  if (fi == NULL || header == NULL)
    return;
  idx = 3;   /* the first two bytes contain the frame type info */
  while (idx < sizeof(fileheadertype) && !stop) {
    header_id = dp16integer(header[idx - 1], header[idx]);
    idx += 3;
    hlen = header[idx - 2];

    switch (header_id) {

    /* mandatory headers */
    case PH_HEADER_END:
      stop = true;
      break;

    case PH_FILE_NUMBER:
      fi->file_number = dp32long(header[idx - 1], header[idx],
				 header[idx + 1], header[idx + 2]);
      break;

    case PH_FILE_NAME:
      for (y = 1; y <= 8; y++)
	fi->file_name[y - 1] = header[idx + y - 2];
      break;

    case PH_FILE_EXT:
      for (y = 1; y <= 3; y++)
	fi->file_ext[y - 1] = header[idx + y - 2];
      break;

    case PH_FILE_SIZE:
      fi->file_size = dp32long(header[idx - 1], header[idx], header[idx + 1],
			       header[idx + 2]);
      break;

    case PH_CREATE_TIME:
      fi->create_time = dp32long(header[idx - 1], header[idx],
				 header[idx + 1], header[idx + 2]);
      break;

    case PH_LAST_MODIFIED_TIME:
      fi->last_modified_time = dp32long(header[idx - 1], header[idx],
					header[idx + 1], header[idx + 2]);
      break;

    case PH_SEU_FLAG:
      fi->seu_flag = header[idx - 1];
      break;

    case PH_FILE_TYPE:
      fi->file_type = header[idx - 1];
      break;

    case PH_BODY_CHECKSUM:
      fi->body_checksum = dp16integer(header[idx - 1], header[idx]);
      break;

    case PH_HEADER_CHECKSUM:
      fi->header_checksum = dp16integer(header[idx - 1], header[idx]);
      header[idx - 1] = 0;
      header[idx] = 0;
      break;

    case PH_BODY_OFFSET:
      fi->body_offset = dp16integer(header[idx - 1], header[idx]);
      break;

    /* extended headers */
    case PH_SOURCE:
      get_hstr(header, idx, hlen, &fi->source);
      break;

    case PH_AX25_UPLOADER:
      for (y = 1; y <= 6; y++)
	fi->ax25uploader[y - 1] = header[idx + y - 2];
      break;

    case PH_UPLOAD_TIME:
      fi->upload_time = dp32long(header[idx - 1], header[idx],
				 header[idx + 1], header[idx + 2]);
      break;

    case PH_DOWNLOAD_COUNT:
      fi->download_count = header[idx - 1];
      break;

    case PH_DESTINATION:
      if (fi->destination == NULL)
	get_hstr(header, idx, hlen, &fi->destination);
      break;

    case PH_AX25_DOWNLOADER:
      if (fi->ax25downloader[0] == ' ') {
	for (y = 1; y <= 6; y++)
	  fi->ax25downloader[y - 1] = header[idx + y - 2];
      }
      break;

    case PH_DOWNLOAD_TIME:
      if (fi->download_time == 0)
	fi->download_time = dp32long(header[idx - 1], header[idx],
				     header[idx + 1], header[idx + 2]);
      break;

    case PH_EXPIRE_TIME:
      fi->expire_time = dp32long(header[idx - 1], header[idx],
				 header[idx + 1], header[idx + 2]);
      break;

    case PH_PRIORITY:
      fi->priority = header[idx - 1];
      break;

    /* optional headers */
    case PH_COMPRESSION_TYPE:
      fi->compression_type = header[idx - 1];
      break;

    case PH_BBS_MESSAGE_TYPE:
      fi->bbs_message_type = header[idx - 1];
      break;

    case PH_BULLETIN_ID_NUMBER:
      z = hlen;
      if (z > 12)   /* wegen des TNN-Fehlers */
	z = 12;
      for (y = 1; y <= z; y++)
	fi->bulletin_id[y - 1] = header[idx + hlen - z + y - 2];
      fi->bulletin_id[z] = '\0';
      break;

    case PH_TITLE:
      get_hstr(header, idx, hlen, &fi->title);
      break;

    case PH_KEYWORDS:
      get_hstr(header, idx, hlen, &fi->keywords);
      break;

    case PH_FILE_DESCRIPTION:
      get_hstr(header, idx, hlen, &fi->file_description);
      break;

    case PH_COMPRESSION_DESCRIPTION:
      get_hstr(header, idx, hlen, &fi->compression_description);
      break;

    case PH_USER_FILE_NAME:
      get_hstr(header, idx, hlen, &fi->user_file_name);
      break;

    /* experimental headers */
    case EH_FHEADER:
      get_hstr(header, idx, hlen, &fi->fheader);
      break;


    }
    idx += hlen;
  }

}


Static boolean dec_bcstfile(Char *rxfrom, boolean is_bbs_file, Char *name1,
			    Char *name2)
{
  boolean Result;
  short handle1;
  Char name3[256];
  Char hs[256];
  PACSAT_FILEINFO *fi;
  long size;
  uchar *header;
  unsigned short crc;
  Char TEMP[256];

  Result = false;

  header = malloc(sizeof(fileheadertype));
  if (header == NULL) {
    strcpy(TEMP, "no mem");
    BcastProtokoll(TEMP);
    return Result;
  }

  fi = malloc(sizeof(PACSAT_FILEINFO));
  if (fi != NULL) {
    clear_fileinfo(fi);
    size = sfsize(name1);

    handle1 = sfopen(name1, FO_READ);
    if (handle1 >= minhandle) {
      read(handle1, header, sizeof(fileheadertype));
      sfclose(&handle1);
      if (header[0] == 0xaa && header[1] == 0x55) {
	fill_fileheader(fi, header);
	if (calc_header_checksum(header, fi->body_offset) ==
	    fi->header_checksum) {
	  if (size == fi->file_size) {
	    if (is_bbs_file)
	      is_bbs_file = valid_for_bbs(fi);

	    if (!is_bbs_file) {
	      if (fi->user_file_name != NULL) {
		strcpy(hs, fi->user_file_name);
		del_path(hs);
		if ((unsigned long)strlen(hs) < 32 &&
		    ((1L << strlen(hs)) & 0x1ffe) != 0)
		  strcat(name2, hs);
		else
		  strcat(name2, "BCASTRX.001");
	      } else {
		strcpy(hs, fi->file_name);
		del_lastblanks(hs);
		if ((unsigned long)strlen(hs) < 32 &&
		    ((1L << strlen(hs)) & 0x1fe) != 0)
		  strcat(name2, hs);
		else
		  strcat(name2, "BCASTRX");
		strcpy(hs, fi->file_ext);
		del_lastblanks(hs);
		if (*hs != '\0')
		  sprintf(name2 + strlen(name2), ".%s", hs);
	      }
	    }

	    validate(name2);

	    filecut(name1, name2, (long)fi->body_offset, 0);

	    crc = file_crc(5, name2, 0, 0, 0);
		/* Checksum 16 BIT, no CRC ! */

	    if (crc == fi->body_checksum) {
	      switch (fi->compression_type) {

	      case 97:
		strcpy(name3, name2);
		validate(name3);
		if (bcasthuffpacker(false, name2, name3, true) == 0) {
		  sfdelfile(name2);
		  strcpy(name2, name3);
		} else {
		  strcpy(TEMP, "unpack error");
		  BcastProtokoll(TEMP);
		}
		break;

	      case 0:
		/* blank case */
		break;

	      default:
		strcpy(TEMP, "unknown compression type");
		BcastProtokoll(TEMP);
		if (fi->compression_description != NULL)
		  BcastProtokoll(fi->compression_description);
		break;
	      }

	      Result = true;

	      if (is_bbs_file) {
		strcpy(name3, name2);
		validate(name3);
		handle1 = sfcreate(name3, FC_FILE);

		if (handle1 >= minhandle) {
		  bcastcreatestatus(true, fi->destination, fi->source,
				    sys_ixtime(), fi->expire_time,
				    fi->file_size - fi->body_offset,
				    fi->bbs_message_type, hs);

		  string_to_file(&handle1, hs, true);
		  string_to_file(&handle1, fi->title, true);
		  if (*fi->bulletin_id != '\0') {
		    sprintf(TEMP, "*** Bulletin-ID: %s ***", fi->bulletin_id);
		    string_to_file(&handle1, TEMP, true);
		  }
		  *TEMP = '\0';
		  string_to_file(&handle1, TEMP, true);
		  if (fi->fheader != NULL)
		    string_to_file(&handle1, fi->fheader, true);
		  del_cr(name2);
		  app_file(name2, handle1, true);
		  sfclose(&handle1);
		  strcpy(hs, rxfrom);
		  cut(hs, 6);
		  bcastsortnewmail(-1, name3, hs);

		}


	      }


	    } else {
	      strcpy(TEMP, "body checksum invalid");
	      BcastProtokoll(TEMP);
	      sfdelfile(name2);
	    }

	  } else {
	    strcpy(TEMP, "invalid length of body");
	    BcastProtokoll(TEMP);
	  }

	} else {
	  strcpy(TEMP, "invalid header checksum");
	  BcastProtokoll(TEMP);
	}
      } else {
	strcpy(TEMP, "invalid header");
	BcastProtokoll(TEMP);
      }
    } else {
      strcpy(TEMP, "file open error");
      BcastProtokoll(TEMP);
    }
    free_fileinfo(fi);
  } else {
    strcpy(TEMP, "no mem");
    BcastProtokoll(TEMP);
  }

  free(header);
  return Result;
}


Static void add_to_sendlist(long offset, long len, pactxdesc *tp)
{
  sendlisttype *hsp, *hsp2;
  long hlen, hoffs, cor, hlen2, hoffs2;

  if (len <= 0 || offset >= tp->FileSize)
    return;

  /* first let's see if we have that already on the list */

  hoffs = offset;
  hlen = len;
  do {
    hsp = tp->sendlist;
    while (hsp != NULL && hlen > 0) {
      if (hsp->offset <= hoffs && hsp->offset + hsp->len > hoffs) {
	if (hsp->offset + hsp->len >= hoffs + hlen)
	  hlen = 0;
	else {
	  cor = hsp->offset + hsp->len - hoffs;
	  hoffs += cor;
	  hlen -= cor;
	}
      }
      if (hlen > 0)
	hsp = hsp->next;
    }

    /* Ok, now, if there is a portion left to send, create a new entry */

    if (hlen > 0) {
      hsp = tp->sendlist;
      while (hsp->next != NULL)
	hsp = hsp->next;
      if (hsp->len > 0) {
	hsp2 = malloc(sizeof(sendlisttype));
	hsp->next = hsp2;
	hsp = hsp2;
	if (hsp != NULL)
	  hsp->next = NULL;
      }

      if (hsp != NULL) {
	if (hsp == tp->sendlist) {  /* root... */
	  hsp->offset = hoffs;
	  hsp->len = hlen;
	  hlen = 0;
	} else {  /* not root... check for interleaving gaps */
	  hsp2 = tp->sendlist;
	  hoffs2 = hoffs;
	  hlen2 = hlen;

	  while (hsp2 != hsp) {
	    if (hsp2->offset > hoffs2 && hsp2->offset < hoffs2 + hlen2)
	      hlen2 = hsp2->offset - hoffs2;
	    hsp2 = hsp2->next;
	  }

	  hsp->offset = hoffs2;
	  hsp->len = hlen2;
	  hoffs += hlen2;
	  hlen -= hlen2;

	}


      } else
	hlen = 0;


    }
  } while (hlen > 0);
}


boolean bcast_file(uchar stnc, uchar sport, Char *qrg, 
  long fid, unsigned short ftype,
  Char *name1, Char *adress, Char *bbs_source, Char *bbs_destination,
  Char *bbs_ax25uploader, long bbs_upload_time, long bbs_expire_time,
  uchar bbs_compression, Char *bbs_bid, Char bbs_msgtype, Char *bbs_title,
  Char *bbs_fheader, unsigned short bodychecksum, boolean delete_after_tx,
  bcastcallbacktype *callbackproc)
{
  boolean Result;
  pactxdesc *tp, *tp2;
  PACSAT_FILEINFO *fi;
  char chan_str[9];

  Result = false;

  fi = malloc(sizeof(PACSAT_FILEINFO));
  if (fi == NULL)
    return Result;

  clear_fileinfo(fi);

  if (gen_bcstfile(fid, ftype, name1, bbs_source, bbs_destination,
		   bbs_ax25uploader, bbs_upload_time, bbs_expire_time,
		   bbs_compression, bbs_bid, bbs_msgtype, bbs_title,
		   bbs_fheader, bodychecksum, &fi)) {
    if (pactxroot == NULL) {
      pactxroot = malloc(sizeof(pactxdesc));
      pactxroot->next = NULL;
      pactxroot->FileID = 0;
    }

    tp = pactxroot;
    while (tp->next != NULL && tp->FileID != 0 && tp->FileID != fid)
      tp = tp->next;

    if (tp->FileID == fid) {
      add_to_sendlist(0, tp->FileSize, tp);
      tp->lasttx = sys_ixtime();
      return true;
    } else {
      if (tp->FileID != 0) {
	tp2 = malloc(sizeof(pactxdesc));
	if (tp2 != NULL) {
	  tp2->next = NULL;
	  tp2->FileID = 0;
	}
	tp->next = tp2;
	tp = tp2;
      }

      if (tp != NULL) {
	if (tp->FileID == 0) {
	  tp->FileID = fid;
	  tp->FileType = ftype;
	  tp->FileHandle = nohandle;
	  tp->FileSize = fi->file_size;
	  tp->tnc = stnc;
	  tp->port = sport;
	  tp->blocksize = max_blocksize;
	  tp->lasttx = sys_ixtime();
	  tp->fileinfo = fi;
	  tp->delposttx = delete_after_tx;
	  tp->not_called = true;
	  if (bbs_upload_time == 0)
	    tp->upload_time = sys_ixtime();
	  else
	    tp->upload_time = bbs_upload_time;
	  tp->callback = *callbackproc;   /* to be called when TX ready */

          tp->UnprotoAddr[0] = '\0';
          if (qrg[0] != '\0') {
            get_chanstr(qrg,chan_str);
            if (chan_str[0] != '\0')
              strcat(tp->UnprotoAddr,chan_str);
          }
	  strcat(tp->UnprotoAddr, "QST-1");
	  if (*adress != '\0') {
	    if (strlen(adress) <= 'r')
	      sprintf(tp->UnprotoAddr + strlen(tp->UnprotoAddr), " %s",
		      adress);
	  }

	  tp->sendlist = malloc(sizeof(sendlisttype));
	  if (tp->sendlist != NULL) {
	    tp->sendlist->next = NULL;
	    tp->sendlist->offset = 0;
	    tp->sendlist->len = 0;

	    add_to_sendlist(0, tp->FileSize, tp);
	    return true;
	  } else {
	    free_fileinfo(fi);
	    return Result;
	  }
	} else {
	  free_fileinfo(fi);
	  return Result;
	}
      } else {
	free_fileinfo(fi);
	return Result;
      }
    }

  } else {
    free_fileinfo(fi);
    return Result;
  }

  return Result;


}


Static void dispose_holelist(holelisttype *hp)
{
  holelisttype *hp2;

  if (hp == NULL)
    return;
  while (hp->prev != NULL)
    hp = hp->prev;
  while (hp != NULL) {
    hp2 = hp->next;
    free(hp);
    hp = hp2;
  }
}


Static void bcast_requests(void)
{
  pacrxdesc *tp;
  holelisttype *hp;
  frametyp fhe;
  unsigned short rl;
  long len2, hstart;
  unsigned short idx;
  boolean ok;
  long tdiff, atime;
  Char hs[256];

  atime = sys_ixtime();
  if (pacrxroot == NULL)
    return;
  tp = pacrxroot;
  while (tp != NULL) {
    if (!tp->receive_ready) {
      if (tp->lastrxother > tp->lastrx)
	tdiff = rx_req_if_mul;
      else
	tdiff = rx_req_delay;
      if (atime - tp->lastrx > tdiff && tp->FileID != 0) {
	fhe[rh_flags - 1] = PF_HOLE + PF_ISREQUEST;
	long32dp(tp->FileID, &fhe[rh_file_id_b1 - 1], &fhe[rh_file_id_b2 - 1],
		 &fhe[rh_file_id_b3 - 1], &fhe[rh_file_id_b4 - 1]);
	integer16dp(tp->blocksize, &fhe[rh_block_b1 - 1],
		    &fhe[rh_block_b2 - 1]);

	idx = rh_block_b2 + 1;
	ok = false;
	hp = tp->holelist;
	while (hp != NULL && !ok) {
	  len2 = hp->len;
	  hstart = hp->offset;

	  do {
	    if (len2 > max_bcastfilesize) {
		  /* hole with an infinite end... */
		    if (hstart == 0)
		    /* This is an impossible case, but you never know hi */
		      len2 = tp->blocksize;
	      else
		len2 = 0;
	      ok = true;
	    }

	    /* Take care of overrunning 2^16 Bytes... */
	    if (len2 < 65535L)
	      rl = len2;
	    else
	      rl = 65535L - 65535L % tp->blocksize;

	    if (rl > 0) {
	      long24dp(hstart, &fhe[idx - 1], &fhe[idx], &fhe[idx + 1]);
	      integer16dp(rl, &fhe[idx + 2], &fhe[idx + 3]);
	      hstart += rl;
	      len2 -= rl;
	      idx += 5;
	    }

	  } while (!(len2 <= 0 || idx > 251 || ok));

	  if (idx > 251)
	    ok = true;
	  hp = hp->next;
	}

	if (idx > rh_block_b2 + 1) {
	  bcastswitchtobc(tp->tnc, true);
	  bcastpushpid(tp->tnc);
	  bcastsetpid(tp->tnc, 0xbb);
	  bcastpushunproto(tp->tnc);
	  strcpy(hs, tp->fromAddr);
	  if (*tp->viaDigi != '\0')
	    sprintf(hs + strlen(hs), " %s", tp->viaDigi);
	  bcastsetunproto(tp->tnc, hs);
	  bcastsend(bcastmonchan(tp->tnc), idx - 1, fhe);
	  bcastpopunproto(tp->tnc);
	  bcastpoppid(tp->tnc);
	  bcastswitchtobc(tp->tnc, false);
	}

	tp->lastrx = atime;

      }

    }
    if (tp != NULL)
      tp = tp->next;
  }
}


/* Sends a portion of a file */

Static unsigned short send_bcast2(unsigned short blocksize, long start,
				  pactxdesc *tp)
{
  unsigned short Result;
  frametyp fhe, fhd;
  unsigned short hl;
  long dfid;
  unsigned short crc, x;
  long fileheadersize;

  Result = 0;
  if (tp == NULL)
    return Result;

  if (blocksize > max_blocksize)
    blocksize = max_blocksize;

  long32dp(tp->FileID, &fhe[fh_file_id_b1 - 1], &fhe[fh_file_id_b2 - 1],
	   &fhe[fh_file_id_b3 - 1], &fhe[fh_file_id_b4 - 1]);
  fhe[fh_file_type - 1] = tp->FileType;

  long24dp(start, &fhe[fh_offset_b1 - 1], &fhe[fh_offset_b2 - 1],
	   &fhe[fh_offset_b3 - 1]);

  hl = blocksize;
  fileheadersize = tp->fileinfo->body_offset;

  if (start < fileheadersize) {
    if (hl > fileheadersize - start)
      hl = fileheadersize - start;

    fhe[fh_flags - 1] = PF_O;

  } else {
    if (hl > tp->FileSize - start) {
      hl = tp->FileSize - start;
      fhe[fh_flags - 1] = PF_O;
    } else
      fhe[fh_flags - 1] = PF_O;
  }


  if (start < fileheadersize) {
    /* copy fileheader   */

    if (hl > max_blocksize_dir)
      hl = max_blocksize_dir;

    for (x = 1; x <= hl; x++)
      fhe[x + fh_data_1 - 2] = tp->fileinfo->fileheader[x - 1 + start];


    /* create a preceeding directory broadcast for WiSP */

    if (hl + dh_headersize <= 254) {
      fhd[dh_flags - 1] = PD_E;
      dfid = tp->FileID + (1 << 28);
      long32dp(dfid, &fhd[dh_file_id_b1 - 1], &fhd[dh_file_id_b2 - 1],
	       &fhd[dh_file_id_b3 - 1], &fhd[dh_file_id_b4 - 1]);
      long32dp(start, &fhd[dh_offset_b1 - 1], &fhd[dh_offset_b2 - 1],
               &fhd[dh_offset_b3 - 1], &fhd[dh_offset_b4 - 1]);
      long32dp(tp->upload_time, &fhd[dh_t_old_b1 - 1], &fhd[dh_t_old_b2 - 1],
	       &fhd[dh_t_old_b3 - 1], &fhd[dh_t_old_b4 - 1]);
      long32dp(tp->upload_time, &fhd[dh_t_new_b1 - 1], &fhd[dh_t_new_b2 - 1],
	       &fhd[dh_t_new_b3 - 1], &fhd[dh_t_new_b4 - 1]);

      for (x = 1; x <= hl; x++)
	fhd[x + dh_data_1 - 2] = tp->fileinfo->fileheader[x - 1 + start];

      crc = 0;
      crcfbb_buf(fhd, dh_headersize + hl, &crc);

      integer16dp(crc, &fhd[dh_headersize + hl + 1], &fhd[dh_headersize + hl]);

      bcastsetpid(tp->tnc, 0xbd);
      bcastsend(bcastmonchan(tp->tnc), dh_headersize + hl + 2, fhd);
      bcastsetpid(tp->tnc, 0xbb);
    }


  } else {  /* read file body    */
    if (tp->FileHandle < minhandle)
      tp->FileHandle = sfopen(tp->fileinfo->body_filename, FO_READ);
    if (tp->FileHandle >= minhandle) {
      lseek(tp->FileHandle, start - fileheadersize, 0);
      read(tp->FileHandle, &fhe[fh_data_1 - 1], hl);
    }
  }



  crc = 0;
  crcfbb_buf(fhe, fh_headersize + hl, &crc);

  integer16dp(crc, &fhe[fh_headersize + hl + 1], &fhe[fh_headersize + hl]);

  bcastsend(bcastmonchan(tp->tnc), fh_headersize + hl + 2, fhe);
  Result = hl;
  tp->lasttx = sys_ixtime();
  return Result;
}


Static boolean bcastsemaphore;


Static void send_bcast_1(short chan, short num_free)
{
  pactxdesc *tp, *mtp;
  unsigned short x, y;
  sendlisttype *hsp;
  short ActiveFiles, FramesPerFile;
  long freebuf;
  short lasttnc;
  Char TEMP[256];
  Char STR1[256];
  unsigned short FORLIM;

  if (pactxroot != NULL) {
    freebuf = num_free;
    tp = pactxroot;
    mtp = tp;
    ActiveFiles = 0;
    lasttnc = -10;
    do {
      if (mtp->sendlist != NULL) {
	if (mtp->sendlist->len > 0)
	  ActiveFiles++;
      }
      mtp = mtp->next;
    } while (mtp != NULL);

    if (freebuf > 270 && ActiveFiles > 0) {
      freebuf = (freebuf - 250) * 16;
	  /* Freier Bereich in Bytes. 200 Puffer frei lassen   */
      FramesPerFile = freebuf / tp->blocksize;
	  /* das ist ein bisschen falsch,  */
      /* da das nicht TNC-selektiv ist */
      FramesPerFile /= ActiveFiles;
      if (FramesPerFile > 50)
	FramesPerFile = 50;

      while (tp != NULL) {
	if (tp->FileID != 0 && tp->sendlist->len > 0) {
	  if (FramesPerFile > 0) {
	    bcastswitchtobc(tp->tnc, true);

	    if (lasttnc != tp->tnc) {
	      if (lasttnc >= 0) {
		if (!bcastshpacsat()) {
		  strcpy(TEMP, " - ");
		  bcastsetmonstat(lasttnc, TEMP);
		  bcastpopmonstat(lasttnc);
		}
		bcastpopunproto(lasttnc);
		bcastpoppid(lasttnc);
	      }

	      lasttnc = tp->tnc;
	      bcastpushpid(lasttnc);
	      bcastpushunproto(lasttnc);
	      if (!bcastshpacsat()) {
		bcastpushmonstat(lasttnc);

		sprintf(TEMP, " - %s",
			bcastgetmycall(STR1, bcastmonchan(lasttnc)));
		bcastsetmonstat(lasttnc, TEMP);
	      }
	      bcastsetpid(lasttnc, 0xbb);
	      bcastsetunproto(lasttnc, tp->UnprotoAddr);

	    }

	    FORLIM = FramesPerFile;
	    for (x = 1; x <= FORLIM; x++) {
	      if (tp->sendlist->len > 0) {
		if (bcastusereingabe()) {
		  goto _L1;

		}
		y = send_bcast2(tp->blocksize, tp->sendlist->offset, tp);
		if (y < tp->sendlist->len) {
		  tp->sendlist->offset += y;
		  tp->sendlist->len -= y;
		} else {
		  if (tp->sendlist->next != NULL) {
		    hsp = tp->sendlist;
		    tp->sendlist = tp->sendlist->next;
		    free(hsp);
		  } else {
		    tp->sendlist->offset = 0;
		    tp->sendlist->len = 0;
		  }
		}

	      }

	    }

	    bcastswitchtobc(tp->tnc, false);
	  }

	}


	tp = tp->next;
      }


_L1:
      if (lasttnc >= 0) {
	bcastswitchtobc(lasttnc, true);
	if (!bcastshpacsat()) {
	  strcpy(TEMP, " - ");
	  bcastsetmonstat(lasttnc, TEMP);
	  bcastpopmonstat(lasttnc);
	}
	bcastpopunproto(lasttnc);
	bcastpoppid(lasttnc);
	bcastswitchtobc(lasttnc, false);
      }
    }
  }
  bcastsemaphore = false;


}



Static void send_bcast(void)
{
  void (*TEMP)(short chan, short num_free);

  if (pactxroot == NULL || bcastsemaphore)
    return;
  bcastsemaphore = true;
  bcastswitchtobc(pactxroot->tnc, true);
  TEMP = send_bcast_1;
  bcasttncbuffer(TEMP, pactxroot->tnc);
  bcastswitchtobc(pactxroot->tnc, false);
}


Static void received_bcast(pacrxdesc *tp)
{
  Char name1[256];
  Char name2[256];
  Char hs[256];
  Char STR1[256], TEMP[256];

  sfclose(&tp->FileHandle);
  sprintf(name1, "%sBCASTRX.%s", BcastTempDir(STR1), tp->FileExt);
  dispose_holelist(tp->holelist);
  tp->holelist = NULL;
  tp->receive_ready = true;
  strcpy(hs, tp->fromAddr);
  if (strpos2(hs, "-", 1) > 0)
    cut(hs, strpos2(hs, "-", 1) - 1);

  if (tp->is_bbs_file) {
    strcpy(name2, BcastNewMailDir(STR1));
    sprintf(name2 + strlen(name2), "$%s.001", hs);
    if (dec_bcstfile(hs, true, name1, name2))
      sfdelfile(name1);
    return;
  }


  strcpy(name2, BcastSaveDir(STR1));
  if (dec_bcstfile(hs, false, name1, name2)) {
    sprintf(TEMP, "file decoded: %s", name2);
    BcastProtokoll(TEMP);
    sfdelfile(name1);
  }
}


Static void abort_rx(pacrxdesc *tp, Char *txt_)
{
  Char txt[256];
  Char STR1[256], TEMP[256];

  strcpy(txt, txt_);
  sfclose(&tp->FileHandle);

  sprintf(TEMP, "%sBCASTRX.%s", BcastTempDir(STR1), tp->FileExt);
  sfdelfile(TEMP);
  dispose_holelist(tp->holelist);
  tp->holelist = NULL;
  tp->receive_ready = true;
  if (*txt != '\0') {
    sprintf(TEMP, "reception aborted: %s", txt);
    BcastProtokoll(TEMP);
  }
}


Static void fill_up(pacrxdesc *tp, long count)
{
  frametyp hi;
  long frag;
  unsigned short x;
  boolean abort;

  for (x = 0; x < sizeof(frametyp); x++)
    hi[x] = 0;
  lseek(tp->FileHandle, 0, 2);
  abort = false;
  frag = sizeof(frametyp);
  while (!abort && count > 0) {
    if (frag > count)
      frag = count;
    if (write(tp->FileHandle, hi, frag) != frag) {
      abort_rx(tp, "disk full");
      abort = true;
    } else
      count -= frag;
  }
}


Static boolean rearrange_holes(long offset, unsigned short size,
			       holelisttype *hp)
{
  boolean Result;
  holelisttype *hp2;
  long zw;
  Char TEMP[256];

  Result = true;
  if (hp->offset == offset) {
    if (hp->len <= size) {  /* most simply case: erase hole */
      if (hp->prev != NULL) {
	hp->prev->next = hp->next;
	if (hp->next != NULL)
	  hp->next->prev = hp->prev;
	free(hp);
	return Result;
      }
      if (hp->next == NULL) {
	hp->offset = 0;
	hp->len = 0;
	return Result;
      }
      hp2 = hp->next;
      *hp = *hp2;
      hp->prev = NULL;
      free(hp2);
      return Result;
    }


    if (hp->len <= size)  /* shortening a hole */
      return Result;
    hp->offset = offset + size;
    if (hp->len <= max_bcastfilesize)
      hp->len -= size;
    return Result;
  }


  if (hp->offset >= offset || hp->offset + hp->len < offset + size)
  {  /* splitting up a hole */
    strcpy(TEMP, "error in holelist");
    BcastProtokoll(TEMP);
    return false;
  }


  if (hp->offset + hp->len == offset + size) {  /* end of hole received */
    hp->len -= size;
    return Result;
  }


  hp2 = malloc(sizeof(holelisttype));
  if (hp2 == NULL) {
    strcpy(TEMP, "no mem for holelist");
    BcastProtokoll(TEMP);
    return false;
  }
  hp2->prev = hp;
  hp2->next = hp->next;
  hp->next = hp2;
  hp2->offset = offset + size;
  zw = hp->len;
  hp->len = offset - hp->offset;
  if (zw <= max_bcastfilesize)
    hp2->len = zw - size - hp->len;
  else
    hp2->len = zw;
  return Result;

  /* We are the root ... */
  /* nothing follows, file is already received */
  /* really split up */
}


Static boolean in_holelist(long offset, unsigned short size, holelisttype **hp)
{
  if (*hp != NULL) {
    while (offset >= (*hp)->offset + (*hp)->len && (*hp)->next != NULL)
      *hp = (*hp)->next;
    return (offset >= (*hp)->offset && offset < (*hp)->offset + (*hp)->len);
  } else
    return false;
}


Static long get_filesize(pacrxdesc *tp, unsigned short size, uchar *info)
{
  long Result, fsiz;
  unsigned short idx, y, z, len, sel;
  Char ufname[256];
  Char dest[256];
  Char file_name[9];
  Char file_ext[4];
  Char msgid[256];
  boolean stop, aborted;
  Char TEMP[256];

  file_name[0] = '\0';
  file_ext[0] = '\0';
  ufname[0] = '\0';
  msgid[0] = '\0';
  dest[0] = '\0';
  fsiz = 0;
  idx = 3;
  stop = false;
  aborted = false;

  do {

    sel = dp16integer(info[idx - 1], info[idx]);

    switch (sel) {

    case PH_HEADER_END:
      stop = true;
      break;

    case PH_FILE_SIZE:
      fsiz = dp32long(info[idx + 2], info[idx + 3], info[idx + 4],
		      info[idx + 5]);
      break;

    case PH_USER_FILE_NAME:
      len = info[idx + 1];
      if (idx + len + 2 <= size)
	z = len;
      else
	z = size - idx - 2;
      for (y = 1; y <= z; y++)
	ufname[y - 1] = info[idx + y + 1];
      ufname[z] = '\0';
      break;

    case PH_FILE_NAME:
      if (idx + 10 <= size)
	z = 8;
      else
	z = size - idx - 2;
      for (y = 1; y <= z; y++)
	file_name[y - 1] = info[idx + y + 1];
      file_name[z] = '\0';
      del_lastblanks(file_name);
      break;

    case PH_FILE_EXT:
      if (idx + 5 <= size)
	z = 3;
      else
	z = size - idx - 2;
      for (y = 1; y <= z; y++)
	file_ext[y - 1] = info[idx + y + 1];
      file_ext[z] = '\0';
      del_lastblanks(file_ext);
      break;

    /*           PH_FILE_TYPE    :   if info^[idx+3] = 1 then
                                        tp^.is_bbs_file := true; */

    case PH_DESTINATION:
      tp->is_bbs_file = true;
      len = info[idx + 1];
      if (idx + len + 2 <= size)
	z = len;
      else
	z = size - idx - 2;
      for (y = 1; y <= z; y++)
	dest[y - 1] = info[idx + y + 1];
      dest[z] = '\0';
      z = strpos2(dest, "@", 1);
      if (z > 0)
	cut(dest, z - 1);
      del_lastblanks(dest);
      break;

    case PH_BULLETIN_ID_NUMBER:
      tp->is_bbs_file = true;
      len = info[idx + 1];
      if (idx + len + 2 <= size) {
	z = len;
	for (y = 1; y <= z; y++)
	  msgid[y - 1] = info[idx + y + 1];
	msgid[z] = '\0';
	if (msgid[0] == '$')   /* der TNN-Fehler */
	  strdelete((void *)msgid, 1, 1);
	cut(msgid, 12);
	strcpy(tp->msgid, msgid);
      }
      break;

    }

    idx += info[idx + 1] + 3;

  } while (!(idx >= size || stop));

  Result = fsiz;

  if (!tp->is_bbs_file) {
    if (*ufname != '\0') {
      sprintf(TEMP, "header received: %s", ufname);
      BcastProtokoll(TEMP);
    } else if (*file_name != '\0') {
      sprintf(TEMP, "header received: %s.%s", file_name, file_ext);
      BcastProtokoll(TEMP);
    } else {
      strcpy(TEMP, "invalid file header");
      BcastProtokoll(TEMP);
    }
  }

  if (*tp->msgid != '\0')
  {  /* if a BID was received, check if it is yet known */
    if (!bcastcheckbid(tp->msgid)) {  /* ok, stop reception */
      abort_rx(tp, "");
      aborted = true;
      Result = 0;
    }
  }

  if (!aborted) {
    if (*dest != '\0') {
      cut(dest, 8);
      /*          if BcastBadname(dest) then begin
                      abort_rx(tp,'');
{               aborted         := true; }
                      get_filesize    := 0;
                  end; */
    }
  }

  return Result;
}


/* There we are... Right now, it is assured the data in the infoblock is     */
/* valid, what means it is truly a pacsat broadcast and the crc of the frame */
/* is correct. Data starts at position 1 of the info-array and has SIZE      */
/* bytes of length. The position of the data within the whole file is        */
/* determined by OFFSET.                                                     */
/* TP holds a pointer on the description structure of this very transmission.*/
/* This routine has to check for reception holes (and maintain a hole list), */
/* has to detect a completely received file and, of course, has to store     */
/* the received data at the right position of the intermediate file. After   */
/* complete reception the file has to be converted (stripped of the File     */
/* Header) and stored anywhere in the file system. Sounds simple, huh?       */

Static void write_bcast(pacrxdesc *tp, long Offset, unsigned short size,
			uchar *info)
{
  holelisttype *hp, *hp2;
  long fsiz;
  Char STR1[256];
  Char TEMP[256];

  if (tp->blocksize < size)
    tp->blocksize = size;
  hp = tp->holelist;
  if (!in_holelist(Offset, size, &hp))  /* ok, write that */
    return;
  if (tp->FileHandle < minhandle) {
    sprintf(TEMP, "%sBCASTRX.%s", BcastTempDir(STR1), tp->FileExt);
    tp->FileHandle = sfopen(TEMP, FO_RW);
  }
  if (tp->FileHandle < minhandle) {
    abort_rx(tp, "open error");
    return;
  }
  if (Offset > tp->tempSize)
    fill_up(tp, Offset - tp->tempSize);
  if (lseek(tp->FileHandle, Offset, 0) != Offset) {
    abort_rx(tp, "seek error");
    return;
  }
  if (write(tp->FileHandle, info, size) != size) {
    abort_rx(tp, "write error");
    return;
  }
  if (tp->tempSize < Offset + size)
    tp->tempSize = Offset + size;
  if (Offset == 0) {
    fsiz = get_filesize(tp, size, info);
    if (fsiz > 0) {
      tp->fheader_ok = true;
      hp2 = tp->holelist;
      while (hp2->next != NULL)
	hp2 = hp2->next;
      hp2->len = fsiz - hp2->offset;
    }
  }

  if (!rearrange_holes(Offset, size, hp)) {
    abort_rx(tp, "administration failure");
    return;
  }


  if (tp->fheader_ok) {
    if (tp->holelist->len == 0)   /* End of File */
      received_bcast(tp);
  }

}


Static void resp_request(long file_id, unsigned short blocksize,
			 unsigned short infosize, uchar *info)
{
  pactxdesc *tp;
  long start;
  unsigned short idx, len;

  if (pactxroot == NULL)
    return;
  tp = pactxroot;
  while (tp->FileID != file_id && tp->next != NULL)
    tp = tp->next;
  if (tp->FileID != file_id)
    return;
  idx = 8;
  while (idx < infosize) {
    start = dp24long(info[idx - 1], info[idx], info[idx + 1]);
    len = dp16integer(info[idx + 2], info[idx + 3]);
    add_to_sendlist(start, len, tp);
	/* put a remark in the file's sendlist */
    idx += 5;
  }
}


Static void set_last_heard_call(Char *call1)
{
  pacrxdesc *tp;
  long atime;

  tp = pacrxroot;
  atime = sys_ixtime();
  while (tp != NULL) {
    if (!strcmp(call1, tp->fromAddr))
      tp->lastrxother = atime;
    tp = tp->next;
  }
}


boolean dec_bcast(uchar pid, uchar rxtnc, Char *call1, Char *call2,
		  Char *digipeat, unsigned short infosize, uchar *inf1)
{
  pacrxdesc *tp, *tp2;
  Char w[21];
  unsigned short crc;
  long offset;
  unsigned short bytes, bits, pbsize;
  long file_id;
  Char hs[256];
  uchar *info;
  boolean lflag;
  unsigned short firstdata;
  boolean result;
  Char STR1[256];
  Char TEMP[256];

  info = inf1;
  result = false;
  if (info == NULL)
    return result;

  set_last_heard_call(call1);
  if (!strcmp(call2, "QST-1") && (pid == 0xf0 || pid == 0xbb || pid == 0xbd))
  {  /* File Broadcast */
    if (infosize <= 8)
      return result;

    if (((info[fh_flags - 1] & PF_DIR) == PF_ISDIR) || 
        (pid == 0xbd))  /* DIR-Broadcast*/
      return true;


    if ((info[fh_flags - 1] & (PF_REQ + PF_O)) != (PF_ISBCAST + PF_OFFSMODE)) {
      strcpy(TEMP, "wrong flags");
      BcastProtokoll(TEMP);
      return result;
    }
    lflag = ((info[fh_flags - 1] & PF_L) == PF_LPRESENT);

    if (lflag) {
      bits = dp16integer(info[fh_length_b1 - 1], info[fh_length_b2 - 1]);
/*      if (bits <= 2048 && bits >= 0) */
      if (bits <= 2048)
	bytes = (bits + 7) >> 3;
      else
	bytes = 0;
      firstdata = fh_data_1_lset;
    } else {
      bytes = infosize - fh_headersize_lunset - 1;
      firstdata = fh_data_1_lunset;
    }


    if (infosize != firstdata + bytes + 1) {
      strcpy(TEMP, "unmatching frame size");
      BcastProtokoll(TEMP);
      return result;
    }

    crc = 0;
    crcfbb_buf(info, infosize, &crc);
    if (crc != 0) {
      strcpy(TEMP, "invalid frame crc");
      BcastProtokoll(TEMP);
      return result;
    }

    file_id = dp32long(info[fh_file_id_b1 - 1], info[fh_file_id_b2 - 1],
		       info[fh_file_id_b3 - 1], info[fh_file_id_b4 - 1]);

    if (file_id == 0) {
      strcpy(TEMP, "invalid file id in frame");
      BcastProtokoll(TEMP);
      return result;
    }
    result = true;

    offset = dp24long(info[fh_offset_b1 - 1], info[fh_offset_b2 - 1],
		      info[fh_offset_b3 - 1]);

    if (pacrxroot == NULL) {
      pacrxroot = malloc(sizeof(pacrxdesc));
      if (pacrxroot != NULL) {
	pacrxroot->FileID = 0;
	pacrxroot->next = NULL;
      }
    }

    if (pacrxroot == NULL) {
      strcpy(TEMP, "no mem");
      BcastProtokoll(TEMP);
      return result;
    }
    tp = pacrxroot;
    while (tp->next != NULL && tp->FileID != 0 &&
	   (tp->FileID != file_id || strcmp(tp->fromAddr, call1)))
      tp = tp->next;

    if (tp->FileID == file_id) {
      tp->lastrx = sys_ixtime();
      if (!tp->receive_ready)
	write_bcast(tp, offset, bytes, &info[firstdata - 1]);
      return result;
    }
    if (tp->FileID != 0) {
      tp2 = malloc(sizeof(pacrxdesc));
      tp->next = tp2;
      if (tp2 != NULL)
	tp2->next = NULL;
      tp = tp2;
      tp->FileID = 0;
    }

    if (tp == NULL) {
      strcpy(TEMP, "invalid error");
      BcastProtokoll(TEMP);
      return result;
    }
    tp->holelist = malloc(sizeof(holelisttype));
    if (tp->holelist == NULL) {
      strcpy(TEMP, "no mem for holelist");
      BcastProtokoll(TEMP);
      return result;
    }
    sprintf(hs, "%sBCASTRX.001", BcastTempDir(STR1));
    validate(hs);
    tp->FileHandle = sfcreate(hs, FC_FILE);
    if (tp->FileHandle < minhandle) {
      strcpy(TEMP, "file create error");
      BcastProtokoll(TEMP);
      return result;
    }
    tp->FileID = file_id;
    get_ext(hs, w);
    strcpy(tp->FileExt, w);
    tp->tempSize = 0;
    tp->tnc = rxtnc;
    tp->lastrx = sys_ixtime();
    tp->lastrxother = tp->lastrx;
    strcpy(tp->fromAddr, call1);
    if (strlen(digipeat) <= 'x')
      strcpy(tp->viaDigi, digipeat);
    else
      tp->viaDigi[0] = '\0';
    tp->is_bbs_file = false;
    tp->msgid[0] = '\0';
    tp->fheader_ok = false;
    tp->blocksize = 0;
    tp->holelist->next = NULL;
    tp->holelist->prev = NULL;
    tp->holelist->offset = 0;
    tp->receive_ready = false;
    tp->holelist->len = over_bcastfilesize;

    write_bcast(tp, offset, bytes, &info[firstdata - 1]);
    return result;
  }


  if (infosize <= 6) {
    strcpy(TEMP, "request frame too short");
    BcastProtokoll(TEMP);
    return result;
  }
  if ((info[rh_flags - 1] & (PF_REQ + PF_CC)) != (PF_ISREQUEST + PF_HOLE)) {
    if (((~info[rh_flags - 1]) & PD_E) == PD_E) {
      strcpy(TEMP, "wrong request flags");
      BcastProtokoll(TEMP);
    }
    return result;
  }
  if (infosize <= 11 || (infosize - 7) % 5 != 0) {
    strcpy(TEMP, "wrong request frame length");
    BcastProtokoll(TEMP);
    return result;
  }
  result = true;

  file_id = dp32long(info[rh_file_id_b1 - 1], info[rh_file_id_b2 - 1],
		     info[rh_file_id_b3 - 1], info[rh_file_id_b4 - 1]);

  pbsize = dp16integer(info[rh_block_b1 - 1], info[rh_block_b2 - 1]);

  resp_request(file_id, pbsize, infosize, info);
  return result;


  /* Request */
}


Static void dispose_bcasttxptr(boolean del_source, long FileID)
{
  pactxdesc *tp, *tp1, *tp2;
  sendlisttype *sl;

  tp = pactxroot;

  while (tp != NULL) {
    if (tp->FileID != FileID) {
      tp = tp->next;
      continue;
    }


    sfclose(&tp->FileHandle);

    if (del_source) {
      if (tp->fileinfo != NULL)
	sfdelfile(tp->fileinfo->body_filename);
    }

    tp1 = pactxroot;
    tp2 = tp1;
    while (tp1 != tp && tp1->next != NULL) {
      tp2 = tp1;
      tp1 = tp1->next;
    }

    if (tp1 == tp) {
      if (tp1 == pactxroot)
	pactxroot = tp1->next;
      else
	tp2->next = tp1->next;

      free_fileinfo(tp1->fileinfo);

      do {
	sl = tp1->sendlist;
	if (sl != NULL) {
	  tp1->sendlist = tp1->sendlist->next;
	  free(sl);
	}
      } while (tp1->sendlist != NULL);

      free(tp1);

    }

    tp = NULL;
  }

}


Static void check_txtimer(void)
{
  pactxdesc *tp;
  long atime, hid;
  bcastcallbacktype cbackp;

  atime = sys_ixtime();
  tp = pactxroot;
  while (tp != NULL) {
    if (tp->FileID != 0) {
      if (tp->sendlist != NULL) {
	if (tp->sendlist->len == 0) {
	  if (tp->delposttx) {
	    hid = tp->FileID;
	    cbackp = tp->callback;
	    dispose_bcasttxptr(true, hid);
	    tp = NULL;
	    cbackp(hid);
	  } else if (tp->not_called) {
	    tp->not_called = false;
	    tp->callback(tp->FileID);
	  }

	  if (tp != NULL) {
	    /* Delete after ... */

	    if (atime - tp->lasttx > tx_timeout) {  /* after 3 min */
	      dispose_bcasttxptr(false, tp->FileID);
	      tp = NULL;
	    } else if (atime - tp->lasttx > tx_closefile)
	      sfclose(&tp->FileHandle);


	  }


	}

      } else {  /* error ! */
	dispose_bcasttxptr(false, tp->FileID);
	tp = NULL;
      }


    }

    if (tp != NULL)
      tp = tp->next;
  }


  /* close file after 10 sec. */
}


Static void dispose_bcastrxptr(pacrxdesc *tp)
{
  pacrxdesc *tp1, *tp2;
  Char name1[256];
  Char STR1[256];
  Char TEMP[256];

  if (tp == NULL)
    return;
  if (!tp->receive_ready) {
    sfclose(&tp->FileHandle);
    sprintf(name1, "%sBCASTRX.%s", BcastTempDir(STR1), tp->FileExt);
    dispose_holelist(tp->holelist);
    tp->holelist = NULL;
    strcpy(TEMP, "rx aborted");
    BcastProtokoll(TEMP);
    sfdelfile(name1);
  }

  tp1 = pacrxroot;
  tp2 = tp1;
  while (tp1 != tp && tp1->next != NULL) {
    tp2 = tp1;
    tp1 = tp1->next;
  }

  if (tp1 != tp)
    return;

  if (tp1 != pacrxroot) {
    tp2->next = tp1->next;
    free(tp1);
    return;
  }
  pacrxroot = pacrxroot->next;
  free(tp1);
}


Static void check_rxtimer(void)
{
  pacrxdesc *tp;
  long atime, mute;

  if (pacrxroot == NULL)
    return;
  atime = sys_ixtime();
  tp = pacrxroot;
  while (tp != NULL) {
    mute = atime - tp->lastrx;

    if ((mute > rx_timeout && !tp->receive_ready) ||
	(mute > rx_ok_timeout && tp->receive_ready)) {
      dispose_bcastrxptr(tp);
      tp = NULL;
    } else if (mute > rx_closefile)
      sfclose(&tp->FileHandle);

    if (tp != NULL)
      tp = tp->next;

  }
}


void bcast_timing(void)
{
  if (pactxroot != NULL) {
    send_bcast();
    check_txtimer();
  } else
    bcastsemaphore = false;

  if (pacrxroot == NULL)
    return;

  if (bcastcreaterequests())
    bcast_requests();
  check_rxtimer();
}


void bcast_exit(void)
{
  /* called at program exit */
  pactxdesc *tp, *tph;
  pacrxdesc *rp, *rph;

  tp = pactxroot;
  while (tp != NULL) {
    tph = tp;
    tp = tp->next;
    dispose_bcasttxptr(tph->delposttx, tph->FileID);
  }
  pactxroot = NULL;

  rp = pacrxroot;
  while (rp != NULL) {
    rph = rp;
    rp = rp->next;
    dispose_bcastrxptr(rph);
  }
  pacrxroot = NULL;
}


unsigned short decode_frameheader(uchar *inf1, unsigned short size, Char *sr)
{
  unsigned short Result;
  Char w[256];
  Char vs[9];
  uchar b;
  unsigned short crc;
  uchar *info;

  sr[0] = '\0';
  info = inf1;
  Result = 1;
  if (info == NULL || size <= 7)
    return Result;
  *w = '\0';
  b = info[fh_flags - 1];

  if (b == 'O' || b == 'N') {  /* ASCII msg */
    strcpy(sr, "ASCII");
    return Result;
  } else {
    int2str((b & PF_VV) >> 1, vs);

    if ((b & PF_REQ) == PF_ISBCAST && (b & PF_DIR) != PF_ISDIR)
    {  /* broadcast frame */
      if ((b & PF_E) == PF_ISLAST)
	strcpy(w, "E");
      if ((b & PF_O) == PF_OFFSMODE)
	strcat(w, "O");
      if ((b & PF_L) == PF_LPRESENT)
	strcat(w, "L");
      sprintf(sr, "v%s TRX %s", vs, w);

      int2hstr(dp32long(info[fh_file_id_b1 - 1], info[fh_file_id_b2 - 1],
			info[fh_file_id_b3 - 1], info[fh_file_id_b4 - 1]), w);
      sprintf(sr + strlen(sr), " ID:$%s", w);

      int2str(info[fh_file_type - 1], w);
      sprintf(sr + strlen(sr), " type:%s", w);

      int2hstr(dp24long(info[fh_offset_b1 - 1], info[fh_offset_b2 - 1],
			info[fh_offset_b3 - 1]), w);
      sprintf(sr + strlen(sr), " offs:$%s", w);

      if ((b & PF_L) == PF_LPRESENT) {
	lint2str(dp16integer(info[fh_length_b1 - 1], info[fh_length_b2 - 1]),
		 w);
	sprintf(sr + strlen(sr), " bits:%s", w);
	Result = 12;
      } else
	Result = 10;

      int2hstr(dp16integer(info[size - 2], info[size - 1]), w);
      sprintf(sr + strlen(sr), " CRC:$%s", w);

      crc = 0;
      crcfbb_buf(info, size, &crc);
      if (crc == 0)
	strcat(sr, " OK");
      else
	strcat(sr, " NG");
      return Result;
    } else if ((b & PF_REQ) == PF_ISBCAST && (b & PF_DIR) == PF_ISDIR) {
      if ((b & PF_E) == PF_ISLAST)
	strcpy(w, "E");
      sprintf(sr, "v%s DIR %s", vs, w);

      int2hstr(dp32long(info[dh_file_id_b1 - 1], info[dh_file_id_b2 - 1],
			info[dh_file_id_b3 - 1], info[dh_file_id_b4 - 1]), w);
      sprintf(sr + strlen(sr), " ID:$%s", w);

      int2hstr(dp32long(info[dh_offset_b1 - 1], info[dh_offset_b2 - 1],
			info[dh_offset_b3 - 1], info[dh_offset_b4 - 1]), w);
      sprintf(sr + strlen(sr), " offs:$%s", w);

      lint2str(dp32long(info[dh_t_old_b1 - 1], info[dh_t_old_b2 - 1],
			info[dh_t_old_b3 - 1], info[dh_t_old_b4 - 1]), w);
      sprintf(sr + strlen(sr), " t_old:%s", w);

      lint2str(dp32long(info[dh_t_new_b1 - 1], info[dh_t_new_b2 - 1],
			info[dh_t_new_b3 - 1], info[dh_t_new_b4 - 1]), w);
      sprintf(sr + strlen(sr), " t_new:%s", w);

      int2hstr(dp16integer(info[size - 2], info[size - 1]), w);
      sprintf(sr + strlen(sr), " CRC:$%s", w);
      crc = 0;
      crcfbb_buf(info, size, &crc);
      if (crc == 0)
	strcat(sr, " OK");
      else
	strcat(sr, " NG");

      return dh_data_1;
    } else if ((b & PF_REQ) == PF_ISREQUEST) {
      b &= PF_CC;
      if (b == PF_START)
	strcpy(w, "START");
      else if (b == PF_STOP)
	strcpy(w, "STOP");
      else if (b == PF_HOLE)
	strcpy(w, "HOLE");
      else
	strcpy(w, "ILLEGAL");
      sprintf(sr, "v%s REQ %s", vs, w);

      int2hstr(dp32long(info[rh_file_id_b1 - 1], info[rh_file_id_b2 - 1],
			info[rh_file_id_b3 - 1], info[rh_file_id_b4 - 1]), w);
      sprintf(sr + strlen(sr), " ID:$%s", w);

      int2hstr(dp16integer(info[rh_block_b1 - 1], info[rh_block_b2 - 1]), w);
      sprintf(sr + strlen(sr), " BLOCKSIZE:$%s", w);

      return 7;
    } else {
      strcpy(sr, "UNKNOWN BROADCAST FORMAT");
      return Result;
    }


  }

  return Result;


  /* directory frame */

  /* request frame */

}


void _bcast_init(void)
{
  static int _was_initialized = 0;
  if (_was_initialized++)
    return;
  pacrxroot = NULL;
  pactxroot = NULL;
  bcastsemaphore = false;


}



/* End. */

void cmd_bcrxstat(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  pacrxdesc *pacrxptr;
  char tmpstr[256];
  char timestring[20];
  struct tm cvtime;
  long totalsize;
  long holesize;
  int holes;
  long percent;
  holelisttype *hlp;
    
  if (pacrxroot == NULL) {
    cmd_display(mode,channel,"No broadcast-RX files active",1);
  }
  else {
    pacrxptr = pacrxroot;
    strcpy(tmpstr,"FileID   RX from   Nr  MsgID        Size     "
                  "Holes RXrdy last RX  Fhead BBS");
    cmd_display(mode,channel,tmpstr,1);
    strcpy(tmpstr,"----------------------------------------"
                  "---------------------------------------");
    cmd_display(mode,channel,tmpstr,1);
    while (pacrxptr != NULL) {
      cvtime = *gmtime(&pacrxptr->lastrx);
      sprintf(timestring,"%2.2u:%2.2u.%2.2u",
              cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec);
      totalsize = pacrxptr->tempSize;
      holes = 0;
      holesize = 0;
      if (!(pacrxptr->receive_ready)) {
        hlp = pacrxptr->holelist;
        while (hlp != NULL) {
          holes++;
          if (holes > 999) holes = 999;
          if (hlp->next != NULL) {
            holesize += hlp->len;
          }
          else {
            if (pacrxptr->fheader_ok) {
              totalsize = hlp->offset + hlp->len;
              holesize += hlp->len;
            }
            else {
              totalsize = hlp->offset;
            }
          }
          hlp = hlp->next;
        }
        percent = ((totalsize-holesize) * 100) / totalsize;
      }
      else {
        percent = 100;
      }
      sprintf(tmpstr,"%08.8x %-9.9s %-3.3s %-12.12s %08.8x  %3u   %3u%% "
                     "%s   %c    %c",
              pacrxptr->FileID,pacrxptr->fromAddr,
              (pacrxptr->receive_ready ? "" : pacrxptr->FileExt),
              pacrxptr->msgid,totalsize,holes,percent,timestring,
              (pacrxptr->fheader_ok ? 'Y' : 'N'),
              (pacrxptr->is_bbs_file ? 'Y' : 'N'));
      cmd_display(mode,channel,tmpstr,1);
      pacrxptr = pacrxptr->next;
    }
  }
}

void cmd_bctxstat(par1,par2,channel,len,mode,str)
int par1;
int par2;
int channel;
int len;
int mode;
char *str;
{
  pactxdesc *pactxptr;
  char tmpstr[256];
  char timestring[20];
  struct tm cvtime;

  if (pactxroot == NULL) {
    cmd_display(mode,channel,"No broadcast-TX files active",1);
  }
  else {
    pactxptr = pactxroot;
    strcpy(tmpstr,"FileID   MsgID        Size     "
                  "last TX ");
    cmd_display(mode,channel,tmpstr,1);
    strcpy(tmpstr,"----------------------------------------"
                  "---------------------------------------");
    cmd_display(mode,channel,tmpstr,1);
    while (pactxptr != NULL) {
      cvtime = *gmtime(&pactxptr->lasttx);
      sprintf(timestring,"%2.2u:%2.2u.%2.2u",
              cvtime.tm_hour,cvtime.tm_min,cvtime.tm_sec);
      sprintf(tmpstr,"%08.8x %-12.12s %08.8x %s",
              pactxptr->FileID,
              (((pactxptr->fileinfo) != NULL) ? 
               ((pactxptr->fileinfo)->bulletin_id) : ""),
              pactxptr->FileSize,timestring);
      cmd_display(mode,channel,tmpstr,1);
      pactxptr = pactxptr->next;
    }
  }
}

#endif
