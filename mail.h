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


/* FidoNet mail packet structures */

#if 0
  As can be seen in the chart below, EMSC3 and FSC45 differ only in the range
  0x08-0x0f.

  This design does use all space in the Type2 header, and hopefully
  will be a final final Type2.

     --- FTS1 --------- --- FSC39 ------- --- FSC45(2.2) -- --- EMSC0003 ----
  00	ONodeL	 ONodeH   ONodeL   ONodeH   ONodeL   ONodeH   ONodeL   ONodeH
  02	DNodeL	 DNodeH   DNodeL   DNodeH   DNodeL   DNodeH   DNodeL   DNodeH
  04	 YearL	  YearH    YearL    YearH  OPointL  OPointH  OPointL  OPointH
  06	MonthL	 MonthH   MonthL   MonthH  DPointL  DPointH  DPointL  DPointH
  08	  DayL	   DayH     DayL     DayH    Fill0    Fill0   DateLL   DateLH
  0A	 HourL	  HourH    HourL    HourH    Fill0    Fill0   DateHL   DateHH
  0C	  MinL	   MinH     MinL     MinH    Fill0    Fill0  Prod16L  Prod16H
  0E	  SecL	   SecH     SecL     SecH    Fill0    Fill0	  94  PVMinor
  10	 BaudL	  BaudH    BaudL    BaudH	02	 00	  02	   00
  12	    02	     00       02       00	02	 00	  02	   00
  14	 ONetL	  ONetH    ONetL    ONetH    ONetL    ONetH    ONetL	ONetH
  16	 DNetL	  DNetH    DNetL    DNetH    DNetL    DNetH    DNetL	DNetH
  18  ProdCode SerialNo ProdCodL  PVMajor ProdCode  PVMajor ProdCode  PVMajor
  1A	   PW1	    PW2      PW1      PW2      PW1	PW2	 PW1	  PW2
  1C	   PW3	    PW4      PW3      PW4      PW3	PW4	 PW3	  PW4
  1E	   PW5	    PW6      PW5      PW6      PW5	PW6	 PW5	  PW6
  20	   PW7	    PW8      PW7      PW8      PW7	PW8	 PW7	  PW8
  22   QOZoneL	QOZoneH  QOZoneL  QOZoneH  QOZoneL  QOZoneH  QOZoneL  QOZoneH
  24   QDZoneL	QDZoneH  QDZoneL  QDZoneH  QDZoneL  QDZoneH  QDZoneL  QDZoneH
  26	  Fill	   Fill     Fill     Fill    OrgD1    OrgD2    OrgD1	OrgD2
  28	  Fill	   Fill CWValidL CWValidH    OrgD3    OrgD4    OrgD3	OrgD4
  2A	  Fill	   Fill ProdCodH  PVMinor    OrgD5    OrgD6    OrgD5	OrgD6
  2C	  Fill	   Fill CapWordL CapWordH    OrgD7    OrgD8    OrgD7	OrgD8
  2E	  Fill	   Fill   OZoneL   OZoneH    DstD1    DstD2    DstD1	DstD2
  30	  Fill	   Fill   DZoneL   DZoneH    DstD3    DstD4    DstD3	DstD4
  32	  Fill	   Fill  OPointL  OPointH    DstD5    DstD6    DstD5	DstD6
  34	  Fill	   Fill  DPointL  DPointH    DstD7    DstD8    DstD7	DstD8
  36	  Fill	   Fill   PData1   PData2   PData1   PData2   PData1   PData2
  38	  Fill	   Fill   PData3   PData4   PData3   PData4   PData3   PData4
  3A	    00	     00       00       00	00	 00	  00	   00


  Suggested logic for packet type detection:

  WORD CWValid	= header[0x28] | header[0x29] << 8;
  WORD CW	= header[0x2C] | header[0x2D] << 8;
  WORD IS_45	= header[0x10] | header[0x11] << 8;
  WORD IS_EMSC3 = header[0x0E];
  WORD PROD16	= header[0x0C] | header[0x0D] << 8;
  WORD PROD	= header[0x18];

  if (CW == 0x0001 && CWValid == 0x0100) {
     /* FSC 39 */
  }
  else if (IS_45 == 0x0002) {
     /* Following the FTSC recommendation, 16bit products do fill 0xFE into
	the 8bit product code and reveal their true product code only in the
	16bit product code field. This seems like a good safeguard check
	additional to the 94 magic value
     */

     if (IS_EMSC3 == 94 &&
	 ((PROD16 <  0x0100 && PROD16 == PROD) ||
	  (PROD16 >= 0x0100 && PROD == 0xFE)) ) {
	/* EMSC 3 */
     }
     else {
	/* FSC 45 */
     }
  }
  else {
     /* FTS 1 */
  }
