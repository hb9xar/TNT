/* tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-97 by Mark Wahl
   For license details see documentation
   monitor frame analysis for ax25-kernel-interface(ax25dump.c)
   created: Mark Wahl DL4YBG 97/02/13
   updated: Johann Hanne DH3MB 99/01/02
   updated: Matthias Hensler 99/03/10
*/

/* This source is based on 'listen' of the ax25-utilities,
   the original headers follow below */
   
/* @(#) $Header: ax25dump.c,v 1.5 91/02/24 20:16:33 deyke Exp $ */

/* AX25 header tracing
 * Copyright 1991 Phil Karn, KA9Q
 */

#include "tnt.h"

#ifdef USE_AX25K
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "ax25k.h"

#define	LAPB_UNKNOWN	0
#define	LAPB_COMMAND	1
#define	LAPB_RESPONSE	2

#define	SEG_FIRST	0x80
#define	SEG_REM		0x7F

#define	PID_SEGMENT	0x08
#define	PID_ARP		0xCD
#define	PID_NETROM	0xCF
#define	PID_IP		0xCC
#define	PID_X25		0x01
#define	PID_TEXNET	0xC3
#define	PID_FLEXNET	0xCE
#define	PID_NO_L3	0xF0

#define	I		0x00
#define	S		0x01
#define	RR		0x01
#define	RNR		0x05
#define	REJ		0x09
#define	U		0x03
#define	SABM		0x2F
#define	SABME		0x6F
#define	DISC		0x43
#define	DM		0x0F
#define	UA		0x63
#define	FRMR		0x87
#define	UI		0x03
#define	PF		0x10
#define	EPF		0x01

#define	MMASK		7

#define	HDLCAEB		0x01
#define	SSID		0x1E
#define	REPEATED	0x80
#define	C		0x80
#define	SSSID_SPARE	0x40
#define	ESSID_SPARE	0x20

#define	ALEN		6
#define	AXLEN		7

#define	W		1
#define	X		2
#define	Y		4
#define	Z		8

extern void gen_stamp(char *buffer,int type);
extern void moni_display(int channel,char *buffer);
extern void moni_display_len(int channel,char *buffer);

extern int moni_para;

char *pax25(char *, unsigned char *);
static int  ftype(unsigned char *, int *, int *, int *, int *, int);
static char *decode_type(int);

#define NDAMA_STRING ""
#define DAMA_STRING " [DAMA]"

