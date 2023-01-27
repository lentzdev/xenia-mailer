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


/* Transfer protocols */

/*  SEAlink - Sliding window file transfer protocol

Original version:
    Version 1.20, created on 08/05/87 at 17:51:40

    Copyright 1986,87 by System Enhancement Associates;
    
    By:  Thom Henderson

This version is greatly modified by: Jac Kersing.
Non-standard SEAlink routines done in 1988 by Jac Kersing.

	 Two routines are provided to implement SEAlink file transfers.

	 int xmtfile(name)	       /+ transmit a file +/
	 char *name;		       /+ name of file to transmit +/

	 return values:
	 - 0 - error during transfer.
	 - 1 - everything went OK.

	 char *rcvfile(name)	       /+ receive a file +/
	 char *name;		       /+ name of file (optional) +/

	 return values:
	 - NULL - error during transfer.
	 - "" (empty string) - empty file received (end of batch).
	 - "name" - received file (name) ok.

*/

#include "zmodem.h"
#include "mail.h"

struct zeros			       /* block zero data structure */
{   long flen;			       /* file length  0 */
    word ftime; 		       /* file time stamp 4 */
    word fdate; 		       /* file date stamp 6 */
    char fnam[16];		       /* original file name 8 */
    char header;		       /* header version number 24 */
    char prog[14];		       /* sending program name (see note) 25 */
    char noacks;		       /* true if ACKing not required 40 */
    char crcmode;		       /* 01 for CRC, 00 for Checksum 41 */
    char fill[87];		       /* reserved for future use 128 */
}   ;

/* I reduced the size of the sending programs name by one. This must
   be done because the compiler will skip one byte (for the word
   boundary) after member header. Not reducing it would result in
   the wrong position for noacks.
*/

#define ACK 0x06
#define NAK 0x15
#define SOH 0x01
#define EOT 0x04
#define SYN 0x16
#define CRC 0x43
#define SUB 0x1a

/* following needed for YOOHOO processing */

#define ENQ    0x05
#define YOOHOO 0xf1
#define TSYNC  0xae

struct	 _Hello
{
word	   signal;	     /* always 'o'     (0x6f)			*/
word	   hello_version;    /* currently 1    (0x01)			*/
word	   product;	     /* product code				*/
word	   product_maj;      /* major revision of the product		*/
word	   product_min;      /* minor revision of the product		*/
char	   my_name[60];      /* Other end's name                        */
char	   sysop[20];	     /* sysop's name                            */
word	   my_zone;	     /* 0== not supported			*/
word	   my_net;	     /* our primary net number			*/
word	   my_node;	     /* our primary node number 		*/
word	   my_point;	     /* 0== not supported			*/
char	   my_password[8];   /* ONLY 6 CHARACTERS ARE SIGNIFICANT !!!!! */
char	   reserved2[8];     /* reserved by Opus			*/
word	   capabilities;     /* see below				*/
word	   zone;
word	   net;
word	   node;
word	   point;
char	   reserved3[4];    /* for non-Opus systems with "approval" orig 12  */
} ; /* size 128 bytes */

/* Ok, now back to SEAlink */

#define WINDOW 6		       /* normal window size */

static int outblk;		       /* number of next block to send */
static int ackblk;		       /* number of last block ACKed */
static int blksnt;		       /* number of last block sent */
static int slide;		       /* true if sliding window */
static int ackst;		       /* ACK/NAK state */
static int numnak;		       /* number of sequential NAKs */
static int chktec;		       /* check type, 1=CRC, 0=checksum */
static int toterr;		       /* total number of errors */
static int ackrep;		       /* true when ACK or NAK reported */
static int ackseen;		       /* count of sliding ACKs seen */
static int canceld;		       /* remote canceled transfer */

/*  Definitions of functions that follow  */

static void ackchk();			/* Check ACK/NAK status */
static void sendblk(FILE *, int);	/* Send one block */
static void shipblk(char *, int, char); /* Ship block to comm-port */
static void sendack(int, int);		/* Send ACK or NAK */
static char *getblock(char *);		/* read a block of data */

int send_SEA (char *fname, char *alias, int doafter, int fsent, int wazoo)
{
	int fd;
	int res;
	int n=wazoo;	    /* set to wazoo to eliminate warning (n is used!) */
	char *p;

	switch (((fsent < 0) ? ZFIN : ZFILE)) {
	       case ZFIN:
		    res = xmtfile(NULL,NULL);
		    return ((res <= 0) ? XFER_ABORT : XFER_OK);

	       case ZFILE:
		    res = xmtfile(fname,alias);

		    if (res <= 0) return (XFER_ABORT);

		    p = alias ? alias : fname;
		    n = (int) strlen(p) - 1;
		    if (n > 3 && (!stricmp(&p[n-3],".PKT") || is_arcmail(p,n))) {
		       remote.nrmail++;
		       remote.kbmail+= kbtransfered;
		    }
		    else {
		       remote.nrfiles++;
		       remote.kbfiles+= kbtransfered;
		    }
		    show_mail();

		    switch (doafter) {
			   case DELETE_AFTER:
				if (unlink(fname))
				   message(6,"!Could not delete %s",fname);
				else
				   message(1,"+Sent-S deleted %s",fname);
				break;
			   case TRUNC_AFTER:
				if ((fd = dos_sopen(fname,O_WRONLY|O_CREAT|O_TRUNC,DENY_ALL)) < 0) {
				   message(6,"!Error truncating %s",fname);
				   break;
				}
				dos_close(fd);
				message(1,"+Sent-S truncated %s",fname);
				break;
			   default:
				message(1,"+Sent-S %s",fname);
				break;
		    }

		    return (XFER_OK);
	}

	return (XFER_ABORT);
} /* send_SEA() */


