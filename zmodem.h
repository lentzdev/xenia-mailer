/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
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


#include "xenia.h"

#define ZDEBUG 0  /* to activate prints for outgoing/incoming info * zvars.c */
#define ZCOMP  0  /* The-Box/Xenia special delivery data-compression (Agl89) */
   
#define TRUE		1
#define FALSE		0
#define END_BATCH	(-1)
#define NOTHING_TO_DO	(-2)
#define DELETE_AFTER	'-'
#define TRUNC_AFTER	'#'
#define NOTHING_AFTER	'@'
#define DO_WAZOO	TRUE
#define DONT_WAZOO	FALSE


#define z_updcrc16(c,crc) (crc16tab[((crc >> 8) & 255) ^ (c)] ^ (crc << 8))
#define z_updcrc32(c,crc) (crc32tab[((int)crc ^ (c)) & 0xff] ^ ((crc >> 8) & 0x00ffffffl))


/* general stuff ----------------------------------------------------------- */
#ifndef XON
#define XON		('Q'&037)	/* Control-Q (^Q) xmit-on character  */
#define XOFF		('S'&037)	/* Control-S (^S) xmit-off character */
#endif
#define CPMEOF		('Z'&037)	/* Control-Z (^Z) End-of-file char   */
#define CAN		('X'&037)	/* Control-X (^X) cancel character   */
#define ZPAD		'*'		/* 052 Padd character begins frames  */
#define ZDLE		030		/* Ctrl-X Zmodem Data Link Escape    */
#define ZDLEE		(ZDLE^0100)	/* Escaped ZDLE as transmitted	     */
#define ZBIN		'A'		/* Binary frame indicator	     */
#define ZHEX		'B'		/* HEX frame indicator		     */
#define ZBIN32		'C'		/* Binary frame with 32 bit FCS      */


/* frame types ------------------------------------------------------------- */
#define ZRQINIT 	0		/* Request receive init 	     */
#define ZRINIT		1		/* Receive init 		     */
#define ZSINIT		2		/* Send init sequence (optional)     */
#define ZACK		3		/* ACK to above 		     */
#define ZFILE		4		/* File name from sender	     */
#define ZSKIP		5		/* To sender: skip this file	     */
#define ZNAK		6		/* Last packet was garbled	     */
#define ZABORT		7		/* Abort batch transfers	     */
#define ZFIN		8		/* Finish session		     */
#define ZRPOS		9		/* Resume transfer at this position  */
#define ZDATA		10		/* Data packet(s) follow	     */
#define ZEOF		11		/* End of file			     */
#define ZFERR		12		/* Fatal read/write error detected   */
#define ZCRC		13		/* Request for file CRC and response */
#define ZCHALLENGE	14		/* Receiver's Challenge              */
#define ZCOMPL		15		/* Request is complete		     */
#define ZCAN		16		/* Other end CANd session with 5*CAN */
#define ZFREECNT	17		/* Request freespace on filesystem   */
#define ZCOMMAND	18		/* Command from sending program      */
#define ZSTDERR 	19		/* Output to stderr, data follows    */


/* ZDLE sequences ---------------------------------------------------------- */
#define ZCRCE		'h'		/* CRC nxt, frame end,hdrpkt follows */
#define ZCRCG		'i'		/* CRC nxt, frame continues non-stop */
#define ZCRCQ		'j'		/* CRC nxt, frame cont, ZACK expect  */
#define ZCRCW		'k'		/* CRC nxt, ZACK expect, frame ends  */
#define ZRUB0		'l'		/* Translate to rubout 0177	     */
#define ZRUB1		'm'		/* Translate to rubout 0377	     */


/* z_getc() return values (internal) --------------------------------------- */
#define GOTOR		0400
#define GOTCRCE 	(ZCRCE|GOTOR)	/* ZDLE-ZCRCE received		     */
#define GOTCRCG 	(ZCRCG|GOTOR)	/* ZDLE-ZCRCG received		     */
#define GOTCRCQ 	(ZCRCQ|GOTOR)	/* ZDLE-ZCRCQ received		     */
#define GOTCRCW 	(ZCRCW|GOTOR)	/* ZDLE-ZCRCW received		     */
#define GOTCAN		(GOTOR|030)	/* 5*CAN seen			     */


