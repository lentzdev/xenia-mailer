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


/*---------------------------------------------------------------------------*/
/* From Binkley/EE 10/07/93 Extended Edition by Thomas Burchard; 18 sep 1993 */
/*---------------------------------------------------------------------------*/
/*			   FAX file reception module			     */
/*									     */
/* This module recieves a fax from a Class 2 Protocol faxmodem, putting it   */
/* in the inbound directory as a set of files with name PAGExxxx.FAX ...     */
/*									     */
/*		 This module was written by Michael Buenter		     */
/*	  Original UNIX sources Henry Minsky 11/02/90	 hqm@ai.mit.edu      */
/*---------------------------------------------------------------------------*/

/* Include this file before any other includes or defines! */

#include "zmodem.h"
#include "mail.h"
#include "faxproto.h"

extern char *mon[];
extern char *timestr_tpl;


/*---------------------------------------------------------------------------*/
/* Local routines							     */
/*---------------------------------------------------------------------------*/
static void fax_notify		  (char *fname, int numpages, int complete);
static int  get_fax_file	  (FILE *fp, int page);
static void add_EOL_padding	  (FILE *fp);
static int  read_g3_stream	  (FILE *fp, int page);
static void get_faxline 	  (char *p);
static void init_swaptable	  (void);
static void init_modem_response   (void);
static void get_modem_result_code (void);
static void fax_status		  (char *str);
static void parse_text_response   (char *str);
static int  faxmodem_receive_page (int page);


/*---------------------------------------------------------------------------*/
/* Private data 							     */
/*---------------------------------------------------------------------------*/
static int     gEnd_of_document;
static byte    swaptable[256];
static boolean swaptableinit = false;
static struct faxmodem_response response;
static ZFAXHDR zfaxhdr;
static QFXHDR  qfxhdr;
static dword   faxsize = 0L;


