/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1995 by Mark Wahl
   For license details see documentation
   include file for huffman compression (comp.h)
   created: Mark Wahl DL4YBG 95/02/05
   updated: Mark Wahl DL4YBG 95/02/05
*/

struct huffencodtab {
  unsigned short code;
  unsigned short len;
};

struct huffdecodtab {
  unsigned short node1;
  unsigned short node2;
};