/* byte positions within header array -------------------------------------- */
#define ZF0		3		/* First flags byte		     */
#define ZF1		2
#define ZF2		1
#define ZF3		0
#define ZP0		0		/* Low order 8 bits of position      */
#define ZP1		1
#define ZP2		2
#define ZP3		3		/* High order 8 bits of file pos.    */


/* bit masks for ZRINIT flags byte ZF0 ------------------------------------- */
#define CANFDX		  01		/* Rx can send and receive true FDX  */
#define CANOVIO 	  02		/* Rx can rec. data during disk I/O  */
#define CANBRK		  04		/* Rx can send a break signal	     */
#define CANRLE		 010		/* Receiver can decode RLE	     */
#define CANLZW		 020		/* Receiver can uncompress	     */
#define CANFC32 	 040		/* Receiver can use 32bit FrameCheck */
#define ESCCTL		0100		/* Receiver expects CTLchars escaped */
#define ESC8		0200		/* Receiver expects 8th bit escaped  */


/* parameters for ZSINIT frame --------------------------------------------- */
#define ZATTNLEN	32		/* Max length of attention string    */
/* bit masks for ZSINIT flags byte ZF0 ------------------------------------- */
#define TESCCTL 	0100		/* Transmitter expects CTLchars esc  */
#define TESC8		0200		/* Transmitter expects 8th bit esc   */


/* parameters for ZFILE frame ---------------------------------------------- */
/* conversion options one of these in ZF0 ---------------------------------- */
#define ZCBIN		1		/* Binary transfer - inhibit conv.   */
#define ZCNL		2		/* Convert NL to local EOL convent.  */
#define ZCRESUM 	3		/* Resume interrupted file transfer  */
/* management include options, one of these ored in ZF1 -------------------- */
#define ZMSKNOLOC	0200		/* Skip file if not present at rx    */
/* management options, one of these ored in ZF1 ---------------------------- */
#define ZMMASK		037		/* Mask for the choices below	     */
#define ZMNEWL		1		/* Transfer if source newer|longer   */
#define ZMCRC		2		/* Transfer if diff.file CRC or len  */
#define ZMAPND		3		/* Append contents to existing?file  */
#define ZMCLOB		4		/* Replace existing file	     */
#define ZMNEW		5		/* Transfer if source newer	     */
#define ZMDIFF		6		/* Transfer if date or len differ    */
#define ZMPROT		7		/* Protect destination file	     */
/* transport options, one of these in ZF2 ---------------------------------- */
#define ZTLZW		1		/* Lempel-Ziv compression	     */
#define ZTRLE		3		/* Run Length encoding		     */
/* extended options for ZF3, bit encoded ----------------------------------- */
#define ZXSPARS 	64		/* Encoding for sparse file ops.     */


/* parameters for ZCOMMAND frame ZF0 (otherwise 0) ------------------------- */
#define ZCACK1		1		/* Acknowledge, then do command      */


#define LZCONV		0				  /* Our ZFILE flags */
#define LZMANAG 	0
#define LZTRANS 	0
#define LZEXT		0

#define ZRFLAGS 	(CANFDX|CANOVIO|CANFC32)  /* ZRINIT options (no BRK) */