/*---------------------------------------------------------------------------*/
/* FAX RECEIVE Routines 						     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* receive fax files into basefilename					     */
boolean fax_receive (void)
{
	char   buf[256];
	dword  result;
	int    count, page;
	long   totalsize;
	char  *p;
	FILE  *fp;

	count = 1;
	do {
	   switch (fax_format) {
		  case FAXFMT_QFX:  p = "QFX"; break;
		  case FAXFMT_ZFAX: p = "FAX"; break;
		  case FAXFMT_RAW:  p = "001"; break;
	   }
	   sprint(buf, "%sXENIA%03d.%s", fax_indir, count++, p);
	} while (fexist(buf) && (count < 1000));
	if (count == 1000) {
	   message(6,"!FAX Couldn't create output file");
	   return (false);
	}
	if ((fp = fopen(buf,"wb")) == NULL) {
	   message(6,"!FAX Couldn't create output file %s", buf);
	   return (false);
	}

	switch (fax_format) {
	       case FAXFMT_QFX:  memset(&qfxhdr,0,sizeof (QFXHDR));
				 fwrite(&qfxhdr,sizeof(QFXHDR),1,fp);
				 break;
	       case FAXFMT_ZFAX: memset(&zfaxhdr,0,sizeof (ZFAXHDR));
				 fwrite(&zfaxhdr,sizeof(ZFAXHDR),1,fp);
				 break;
	       case FAXFMT_RAW:  break;
	}

	if (!swaptableinit) init_swaptable();

	init_modem_response();
	gEnd_of_document = false;
	response.fcon	 = true;		/* we already connected */

	page = 0;
	result = 0;
	totalsize = 0L;

	while (gEnd_of_document == false) {
	      if (fax_format == FAXFMT_QFX)
		 qfxhdr.page_ofs[page] = ftell(fp);
	      result = get_fax_file(fp,page);
	      if (loglevel==0)
		 message(0,">FAX get_fax_file returns = %lu", result);

	      faxsize = ftell(fp);
	      if (fax_format == FAXFMT_RAW) {
		 if (result == PAGE_GOOD || faxsize > 2048)
		    message(2,"+FAX File received %s (%ld)",buf,faxsize);
		 totalsize += faxsize;
		 fclose(fp);
	      }

	      switch (result) {
		     case PAGE_GOOD:
			  page++;
			  if (fax_format == FAXFMT_RAW) {
			     sprint(buf, "%sXENIA%03d.%03d", fax_indir, count, page);
			     if ((fp = fopen(buf,"wb")) == NULL) {
				message(6,"!FAX Couldn't create output file %s", buf);
				gEnd_of_document = true;
				break;
			     }
			  }
			  break;

		     case PAGE_HANGUP:
			  gEnd_of_document = true;
			  if (fax_format == FAXFMT_RAW)
			     unlink(buf);
			  break;

		     default:
			  message(3,"-FAX Error during transmission");
			  gEnd_of_document = true;
			  break;
	      }
	}

	if (page > 0 || faxsize > 2048) {
	   message(3,"+FAX received %d page%s%s",
		     page, page == 1 ? "" : "s",
		     result == PAGE_HANGUP ? "" : " (partial)");

	   if (fax_format != FAXFMT_RAW)
	      message(2,"+FAX File received %s (%ld)",buf,faxsize);

	   if (fax_format == FAXFMT_QFX) {
	      fseek(fp,0L,SEEK_SET);
	      strncpy(qfxhdr.title,"QLIIFAX ",8);
	      qfxhdr.pages = (word) page;
	      // qfxhdr.scan_last = qfxhdr.scan_total = 0;
	      qfxhdr.scale_hor = (word) (response.T30.vr ? 1 : 2);
	      qfxhdr.scale_ver = 1;
	      qfxhdr.page_ofs[page] = faxsize - 1;
	      fwrite(&qfxhdr,sizeof(QFXHDR),1,fp);
	      fclose(fp);
	   }
	   else if (fax_format == FAXFMT_ZFAX) {
	      fseek(fp,0L,SEEK_SET);
	      strcpy(zfaxhdr.title,"ZyXEL");
	      zfaxhdr.version	 = 0x0300;
	      zfaxhdr.scanwidth  = (word) response.T30.wd;
	      zfaxhdr.pagecount  = (word) page;
	      zfaxhdr.resolution = (word) response.T30.vr;
	      if (response.T30.df)
		 zfaxhdr.resolution += (word) 2;
	      fwrite(&zfaxhdr,sizeof(ZFAXHDR),1,fp);
	      fclose(fp);
	   }
	   else
	      faxsize = totalsize;

	   fax_notify(buf,page,result == PAGE_HANGUP);
	}
	else {
	   message(3,"-FAX Received less than one page");
	   fclose(fp);
	   unlink(buf);
	}

	return ((page > 0 || faxsize > 2048) ? true : false);
}/*fax_receive()*/


