/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Based off The-Box Mailer (C) Jac Kersing
 * File transfer routines (use SEALINK.C for actual transfer)
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

#define ACK 0x06
#define NAK 0x15
#define SOH 0x01
#define EOT 0x04
#define SYN 0x16
#define CAN 0x18
#define CRC 0x43
#define SUB 0x1a

/* Ok, here we start....... */

char *rcvfname()
{
    static char filenam[30];		/* The received filename */
    char checksum;			/* Checksum of name */
    int tries;				/* times tried to get the name */
    int ch;				/* received character */
    int chrs;				/* number of characters received */
    char *p;				/* pointer in filename */
    char *errptr;			/* pointer to error string */

    tries = 0;
    filewincls("Modem7 Filename Receive",1);
    message(1,"+Modem7 Filename RECV");

    for (;;) {
	win_setpos(file_win,14,2);
	win_clreol(file_win);
	do {
	   if (++tries > 20) {		  /* Enough! We won't go on forever! */
	      errptr = msg_manyerrors;
	      goto rfnabort;
	   }
	   com_putbyte(NAK);
	   if (!carrier() && !igncarr) {		/* ehh? no carrier?? */
	      errptr = msg_carrier;
	      goto rfnabort;
	   }
	   if (keyabort()) {		    /* or is the operator impatient? */
	      errptr = msg_oabort;
	      goto rfnabort;
	   }
	   ch = com_getc(30);
	   if (ch == CAN && ((ch = com_getc(10)) == CAN)) {
	      errptr = msg_cancelled;
	      goto rfnabort;
	   }
	   else if (ch == EOT) {
	      filemsg(msg_endbatch);
	      message(1,"+7RECV: %s",msg_endbatch);
	      com_putbyte(ACK);
	      return ("");			      /* all files received! */
	   }
	   else if (ch == EOF)
	      rfn_error(tries,msg_timeout);
	   else if (ch != ACK)
	      filemsg(msg_garbage,tries,ch);
	} while (ch != ACK);			      /* begin of filename?? */

	checksum = 0;			/* yep, init. */
	p = filenam;
	chrs = 0;

	do {
	   if (!carrier() && !igncarr) {      /* where has the carrier gone? */
	      errptr = msg_carrier;
	      goto rfnabort;
	   }
	   if (keyabort()) {				  /* operator again? */
	      errptr = msg_oabort;
	      goto rfnabort;
	   }
	   ch = com_getc(10);
	   if (ch == EOF)
	      rfn_error(tries,msg_timeout);
	   else if (ch == EOT) {
	      filemsg(msg_endbatch);
	      message(1,"+7RECV: %s",msg_endbatch);
	      com_putbyte(ACK);
	      return ("");			      /* all files received! */
	   }
	   else if (ch == 'u')
	      rfn_error(tries,msg_badfname);
	   checksum += ch;
	   if (ch != 0x1a) {
	      win_xyprint(file_win,14 + chrs,2,"%c",(short) ch);
	      *p++ = ch;
	      com_putbyte(ACK);
	      chrs++;
	   }
	} while (ch != 'u' && ch != 0x1a && ch != EOF);

	if (ch == 0x1a) {
	   com_putbyte(checksum & 0xff);
	   if (com_getc(10) == ACK)
	      break;
	   rfn_error(tries,msg_badfname);
	}
    }

    *p = '\0';
    filemsg(msg_complfname);
    message(1,"+7RECV: %s %s",msg_complfname,filenam);
    return (filenam);

rfnabort:
    filemsg(errptr);
    message(1,"-7RECV: %s",p);
    return (NULL);
}

void rfn_error(int retries, char *errorstr)
{
	filemsg("Retry %d: %s",retries,errorstr);
	message(1,"-7RECV retry %d: %s",retries,errorstr);
}

void unparse(char *to, char *from)	   /* Modem7 filename to DOS name */
{
    char *p, *q;

    q = to;
    for (p = from; *p && p < from + 8 && *p != ' ' ; *q++ = *p++) ;
    *q = '\0';
    if (!*p) return;
    *q++ = '.';
    for (p = from + 8; *p && p < from + 12 && *p != ' '; *q++ = *p++) ;
    *q = 0;
}

int batchdown (void)
{
    char *temp, *result, name[40];

    com_purge();			/* Empty buffers */
    com_dump();

    do {
       *name='\0';
       if (modem) {			/* Modem7 filename wanted? */
	  temp = rcvfname();
	  if (!temp) return (0);	/* Error of some sort */
	  if (!*temp) return (1);	/* End of transfer */
	  if (!telink) {		/* use telink name, if poss. */
	     unparse(name,temp);	/* no telink, use this name */
	     message(1,"+7RECV %s",name);
	     if (fexist(name)) {
		unique_name(name);	/* should print dup -> in file_win! */
		message(1,"+7RECV: Dup file renamed: %s",name);
	     }
	  }
       }

       result = rcvfile((wzgetfunc == batchdown) ? inbound : download,
			telink ? NULL : name);
       if (result) {
	  if (wzgetfunc == batchdown) {
	     extra.nrfiles++;
	     extra.kbfiles += (word) kbtransfered;
	     ourbbs.nrfiles++;
	     ourbbs.kbfiles += kbtransfered;
	     show_stats();
	     show_mail();
	  }
       }
    } while (result && *result);	/* really got something? */

    return (result ? 1 : 0);
}