static FILE *f;


/*  File transmitter logic */
static struct stat fst; 		/* data about file		     */

int xmtfile (char *name, char *alias)	/* transmit a file		     */
{					/* name of file to send, opt alias   */
    long t1;				/* timers			     */
    int endblk; 			/* block number of EOT		     */
    int count;				/* counter needed for setup	     */
    char zero[130];			/* block zero data		     */
    char fn[NAMELEN], path[PATHLEN];	/* extract file name		     */
    char *errptr;			/* pointer to error msg 	     */

    if ((arqlink || cur_speed > 2400UL) && !no_overdrive)
       ackless = 1;

    filewincls("SEAlink Transmit",2);

    memset(zero,0,128); 		/* clear out data block */

    if (name && *name) {		/* if sending a file */
       if ((f = sfopen(name,"rb",DENY_NONE)) == NULL) {
	  filemsg("Can't open file");
	  message(6,"-XSEND: Unable to open %s",name);
	  return (0);
       }

       if (modem) {			/* if modem7 filename wanted */
	  if (!xmtfname((alias != NULL) ? alias : name)) {
	     return (0);
	  }
       }

       getstat(name, &fst);	     /* get file statistics */
       splitfn(path, fn, name);

       endblk =(int) ((fst.st_size+127)/128) + 1;

       message(2,"+XSEND %s%s%s (%ldb), %d min.",
	       name, alias ? " -> " : "", alias ? alias : "",
	       fst.st_size,
	       (int)((((endblk - (xmodem && !telink ? 1 : 0)) *
		       1340L) / line_speed) + 59) / 60);

       win_xyputs(file_win,14,2,fn);
       if (alias != NULL)
	  win_print(file_win,"  ->  %s",alias);
       win_xyprint(file_win,14,5,"%8ld",fst.st_size);
       xfertime(fst.st_size);

       if (telink || !xmodem) {
	  *(long *)zero = intell(fst.st_size);
	  if (!telink) *(long *) &zero[4] = intell(fst.st_mtime);
#if OS2 || WINNT
	  { char fn2[NAMELEN];
	    MakeShortName((alias != NULL) ? alias : fn, fn2);
	    strcpy(&zero[8], fn2);
	  }
#else
	  strcpy(&zero[8], (alias != NULL) ? alias : fn);
#endif

#if 0
	  if (xenpoint_ver)
	     sprint(&zero[25], "XenPoint %s",
			       xenpoint_ver);
	  else
#endif
	     sprint(&zero[25], "Xenia%s %d.%02d+",
			       XENIA_SFX, XEN_MAJ, XEN_MIN);
	  zero[39] = 0;

	  if (telink) {
	     for (count = (int) strlen(&zero[8]); count <= 15; count++)
		 zero[count+8] = ' ';
	     zero[40] = 0;
	     zero[41] = 1;  /* Telink CRC flag */
	  }
	  else {
	     zero[40] = ackless;
	     zero[41] = 0;
	  }
       }
    }
    else {				/* NULL end of batch */
       win_xyputs(file_win,14,2,msg_endbatch);
       endblk = 0;			/* fake for no file */
    }

    throughput(0,0L);

    outblk = 1; 			/* set starting state */
    ackblk = -1;
    blksnt = slide = ackst = numnak = toterr = ackrep = ackseen = canceld = 0;
    chktec = 2; 			/* undetermined */

    if (xmodem) com_purge();		/* remove all NAKs already there */
					/* to prevent loss of synchronisation */

    t1 = timerset(300); 		/* time limit for first block */

    while (ackblk < endblk) {		/* while not all there yet */
	  if (!carrier() && !igncarr) {
	     errptr = msg_carrier;
	     goto abort;
	  }
	  if (keyabort()) {
	     errptr = msg_oabort;
	     goto abort;
	  }
	  if (timeup(t1)) {
	     errptr = msg_fatimeout;
	     goto abort;
	  }

	  if (outblk <= ackblk + (slide ? WINDOW : 1)) {
	     if (outblk < endblk) {
		if (outblk > 0)
		   sendblk(f,outblk);
		else 
		   shipblk(zero, 0, telink ? SYN : SOH);

		if (ackless && slide)
		   ackblk = outblk;
	     }
	     else if (outblk == endblk)
		com_putbyte(EOT);

	     outblk++;
	     t1 = timerset(300);      /* time limit between blocks */
	  }

	  ackchk();
	  if (numnak > 10) {
	     errptr = msg_manyerrors;
	     goto abort;
	  }
	  if (canceld) {
	     errptr = msg_cancelled;
	     goto abort;
	  }
    }

    if (slide) sealink = 1;

    throughput(1,fst.st_size);	       /* What's the efficency? */
  
    if (endblk)
       fclose(f);
    kbtransfered = (endblk + 7) / 8;
    return (1); 			/* exit with good status */

abort:
    filemsg(errptr);
    filemsg("-XSEND: %s",errptr);
    if (endblk)
       fclose(f);
    return (0 - canceld);		/* exit with bad status */
}