/*---------------------------------------------------------------------------*/
void fax_notify (char *fname, int numpages, int complete)
{
	char  buffer[128], work[128];
	long  tim;
	FILE *fp;
	struct _pkthdr	pkthdr;
	struct _pktmsgs pktmsg;
	struct dosdate_t d;
	struct dostime_t t;
	static int normscan[] = { 0, 5, 10, 10, 20, 20, 40, 40 };
	static int finescan[] = { 0, 5,  5, 10, 10, 20, 20, 40 };

	sprint(buffer,"%sXENPKT%02x.000",localpath,task);
	unique_name(buffer);
	if ((fp = fopen(buffer,"wb")) == NULL) {
	   message(6,"!FAX Couldn't create notify packet");
	   return;
	}

	memset(&pkthdr,0,sizeof (struct _pkthdr));
	memset(&pktmsg,0,sizeof (struct _pktmsgs));
	_dos_getdate(&d);
	_dos_gettime(&t);
	tim = time(NULL);

	pkthdr.ph_yr	  = (short) inteli(d.year);
	pkthdr.ph_mo	  = (short) (inteli(d.month) - 1);
	pkthdr.ph_dy	  = inteli(d.day);
	pkthdr.ph_hr	  = inteli(t.hour);
	pkthdr.ph_mn	  = inteli(t.minute);
	pkthdr.ph_sc	  = inteli(t.second);
	pkthdr.ph_rate	  = (short) inteli(cur_speed < 65535UL ? (word) cur_speed : 9600);
	pkthdr.ph_ver	  = inteli(2);
	pkthdr.ph_prodh   = 0;
	pkthdr.ph_prodl   = isXENIA;
	pkthdr.ph_revh	  = XEN_MAJ;
	pkthdr.ph_revl	  = XEN_MIN;
	pkthdr.ph_cwd	  = inteli(1);	/* CapWord: can handle type 2+ */
	pkthdr.ph_cwdcopy = inteli(256);/* CapWordCopy = byteswapped   */
	/*strncpy(pkthdr.ph_pwd,ourbbs.password,6);*/
	pkthdr.ph_ozone1  = pkthdr.ph_ozone2 = (short) inteli(aka[0].zone);
	pkthdr.ph_dzone1  = pkthdr.ph_dzone2 = (short) inteli(aka[0].zone);
	pkthdr.ph_onet	  = (short) inteli(aka[0].net);
	pkthdr.ph_auxnet  = inteli(0);
#if 0	/* Enable this for FSC-0048 instead of FSC-0039 */
	if (aka[0].point) {
	   pkthdr.ph_onet   = inteli(-1);
	   pkthdr.ph_auxnet = inteli(aka[0].net);
	}
#endif
	pkthdr.ph_dnet	  = (short) inteli(aka[0].net);
	pkthdr.ph_onode   = (short) inteli(aka[0].node);
	pkthdr.ph_dnode   = (short) inteli(aka[0].node);
	pkthdr.ph_opoint  = (short) inteli(aka[0].point);
	pkthdr.ph_dpoint  = (short) inteli(aka[0].point);
	/* *(long *) pkthdr.ph_spec = intell(xenkey->serial); */

	pktmsg.pm_ver	 = inteli(2);
	pktmsg.pm_attr	 = inteli(0x0111);     /* flags Local/Attach/Private */
	pktmsg.pm_cost	 = 0;
	pktmsg.pm_onet	 = (short) inteli(aka[0].net);
	pktmsg.pm_dnet	 = (short) inteli(aka[0].net);
	pktmsg.pm_onode  = (short) inteli(aka[0].node);
	pktmsg.pm_dnode  = (short) inteli(aka[0].node);

	if (!fwrite(&pkthdr,sizeof (struct _pkthdr),1,fp) ||
	    !fwrite(&pktmsg,sizeof (struct _pktmsgs),1,fp)) {
	   fclose(fp);
	   unlink(buffer);
	   return;
	}

	sprint(work,timestr_tpl,
	       d.day, mon[d.month - 1], d.year % 100,
	       t.hour, t.minute, t.second);
	fwrite(work,strlen(work)+1,1,fp);
	fwrite(ourbbs.sysop,strlen(ourbbs.sysop)+1,1,fp);
	strcpy(work,"Xenia FAX Server");
	fwrite(work,strlen(work)+1,1,fp);
	fwrite(fname,strlen(fname)+1,1,fp);

	if (aka[0].point) {
	   fprint(fp,"\001FMPT %d\r\n",aka[0].point);
	   fprint(fp,"\001TOPT %d\r\n",aka[0].point);
	}

	fprint(fp,"\001MSGID: %d:%d/%d%s%s %08lx\r\n",
		    aka[0].zone,aka[0].net,aka[0].node,
		    pointnr(aka[0].point),zonedomabbrev(aka[0].zone),
		    tim);

	fprint(fp,"\001FLAGS FAX\r\n");
#if 0
	if (xenpoint_ver)
	   fprint(fp,"\001PID: XenPoint %s %s OpenSource\r\n",xenpoint_ver,xenpoint_reg);
	else
#endif
	{
	   fprint(fp,"\001PID: Xenia%s %s OpenSource\r\n", XENIA_SFX, VERSION);
	}

	fprint(fp,"Received FAX notification\r\n");
	fprint(fp,"\r\n");

	if (!complete) {
	   fprint(fp,"  *** NOTE: Session failed - not all pages may have been received\r\n");
	   fprint(fp,"\r\n");
	}

	fprint(fp,"Filename   : %s\r\n",
		  fname);
	fprint(fp,"Pages      : %d (%lu bytes)\r\n",
		  numpages, faxsize);
	fprint(fp,"Remote ID  : %s\r\n",
		  response.remote_id[0] ? response.remote_id :
					  (callerid[0] ? callerid : "Unknown"));
	fprint(fp,"\r\n");

	fprint(fp,"Bit rate   : %lu bps\r\n",
		  ((dword) response.T30.br + 1) * 2400UL);
	fprint(fp,"Correction : ECM %s\r\n",
		  !response.T30.ec ? "disabled" :
		  (response.T30.ec == 2 ? "256 bytes/frame" : "64 bytes/frame"));
	fprint(fp,"Binary file: BFT %sabled\r\n",
		  response.T30.bf ? "en" : "dis");
	fprint(fp,"\r\n");

	fprint(fp,"Resolution : %s\r\n",
		  response.T30.vr ? "Fine (196 lpi)" : "Normal (98 lpi)");
	fprint(fp,"Scan time  : %d ms\r\n",
		  response.T30.vr ? finescan[response.T30.st] : normscan[response.T30.st]);
	fprint(fp,"Compression: %s\r\n",
		  response.T30.df ? "2D modified Read" : "1D modified Huffman");
	fprint(fp,"\r\n");

	fprint(fp,"Page width : %d mm\r\n",
		  response.T30.ln == 2 ? 303 :
		  (response.T30.ln ? 255 : 215));
	fprint(fp,"Page length: %s\r\n",
		  response.T30.ln == 2 ? "Unlimited" :
		  (response.T30.ln ? "B4" : "A4"));

	fwrite("\r\n\r\n\0\0\0",7,1,fp);
	fclose(fp);

	while (sprint(work,"%s%08lx.PKT",localpath,tim), fexist(work)) tim++;
	rename(buffer,work);

	got_fax = got_mail = 1;
}/*fax_notify()*/