/* stuff needed for my implementation -------------------------------------- */
/* some values which might change at some stage (performance question) ----- */
#define KSIZE		1024
#define Z_BUFLEN	(8*KSIZE)	/* Size of Zmodem databuffer	     */
#define Z_MAXBLKLEN	((word) (wazoo ? Z_BUFLEN : KSIZE)) /* Max size of datasubpkt */
#define Z_RETRIES	10		/* Nr. of retries in case of error   */
#define Z_TIMEOUT	50		/* Header-read routine timeout	     */
#define Z_BDD		1200		/* Brain (dead or damaged) timeout   */
#define Z_GARBAGE	((word) (cur_speed > 65535UL ? 65535U : cur_speed))  /* how many garbage-chars before err */
/* (error) conditions which might be returned to OSI level 5 (session layer) */
#define OK		0		/* All went just fine		     */
#define ERROR		(-1)		/* Just too many errors (retries)    */
#define ERRFILE 	(-2)		/* Error while opening/reading file  */
#define GARBAGE 	(-3)		/* Garbage count exceeded	     */
#define BADZDLE 	(-4)		/* Bad ZDLE sequence (by z_getc())   */
#define BADHEX		(-5)		/* Bad hex sequence (by z_gethex())  */
#define HDRCRC		(-6)		/* CRC-error in header-packet	     */
#define DATACRC 	(-7)		/* CRC-error in data-subpacket	     */
#define DATALONG	(-8)		/* Data subpacket too long	     */
#define TIMEOUT 	(-9)		/* Timeout in receive-hdr routine    */
#define BDD		(-10)		/* Brain (dead or damaged) timeout   */
#define RCDO		(-11)		/* Carrier lost error		     */
#define SYSABORT	(-12)		/* Aborted by operator (ESC)	     */
#define HDRZCRCW	(-13)		/* gethdr() got ZCRCW (err-recovery) */
#if ZCOMP
#define BADSDSEQ	(-14)		/* Bad Special Delivery Sequence     */
#define FTOFFSET	14		/* Negative offset into frametypes[] */
#else
#define FTOFFSET	13
#endif
#define FRTYPES 	20		/* No. frametypes excl. negatives    */

#define NOHDR		0		/* Header types HEX, BIN & BIN32     */
#define HEXHDR		1
#define BINHDR		2
#define BIN32HDR	3

/* zmodem - our own (The-Box Special Delivery <tm>) compression methods ---- */
#define ZC_NONE 	0		/* No Compression (used as fallback) */
#define ZC_RCP		1		/* Repeated Character Packing	     */
#define ZC_LZA12	2		/* Lempel-Ziv Shrink 12 compression  */
#define ZC_LZA13	3		/* Lempel-Ziv Shrink 13 compression  */


extern char txhdr[4], rxhdr[4]; 	/* Header buffer		     */
extern int	      rxflags;		/* Capability-flags		     */
extern int	      rxbuflen; 	/* Length of buffer		     */
extern long txpos,    rxpos;		/* Position in file		     */
extern int  crc32t,   crc32r;		/* True for CRC-32 session	     */
extern int	      rxcount;		/* Length of received data subpacket */
extern int  directzap;			/* Direct ZedZap (EMSI protocol opt) */
#if ZCOMP
extern int  zc_maxblklen;		/* Max length of SD data subpackets  */
#endif
extern char *frametypes[];		/* Used to inform the user...	     */
#if ZDEBUG
extern char *hdrtypes[];
extern char *endtypes[];
#endif
extern word crc16tab[]; 		/* CRC-16 table for fast updcrc16()  */
extern long crc32tab[]; 		/* CRC-32 table for fast updcrc32()  */

long z_hdrtolong();


/*--------------------------------------------------------------------------*/
/* ASCII MNEMONICS							    */
/*--------------------------------------------------------------------------*/
#define NUL 0x00
#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ENQ 0x05
#define ACK 0x06
#define BEL 0x07
#ifndef BS
#define BS  0x08
#endif
#define HT  0x09
#define LF  0x0a
#define VT  0x0b
#define FF  0x0c
#define CR  0x0d
#define SO  0x0e
#define SI  0x0f
#define DLE 0x10
#define DC1 0x11
#define DC2 0x12
#define DC3 0x13
#define DC4 0x14
#define NAK 0x15
#define SYN 0x16
#define ETB 0x17
/* #define CAN 0x18 -- Already defined above */
#define EM  0x19
#define SUB 0x1a
#define ESC 0x1b
#define FS  0x1c
#define GS  0x1d
#define RS  0x1e
#define US  0x1f


/* end of zmodem.h */