void parse(char *to,char *from)    /* Makes Modem7 filename from DOS name */
{
    char *p,*q;

    for (p = to; p < to + 20; *p++ = ' ') ;	/* fill with spaces first */
    p = to;
    for (q = from; *q && q < from + 8 && *q != '.'; *p++ = *q++) ;
    if (!*q) { *(to + 11) = '\0'; return; }
    while (*q && *q != '.') q++;
    if (*q == '.') q++;
    for (p = to + 8; *q && q < from + 20; *p++ = *q++) ;
    *(to + 11) = '\0';
}

int xmtfname(char *name)			/* Modem7 send filename */
{
    char *p;					/* pointer to filename */
    char tname[128];				/* temp. buffer for filename */
    char rubbish[128];				/* rubbish, only temp storage */
    char fname[128];				/* filename to send */
    int tries;					/* number of tries */
    int ch;					/* received character */
    int chrs;					/* characters sent */
    char checksum;				/* checksum of filename */
    long t;					/* timer stuff */
    char *errptr;				/* pointer to error string */

    t = timerset(600);
    tries = 0;

    splitfn(rubbish,tname,name);
#if OS2 || WINNT
    MakeShortName(tname,rubbish);
    parse(fname,rubbish);
#else
    parse(fname,tname);
#endif
    filewincls("Modem7 Filename Transmit",1);
    message(1,"+Modem7 Filename SEND %s",name);

    for (;;) {
	win_setpos(file_win,14,2);
	win_clreol(file_win);
	p = fname;
	if (++tries > 20) {		/* limit=20, not the sky!! */
	   errptr = msg_manyerrors;
	   goto xabort;
	}

	do {
	   if (timeup(t)) {
	      errptr = msg_fatimeout;
	      goto xabort;
	   }
	   if (!carrier() && !igncarr) {/* carrier lost? */
	      errptr = msg_carrier;
	      goto xabort;
	   }
	   if (keyabort()) {		/* operator intervening? */
	      errptr = msg_oabort;
	      goto xabort;
	   }
	   ch = com_getbyte();
	   if (ch != EOF && ch != NAK && ch != 'C')
	      filemsg(msg_garbage,tries,ch);
	} while (ch != NAK && ch != 'C');

	com_putbyte(ACK);		   /* signal begin of filename */
	chrs = 0;
	checksum = 0x1a;

	do {
	   com_putbyte(*p);
	   checksum += *p;
	   win_xyprint(file_win,14 + chrs,2,"%c",(short) (*p++));
	   chrs++;
	   ch = com_getc(10);
	} while (*p && ch == ACK);

	if (ch == ACK) {
	   com_putbyte(0x1a);
	   if ((char)com_getc(50) == checksum) {
	      com_putbyte(ACK);
	      filemsg(msg_complfname);
	      message(1,"+7SEND: %s",msg_complfname);
	      return (1);
	   }
	}
	com_putbyte('u');
	filemsg("Retry %d: %s",tries,msg_badfname);
	message(1,"-7SEND retry %d: %s",tries,msg_badfname);
    }

xabort:
    filemsg(errptr);
    message(1,"-7SEND: %s",errptr);
    return (0);
}

char *nextfile(char *file,char *next)  /* Copy the next filename to buffer */
{
	int count;

	while (*next && isspace(*next)) next++;
	if (!*next) return NULL;
	for (count=0;*next && count<64 && !isspace(*next);)
	    *file++=*next++;
	*file='\0';
	while (*next && isspace(*next)) next++;
	return (next);
}

char *batchup(char *namelist,char pr)	/* Batch send		    */
/*char *namelist;			  /+ list of files to send    */
/*char pr;				  /+ protocol to use	      */
{
    char *next; 			/* pointer in filelist	    */
    char file[80];			/* name of current file     */
    char path[80];			/* path for file	    */
    char tname[80];			/* temp. buffer 	    */
    char *fname;			/* name returned by ffirst  */
    int test;				/* general purpose...	    */

    next = namelist;

    while (next = nextfile(file,next), next) {	    /* get the next filename */
	  fname = ffirst(file); 	/* fname points to plain name */
	  if (!fname)
	     message(1,"-BATCH: No files match %s",file);
	  else {
	     do {
		splitfn(path,tname,file);   /* remove filename	    */
		strcat(path,fname);	    /* and add non-wildcard */
#ifndef NOHYDRA
		if	(pr=='H') test = hydra(path,NULL);
		else
#endif
		if (pr=='Z') test = send_Zmodem(path,NULL,0,0,0);
		else		  test = xmtfile(path,NULL);
	     } while ((fname = fnext()), test != XFER_ABORT && fname);
	     fnclose();
	     if (test == XFER_ABORT) {
		send_can();
		message(1,"-BATCH: one ore more files not sent!");
		return (next);
	     }
	  } /* end of else part, all files with name <file> are sent */
    } /* end of while, list is empty now */

    return (NULL);
}

void end_batch()
{
    long t;				/* timer */
    int ch;				/* received character */

    t = timerset(150);
    do {
       if (!carrier() && !igncarr) {	/* carrier lost? */
	  message(1,"-BATCH: %s",msg_carrier);
	  return;
       }
       if (keyabort()) {		/* operator doing nasty? */
	  message(1,"-BATCH: %s",msg_oabort);
	  return;
       }

       ch = com_getbyte();
       if (ch == 'C' || ch == NAK) {
	  com_putbyte(EOT);
	  t = timerset(10);
       }
     } while (!timeup(t));

     com_purge();
     message(1,"+BATCH: %s",msg_endbatch);
}


/* end of xfer.c */