/*---------------------------------------------------------------------------*/
/* This executes the +FDR receive page command, and looks for		     */
/* the proper CONNECT response, or, if the document is finished,	     */
/* looks for the FHNG/FHS code. 					     */
/*									     */
/* returns:								     */
/*  PAGE_GOOD		     no error conditions occured during reception    */
/*  PAGE_HANGUP 	     normal end of transmission 		     */
/*  PAGE_ERROR		     something's wrong                               */
/*									     */
static int get_fax_file (FILE *fp, int page)
{
	int result;

	if (loglevel==0)
	   message(0,">FAX [get_fax_file]");

	if (!(result = faxmodem_receive_page(page))) {
	   if (!page)
	      message(3,"=FAX Connect with: %s",
			response.remote_id[0] ? response.remote_id : "Unknown");

	   message(0," FAX receive page %d", page + 1);

	   message(page ? 0 : 2," vr%d br%d wd%d ln%d df%d ec%d bf%d st%d",
		     response.T30.vr, response.T30.br, response.T30.wd,
		     response.T30.ln, response.T30.df, response.T30.ec,
		     response.T30.bf, response.T30.st);

	   result = read_g3_stream(fp, page);
	}

	/* some software likes 6 EOLs at the end of a page */
	if (fax_options & FAXOPT_EOLPAD)
	   add_EOL_padding(fp);

	return (result);
}/*get_fax_file()*/


/*---------------------------------------------------------------------------*/
static void add_EOL_padding (FILE *fp)
{
	register int i;

	/* 00000000 00010000 00000001 | 00 10 01 - 2 EOL bitsequences */
	/* 00000000 00001000 10000000 | 00 08 80 - Reverse bit order  */

	for (i = 0; i < 3; i++) {
	    fputc(0x00, fp);
	    fputc(0x08, fp);
	    fputc(0x80, fp);
	}
}/*add_EOL_padding()*/