/*  The various ACK/NAK states are:
    0:	 Ground state, ACK or NAK expected.
    1:	 ACK received
    2:	 NAK received
    3:	 ACK, block# received
    4:	 NAK, block# received
    5:	 Returning to ground state
*/

static void ackchk()		       /* check for ACK or NAK */
{
    int c;			       /* one byte of data */
    static int rawblk;		       /* raw block number */

    ackrep = 0; 		       /* nothing reported yet */

    while ((c = com_getbyte()) != EOF) {
	  if (ackst == 3 || ackst == 4) {
	     slide = 0; 	       /* assume this will fail */
	     if (rawblk == (c ^ 0xff)) { /* see if we believe the number */
		rawblk = outblk - ((outblk - rawblk) & 0xff);
		if (rawblk >= 0 && rawblk <= outblk && rawblk > outblk - 128) {
		   if (ackst == 3) {	/* advance for an ACK */
		      ackblk = ackblk > rawblk ? ackblk : rawblk;
		      slide = 1;
		      if (ackless && ++ackseen > 10) {
			 ackless = 0;	/* receiver not ACKless */
			 filemsg(msg_overdrive);
		      }

		      if (rawblk)
			 win_xyprint(file_win,14,7,"%8ld",
				     rawblk ? (rawblk  - 1L) * 128L : 0L);
		   }
		   else {		/* else retransmit for a NAK */
		      outblk = rawblk < 0 ? 0 : rawblk;
		      slide = numnak < 4;
		      filemsg("Byte %ld NAK: Resending from %ld",
			      ftell(f), outblk * 128L);
		      message(0,"-Byte %ld NAK: Resending from %ld",
			      ftell(f), outblk * 128L);
		   }

		   ackrep = 1;		/* we reported something */
		}
	     }

	     ackst = 5; 		/* return to ground state */
	  }

	  if (ackst == 1 || ackst == 2) {
	     rawblk = c;
	     ackst += 2;
	  }

	  if (!slide || ackst == 0) {
	     if (c == ACK) {
		if (!slide) {
		   ackblk++;
		   win_xyprint(file_win,14,7,"%8ld", (ackblk  - 1L) * 128L);
		   ackrep = 1;		/* we reported an ACK */
		}

		ackst = 1;
		numnak = 0;
	     }
	     else if (c == 'C' || c == NAK) {
		if (chktec > 1) {	/* if method not determined yet */
		   chktec = (c == 'C'); /* then do what rcver wants */
		   if (xmodem) ackblk = 0;
		}

		com_dump();	    /* purge pending output */
		tdelay(6);	    /* resynch */
		if (!slide) {
		   outblk = ackblk + 1;
		   filemsg("NAK: Resending from %ld", outblk * 128L);
		   message(0,"-NAK: Resending from %ld",outblk * 128L);
		   ackrep = 1;	  /* we reported a negative ACK */
		}

		ackst = 2;
		numnak++;
		if (blksnt) toterr++;
	     }
	     else if (c == CAN)
		canceld = 1;
	     else		/* no ACK, no NAK, no CAN --> garbage */
		ackrep = 1;
	 }

	 if (ackst == 5)
	    ackst = 0;
    }
}

static void sendblk(FILE *f,int blknum) /* send one block */
/* FILE *f;				    file to read from */
/* int blknum;				    block to send */
{
    long blkloc;			/* address of start of block */
    char buf[128];			/* one block of data */

    if (blknum != blksnt + 1) { 	/* if jumping */
       blkloc = (long)(blknum - 1) * 128L;
       fseek(f,blkloc,0);		/* move where to */
    }
    blksnt = blknum;

    memset(buf,26,128); 		/* fill buffer with control Zs */
    com_disk(true);
    fread(buf,1,128,f); 		/* read in some data */
    com_disk(false);
    if (blknum == 1 && mail) {
       struct _pkthdr *phdr = (struct _pkthdr *) buf;

       phdr->ph_rate = (short) inteli(cur_speed < 65535UL ? (word) cur_speed : 9600);
#if 0
       if (((byte) phdr->ph_prodl) == isXENIA) {
	  phdr->ph_revh = XEN_MAJ;
	  phdr->ph_revl = XEN_MIN;
	  /* *(long *) phdr->ph_spec = intell(xenkey->serial); */
       }
#endif
    }
    shipblk(buf,blknum,SOH);		/* pump it out the comm port */
    win_xyprint(file_win,14,6,"%8ld",ftell(f));
    xfertime(fst.st_size - ftell(f));
}

static void shipblk(char *blk, int blknum, char hdr)
					   /* physically ship a block */
