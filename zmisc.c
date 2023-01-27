/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Based off The-Box Mailer (C) Jac Kersing
 *
 * This file is part of Xenia Mailer.
 * Xenia Mailer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include "zmodem.h"

static char hex[] = "0123456789abcdef"; 	 /* put char to modem in hex */
#define z_puthex(c) { com_bufbyte(hex[((c)&0xf0)>>4]); com_bufbyte(hex[(c)&0x0f]); }

static void z_putb32hdr (int type, char *hdr);
static int  z_gethhdr	(char *hdr);
static int  z_getbhdr	(char *hdr);
static int  z_getb32hdr (char *hdr);
static void z_put32data (byte *buf, int length, int frameend);
static int  z_get32data (char *buf, int length);

/*---------------------------------------------------------------------------*/
void z_longtohdr(long parm)		       /* put long into frame header */
{
	txhdr[ZP0] = parm;
	txhdr[ZP1] = parm>>8;
	txhdr[ZP2] = parm>>16;
	txhdr[ZP3] = parm>>24;
} /* z_longtohdr() */


/*---------------------------------------------------------------------------*/
long z_hdrtolong(register byte *hdr)  /* get long from frame header */
/*register byte *hdr;			     /+ hdr to use - trick: unsigned */
{
	long l;

	l = hdr[ZP3];
	l = (l<<8) | hdr[ZP2];
	l = (l<<8) | hdr[ZP1];
	l = (l<<8) | hdr[ZP0];

	return (l);
} /* z_hdrtolong() */


/*---------------------------------------------------------------------------*/
void z_puts(register char *str) 		      /* put string to modem */
/* register char *str;	/+ Process \335 (break-signal) and \336 (sleep 1 sec) */
{
	register int c;
	long t;

	while (c=*str++, c &= 0xff) {
	      switch (c) {
		     case '\335':
			  com_break();
			  break;
		     case '\336':
			  t = timerset(10);
			  while (!timeup(t)) win_idle();
			  break;
		     default:
			  com_bufbyte(c);
			  break;
	      }
	}

	com_flush();
} /* z_puts() */


/*---------------------------------------------------------------------------*/
void z_putc(register int c)				/* put char to modem */
/*register int c;     /+ use ZDLE encoding (XON, XOFF, and CR following '@') */
{
	static int lastc=0;

	c &= 0xff;

	if (directzap) {
	   com_bufbyte(c);
	   if (c == ZDLE)
	      com_bufbyte(ZDLEE);
	}
	else {
	   switch (c) {
		  case 015:
		  case 0215:
		       if ((lastc & 0x7f) != '@')
			  goto normalc;
		  case 020:
		  case 0220:
		  case 021:
		  case 0221:
		  case 023:
		  case 0223:
		  case ZDLE:
		       com_bufbyte(ZDLE);
		       c ^= 0x40;
		  default:
normalc:	       com_bufbyte(lastc = c);
	   }
	}
} /* z_putc() */


/*---------------------------------------------------------------------------*/
int z_get(int timeout)		      /* read char from modem (eat XON/XOFF) */
/*t timeout;					  /+ timeout in 10th of secs */
{
	register int c;
	register word garbage;
	long t;

	garbage = Z_GARBAGE;	      /* just a 'hanging prevention' counter */

	while ((c=com_getbyte()) != EOF) {	/* attempt to be faster if the */
	      switch (c & 0x7f) {	      /* inputbuffer is not empty... */
		     case XON:
		     case XOFF:
			  if (!directzap) {
			     if (--garbage > 0) break;
			     return (GARBAGE);
			  }
			  /*fallthrough to default*/
		     default:
			  return (c&0xff);
	      }
	      if (!carrier()) return (RCDO);
	}

	t = timerset((word) timeout);
	for (;;) {
	    while (!com_scan()) {
		  if (!carrier()) return (RCDO);
		  if (timeup(t)) return (TIMEOUT);
		  win_idle();
	    }

	    c = com_getbyte();
	    switch (c & 0x7f) {
		   case XON:
		   case XOFF:
			if (!directzap) {
			   if (--garbage > 0) break;
			   return (GARBAGE);
			}
			/*fallthrough to default*/
		   default:
			return (c&0xff);
	    }
	}
} /* z_get() */