/*---------------------------------------------------------------------------*/
/* Reads a data stream from the faxmodem, unstuffing DLE characters.	     */
/* Returns the +FET value (2 if no more pages) or 4 if hangup.		     */
/*									     */
static int read_g3_stream (FILE *fp, int page)
{
	short  c;
	char   e_input[11];
	byte  *secbuf, *p;

	alloc_msg("pre fax recv");

	if (loglevel==0)
	   message(0,">FAX [read_g3_stream]");

	response.post_page_response_code = -1;	/* reset page codes	     */
	response.post_page_message_code  = -1;

	com_purge();				/* flush any echoes	     */
						/* or return codes	     */
	if ((secbuf = (byte *) calloc(1,1024)) == NULL)
	   goto fax_error;

	p = secbuf;

	if (loglevel==0)
	   message(0,">FAX DC2  [read_g3_stream]");
	print("\n");
	fax_status(ultoa(faxsize,e_input,10));

	com_putbyte((byte) DC2);		/* Send DC2 to start phase C */
						/* data stream		     */

	while (!(fax_options & FAXOPT_DCD) || carrier()) {
	      if (!com_scan()) {		/* if nothing ready yet,     */
		 win_idle();
		 continue;			/* just process timeouts     */
	      }

	      c = (short) com_getbyte();	/* get a character	     */

	      if (c == DLE) {			/* DLE handling 	     */
		 if ((c = (short) com_getc(200)) != EOF) {
		    switch (c) {
		    //	   case SUB:   /* DLE DLE */
		    //		*p++ = swaptable[(byte) DLE];
		    //		*p++ = swaptable[(byte) DLE];
		    //		faxsize += 2;
		    //		break;

		    //	   case SOH:   /* SOH */
		    //	   case ETB:   /* ETB */
			   case DLE:   /* DLE */
				*p++ = swaptable[(byte) c];
				faxsize++;
				break;

			   case ETX:   /* end of stream */
				goto end_page;
		    }
		 }
		 else {
		    /*faxsize = 0L;*/
		    goto fax_error;			/* give up	     */
		 }
	      }
	      else /*if (faxsize || (c & 0x0f) == NUL)*/ {  /* F0 bitswapped!	 */
		 *p++ = swaptable[(byte) c];  /* Ignore page leading garbage */
		 faxsize++;
	      }

	      if (faxsize && !(faxsize % 1024)) {
		 fax_status(ultoa(faxsize,e_input,10));
		 if (fwrite(secbuf,1,1024,fp) != 1024)
		    goto fax_error;			/* hoppala	     */
		 p = secbuf;
		 win_idle();
	      }
	}

end_page:
	if (faxsize % 1024) {
	   if (fwrite(secbuf, 1, (size_t)(faxsize % 1024), fp) != (size_t)(faxsize % 1024))
	      goto fax_error;				/* hoppala	     */
	   fax_status(ultoa(faxsize,e_input,10));
	}

	free(secbuf);

	alloc_msg("post fax recv");

	if (loglevel==0)
	   message(0,">FAX Waiting for +FET/+FHNG  [read_g3_stream]");
	c = 0;
	while (response.post_page_message_code == -1) { /* wait for +FET     */
	      get_modem_result_code();
	      c++;
	      if (!response.post_page_response_code || c > 5 || response.error)
		 return (PAGE_ERROR);
	      if (response.hangup_code != -1)
		 return (PAGE_HANGUP);
	}

	return (PAGE_GOOD);

fax_error:
	if (secbuf) free(secbuf);
	message(6,"!FAX Error receiving page");
	get_modem_result_code();

	alloc_msg("post fax recv");

	return (PAGE_ERROR);
}/*read_g3_stream()*/