/* char *blk;				   data to be shipped */
/* int blknum;				   number of block */
/* char hdr;				   block header byte */
{
    char *b = blk;			/* data pointer */
    word crc = 0;			/* CRC check value */
    int n;				/* index */

    com_bufbyte(hdr);			   /* block header */

    if (hdr != 0x1f) {			/* no block no's if YOOHOO */
       com_bufbyte(blknum);		   /* block number */
       com_bufbyte(blknum ^ 0xff);	   /* block number check value */
    }

    for (n = 0; n < 128; n++) { 	/* ship the data */
	com_bufbyte(*b);
	if (!chktec || (telink && blknum == 0))
	   crc += *b++;
	else crc = crc_update(crc,(word) (*b++));
    }

    if (chktec && !(telink && blknum == 0)) {  /* send proper check value */
       crc = crc_finish(crc);
       com_bufbyte(crc >> 8);
       com_bufbyte(crc & 0xff);
    }
    else
       com_bufbyte(crc);

    com_bufflush();
}

/*  File receiver logic */

char *rcvfile(char *path,char *name)	/* receive file */
{
    int c;				/* received character */
    int tries;				/* retry counter */
    long t1;				/* timer */
    int blknum; 			/* desired block number */
    int inblk;				/* this block number */
    FILE *f = NULL;			/* file */
    char buf[128];			/* data buffer */
    char tmpname[100];			/* name of temporary file */
    char tmpbuf[100];			/* temp buffer */
    static char outname[100];		/* name of final file */
    char zero[130];			/* file header data storage */
    int endblk; 			/* block number of EOT, if known */
    long left;				/* bytes left to output */
    long flsize;			/* lenght we transfer THIS time */
    long flend; 			/* end of the existing file (resync) */
    int n;				/* index */
    char *why;				/* status */
    long diskfree;
    char *errptr;			/* pointer to error msg */

    filewincls("SEAlink Receive",2);

    if (path) strcpy(outname,path);	/* prepare path */
    else *outname = '\0';
    if (name && *name) {
       strcat(outname,name);		/* use name if given */
       win_xyputs(file_win,14,2,name);
       win_xyprint(file_win,14,5,"%8s","---");
       win_xyputs(file_win,31,5,"---");
    }
    *tmpbuf = '\0';

    blknum = (name && *name) ? 1 : 0;	/* first block we must get */
    tries  = -10;			/* kludge for first time around */
    chktec = 1; 			/* try for CRC error checking */
    toterr = 0; 			/* no errors yet */
    endblk = 0; 			/* we don't know the size yet */
    ackless = 0;			/* we don't know about this yet */
    flend = 0L; 			/* suppose a new file (resync) */
    memset(zero,0,128); 		/* or much of anything else */
    flsize = left = 0;			/* don't know how much is left */

    if (com_scan())			/* kludge for adaptive Modem7 */
       goto nextblock;

    throughput(0,0L);			/* Init */
    
nakblock:				/* we got a bad block */
    if (blknum > 1) toterr++;
    if (++tries > 10) {
       errptr = msg_manyerrors;
       goto abort;
    }
    if (tries == 0)			/* if CRC isn't going */
       chktec = 0;			/* then give checksum a try */

    sendack(0,blknum);			/* send the NAK */

    if (ackless && toterr > 20) {	/* if ackless mode isn't working */
       ackless = 0;			/* then shut it off */
       filemsg(msg_overdrive);
       message(1,"-%s",msg_overdrive);
    }
    goto nextblock;

ackblock:			       /* we got a good block */
    win_xyprint(file_win,14,6,"%8ld",f ? ftell(f) : 0L);
    win_xyprint(file_win,14,7,"%8ld",f ? ftell(f) : 0L);

nextblock:			       /* start of "get a block" */
    if (!carrier() && !igncarr) {
       errptr = msg_carrier;
       goto abort;
    }
    if (keyabort()) {
       errptr = msg_oabort;
       goto abort;
    }

    t1 = timerset(30);			/* timer to start of block */
    while (!timeup(t1)) {
	  c = com_getbyte();
	  if (c == EOT) {
	     if (!endblk || endblk == blknum)
		goto endrcv;
	  }
	  else if (c == SOH || (telink && c == SYN)) {
	     inblk = com_getc(5);
	     if (com_getc(5) == (inblk ^ 0xff))
		goto blockstart;	/* we found a start */
	  }
	  else if (c == CAN && ((c = com_getc(20)) == CAN)) {
	     errptr = msg_cancelled;
	     goto abort;
	  }
    }
    filemsg("Byte %ld: %s", f ? ftell(f) : 0L,msg_timeout);
    message(0,"-Byte %ld: %s", f ? ftell(f) : 0L,msg_timeout);
    goto nakblock;

blockstart:			       /* start of block detected */
    c = blknum & 0xff;

    if (inblk == 0 && blknum <= 1) {   /* if this is the header */
       if (telink) chktec=0;
       if ((why = getblock(zero)) == 0) {
	  if (telink && tries<0) chktec = 1;
	  sendack(1,inblk);	   /* ack the header */

	  if (!telink) sealink = 1;   /* sealink detected */

	  /* if no default name, use xmitted one */
	  if (!name || !*name)	{
	     if (telink) {		/* terminate filename */
		why = &zero[8];
		while (*why > 33) ++why;
		*why = '\0';
	     }
	     strcat(outname,&zero[8]);
	     name = &zero[8];
	     win_xyputs(file_win,14,2,&zero[8]);
	  }

	  left = flsize = intell(*(long *)zero);
	  if (left) {			/* length to transfer */
	     endblk = (int) ((left + 127L) / 128L) + 1;
	     diskfree = freespace(path);
	     if (flsize + 10240 > diskfree) {
		filemsg("Not enough diskspace: %ld > %ld",
			flsize + 10240, diskfree);
		message(6,"!Not enough diskspace: %ld > %ld",
			flsize + 10240, diskfree);
		return (NULL);
	     }
	  }

	  flend = 0L;			/* End of (existing) file */

	  if (is_arcmail(outname,(int) strlen(outname)) && fexist(outname)) {
	     /* I must implement the further checking for
		re-sync here. Will be done in the future. */
	     sprint(tmpbuf,"%s -> ",outname);
	     unique_name(outname);
	     strcat(tmpbuf,outname);
	     got_arcmail = 1;
	  }
	  else
	     strcpy(tmpbuf,outname);

	  if (endblk) {
	     sprint(tmpname," (%ldb)",flsize);
	     strcat(tmpbuf,tmpname);
	  }

	  if (zero[25]) {
	     /*if (mail && !(wzgetfunc == batchdown)) {*/
	     if (mail)
		win_xyputs(mailer_win,10,4,&zero[25]);
	     sprint(tmpname," from %s",&zero[25]);
	     strcat(tmpbuf,tmpname);
	  }

	  if (ackless != zero[40] && !telink) {   /* note variant employed */
	     ackless = zero[40];
	     sprint(tmpname,"Overdrive %sengaged",ackless ? "" : "dis");
	     filemsg(tmpname);
	     message(1,"+%s",tmpname);
	  }

	  blknum = 1;			/* now we want first data block */
	  goto ackblock;
       }
       else {
	  if (telink && tries < 0) chktec = 1;
	  goto nakblock;		/* bad header block */
       }
    }

    else if (inblk == c) {		/* if this is the one we want */
       if ((why = getblock(buf)) == 0) {/* else if we get it okay */
	  if (!ackless) 		/* if we're sending ACKs */
	     sendack(1,inblk);		/* then ACK the good data */

	  if (blknum == 1) {
	     message(2,"+XRECV %s",*tmpbuf ? tmpbuf : outname);
	     win_xyprint(file_win,14,5,"%8ld",flsize);
	     xfertime(flsize);

	     n = ((int) strlen(outname)) - 1;
	     if (n > 3 && !stricmp(&outname[n-3],".REQ")) {
		sprint(outname,"%sXENREQ%02x.000",inbound,task);
		unique_name(outname);
	     }

	     if ((f = sfopen(outname, flend ? "r+b" : "wb",DENY_ALL)) == NULL) {
		sprint(tmpbuf,"!Cannot %s %s",
		       flend ? "open" : "create", outname);
		errptr = tmpbuf;
		goto abort;
	     }
	     else
		fseek(f,flend,0);

	     if (mail) {
		struct _pkthdr *phdr = (struct _pkthdr *) buf;
		word prod, pmaj, pmin;

		found_zone  = inteli(phdr->ph_ozone1);
		if (!found_zone) {
		   found_zone = inteli(phdr->ph_ozone2);
		   if (!found_zone) found_zone = ourbbs.zone;
		}
		found_net   = inteli(phdr->ph_onet);
		found_node  = inteli(phdr->ph_onode);
		found_point = 0;
		prod = phdr->ph_prodl;
		pmaj = phdr->ph_revh;
		pmin = 0;

		/* Packet type 2: FSC-39/48 validation: check for type 2+    */
		if (inteli(phdr->ph_ver) == 2) {
		   if (buf[44] == buf[41] && buf[45] == buf[40] &&
		       (inteli(phdr->ph_cwd) & 0x0001)) {
		      found_point = inteli(phdr->ph_opoint);
		      if (found_point && found_net == -1)
			 found_net = inteli(phdr->ph_auxnet);
		      pmin = phdr->ph_revl;
		      prod |= (word) ((phdr->ph_prodh << 8) & 0xff00);
		   }
		   else {     /* FSC-45/Pkt94 validation: check for type 2.2 */
		      struct _pkthdr94 *phdr94 = (struct _pkthdr94 *) buf;

		      if (phdr94->ph_subver == 2) {   /* speed can't be 2    */
			 found_point = inteli(phdr94->ph_opoint);
			 if (phdr94->ph_id94 == 94 &&
			     ((phdr94->ph_prod16 <  0x0100 && phdr94->ph_prod16 == phdr94->ph_prodl) ||
			      (phdr94->ph_prod16 >= 0x0100 && phdr94->ph_prodl == 0xFE)) ) {
			    pmin = phdr94->ph_revl;
			    prod = phdr94->ph_prod16;
			 }
		      }
		   }
		}

		for (n = 0; n < 7; n++) {
		    found_pwd[n] = phdr->ph_pwd[n];
		    if (!found_pwd[n] || found_pwd[n] == ' ') break;
		}
		found_pwd[n] = '\0';

		/*
		if (prod == isXENIA)
		   found_sernr = intell(*(long *) phdr->ph_spec);
		else
		   found_sernr = -1L;
		*/

		showprod(prod,pmaj,pmin);
	     }
	  }

	  if (endblk && left < 128) {	     /* write data to disk */
	     com_disk(true);
	     n = fwrite(buf,(int) left,1,f);
	     com_disk(false);
	     left = 0;
	  }
	  else {
	     com_disk(true);
	     n = fwrite(buf,128,1,f);
	     com_disk(false);
	     if (endblk) left -= 128;
	  }

	  if (n < 0) {
	     errptr = "Write error (disk full?)";
	     goto abort;
	  }

	  throughput(2,ftell(f));
	  xfertime(flsize - ftell(f));
	  tries = 0;		   /* reset try count */
	  blknum++;		   /* we want the next block */
	  goto ackblock;
       }
       else {
	   filemsg("Byte %ld: %s", f ? ftell(f) : 0L,why);
	   message(0,"-Byte %ld: %s", f ? ftell(f) : 0L,why);
	   goto nakblock;	    /* ask for a resend */
       }
    }
    else if (inblk < c || inblk > c + 100) { /* else if resending what we have */
       getblock(buf);			/* ignore it */
       sendack(1,inblk);		/* but ack it */
       filemsg("Byte %ld: Duplicate packet", f ? ftell(f) : 0L);
       message(0,"-Byte %ld: Duplicate packet", f ? ftell(f) : 0L);
       goto ackblock;
    }
    else {				/* else if running ahead */
       if (!xmodem) goto nextblock;
       errptr = "Lost Synchronization";
       goto abort;
    }

endrcv:
    if (!xmodem && blknum > 0) { /* NAK only if sealink and something received */
       sendack(0,blknum);
       if (com_getc(20) != EOT)
	  goto nakblock;
    }
    sendack(1,blknum);
    com_purge();			/* remove garbage */

    throughput(1,flsize - flend);

    if (blknum > 1) {			/* if we really got anything */
       fclose(f);

       if (zero[6] || zero[7]) {	/* set stamp, if known */
	  *(long *) &zero[4] = intell(*(long *) &zero[4]);
	  setstamp(outname,*(long *)&zero[4]);
       }

       kbtransfered = (blknum + 7) / 8;
       return (outname);		/* signal what file we got */
    }

    com_putbyte(EOT);			   /* else no real file */
    com_putbyte(EOT);
    com_putbyte(EOT);
    com_putbyte(EOT);
    com_putbyte(EOT);
    return ("");			/* signal end of transfer */

abort:
    filemsg(errptr);
    filemsg("-XRECV: %s",errptr);

    if (blknum > 1 && f != NULL) {
       fclose(f);			/* don't close a non-existing file!! */
       if (fexist(outname))
	  unlink(outname);
    }

    return (NULL);
}