#endif


struct _pkthdr			/* FSC-0039.004 / FSC-0048.002 ------------- */
{  short ph_onode,		/* originating node			     */
	 ph_dnode;		/* destination node			     */
   short ph_yr, ph_mo, ph_dy,	/* date packet was assembled		     */
	 ph_hr, ph_mn, ph_sc;	/* time packet was assembled		     */
   word  ph_rate,		/* packet speed rate			     */
	 ph_ver;		/* packet version			     */
   short ph_onet,		/* originating net			     */
	 ph_dnet;		/* destination net			     */
   byte  ph_prodl,		/* product code (low order)		     */
	 ph_revh;		/* revision (high order)		     */
   char  ph_pwd[8];		/* password				     */
   short ph_ozone1,		/* originating zone (as in QMail/ZMailQ/SEA) */
	 ph_dzone1,		/* destination zone			     */
	 ph_auxnet;		/* auxiliary net			     */
   word  ph_cwdcopy;		/* capability word COPY (here highbyte 1st)  */
   byte  ph_prodh,		/* product code (high order)		     */
	 ph_revl;		/* revision (low order) 		     */
   word  ph_cwd;		/* capability word			     */
   short ph_ozone2,		/* originating zone (as in FD etc)	     */
	 ph_dzone2,		/* destination zone			     */
	 ph_opoint,		/* originating point			     */
	 ph_dpoint;		/* destination point			     */
   char  ph_spec[4];		/* product specific data		     */
};


struct _pkthdr94		/* FSC-0045.001 / EMSC-003 / Aka Pkt94 ----- */
{  short ph_onode,		/* originating node			     */
	 ph_dnode,		/* destination node			     */
	 ph_opoint,		/* originating point			     */
	 ph_dpoint;		/* destination point			     */
   long  ph_date;		/* Pkt94: UNIX stamp when pkt was assembled  */
   word  ph_prod16;		/* Pkt94: 16-bit product code		     */
   byte  ph_id94,		/* Pkt94: Set to 94 decimal (magic number)   */
	 ph_revl;		/* Pkt94: Revision (low order)		     */
   word  ph_subver,		/* Packet sub-version (2) (speed never 2)    */
	 ph_ver;		/* packet version (2)			     */
   short ph_onet,		/* originating net			     */
	 ph_dnet;		/* destination net			     */
   byte  ph_prodl,		/* product code (low order)		     */
	 ph_revh;		/* revision (high order)		     */
   char  ph_pwd[8];		/* password				     */
   short ph_ozone,		/* originating zone			     */
	 ph_dzone;		/* destination zone			     */
   char  ph_odomain[8], 	/* originating domain			     */
	 ph_ddomain[8]; 	/* destination domain			     */
   char  ph_spec[4];		/* product specific data		     */
};


struct _pktmsgs 		/* Packed type 2 message header 	     */
{  short pm_ver,		/* message version			     */
	 pm_onode,		/* originating node			     */
	 pm_dnode,		/* destination node			     */
	 pm_onet,		/* originating net			     */
	 pm_dnet,		/* destination net			     */
	 pm_attr,		/* message attributes			     */
	 pm_cost;		/* message cost, in cents		     */
};


/* end of mail.h */