/*---------------------------------------------------------------------------*/
/* Class 2 Faxmodem Protocol Functions					     */
/*									     */
/* Taken from EIA Standards Proposal No. 2388: Proposed New Standard	     */
/* "Asynchronous Facsimile DCE Control Standard"			     */
/* (if approved, to be published as EIA/TIA-592)			     */
/*---------------------------------------------------------------------------*/
/* reads a line of characters, terminated by a newline			     */
static void get_faxline (char *p)
{
	char *resp;

	resp = mdm_gets(40,false);
	strcpy(p,resp ? resp : "");
	if (loglevel==0 && *p && strnicmp(p,"AT",2))
	   message(0,"<FAX %s", p);		/* pop it out on the screen  */
}/*get_faxline()*/


/*---------------------------------------------------------------------------*/
static void init_swaptable (void)
{
	int i, j;

	for (i = 0; i < 256; i++) {
	    if (fax_options & FAXOPT_NOSWAP)
	       j = i;
	    else   /* swap the low order 4 bitswith the high order */
	       j = (((i & 0x01) << 7) |
		    ((i & 0x02) << 5) |
		    ((i & 0x04) << 3) |
		    ((i & 0x08) << 1) |
		    ((i & 0x10) >> 1) |
		    ((i & 0x20) >> 3) |
		    ((i & 0x40) >> 5) |
		    ((i & 0x80) >> 7));
	    swaptable[i] = (byte) j;
	}

	swaptableinit = true;
}/*init_swaptable()*/


/*---------------------------------------------------------------------------*/
/* Initialize a faxmodem_response struct				     */
static void init_modem_response (void)
{
	response.remote_id[0]		  = '\0';
	response.fcon			  = false;
	response.connect		  = false;
	response.ok			  = false;
	response.error			  = false;
	response.hangup_code		  = -1;
	response.post_page_response_code  = -1;
	response.post_page_message_code   = -1;
	response.T30.ec 		  = 0;
	response.T30.bf 		  = 0;
}/*init_modem_response()*/


/*---------------------------------------------------------------------------*/
/* This function parses numeric responses from the faxmodem.		     */
/* It fills in any relevant slots of the faxmodem_response structure.	     */
static void get_modem_result_code (void)
{
	char buf[256];
	long t;

	if (loglevel==0)
	   message(0,">FAX [get_modem_result_code]");

	t = timerset(40);

	while (!timeup(t)) {
	      buf[0] = '\0';
	      get_faxline(buf);

	      if (buf[0]) {
		 parse_text_response(buf);
		 return;
	      }
	}
}/*get_modem_result_code()*/


/*---------------------------------------------------------------------------*/
static void fax_status (char *str)
{
	print("%s\r",str);
}/*fax_status()*/