static void sendack(int acknak,int blknum)  /* send an ACK or a NAK */
/*int acknak;				   1=ACK, 0=NAK */
/*int blknum;				   block number */
{
    if (acknak) 			/* send the right signal */
       com_bufbyte(ACK);
    else if (chktec && blknum < 2)
       com_bufbyte('C');
    else
       com_bufbyte(NAK);

    if (!xmodem) {			/* some Xmodemprgs don't like blknums */
       com_bufbyte(blknum);		   /* block number */
       com_bufbyte(blknum^0xff);	   /* block number check */
    }
    com_flush();
}

static char *getblock(char *buf)	/* read a block of data */
{
    word ourcrc = 0, hiscrc;		/* CRC check values */
    int c;				/* one byte of data */
    int n;				/* index */
    int timeout = (ackless || iplink) ? 200 : 10;   /* short block timeout */

    for (n = 0; n < 128; n++) {
	if ((c = com_getc(timeout)) == EOF)
	   return "Short packet";

	if (chktec)
	   ourcrc = crc_update(ourcrc,(word) c);
	else ourcrc += (word) c;
	*buf++ = c;
    }

    if (chktec) {
       ourcrc = crc_finish(ourcrc);
       hiscrc = (word) (com_getc(timeout) << 8);
       hiscrc |= (word) com_getc(timeout);
    }
    else {
       ourcrc &= 0xff;
       hiscrc = (word) (com_getc(timeout) & 0xff);
    }

    if (ourcrc == hiscrc)
       return (NULL);			/* block is good */
    else if (chktec)
       return (msg_datacrc);		/* else CRC error */
    else
       return ("Data checksum error");	/* or maybe checksum error */
}

