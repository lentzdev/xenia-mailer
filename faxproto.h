/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * From Binkley/EE 21/12/92 Extended Edition by Thomas Burchard; 18 sep 1993
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


/* Constants for Class 2 commands */

/* exit codes  */
#define FAXSENT   0
#define FAXINSYNC 0 
#define FAXNOSYNC 1
#define FAXNODIAL 2
#define FAXBUSY   3
#define FAXHANG   4
#define FAXERROR  5


/* My own page reception codes */
#define PAGE_GOOD	 0
#define ANOTHER_DOCUMENT 1
#define END_OF_DOCUMENT  2
#define PAGE_HANGUP	 4
#define PAGE_ERROR	 5


/* ------------------------------------------------------------------------- */
/* Class 2 session parameters				*/

/* Set desired transmission params with +FDT=DF,VR,WD,LN
   DF = Data Format :	0  1-d huffman
			*1 2-d modified Read
			*2 2-d uncompressed mode
			*3 2-d modified modified Read
  
   VR = Vertical Res :	0 Normal, 98 lpi
			1 Fine, 196 lpi

   WD = width : 	0  1728 pixels in 215 mm
			*1 2048 pixels in 255 mm
  
   LN = page length :	0 A4, 297 mm
			1 B4, 364 mm
			2  Unlimited 
  
   EC = error correction :	0 disable ECM
  
   BF = binary file transfer :	0 disable BFT
  
   ST = scan time/line :	VR = normal	VR = fine
			   0	0 ms		0 ms
  
*/

/* data format */
#define DF_1DHUFFMAN	0
#define DF_2DMREAD	1
#define DF_2DUNCOMP	2
#define DF_2DMMREAD	3

/* vertical resolution */
#define VR_NORMAL	0
#define VR_FINE 	1

/* width */
#define WD_1728 	0
#define WD_2048 	1

/* page length */
#define LN_A4		0
#define LN_B4		1
#define LN_UNLIMITED	2

/* Speed rate */
#define BR_2400    0
#define BR_4800    1
#define BR_7200    2
#define BR_9600    3


/* ------------------------------------------------------------------------- */
/* A T.30 DIS frame, as sent by the remote fax machine */
struct T30Params {
  int vr; /* VR = Vertical Res :	0 Normal, 98 lpi
					1 Fine, 196 lpi
	  */
  int br; /* BR = Bit Rate :	br * 2400BPS
	  */
  int wd; /* Page Width : 
	     0 1728 pixels in 215 mm
	     1 2048 pixels in 255 mm
	     2 2432 pixels in 303 mm 
	  */
  int ln; /* Page Length :
	     0 A4, 297 mm
	     1 B4, 364 mm
	     2 unlimited
	  */
  int df; /* Data compression format :
	     0 1-D modified Huffman
	     1 2-D modified Read
	     2 2-D unompressed mode (?)
	  */
  int ec; /* Error Correction :
	     0 disable ECM
	     1 enable ECM 64 bytes/frame
	     2 enable CM 256B/frame
	  */
 int bf; /* Binary File Transfer 
	    0 disable BFT
	    1 enable BFT
	  */
 int st; /* Scan Time (ms) :
	      VR Normal   Fine
	    0	 0	  0 ms
	    1	 5	  5
	    2	 10	  5
	    3	 10	  10
	    4	 20	  10
	    5	 20	  20 
	    6	 40	  20
	    7	 40	  40
	  */
};


/* ------------------------------------------------------------------------- */
struct faxmodem_response {
  char	  remote_id[50];		/* +FCSI remote id		     */
  int	  hangup_code;			/* +FHNG code			     */
  int	  post_page_response_code;	/* +FPTS code			     */
  int	  post_page_message_code;	/* +FET code			     */
  boolean fcon; 			/* true if +FCON seen		     */
  boolean connect;			/* true if CONNECT msg seen	     */
  boolean ok;				/* true if OK seen		     */
  boolean error;			/* true if ERROR or NO CARRIER seen  */
  struct T30Params T30; 		/* Session params: parsed from +FDCS */
};