/*---------------------------------------------------------------------------*/
int z_get7(int timeout) 		       /* read 7bits char from modem */
/*t timeout;					  /+ timeout in 10th of secs */
{
	register int c;

	return (((c = z_get(timeout)) < 0) ? c : c&0x7f);
} /* z_get7() */


/*---------------------------------------------------------------------------*/
int z_getc(int timeout) 	/* read 8bits char from modem (process ZDLE) */
/*gister int timeout;				  /+ timeout in 10th of secs */
{
	register int c;
	register int cancount;

	if ((c = z_get(timeout)) != ZDLE) return (c);

	cancount = 4;	/* ZDLE is the first CAN */
	switch (c = z_get(timeout)) {
	       case GARBAGE:
	       case RCDO:
	       case TIMEOUT:
		    return (c);
	       case CAN:
again:		    c = z_get(timeout);
		    if (c==CAN) {
		       if (--cancount > 0) goto again;
		       return (GOTCAN);
		    }
		    return ((c < 0) ? c : BADZDLE);
	       case ZCRCE:
	       case ZCRCG:
	       case ZCRCQ:
	       case ZCRCW:
		    return (c | GOTOR);
	       case ZRUB0:
		    return (0x7f);
	       case ZRUB1:
		    return (0xff);
	       default:
		    return ((c < 0) ? c :
			    ((c & 0x60) == 0x40) ? (c ^ 0x40) : BADZDLE);
	}
} /* z_getc() */


/*---------------------------------------------------------------------------*/
int z_gethex(int timeout)		 /* read (7bits) hex-char from modem */
/*t timeout;					  /+ timeout in 10th of secs */
{
	register int c, n;

	if ((n = z_get(timeout)) < 0) return (n);
	n -= '0';
	if (n > 9) n -= ('a' - ':');
	if (n & ~0x0f) return (BADHEX);

	if ((c = z_get(timeout)) < 0) return (c);
	c -= '0';
	if (c > 9) c -= ('a' - ':');
	if (c & ~0x0f) return (BADHEX);

	return ((n << 4) | c);
} /* z_gethex() */


/*---------------------------------------------------------------------------*/
void z_puthhdr(int type, register byte *hdr)	 /* send HEX header */
/*t type;						       /+ frame type */
/*gister byte *hdr;				      /+ hdr data   */
{
	register int i;
	register word crc;

#if ZDEBUG
	fprint(stderr,"--> %s%s (%lx)\n",
	       hdrtypes[HEXHDR],frametypes[type+FTOFFSET],z_hdrtolong(hdr));
#endif

	com_bufbyte(ZPAD);
	com_bufbyte(ZPAD);
	com_bufbyte(ZDLE);

	com_bufbyte(ZHEX);
	z_puthex(type);

	crc = (word) z_updcrc16(type,0);
	for (i=4; --i>=0;) {
	    z_puthex(*hdr);
	    crc = (word) z_updcrc16(*hdr++,crc);
	}

	z_puthex(crc >> 8);
	z_puthex(crc);

	com_bufbyte('\r');
	com_bufbyte('\n');
	if (type!=ZFIN && type!=ZACK)
	   com_bufbyte(XON);
	if (type != ZDATA) com_flush();
	else		   com_bufflush();
} /* z_puthhdr() */


/*---------------------------------------------------------------------------*/
void z_putbhdr(int type, register byte *hdr)	 /* send BIN header */
/*t type;						       /+ frame type */
/*gister byte *hdr;					/+ hdr data */
{
	register int i;
	register word crc;

	com_bufbyte(ZPAD);
	com_bufbyte(ZDLE);

	if (crc32t)
	   z_putb32hdr(type,(char *) hdr);
	else {
#if ZDEBUG
	   fprint(stderr,"--> %s%s (%lx)\n",
		  hdrtypes[BINHDR],frametypes[type+FTOFFSET],z_hdrtolong(hdr));
#endif

	   com_bufbyte(ZBIN);
	   z_putc(type);

	   crc = (word) z_updcrc16(type,0);
	   for (i=4; --i>=0;) {
	       z_putc(*hdr);
	       crc = (word) z_updcrc16(*hdr++,crc);
	   }

	   z_putc(crc >> 8);
	   z_putc(crc);
	}

	if (type != ZDATA) com_flush();
	else		   com_bufflush();
} /* z_putbhdr() */