int send_YOOHOO();
int get_YOOHOO();
int send_Hello();
void yhs_error();

int send_YOOHOO()		       /* send YOOHOO packet */
{
    if (send_Hello()) {
       if (com_getc(120) == YOOHOO)
	  return (get_YOOHOO(0));
       message(1,"-YooHOO RECV: %s",msg_noyoohoo);
    }
    return (0);
}

int send_Hello()		    /* Send hello packet */
{
    int errors; 			/* error count */
    struct _Hello hello;		/* our 'nice' hello packet */
    int ch;				/* character from modem */
    char *p;				/* pointer to result string */

    errors=0;

    filewincls("YooHOO Transmit",0);
    message(1,"+YooHOO SEND");

    memset((char *) &hello, 0, sizeof(struct _Hello));	     /* clear packet */
    
    hello.signal	= inteli('o');		/* protocol requires it      */
    hello.hello_version = inteli(1);		/* pfeww, early version      */
    
    hello.product	= inteli(isXENIA);	/* tell them what I'm using  */
    hello.product_maj	= inteli(XEN_MAJ);
    hello.product_min	= inteli(XEN_MIN);	/* and which version	     */

    strncpy(hello.my_name, ourbbs.name, 52);
    hello.my_name[53] = '\0';
    strncpy(hello.sysop, ourbbs.sysop, 19);
    hello.sysop[19] = '\0';

    strncpy(hello.my_password,ourbbs.password,8);

    hello.my_zone  = (word) inteli(ourbbs.zone);
    hello.my_net   = (word) inteli(ourbbs.net);
    hello.my_node  = (word) inteli(ourbbs.node);
    hello.my_point = (word) inteli(ourbbs.point);

    if (ourbbs.point && ourbbs.usepnt != 2) {
       if (remote.zone == ourbbs.zone && remote.net == ourbbs.net &&
	   (remote.node == ourbbs.node || ourbbs.usepnt == 1)) {
	  hello.my_net	= (word) inteli(ourbbs.pointnet);
	  hello.my_node = (word) inteli(ourbbs.point);
       }
       else {
	  hello.my_net	= (word) inteli(ourbbs.net);
	  hello.my_node = (word) inteli(-1);
       }
       hello.my_point = (word) inteli(0);
    }

    hello.capabilities = (word) inteli(DIETIFNA |
			 ((ourbbs.capabilities&ZED_ZAPPER) ? ZED_ZAPPER : 0 ) |
			 ((ourbbs.capabilities&ZED_ZIPPER) ? ZED_ZIPPER : 0 ) |
			 ((ourbbs.capabilities&DOES_HYDRA) ? DOES_HYDRA : 0 ) |
			 ((caller && (ourbbsflags&NO_REQ_ON)) ? 0 : WZ_FREQ ));
    hello.capabilities &= ~inteli(EMSI_SLK | EMSI_DZA);

    if ((p = zonedomabbrev(ourbbs.zone)) != NULL) {
       if (strlen(hello.my_name) + strlen(p) > 57)
	  hello.my_name[57 - strlen(p)] = '\0';
       strcpy(&hello.my_name[strlen(hello.my_name) + 1],p+1);
       hello.capabilities |= DO_DOMAIN;
    }
    else {
       /* TB's own (OLD) invented place to send serial# in YooHoo packet... */
       /* *((long *) &hello.my_name[54]) = intell(xenkey->serial); */
    }

    chktec = 1;
    
send:
    if (errors++ > 9) {
       p = msg_manyerrors;
       goto yhsfailed;
    }

    shipblk((char *)&hello, 1, 0x1f); /* Ok, send packet now */
    com_purge();

    while (carrier()) {
	  if (keyabort()) {
	     p = msg_oabort;
	     goto yhsfailed;
	  }
	  switch (ch = com_getc(iplink ? 300 : 100)) {
		 case ACK:
		      filemsg(msg_success);
		      message(1,"+YooHOO SEND: %s",msg_success);
		      return (1);  /* Ok! */
		 case ENQ:
		      yhs_error(errors,msg_enquiry);
		      goto send;
		 case '?':
		      yhs_error(errors,msg_datacrc);
		      goto send;
		 default :
		      if (ch < 0) {  /* timeout */
			 p = msg_fatimeout;
			 goto yhsfailed;
		      }
		      filemsg(msg_garbage,errors,ch);
	  }
    }

    p = "Carrier lost (password failure?)";

yhsfailed:
    filemsg(p);
    message(2,"-YooHOO SEND: %s",p);
    return (0);
}