/* ----------------------------------------------------------------------------
 ZyXEL ZFAX format stuff

 E.1 .FAX file format
 -------------------------
	For the .FAX format, the *.fax file contains a file header and
 a CCITT T.4 compressed fax data. The header is a 16-byte data, they are
	Bytes 0 - 4 : title  -> "ZyXEL"
	Byte  5     : 0
	Byte  6 - 7 : version number
	Byte  8 - 9 : reserved
	Bytes 10-11 : Page scan width, A4=1728, B4=2048, A3=2432.
	Bytes 12-13 : page count, pages number in this file, the page is
			seperated by the RTC code.
	Bytes 14-15 : Resolution and coding scheme,
			0 - 1-D Normal
			1 - 1-D High
			2 - 2-D Normal
			3 - 2-D High

	The T.4 fax data is just the data received from U1496 Modem/Fax.
 You can get the compress/decompress scheme from the CCITT blue book, or
 you can obtain some information from the TIFF standard documents.

			Table E.1  FAX File Format
    ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    ³ Record ID     ³ Field ID	 ³ name   ³ length ³ pos  ³ remark	     ³
    ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
    ³ header	    ³ title	 ³ TITLE  ³ 5	   ³  0   ³ "ZyXEL"	     ³
    ³		    ³		 ³	  ³ 1	   ³  5   ³ 0		     ³
    ³		    ³ Version	 ³	  ³ 2	   ³  6   ³ 0300H	     ³
    ³		    ³ reserved	 ³	  ³ 2	   ³  8   ³ 0		     ³
    ³		    ³ page width ³ PGWIDTH³ 2	   ³ 10   ³		     ³
    ³		    ³ page count ³ PAGECNT³ 2	   ³ 12   ³		     ³
    ³		    ³ coding	 ³ CODING ³ 2	   ³ 14   ³ 0 : 1-DN	     ³
    ³		    ³		 ³	  ³	   ³	  ³ 1 : 1-DH	     ³
    ³		    ³		 ³	  ³	   ³	  ³ 2 : 2-DN	     ³
    ³		    ³		 ³	  ³	   ³	  ³ 3 : 2-DH	     ³
    ³		    ³		 ³	  ³ÄÄÄÄÄ   ³	  ³		     ³
    ³		    ³		 ³ FH	  ³ 16	   ³	  ³		     ³
    ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
    ³ Fax data	    ³	The data format of the following data is for	     ³
    ³		    ³	CCITT T.4 compression scheme.			     ³
    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
*/

typedef struct _zfaxhdr ZFAXHDR;
struct _zfaxhdr {			/* --------------------------------- */
	char title[6];			/* Title	   "ZyXEL"NUL	     */
	word version;			/* Version	   0x0300	     */
	word reserved;			/* Reserved	   0		     */
	word scanwidth; 		/* Page ScanWidth  A4=1728	(wd) */
	word pagecount; 		/* Page Count	   1-n		     */
	word resolution;		/* Res & coding    0-3 (see above)   */
};


/* ----------------------------------------------------------------------------

			  Smith Micro Software, Inc.
		       .QFX File Format Documentation


The QFX file header is exactly 1536 bytes long.  All integers and long integers
in the header are in Intel byte order.	This means that the low order bytes
are stored first.  Example the number 1 stored as an integer in Intel format
in hex would be 0x0100.

Bytes		Description
-----		-----------
[0-7]		QFX Identifier = "QLIIFAX*" where * is a null character.
[8-9]		Number of pages in the QFX file in integer format.
[10-11] 	Number of scan lines in last page of QFX in integer format.
[12-15] 	Number of total scan lines for all pages in long integer
		format.  This number will be equal to the number above if the
		fax is one page.
[16-17] 	Horizontal scaling in integer format. 1 = High res (200x200),
		2 = Normal res (200x100)
[18-19] 	Vertical scaling in integer format.  always = 1.
[20-31] 	Reserved for future expansion, should be all zeros.
[32-1535]	Pointers to pages in long integer format.
		Page 1 always starts at position 1536, therefore bytes
		[32-35] always = 1536.	If the QFX file is one page, bytes
		[36-39] will contain a pointer to the last byte of page
		data.  If the QFX file is two pages, bytes [36-39] will
		contain the pointer to the second page of data, which will
		follow the first page.	[40-43] will then contain the pointer
		to the end of the second page of data.	After all the page
		pointers, the file should be filled with zeros up to 1535.
[1536-EOF]	T4 page data.  This T4 is byte aligned, and bit reversed.
		Each page of T4 is terminated with 6 EOL's.  See CCITT
		Recommendation T.4 for full documentation on this coding
		scheme.

The .QFX file format is proprietary to Smith Micro Software, Inc. and this
documentation is provided to assist users of Quick Link II Fax (tm) products.
Commercial use of this file format without express written permission from
Smith Micro Software, Inc. is prohibited.
*/

typedef struct _qfxhdr QFXHDR;
struct _qfxhdr {
	char  title[8]; 	//  0-7
	word  pages;		//  8-9
	word  scan_last;	// 10-11
	dword scan_total;	// 12-15
	word  scale_hor;	// 16-17
	word  scale_ver;	// 18-19
	byte  fill[12]; 	// 20-31
	dword page_ofs[376];	// 32-1535
} qfxhdr;


/* end of faxproto.h */