/*---------------------------------------------------------------------------*/
static void z_putb32hdr(int type,register char *hdr)
				 /* send BIN32 header (z_putbhdr() static) */
/*int type;							 frame type */
/*register char *hdr;						   hdr data */
{
	register int i;
	register long crc;

#if ZDEBUG
	fprint(stderr,"--> %s%s (%lx)\n",
	       hdrtypes[BIN32HDR],frametypes[type+FTOFFSET],z_hdrtolong(hdr));
#endif

	com_bufbyte(ZBIN32);
	z_putc(type);

	crc = 0xffffffffl;
	crc = z_updcrc32(type,crc);
	for (i=4; --i >= 0;) {
	    z_putc(*hdr);
	    crc = z_updcrc32(*hdr++,crc);
	}

	crc = ~crc;
	for (i=4; --i >= 0;) {
	    z_putc((word)crc);
	    crc >>= 8;
	}
} /* z_putb32hdr() */


/*---------------------------------------------------------------------------*/
int z_gethdr(char *hdr) 		       /* receive header (all types) */
{
	register int c;
	register word garbage;
	int hadpad;
	int cancount;
#if ZDEBUG
	int rxhdrtype = NOHDR;
#endif

	garbage = Z_GARBAGE;
	cancount = 5;
	hadpad = 0;

again:	if (keyabort()) {
	   c = SYSABORT;
	   goto ret;
	}

	switch (c = z_get7(Z_TIMEOUT)) {
	       case ZPAD:			/* Exactly what we wanted    */
		    hadpad++;
		    goto gotcrap;		/* To protect us of too many */
	       case RCDO:
	       case TIMEOUT:
	       case GARBAGE:
		    goto ret;
	       case CAN:			/* Which also is a ZDLE      */
		    if (hadpad) break;		/* Even more what we wanted  */
gotcan: 	    if (--cancount > 0) goto again;
		    c = ZCAN;
		    goto ret;
	       case ZCRCW:
		    if (cancount < 5 && z_get7(1) < 0) {   /* at least 1 CAN */
		       c = HDRZCRCW;
		       goto ret;
		    }
	       default:
gotcrap:	    cancount = 5;
		    if (--garbage > 0) goto again;
		    c = GARBAGE;
		    goto ret;
	}

	/* Now we've got a ZPAD (or two) and a ZDLE (almost start of header) */

	switch (c = z_get7(Z_TIMEOUT)) {
	       case ZHEX:
		    crc32r = 0;
#if ZDEBUG
		    rxhdrtype = HEXHDR;
#endif
		    c = z_gethhdr(hdr);
		    break;
	       case ZBIN:
		    crc32r = 0;
#if ZDEBUG
		    rxhdrtype = BINHDR;
#endif
		    c = z_getbhdr(hdr);
		    break;
	       case ZBIN32:
		    crc32r = 1;
#if ZDEBUG
		    rxhdrtype = BIN32HDR;
#endif
		    c = z_getb32hdr(hdr);
		    break;
	       case RCDO:
	       case TIMEOUT:
	       case GARBAGE:
		    goto ret;
	       case CAN:
		    goto gotcan;
	       default:
		    hadpad = 0;     /* reset because we're doing it all over */
		    goto gotcrap;
	}

	rxpos = z_hdrtolong((byte *) hdr);

ret:	if (c == GOTCAN) c = ZCAN;
#if ZDEBUG
	if ((c < -FTOFFSET) || (c >= FRTYPES))
	   fprint(stderr,"<-- %s%d (%lx)\n",
		  hdrtypes[rxhdrtype],c,rxpos);
	else
	   fprint(stderr,"<-- %s%s (%lx)\n",
		  hdrtypes[rxhdrtype],frametypes[c+FTOFFSET],rxpos);
#endif

	return (c);
} /* z_gethdr() */