/*---------------------------------------------------------------------------*/
static void parse_text_response (char *str)
{
	char *p;

	if (!strncmp("+FCO",str,4))
	   response.fcon = true;

	else if (!strncmp(str,"OK",2))
	   response.ok = true;

	else if (!strncmp(str,"CONNECT",7))
	   response.connect = true;

	else if (!strncmp(str,"NO CARRIER",10) ||
		 !strncmp(str,"ERROR",5)) {
	   response.error = true;
	   response.hangup_code = 0;
	}

	else if (!strncmp(str,"+FDCS",5) ||		/* class 2   */
		 !strncmp(str,"+FDIS",5))		/* delete    */
	   sscanf(str+6,"%d,%d,%d,%d,%d,%d,%d,%d",
		&response.T30.vr,&response.T30.br,&response.T30.wd,
		&response.T30.ln,&response.T30.df,&response.T30.ec,
		&response.T30.bf,&response.T30.st);

	else if (!strncmp(str,"+FTSI",5)) {		/* class 2   */
	   str += 6;
	   if (!caller) goto id;
	}
	else if (!strncmp(str,"+FCSI",5)) {		/* class 2 ? */
	   str += 6;
	   if (caller) goto id;
	}

	else if (!strncmp(str,"+FCS",4))  /* (was FDCS)    class 2.0 */
	   sscanf(str+5,"%d,%d,%d,%d,%d,%d,%d,%d",
		&response.T30.vr,&response.T30.br,&response.T30.wd,
		&response.T30.ln,&response.T30.df,&response.T30.ec,
		&response.T30.bf,&response.T30.st);

	else if (!strncmp(str,"+FHNG",5))
	   sscanf(str+6,"%d",&response.hangup_code);

	else if (!strncmp(str,"+FHS",4))
	   sscanf(str+5,"%d",&response.hangup_code);

	else if (!strncmp(str,"+FPTS",5))		/* class 2   */
	   sscanf(str+6,"%d",&response.post_page_response_code);

	else if (!strncmp(str,"+FPS",4))		/* class 2.0 */
	   sscanf(str+5,"%d",&response.post_page_response_code);

	else if (!strncmp(str,"+FET",4))
	   sscanf(str+5,"%d",&response.post_page_message_code);

	else if (!strncmp(str,"+FTI",4)) {		/* class 2.0	 */
	   str += 5;
	   if (!caller) goto id;
	}

	else if (!strncmp(str,"+FCI",4)) {		/* CSI->CI ?	 */
	   str += 5;
	   if (caller) goto id;
	}

	else if (!strncmp(str,"+FPI",4)) {		/* polling st.ID */
	   str += 5;					/* agl: caller, not? */
	   if (!*response.remote_id) goto id;
	}

	return;

id:	while (*str && (isspace(*str) || *str == '\"')) str++;
	if (*str) strcpy(response.remote_id,str);
	p = strchr(response.remote_id,'\"');
	if (p) *p = '\0';
}/*parse_text_response()*/


/*---------------------------------------------------------------------------*/
/* Action Commands							     */
/* Receive a page							     */
/* after receiving OK,							     */
/* send +FDR								     */
/* This is somewhat ugly, because the FDR command can return		     */
/* a couple of possible results;					     */
/* If the connection is valid, it returns something like		     */
/*  +FCFR								     */
/*  +FDCS: <params>							     */
/*  CONNECT								     */
/*									     */
/* If, on the other hand, the other machine has hung up, it returns	     */
/* +FHNG: <code>  or							     */
/* +FHS: <code> 							     */
/*									     */
/* and if the connection was never made at all, it returns ERROR	     */
/* (actually numeric code 4)						     */
/*									     */
/* faxmodem_receive_page returns values:				     */
/* PAGE_GOOD	 page reception OK, data coming 			     */
/* PAGE_HANGUP	 normal hangup						     */
/* PAGE_ERROR	 page error						     */
static int faxmodem_receive_page (int page)
{
	long t;
	char buf[100];

	faxsize = 0L;
	response.connect = response.ok = false;

	/* We wait until a string "OK" is seen
	   or a "+FHNG"
	   or a "ERROR" or "NO CARRIER"
	   or until 40 seconds for a response.
	*/

	t = timerset(400);

	if (loglevel==0)
	   message(0,">FAX Waiting for OK  [faxmodem_receive_page]");

	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page) &&
	       !response.ok) {
	      get_faxline(buf);
	      parse_text_response(buf);

	      if (response.hangup_code != -1)
		 return (PAGE_HANGUP);

	      if (response.error)
		 return (PAGE_ERROR);
	}

	if (!response.ok)
	   return (PAGE_ERROR);


	mdm_puts("``AT+FDR|");
	if (loglevel==0)
	   message(0,">FAX AT+FDR  [faxmodem_receive_page]");

	/* We wait until either a string "CONNECT" is seen
	   or a "+FHNG"
	   or until 40 seconds for a response.
	*/

	t = timerset(400);

	if (loglevel==0)
	   message(0,">FAX Waiting for CONNECT  [faxmodem_receive_page]");

	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page)) {
	      get_faxline(buf);
	      parse_text_response(buf);

	      if (response.connect == true)
		 return (PAGE_GOOD);

	      if (response.hangup_code != -1)
		 return (PAGE_HANGUP);

	      if (response.error)
		 return (PAGE_ERROR);
	}

	return (PAGE_ERROR);
}/*faxmodem_receive_page()*/


