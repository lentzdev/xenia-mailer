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


#define NUMREQSECLEVELS 11
#define NUMRSPCODES	26
#define MAXRSPTEXT	4096


#define REQ_BRKEXACT 0x0001	/* break off scan after hit on next template */
#define REQ_BRKHIT   0x0002	/* break off scan after hit in next filesrch */
#define REQ_NOWILDS  0x0004	/* don't check this one if wilds in scanspec */
#define REQ_EXACT    0x0008	/* only check if exact match on template     */
#define REQ_NOCOUNT  0x0010	/* don't add next files to countlimit        */
#define REQ_NOSIZE   0x0020	/* don't add next files to sizelimit (in K)  */
#define REQ_NOTIMER  0x0040	/* don't check filexfer time and don't add   */
#define REQ_NORESP   0x0080	/* don't put any fail info in response file  */
#define REQ_NOEVENT  0x0100	/* don't check whether this is no-req event  */
#define REQ_GROUP    (REQ_BRKEXACT|REQ_BRKHIT|REQ_NOCOUNT|REQ_NOSIZE|REQ_NOTIMER|REQ_NORESP)
#define REQ_MAGIC    0x1000	/* magic filename: send all in list, count 1 */
#define REQ_SERVICE  0x2000	/* service request: interpret params[128]    */


struct _reqhdr {
	struct {
	    word countlimit;	/* limit in number of requests for reqlevel  */
	    word sizelimit;	/* limit in size of requests for reqlevel    */
	} reqsec[NUMREQSECLEVELS];
	word idxnum;		/* number of index records		     */
	word rsplen;		/* total length of response information      */
	word rspofs_header;	/* offset in response info to header text    */
	word rspofs_footer;	/* offset in response info to footer text    */
	word rspofs_fileinfo;	/* offset in response info to fileinfo text  */
	struct {
	    word minlevel;	/* minimum level for response for this code  */
	    word maxlevel;	/* maximum level for response for this code  */
	    word rspofs_code;	/* offset in response info to code text      */
	} rsp_codeinfo[NUMRSPCODES];
};


struct _reqidx {		/* structure of compiled okfile in dir-list  */
       word flags;		/* break,group,magic,nocount,service,etc     */
       word minlevel;		/* minimum level user must have 	     */
       word maxlevel;		/* maximum level user is allowed to have     */
       long offset;		/* long offset in file with filenames	     */
       word number;		/* number of files for this filespec	     */
       char filespec[12];	/* filename/filespec/magic/service	     */
       char password[8];	/* password needed for access to this spec   */
       char params[128];	/* filepaths for normals, or service parms   */
};				/* sizeof (struct _okidx) == 160	     */


struct _reqdir {		/* structure of filenames indexed by _reqidx */
       char fname[12];
       long fsize;
       long fstamp;
};

				/* Request Response definitions 	     */
#define RSP_NOTFOUND	  0	/* Default - nothing found		     */
#define RSP_NOREQUESTS	  1	/* Management:	  No request.idx/request.dir */
#define RSP_NOREQONUS	  2	/* Logic errors:  No requests on us allowed  */
#define RSP_NOREQEVENT	  3	/*		  No requests allowed event  */
#define RSP_LOLEVEL	  4	/*		  sec-level too low	     */
#define RSP_HILEVEL	  5	/*		  sec-level too high	     */
#define RSP_PASSWORD	  6	/*		  missing or faulty password */
#define RSP_COUNT	  7	/*		  count limit exceeded	     */
#define RSP_SIZE	  8	/*		  size	limit exceeded	     */
#define RSP_TIMER	  9	/*		  time limit exceeded	     */
#define RSP_NOLOUPDATE	 10	/*		  no older files found	     */
#define RSP_NOHIUPDATE	 11	/*		  no newer files found	     */
#define RSP_FILE	 12	/* Ok, processed: normal file request	     */
#define RSP_LOUPDATE	 13	/*		  (-) file update request    */
#define RSP_HIUPDATE	 14	/*		  (+) file update request    */
#define RSP_MAGIC	 15	/*		  magic file request	     */
#define RSP_SERVICE	 16	/*		  service request	     */
#define RSP_ERRFILE	 17	/* Process error: Error processing file req  */
#define RSP_ERRLOUPDATE  18	/*		  Ditto, for old update req  */
#define RSP_ERRHIUPDATE  19	/*		  Ditto, for new update req  */
#define RSP_ERRMAGIC	 20	/*		  Error processing magic     */
#define RSP_ERRSERVICE	 21	/*		  Error processing service   */


/* end of request.h */