/* Dump an AX.25 packet header */
void ax25_dump(unsigned char *data, int length, char *port)
{
	char tmp[15];
	int ctlen, nr, ns, pf, pid, type, end, cmdrsp, extseq;
	char *dama;
	
	char temp[257];
	char monhead[257];
	
	monhead[0] = '\0';

	/* Extract the address header */
	if (length < (AXLEN + AXLEN + 1)) {
		/* Something wrong with the header */
		/*lprintf(T_ERROR, "AX25: bad header!\n");*/
		return;
	}

	if ((data[AXLEN + ALEN] & SSSID_SPARE) == SSSID_SPARE) {
		extseq = 0;
		/*lprintf(T_PROTOCOL, "AX25: ");*/
	} else {
		extseq = 1;
		/*lprintf(T_PROTOCOL, "EAX25: ");*/
	}

	if ((data[AXLEN + ALEN] & ESSID_SPARE) == ESSID_SPARE)
		dama = NDAMA_STRING;
	else
		dama = DAMA_STRING;

	if(port) { /* DH3MB */
		sprintf(temp, "%s:",port);
        	strcat(monhead,temp);
        }
	sprintf(temp, "fm %s",pax25(tmp,data + AXLEN));
        strcat(monhead,temp);
	sprintf(temp, " to %s",pax25(tmp, data));
        strcat(monhead,temp);

	cmdrsp = LAPB_UNKNOWN;

	if ((data[ALEN] & C) && !(data[AXLEN + ALEN] & C))
		cmdrsp = LAPB_COMMAND;

	if ((data[AXLEN + ALEN] & C) && !(data[ALEN] & C))
		cmdrsp = LAPB_RESPONSE;

	end = (data[AXLEN + ALEN] & HDLCAEB);

	data   += (AXLEN + AXLEN);
	length -= (AXLEN + AXLEN);

	if (!end) {
		sprintf(temp, " via");
		strcat(monhead,temp);

		while (!end) {
			/* Print digi string */
			sprintf(temp," %s%s", pax25(tmp, data), (data[ALEN] & REPEATED) ? "*" : "");
			strcat(monhead,temp);
			
			end = (data[ALEN] & HDLCAEB);
			
			data   += AXLEN;
			length -= AXLEN;
		}
	}
	
	if (length == 0) return;
	
	ctlen = ftype(data, &type, &ns, &nr, &pf, extseq);
	
	data   += ctlen;
	length -= ctlen;

	sprintf(temp, " ctl %s", decode_type(type));
	strcat(monhead,temp);

	if ((type & 0x3) != U) {  /* I or S frame? */
		sprintf(temp, "%d", nr);
		strcat(monhead,temp);
	}

	if (type == I) {
		sprintf(temp, "%d", ns);
		strcat(monhead,temp);
	}

	switch (cmdrsp) {
		case LAPB_COMMAND:
			if (pf) sprintf(temp, "+");
			else sprintf(temp, "^");
			strcat(monhead,temp);
			break;
		case LAPB_RESPONSE:
			if (pf) sprintf(temp, "-");
			else sprintf(temp, "v");
			strcat(monhead,temp);
			break;
		default:
			break;
	}

	if (type == I || type == UI) {
		if ((type == I) && (!(moni_para & MONI_INFO))) return; 
		if ((type == UI) && (!(moni_para & MONI_UNPR))) return; 
		/* Decode I field */
		if (length > 0) {        /* Get pid */
			pid = *data++;
			length--;
		
			sprintf(temp," pid %X%s",pid,dama);
			strcat(monhead,temp);
			gen_stamp(temp,ST_MONI);
			strcat(monhead,temp);
			moni_display(0,monhead);
			if (length > 256) {
			  moni_display_len(0,"\017[frame too long]");
			}
			else {
			  memcpy(temp+1,data,length);
			  temp[0] = (char)length - 1;
			  moni_display_len(0,temp);
			}
		}
	} else if (type == FRMR && length >= 3) {
		if (!(moni_para & MONI_SUPV)) return; 
		sprintf(temp,"%x%x%x",data[0],data[1],data[2]);
		strcat(monhead,temp);
		gen_stamp(temp,ST_MONI);
		strcat(monhead,temp);
		moni_display(0,monhead);
	} else {
		if (!(moni_para & MONI_SUPV)) return; 
		sprintf(temp,"%s", dama);
		strcat(monhead,temp);
		gen_stamp(temp,ST_MONI);
		strcat(monhead,temp);
		moni_display(0,monhead);
	}
}

static char *decode_type(int type)
{
	switch (type) {
		case I:
			return "I";
		case SABM:
			return "SABM";
		case SABME:
			return "SABME";
		case DISC:
			return "DISC";
		case DM:
			return "DM";
		case UA:
			return "UA";
		case RR:
			return "RR";
		case RNR:
			return "RNR";
		case REJ:
			return "REJ";
		case FRMR:
			return "FRMR";
		case UI:
			return "UI";
		default:
			return "[invalid]";
	}
}

char *pax25(char *buf, unsigned char *data)
{
	int i, ssid;
	char *s;
	char c;
	
	s = buf;
	
	for (i = 0; i < ALEN; i++) {
		c = (data[i] >> 1) & 0x7F;

		if (!isalnum(c) && c != ' ') {
			strcpy(buf, "[invalid]");
			return buf;
		}
		
		if (c != ' ') *s++ = c;
	}	

	if ((ssid = (data[ALEN] & SSID)) != 0)
		sprintf(s, "-%d", ssid >> 1);
	else
		*s = '\0';

	return(buf);
}

static int ftype(unsigned char *data, int *type, int *ns, int *nr, int *pf, int extseq)
{
	if (extseq) {
		if ((*data & 0x01) == 0) {	/* An I frame is an I-frame ... */
			*type = I;
			*ns   = (*data >> 1) & 127;
			data++;
			*nr   = (*data >> 1) & 127;
			*pf   = *data & EPF;
			return 2;
		}
		if (*data & 0x02) {
			*type = *data & ~PF;
			*pf   = *data & PF;
			return 1;
		} else {
			*type = *data;
			data++;
			*nr   = (*data >> 1) & 127;
			*pf   = *data & EPF;
			return 2;
		}
	} else {
		if ((*data & 0x01) == 0) {	/* An I frame is an I-frame ... */
			*type = I;
			*ns   = (*data >> 1) & 7;
			*nr   = (*data >> 5) & 7;
			*pf   = *data & PF;
			return 1;
		}
		if (*data & 0x02) {		/* U-frames use all except P/F bit for type */
			*type = *data & ~PF;
			*pf   = *data & PF;
			return 1;
		} else {			/* S-frames use low order 4 bits for type */
			*type = *data & 0x0F;
			*nr   = (*data >> 5) & 7;
			*pf   = *data & PF;
			return 1;
		}
	}
}
#endif