void yhs_error(int retries,char *errorstr)
{
	filemsg("Retry %d: %s",retries,errorstr);
	message(1,"-YooHOO SEND Retry %d: %s",retries,errorstr);
}

get_YOOHOO(int send_too)		/* get YOOHOO from remote system */
{
    int errors; 			/* number of errors */
    struct _Hello hello;		/* space to store the data */
    int ch;				/* character */
    long t, t1; 			/* timer */
    char *p;				/* returncode of getblock */
    word prod, pmaj, pmin;
    char adrbuf[128];

    if (remote.capabilities > 0) return (-1);

    com_purge();
    com_dump();

    filewincls("YooHOO Receive",0);
    message(1,"+YooHOO RECV");

    ackless=0;
    chktec=1;

    errors=0;
recyoo:
    t = timerset(200);				/* 20 secs max */
    
    while (!timeup(t)) {
	  com_purge();
	  com_putbyte(ENQ);

	  t1 = timerset(20);
	  while (!timeup(t1)) {
		if (!carrier() && !igncarr) {
		   p = msg_carrier;
		   goto failed;
		}
		if (keyabort()) {
		   p = msg_oabort;
		   goto failed;
		}
		ch = com_getbyte();
		if (ch == 0x1f)
		   goto loop;
	  }
    }
    p = msg_fatimeout;
    goto failed;

loop:
    p = getblock((char *) &hello);
    if (p) {
       if (++errors > 10) {
	  p = msg_manyerrors;
	  goto failed;
       }
       filemsg("Retry %d: %s",errors,p);
       message(1,"-YooHOO RECV Retry %d: %s",errors,p);

       com_purge();		   /* try again */
       com_putbyte('?');
       goto recyoo;
    }

    filemsg(msg_success);
    message(1,"+YooHOO RECV: %s",msg_success);

    /* All OK! */
    rem_adr[0].zone  = hello.my_zone ? inteli(hello.my_zone) : ourbbs.zone;
    rem_adr[0].net   = inteli(hello.my_net);
    rem_adr[0].node  = inteli(hello.my_node);
    rem_adr[0].point = inteli(hello.my_point);
    remap(&rem_adr[0].zone,&rem_adr[0].net,&rem_adr[0].node,&rem_adr[0].point,&rem_adr[0].pointnet);
    num_rem_adr = 1;
    if (!caller) {
       remote.zone     = rem_adr[0].zone;
       remote.net      = rem_adr[0].net;
       remote.node     = rem_adr[0].node;
       remote.point    = rem_adr[0].point;
       remote.pointnet = rem_adr[0].pointnet;
    }

    remote.capabilities = inteli(hello.capabilities) | DIETIFNA;
    remote.capabilities &= ~(EMSI_SLK | EMSI_DZA);
/* ----------- Protection for changeover from HYDRA Mk.II -> Mk.III -> Mk.IV */
#if 0
    if (!caller)
       remote.capabilities &= ~DOES_HYDRA;
#endif
/* ----------- */
    remote.options	= 0;
    prod = inteli(hello.product);
    pmaj = inteli(hello.product_maj);
    pmin = inteli(hello.product_min);
    strcpy(remote.sysop,hello.sysop);
    strcpy(remote.name,hello.my_name);

    /*
    found_sernr = (prod == isXENIA) ? intell(*(long *)&hello.my_name[54]) : -1L;
    */

    if (hello.my_name[0] == '\n')	  /* FrontDoor puts a \n in unlisted */
       strcpy(hello.my_name,&hello.my_name[1]);
    win_xyputs(mailer_win,10,2,hello.my_name);
    flag_set(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point);
    if (rem_adr[0].pointnet)
       flag_set(rem_adr[0].zone,rem_adr[0].pointnet,rem_adr[0].point,0);
    sprint(adrbuf,"%d:%d/%d%s%s",
		  rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		  pointnr(rem_adr[0].point), zonedomabbrev(rem_adr[0].zone));
    last_set(caller ? &last_out : &last_in, adrbuf);
    if (!caller && callerid[0])
       write_clip(remote.sysop,"",adrbuf,remote.name);
    show_stats();
    message(2,"=Address: %s", adrbuf);

    if (hello.my_name[0])
       strcpy(remote.name,hello.my_name);
    if (remote.name[0]) {
       win_xyputs(mailer_win,10,2,remote.name);
       message(2,"=Name: %s",remote.name);
    }

    showprod(prod,pmaj,pmin);

    if (caller) {
       if (rem_adr[0].zone != remote.zone || rem_adr[0].net != remote.net ||
	   ((remote.nodeflags & VIAHOST) && rem_adr[0].node != 0) ||
	   rem_adr[0].node != remote.node || rem_adr[0].point != remote.point)
	  message(2,"=Got %d:%d/%d%s when calling %d:%d/%d%s",
		    rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,pointnr(rem_adr[0].point),
		    remote.zone,remote.net,remote.node,pointnr(remote.point));
    }
    else {
       strcpy(ourbbs.password,get_passw(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point));
       set_alias(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point);
       win_xyprint(mailer_win,2,1,"Connected to %d:%d/%d%s",
		   rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,pointnr(rem_adr[0].point));
       win_print(mailer_win," as %d:%d/%d%s",
		 ourbbs.zone, ourbbs.net, ourbbs.node, pointnr(ourbbs.point));
       win_clreol(mailer_win);
       statusmsg("Incoming call from %d:%d/%d%s%s",
		 rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		 pointnr(rem_adr[0].point), zonedomabbrev(rem_adr[0].zone));
    }

    if (hello.sysop[0])
       strcpy(remote.sysop,hello.sysop);
    if (remote.sysop[0]) {
       win_xyputs(mailer_win,10,3,remote.sysop);
       message(2,"=Sysop: %s",remote.sysop);
    }

    strncpy(remote.password,hello.my_password,8);
    remote.password[8] = '\0';

    if (ourbbs.password[0]) {
       if (strnicmp(ourbbs.password,remote.password,6)) {	/* 6 signif. */
	  message(6,"!Password mismatch '%s' <> '%s'",
		    ourbbs.password,remote.password);
	  if (!caller || !(ourbbsflags & NO_PWFORCE))
	     return (false);
       }

       if (/*caller ||*/ remote.password[0]) {
	  message(2,"+Password protected session");
	  inbound = safepath;
       }
       else {
	  message(2,"-Remote presented no password");
	  if (caller && (ourbbsflags & NO_PWFORCE))
	     inbound = safepath;
       }
    }
    else if (remote.password[0])	      /* They have pw but we haven't */
       message(2,"=Remote proposes password '%s'",remote.password);

    com_purge();
    com_putbyte(ACK);
    com_putbyte(YOOHOO);
    if (!send_too)  /* we don't have to send, so report success */
       return (1);

    for (errors = 0; errors < 10 && carrier(); errors++) {
	if ((ch = com_getc(10)) == ENQ)
	   return (send_Hello());
	else if (ch > 0) {
	   filemsg(msg_garbage,errors,ch);
	   com_purge();
	}
	com_putbyte(YOOHOO);
    }
    message(2,"-YooHOO SEND: %s",msg_noyoohoo);
    return (0);

failed:
    filemsg(p);
    message(2,"-YooHOO RECV: %s",p);
    return (0);
}


/* end of sealink.c */