/*---------------------------------------------------------------------------*/
static int z_gethhdr(register char *hdr)
				   /* receive HEX header (z_gethdr() static) */
{
	register int c;
	register int i;
	register word crc;
	register int rxtype;

	if ((rxtype = z_gethex(Z_TIMEOUT)) < 0) return (rxtype);
	crc = (word) z_updcrc16(rxtype, 0);

	for (i=4; --i >= 0;) {
	    if ((c = z_gethex(Z_TIMEOUT)) < 0) return (c);
	    crc = (word) z_updcrc16(c,crc);
	    *hdr++ = c;
	}

	if ((c = z_gethex(Z_TIMEOUT)) < 0) return (c);
	crc = (word) z_updcrc16(c,crc);
	if ((c = z_gethex(Z_TIMEOUT)) < 0) return (c);
	crc = (word) z_updcrc16(c,crc);

	if (crc & 0xffff) return (HDRCRC);

	if ((c = z_get7(Z_TIMEOUT)) < 0) return (c);	      /* strip cr/lf */
	if (c == '\r')
	   if ((c = z_get7(Z_TIMEOUT)) < 0) return (c);

	return (rxtype);
} /* z_gethhdr() */


/*---------------------------------------------------------------------------*/
static int z_getbhdr(register char *hdr)
				   /* receive BIN header (z_gethdr() static) */
{
	register int c;
	register int i;
	register word crc;
	register int rxtype;

	if ((rxtype = z_getc(Z_TIMEOUT)) & ~0xff) return (rxtype);
	crc = (word) z_updcrc16(rxtype, 0);

	for (i=4; --i >= 0;) {
	    if ((c = z_getc(Z_TIMEOUT)) & ~0xff) return (c);
	    crc = (word) z_updcrc16(c,crc);
	    *hdr++ = c;
	}

	if ((c = z_getc(Z_TIMEOUT)) & ~0xff) return (c);
	crc = (word) z_updcrc16(c,crc);
	if ((c = z_getc(Z_TIMEOUT)) & ~0xff) return (c);
	crc = (word) z_updcrc16(c,crc);

	if (crc & 0xffff) return (HDRCRC);

	return (rxtype);
} /* z_getbhdr() */


/*---------------------------------------------------------------------------*/
static int z_getb32hdr(register char *hdr)
				 /* receive BIN32 header (z_gethdr() static) */
{
	register int c;
	register int i;
	register long crc;
	register int rxtype;

	if ((rxtype = z_getc(Z_TIMEOUT)) & ~0xff) return (rxtype);
	crc = 0xffffffffl;
	crc = z_updcrc32(rxtype, crc);

	for (i=4; --i >= 0;) {
	    if ((c = z_getc(Z_TIMEOUT)) & ~0xff) return (c);
	    crc = z_updcrc32(c,crc);
	    *hdr++ = c;
	}

	for (i=4; --i >= 0; ) {
	    if ((c = z_getc(Z_TIMEOUT)) & ~0xff) return (c);
	    crc = z_updcrc32(c,crc);
	}

	if (crc != 0xdebb20e3l) return (HDRCRC);

	return (rxtype);
} /* z_getb32hdr() */


/*---------------------------------------------------------------------------*/
void z_putdata(register byte *buf, register int length,
	     register int frameend)		  /* send BIN data subpacket */
{
	register word crc;

	if (crc32t)
	   z_put32data(buf,length,frameend);
	else {
	   crc = 0;

#if ZDEBUG
	   fprint(stderr,"--> DATA %s (%d)\n",
		  endtypes[frameend-ZCRCE&3],length);
#endif

	   for (; --length >= 0;) {
	       z_putc(*buf);
	       crc = (word) z_updcrc16(*buf++,crc);
	   }
	   com_bufbyte(ZDLE);
	   com_bufbyte(frameend);

	   crc = (word) z_updcrc16(frameend,crc);
	   z_putc(crc >> 8);
	   z_putc(crc);
	}

	if (frameend == ZCRCW) com_flush();
	else		       com_bufflush();
} /* z_putdata() */