/*---------------------------------------------------------------------------*/
#if 0

boolean fax_transmit (void)
{
	char   buf[256];
	char   e_input[11];
	long   t;
	int    page;
	int    c;
	FILE  *fp;

	sprint(buf,"%sXENIA001.FAX",fax_indir);
	if ((fp = fopen(buf,"rb")) == NULL) {
	   message(6,"!FAX Couldn't open input file %s", buf);
	   return (false);
	}

	fread(&zfaxhdr,sizeof(ZFAXHDR),1,fp);

	if (!swaptableinit) init_swaptable();

	init_modem_response();
	gEnd_of_document = false;
	response.fcon	 = true;		/* we already connected */

	response.connect = response.ok = false;

	page = 0;

	/* We wait until a string "OK" is seen
	   or a "+FHNG"
	   or a "ERROR" or "NO CARRIER"
	   or until 40 seconds for a response.
	*/

	t = timerset(400);

	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page) &&
	       !response.ok) {
	      get_faxline(buf);
	      parse_text_response(buf);

	      if (response.hangup_code != -1)
		 goto err;

	      if (response.error)
		 goto err;
	}

	if (!response.ok)
	   goto err;

	mdm_puts("``AT+FDT|");

	/* We wait until either a string "CONNECT" is seen
	   or a "+FHNG"
	   or until 40 seconds for a response.
	*/

	t = timerset(400);

	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page)) {
	      get_faxline(buf);
	      parse_text_response(buf);

	      if (response.connect == true)
		 break;

	      if (response.hangup_code != -1)
		 goto err;

	      if (response.error)
		 goto err;
	}

	if (!response.connect)
	   goto err;

	if (!page)
	   message(3,"=FAX Connect with: %s",
		     response.remote_id[0] ? response.remote_id : "Unknown");

	message(0," FAX transmit page %d", page + 1);

	message(page ? 0 : 2," vr%d br%d wd%d ln%d df%d ec%d bf%d st%d",
		  response.T30.vr, response.T30.br, response.T30.wd,
		  response.T30.ln, response.T30.df, response.T30.ec,
		  response.T30.bf, response.T30.st);

	c = 0;
	t = timerset(40);
	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page))
	      if ((c = com_getbyte()) == XON) break;

	if (c != XON) {
	   message(6,"!No XON received");
	   goto err;
	}

	print("\n");
	faxsize = 0L;
	print("\n");
	fax_status(ultoa(faxsize,e_input,10));

	while ((c = getc(fp)) != EOF) {
	      if (!faxsize && (c & 0xf0) != NUL)
		 continue;		      /* Ignore page leading garbage */

	      if (c == DLE)
		 com_putbyte(DLE);
	      com_putbyte(c);

	      if (!(++faxsize % 1024))
		 fax_status(ultoa(faxsize,e_input,10));
	}

	com_putbyte(DLE);
	com_putbyte(ETX);
	fax_status(ultoa(faxsize,e_input,10));

	response.ok = false;
	t = timerset(100);

	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page) &&
	       !response.ok) {
	      get_faxline(buf);
	      parse_text_response(buf);

	      if (response.hangup_code != -1)
		 goto err;

	      if (response.error)
		 goto err;
	}

	if (!response.ok)
	   goto err;

	mdm_puts("``AT+FET=2|");

	response.ok = false;
	t = timerset(100);

	while (!timeup(t) &&
	       (!(fax_options & FAXOPT_DCD) || carrier() || !page) &&
	       !response.ok) {
	      get_faxline(buf);
	      parse_text_response(buf);

	      if (response.error)
		 goto err;
	}

	if (!response.ok)
	   goto err;

	fclose(fp);
	return (true);

err:	fclose(fp);
	return (false);
}/*fax_transmit()*/

#endif


/* end of faxrcv.c */