/*---------------------------------------------------------------------------*/
static void z_put32data(register byte *buf,register int length,
		      register int frameend) /* send BIN32data (z_putdata()) */
{
	register long crc;

	crc = 0xffffffffl;

#if ZDEBUG
	fprint(stderr,"--> DATA32 %s (%d)\n",
	       endtypes[frameend-ZCRCE&3],length);
#endif

	for (; --length >= 0;) {
	    z_putc(*buf);
	    crc = z_updcrc32(*buf++,crc);
	}
	com_bufbyte(ZDLE);
	com_bufbyte(frameend);

	crc = z_updcrc32(frameend,crc);
	crc = ~crc;

	for (length=4; --length >= 0;) {
	    z_putc((word)crc);
	    crc >>= 8;
	}
} /* z_put32data() */


/*---------------------------------------------------------------------------*/
int z_getdata(register char *buf, int length)  /* receive BIN data subpacket */
{
	register int c;
	register word crc;
	register char *end;
	int d;

	if (crc32r)
	   return (z_get32data(buf,length));

	crc = 0;
	rxcount = 0;
	end = buf + length;
	d = 0;

	while (buf <= end) {
	      if ((c = z_getc(Z_TIMEOUT)) & ~0xff) {
		 switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
			     d = c;
			     crc = (word) z_updcrc16(c&0xff,crc);

			     if ((c = z_getc(Z_TIMEOUT)) & ~0xff) goto ret;
			     crc = (word) z_updcrc16(c,crc);
			     if ((c = z_getc(Z_TIMEOUT)) & ~0xff) goto ret;
			     crc = (word) z_updcrc16(c,crc);

			     if (crc & 0xffff) c = DATACRC;
			     else	       c = OK;
			     rxcount = length - (int) (end - buf);
			     goto ret;
			case GOTCAN:
			     c = ZCAN;
			default:
			     goto ret;
		 }
	      }
	      *buf++ = c;
	      crc = (word) z_updcrc16(c,crc);
	}

	c = DATALONG;

ret:
#if ZDEBUG
	fprint(stderr,"<-- DATA");
	if (d) fprint(stderr," %s",endtypes[d-ZCRCE&3]);
	fprint(stderr," (%d)",rxcount);
	if (c != OK) fprint(stderr," %s",frametypes[c+FTOFFSET]);
	fprint(stderr,"\n");
#endif

	return ((c==OK) ? d : c);
} /* z_getdata() */


/*---------------------------------------------------------------------------*/
static int z_get32data(register char *buf, int length)
				      /* rcv BIN32 data subpkt (z_getdata()) */
{
	register int c;
	register long crc;
	register char *end;
	register int i;
	int d;

	crc = 0xffffffffl;
	rxcount = 0;
	end = buf + length;
	d = 0;

	while (buf <= end) {
	      if ((c = z_getc(Z_TIMEOUT)) & ~0xff) {
		 switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
			     d = c;
			     crc = z_updcrc32(c&0xff,crc);

			     for (i=4; --i >= 0; ) {
				 if ((c = z_getc(Z_TIMEOUT)) & ~0xff) goto ret;
				 crc = z_updcrc32(c,crc);
			     }

			     if (crc != 0xdebb20e3l) c = DATACRC;
			     else		     c = OK;
			     rxcount = length - (int) (end - buf);
			     goto ret;
			case GOTCAN:
			     c = ZCAN;
			default:
			     goto ret;
		 }
	      }
	      *buf++ = c;
	      crc = z_updcrc32(c,crc);
	}

	c = DATALONG;

ret:
#if ZDEBUG
	fprint(stderr,"<-- DATA32");
	if (d) fprint(stderr," %s",endtypes[d-ZCRCE&3]);
	fprint(stderr," (%d)",rxcount);
	if (c != OK) fprint(stderr," %s",frametypes[c+FTOFFSET]);
	fprint(stderr,"\n");
#endif

	return ((c==OK) ? d : c);
} /* z_get32data() */


/* end of zmisc.c */
