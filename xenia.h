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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <sys\stat.h>
#include "window.h"
#include "period.h"
#include "sched.h"
#include "emsi.h"
#include "hydra.h"
#include "crc.h"

#ifdef __WATCOMC__
#define WATCOM		1
#define TC		0
#else
#define WATCOM		0
#define TC		1
#endif
#define TURBOST 	0
#define MSC		0
#define MWC		0
#define ATARIST 	0
#define JAC		0

#ifndef WINNT
#ifdef __NT__
#define WINNT		1
#else
#define WINNT		0
#endif
#endif

#ifndef OS2
#ifdef __OS2__
#define OS2		1
#else
#define OS2		0
#endif
#endif

#ifndef PC
#ifdef __MSDOS__
#define PC		1
#else
#define PC		0
#endif
#endif

/* Computer dependend stuff */
#if	OS2
#if	0 /*WATCOM*/
#include "snserver.h"
#else
#include "snsrv_32.h"
#endif
#define INCL_DOSPROCESS
#define INCL_DOSNMPIPES
#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES
#include <bse.h>
#include "mcp.h"
#endif

#if	WATCOM
#include <direct.h>   /* for mkdir() */
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#endif

#if	TC
#define delay	blabla
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <alloc.h> /* only for heapcheck() */
#include <dir.h>   /* for mkdir() */
#undef	delay
#endif

#if	TURBOST
#define delay	blabla
#include <stdlib.h>
#include <ext.h>
#undef	delay
#endif

#if ATARIST==0
#define intell(x)	(x)
#define inteli(x)	(x)
#else
long intell();
#endif

#if WINNT | OS2 | PC | MWC | TURBOST
#if 0
#define BREAD	   "rb"
#define BWRITE	   "w+b"
#define BRDWR	   "r+b"
#define TREAD	   "rt"
#define TWRITE	   "w+t"
#define TRDWR	   "r+t"
#define TAPPEND    "at"
#endif
#define DENY_ALL   0x0000
#define DENY_RDWR  0x0010
#define DENY_WRITE 0x0020
#define DENY_READ  0x0030
#define DENY_NONE  0x0040
#endif

#if WINNT || OS2
#define PATHLEN 256
#define NAMELEN 256
#else
#define PATHLEN 128
#define NAMELEN  13
#endif
#define LINELEN 256

#if (PC && TC)
/*if (heapcheck() < 0) message(6,"!Heap corrupted!");*/
#define alloc_msg(place) {\
if (alloc_debug)     message(6,"]Core(%s)=%-10ld",place,coreleft());\
}
#else
#define alloc_msg(place)
#endif

typedef struct _answer ANSWER;
struct _answer {			/* --------------------------------- */
	char   *command;		/* Modem command string 	     */
	char   *pattern;		/* RING pattern to match	     */
	ANSWER *next;			/* Ptr to next item, NULL = end list */
};

typedef struct _mdmrsp MDMRSP;
struct _mdmrsp {			/* --------------------------------- */
	char   *string; 		/* Modem response string	     */
	dword	code;			/* Rsp.code MDMRSP_.... or con.speed */
	MDMRSP *next;			/* Ptr to next item, NULL = end list */
};

typedef struct _mdmcause MDMCAUSE;
struct _mdmcause {			/* --------------------------------- */
	word	  cause;		/* Cause code 0x0000 - 0xffff	     */
	char	 *description;		/* Modem error cause description     */
	MDMCAUSE *next; 		/* Ptr to next item, NULL = end list */
};

typedef struct _freedial FREEDIAL;
struct _freedial {		    /* Freepoll feature (dial)		     */
	char	 *address;	    /* address ("z:n/n" etc)		     */
	FREEDIAL *next; 	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _freeanswer FREEANSWER;
struct _freeanswer {		    /* Freepoll feature (answer)	     */
	char	   *address;	    /* address ("z:n/n" etc)		     */
	char	   *callerid;	    /* CallerID of this address 	     */
	dword	    minbytes;	    /* minimum # bytes waiting for answer    */
	FREEANSWER *next;	    /* Pointer to next struct (NULL=no more) */
};

struct _modem
{
  ANSWER   *answer;	/* Modem answer commands (f.i. 'ATA') (linked list)  */
  char	   *reject;	/* Modem reject answer command string		     */
  char	   *hangup;	/* string to terminate a call and/or reset TE/modem  */
  char	   *setup;	/* string for modem set-up (f.i. 'ATZ|ATS2=255')     */
  char	   *reset;	/* string for modem reset (leaving the program)      */
  char	   *busy;	/* string to busy modem. (f.i. ATH1)		     */
  char	   *faxdial;	/* string to send before making a FAX call	     */
  char	   *terminit;	/* string to send before entering terminal mode      */
  char	   *data;	/* string to send after receiving 'DATA' response    */
  char	   *aftercall;	/* string to send after outbound call is finished    */
  MDMRSP   *rsplist;	/* Modem responses (linked list)		     */
  MDMCAUSE *causelist;	/* Modem error causes (linked list)		     */
  FREEDIAL *freedial;	/* freepoll_dial feature			     */
  FREEANSWER *freeanswer; /* freepoll_answer feature			     */
  char	   *freepattern;/* freepoll dial pattern, stop dialling if only poll */
  int	    port;	/* comport (not used for Atari ST version)	     */
  char	   *comdev;	/* com device name (OS/2)			     */
  boolean   shareport;	/* don't close port when calling external programs   */
  dword     speed;	/* max. speed of modem (incoming calls) 	     */
  word	    divisor;	/* comport uses clock-doubler so set speed /2 /3 /4  */
  boolean   slowmodem;	/* is the modem a bit slow??			     */
  word	    rings;	/* how many rings before answering		     */
  word	    maxrings;	/* maximum number of rings to wait when dialing      */
  word	    timeout;	/* number of seconds to wait for connect (orig&ans)  */
  boolean   lock;	/* speed locked at the max. rate?		     */
  boolean   slowdisk;	/* Suspend async I/O during disk access 	     */
  byte	    handshake;	/* Handshake none, slow (0x09), hard (0x02), or both */
  byte	    fifo;	/* FIFO trigger level (0=off) (not with FOSSIL)      */
  boolean   nofossil;	/* Disable FOSSIL support (for testing purposes)     */
  boolean   winfossil;	/* use WinFOSSIL				     */
  int	    dcdmask;	/* Carrier detect mask (default = 128)		     */
} ;

struct _nodeinfo
{
  int	zone;		/* zone of this node */
  int	net;		/* network of this node */
  int	node;		/* node number of this node */
  int	point;		/* point number (no node B-) */
  int	usepnt; 	/* use HCC kludge? */
  int	pointnet;	/* the pointnet (our/bosses) */
  char	name[60];	/* name of this node */
  char	phone[60];	/* phone number of this node */
  dword speed;		/* max. speed this node supports */
  int	cost;		/* cost of sending a message to this node (in cents) */
  int	nodeflags;	/* flags field from nodelist record */
  char	password[40];	/* password for mailing to this node */
  char	sysop[60];	/* sysop of this node (YOOHOO/2U2) */
  int	capabilities;	/* capabilities of the other side */
  int	options;	/* options of the other side (EMSI) */
  int	nrmail; 	/* nr of mailpackets */
  int	kbmail; 	/* Kbytes mail received */
  int	nrfiles;	/* number of files (no requests!) */
  int	kbfiles;	/* Kbytes files received */
} ;

typedef struct _clip CLIP;
struct _clip {
	char *username, *location, *address, *system;
};

typedef struct _domain DOMAIN;
struct _domain {			/* --------------------------------- */
	char	*domname;		/* Full domain name		     */
	char	*abbrev;		/* 8-char abbreviation		     */
	char	*idxbase;		/* base of idx filename (NULL=none)  */
	byte	 number;		/* Number of index used 	     */
	long	 timestamp;		/* Timestamp of index used	     */
	long	 flagstamp;		/* Timestamp of the change flag file */
	boolean  errors;		/* Can't use THIS index now          */
	char	*hold;			/* Xenia: Outbound directory base    */
	DOMAIN	*next;			/* Ptr to next item, NULL = end list */
};
typedef struct _dompat DOMPAT;
struct _dompat {			/* --------------------------------- */
	char   *pattern;		/* Domain pattern for conversion     */
	DOMAIN *domptr; 		/* Pointer to right domain structure */
	DOMPAT *next;			/* Ptr to next item, NULL = end list */
};
typedef struct _domzone DOMZONE;
struct _domzone {			/* --------------------------------- */
	int	 zone;			/* Zone number			     */
	DOMAIN	*domptr;		/* Pointer to right domain structure */
	DOMZONE *next;			/* Ptr to next item, NULL = end list */
};

struct _dial
{
  int	zone;		/* zonenumber of this node */
  int	net;		/* network number */
  int	node;		/* nodenumber */
  int	point;
  char	phone[60];	/* phonenumber of this node */
  dword speed;		/* speed of this BBS */
};

struct _aka		/* structure for our addresses */
{
  int	zone;
  int	net;		/* netnumber */
  int	node;		/* nodenumber */
  int	point;
  int	pointnet;
  int	usepnt;
};

typedef struct _alias ALIAS;
struct _alias { 		    /* Structure for AKA aliasing	     */
	int    my_aka;		    /* Which of our AKAs to use 	     */
	char  *pattern; 	    /* Address or CallerID pattern to match  */
	ALIAS *next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _pwd PWD;
struct _pwd {			    /* Structure for passwords		     */
	int   zone, net, node, point;
	char *password; 	    /* Password to use for this address      */
	PWD  *next;		    /* Pointer to next struct (NULL=no more) */
};

struct _rem_adr {	/* remote address struct (array) */
	int zone, net, node, point;
	int pointnet;
};

struct _emsi_ident {	/* EMSI extra field 'IDENT', nodelist info */
	char *name, *city, *sysop, *phone, *speed, *flags;
};

typedef struct _iemsi_profile IEMSI_PROFILE;
struct _iemsi_profile {
	char *name, *alias, *location, *data, *voice;
	long birthdate;
};
typedef struct _iemsi_pwd IEMSI_PWD;
struct _iemsi_pwd {
	char	  *pattern;
	char	  *password;
	IEMSI_PWD *next;
};

#define APP_MODEM  0x01
#define APP_BBS    0x02
#define APP_MAIL   0x04
#define APP_SPAWN  0x08
#define APP_BATCH  0x10
#define APP_DROP   0x20
#define APP_EXIT   0x40

typedef struct _extapp EXTAPP;
struct _extapp {		    /* List of ext.progs for modem/bbs/mail  */
	byte	     flags;	    /* SPAWN/BATCH/DROP/EXIT, MODEM/BBS/MAIL */
	PERIOD_MASK  per_mask;	    /* Period activity mask		     */
	char	    *string;	    /* mdm connect str, waitdata or login    */
	char	    *password;	    /* pwd: NULL=none, ""=loginCR, "*"=any   */
	char	    *batchname;     /* Full name of batchfile to call	     */
	char	    *description;   /* Description string of this thingie    */
	EXTAPP	    *next;	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _dialxlat DIALXLAT;
struct _dialxlat {		    /* Dial translation information	     */
	char	 *number;	    /* String to scan at start of phone no.  */
	char	 *prefix;	    /* Prefix to add			     */
	char	 *suffix;	    /* Suffix to add			     */
	char	 *pattern;	    /* Pattern of z:n/n, phone # or flags    */
	DIALXLAT *next; 	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _prefixsuffix PREFIXSUFFIX;
struct _prefixsuffix {		    /* Dial prefix/suffix commands	     */
	char	     *pattern;	    /* Pattern of z:n/n, phone # or flags    */
	char	     *prefix;	    /* Prefix to add			     */
	char	     *suffix;	    /* Suffix to add			     */
	PREFIXSUFFIX *next;	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _phone PHONE;
struct _phone { 		    /* Phone numbers (override or multiline) */
	int    zone, net,	    /* Address of system		     */
	       node, point;
	char  *number;		    /* Phone number to use		     */
	char  *nodeflags;	    /* Nodelist style flags to use	     */
	PHONE *next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _okdial OKDIAL;
struct _okdial {		    /* Patterns allowed to call, or not      */
	boolean  allowed;	    /* Set if this one allowed, clear if not */
	char	*pattern;	    /* Address, flags or phone# of system    */
	OKDIAL	*next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _dialback DIALBACK;
struct _dialback {		    /* Dialback feature 		     */
	byte	     flags;	    /* APP_BBS=bbs event / APP_MAIL=always   */
	PERIOD_MASK  per_mask;	    /* Period activity mask		     */
	char	    *number;	    /* Phone number to use		     */
	char	    *nodeflags;     /* Nodelist style flags to use	     */
	char	    *string;	    /* String in waitdata		     */
	char	    *password;	    /* pwd: NULL=none, ""=loginCR, "*"=any   */
	DIALBACK    *next;	    /* Pointer to next struct (NULL=no more) */
};

#if 0
typedef struct _returncall RETURNCALL;
struct _returncall {		    /* Returncall feature		     */
	char	   *pattern;	    /* Address or phone# of dialback system  */
	char	   *string;	    /* String to send to ask for dialback    */
	RETURNCALL *next;	    /* Pointer to next struct (NULL=no more) */
};
#endif

typedef struct _ftpmail FTPMAIL;
struct _ftpmail {		    /* FTPMAIL (client) facility	     */
	int	 zone, net,	    /* Address of system		     */
		 node, point;
	char	*host;		    /* Host   for FTP login		     */
	int	 port;		    /* Port   for FTP login		     */
	char	*userid;	    /* UserID for FTP login		     */
	char	*passwd;	    /* PassWD for FTP login		     */
	FTPMAIL *next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _script SCRIPT;
struct _script {		    /* List of mailer/uucp/terminal scripts  */
	char   *tag;		    /* Name of this script		     */
	char   *string; 	    /* The script itself (see script.c)      */
	SCRIPT *next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _mailscript MAILSCRIPT;
struct _mailscript {		    /* List of when to use mailscript....    */
	char	   *loginname;	    /* Login name to use in script	     */
	char	   *password;	    /* Password to use in script	     */
	char	   *pattern;	    /* FidoNet address / phone# 	     */
	SCRIPT	   *scriptptr;	    /* Ptr to script that is to be used      */
	MAILSCRIPT *next;	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _termscript TERMSCRIPT;
struct _termscript {		    /* List of when to use termscript....    */
	char	   *loginname;	    /* Login name to use in script	     */
	char	   *password;	    /* Password to use in script	     */
	char	   *pattern;	    /* FidoNet address / phone# / uucpname   */
	SCRIPT	   *scriptptr;	    /* Ptr to script that is to be used      */
	TERMSCRIPT *next;	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _servreq SERVREQ;
struct _servreq {		    /* Server requests			     */
	char	*filepat;	    /* Filename pattern to check for	     */
	char	*progname;	    /* Executable or batch to start	     */
	char	*description;	    /* Name/description of this service      */
	SERVREQ *next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _servlst SERVLST;
struct _servlst {
	char	*pathname;	    /* Path/filename of server request file  */
	SERVREQ *servptr;	    /* Pointer to correct SERVREQ record     */
	SERVLST *next;		    /* Pointer to next struct (NULL=no more) */
};

typedef struct _timesync TIMESYNC;
struct _timesync {
	int	  zone, net,	    /* Address of system		     */
		  node, point;
	long	  timediff,	    /* Timezone difference in seconds	     */
		  maxdiff;	    /* Maximum clock difference in seconds   */
	boolean   dstchange;	    /* Accept DST changes (clock +/- 1 hour) */
	TIMESYNC *next; 	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _mailhour MAILHOUR;
struct _mailhour {
	int	     zone;	    /* Zone # to which this mailhour applies */
	PERIOD_MASK  per_mask;	    /* Period activity mask		     */
	MAILHOUR    *next;	    /* Pointer to next struct (NULL=no more) */
};

typedef struct _outblist OUTBLIST;
struct _outblist {		    /* List of stuff in the outbound dirs    */
	short	  zone, net,	    /* Address of system		     */
		  node, point;
	word	  types;	    /* Types of outbound stuff OUTB_xxxxx    */
	byte	  status;	    /* Status of this item		     */
	dword	  waitbytes,	    /* Number of outbound bytes on hold      */
		  outbytes;	    /* Number of outbound bytes -the rest-   */
	long	  oldestitem;	    /* Timestamp of oldest outbound item     */
	word	  noco, conn;	    /* Number of no connects, connects	     */
	long	  lastattempt;	    /* Timestamp (UNIX) of last call attempt */
	OUTBLIST *next; 	    /* Pointer to next struct (NULL=no more) */
};
#define OUTB_NORMAL	0x0001	    /* Types:  Normal			     */
#define OUTB_CRASH	0x0002	    /*	       Crash			     */
#define OUTB_IMMEDIATE	0x0004	    /*	       Immediate		     */
#define OUTB_HOLD	0x0008	    /*	       Hold / Wait for Pickup	     */
#define OUTB_DIRECT	0x0010	    /*	       Direct (behaviour as normal)  */
#define OUTB_ALONG	0x0020	    /*	       Along with mail		     */
#define OUTB_POLL	0x0040	    /*	       Poll marker		     */
#define OUTB_REQUEST	0x0080	    /*	       Request			     */
#define OUTB_QUERY	0x0100	    /*	       Query response		     */
#define OUTB_OTHER	     0	    /* Status: - All remaining		     */
#define OUTB_UNKNOWN	     1	    /*	       ! Unkown destination	     */
#define OUTB_TOOBAD	     2	    /*	       x Too many bad tries	     */
#define OUTB_TRIED	     3	    /*	       * Tried, will call again      */
#define OUTB_WILLCALL	     4	    /*	       + Will call this one	     */
#define OUTB_NOOUT	     5	    /*	       r Receive only event	     */
#define OUTB_CM 	     6	    /*	       c Would go, but is non-CM     */
#define OUTB_LOCAL	     7	    /*	       l Would go, but is non-local  */
#define OUTB_MAILHOUR	     8	    /*	       # Would go, but only in #ZMH  */
#define OUTB_OKDIAL	     9	    /*	       d Would go, but not okdial    */
#define OUTB_TOOSMALL	    10	    /*	       < Would go, but too small yet */
#define OUTB_DONE	    11	    /*	       $ Done, removed in next scan  */
#define OUTB_WHOKNOWS	    12	    /*	       ? We'll see at next scan      */


/* global variable 'bbstype' -------------------------------------------- */
#define BBSTYPE_NONE	0
#define BBSTYPE_MAXIMUS 1
#define BBSTYPE_RA	2


struct _extrainfo	/* to store the info needed for the statuswindow */
{
  word	calls;		/* total number of calls	     */
  word	human;		/* number of human callers (BBS)     */
  word	mailer; 	/* number of mailer calls (received) */
  word	called; 	/* number of times we called	     */
  word	nrmail; 	/* nr of mailpackets		     */
  word	kbmail; 	/* Kbytes mail received 	     */
  word	nrfiles;	/* number of files (no requests!)    */
  word	kbfiles;	/* Kbytes files received	     */
};

struct _win_colours {
	      word normal, border, title, template, hotnormal;
};

struct _menu_colours {
	      word normal, hotnormal, select, inactive, border, title;
};

struct _select_colours {
	      word normal, select, border, title, scrollbar, scrollselect;
};

struct _xenwin_colours {
       struct _win_colours    desktop,
			      top, schedule, stats, status, outbound,
			      modem, log, mailer,
			      term, statline, file, help, script;
       struct _menu_colours   menu;
       struct _select_colours select;
};

/*
 * And now for prototypes!!! Can be done at this place only because ALL
 * structures defined above should be known (for prototypes).
 */
#include "proto.h"


#if WINNT
#define XENIA_OS    "Win"
#define XENIA_SFX   "/Win"
#elif OS2
#define XENIA_OS    "OS/2"
#define XENIA_SFX   "/2"
#elif PC
#define XENIA_OS    "DOS"
#define XENIA_SFX   "/DOS"
#elif ATARIST
#define XENIA_OS    "ST"
#define XENIA_SFX   "/ST"
#endif

#define PRGNAME     "Xenia Mailer"
#define VERSION     "1.99.01"
#define isXENIA     0xfe	/* A not really ORIGINAL IFNA product code!  */
#define XEN_MAJ     1
#define XEN_MIN     99
#define XEN_MINOR   01		/* 1.92.06 24 Oct 91 AGL First EMSI/HYDRA    */
				/* 1.92.07 07 Nov 91 AGL Bugs, Skip method   */
				/* 1.92.08 22 Jun 92 AGL FSC-0048 (type 2+)  */
				/* 1.92.09 11 Jul 92 AGL MODEL=L + async lib */
				/* 1.92.10 17 Aug 92 AGL HYDRA Mk.II is here */
				/* 1.92.11 13 Oct 92 AGL HYDRA Mk.III now    */
				/* 1.97.01 17 Nov 92 JK  == 1.92.11	     */
				/* 1.97.02 29 Dec 92 AGL Continued work      */
				/* 1.97.03 10 Mar 93 AGL From Jac	     */
				/* 1.97.04 24 Mar 93 AGL SEAlink fixes	     */
				/* 1.97.05 07 Apr 93 AGL Nodelist opening    */
				/*			 ... development     */
				/* 1.97.06 06 May 93 AGL Multiline!!	     */
				/* 1.97.07 05 Jun 93 AGL .OK file, IEMSI+cht */
				/* 1.97.08 08 May 93 AGL X00fossil extention */
				/* 1.97.09 14 Sep 93 AGL Servreq, FAXrcv     */
				/* 1.97.10 01 Nov 93 AGL Allow config mdmrsp */
				/* Split date for sources		     */
				/* 1.97.11 03 Dec 93 AGL bufbyte & hangprot. */
				/* 1.97.12 09 Jan 94 AGL various, OS/2 start */
				/* 1.97.13 25 Mar 94 AGL New outbound scan   */
				/* ========================================= */
				/* 1.97.14 15 Apr 94 AGL New name: Xenia     */
				/* 1.97.15 24 May 94 AGL new screen layout   */
				/* 1.97.16 29 Jun 94 AGL new beta version    */
				/* 1.97.17 01 Aug 94 AGL removed old 3D code */
				/* 1.97.18 18 Aug 94 AGL various bugfixes    */
				/* 1.97.19 12 Sep 94 AGL various bugfixes    */
				/* 1.97.20 30 Sep 94 AGL various bugfixes    */
				/* 1.97.21 05 Oct 94 AGL various bugfixes    */
				/* 1.97.22 15 Oct 94 AGL various bugfixes    */
				/* 1.97.23 23 Oct 94 AGL DOS ver. less mem?  */
				/* 1.97.24 27 Oct 94 AGL beta sendout of .23 */
				/* 1.97.25 07 Nov 94 AGL feature lock	     */
				/* 1.97.26 30 Nov 94 AGL various bugfixes    */
				/* 1.97.27 22 Dec 94 AGL various bugfixes    */
				/* 1.97.28 16 Jan 95 AGL selective answer    */
				/* 1.98    18 Jan 95 AGL ready for release   */
				/* ========================================= */
				/* 1.98.01 21 Sep 95 AGL key code added      */
				/*	   10 Oct 95 AGL released	     */
				/* 1.98.02 17 Oct 95 AGL internal OS/2 comms */
				/* 1.98.03 21 Oct 95 AGL various bugfixes    */
				/* 1.98.04 09 Mar 96 AGL bugfix, add periods */
				/* 1.98.05 04 Apr 96 AGL 1st evt->per cvt.   */
				/* 1.98.06 29 Apr 96 AGL Win95/NT port try!  */
				/* 1.99.00 1998-1999 AGL OpenSource prepare  */
				/* 1.99.01 May 2001  AGL OpenSource          */

#define N_FLAGS     12		/* Number of different mail flags */

#ifdef XENIA_MAIN	      /* only present in Xenia external for the rest */

/* For the main program itself... */
WIN_IDX top_win, schedule_win, status_win, outbound_win,
	modem_win, log_win, help_win, mailer_win, file_win, script_win;
struct _xenwin_colours xenwin_colours;
/* char progname = "Xenia";	/+ defined in program body +/ */
#if 0
char	 *xenpoint_ver = NULL;	/* XenPoint version string		     */
char	 *xenpoint_reg = NULL;	/* XenPoint registration string 	     */
#endif
char *funkeys[13];		/* function keys in terminalmode	     */
char *homepath = "";		/* homepath for Xenia			     */
char *last_in  = NULL;		/* Address of last inbound mailer call	     */
char *last_bbs = NULL;		/* User name of last inbound BBS call	     */
char *last_out = NULL;		/* Address of last outbound mailer call      */
char   *apptext       = NULL;	/* Tell humums how to enter application      */
char   *noapptext     = NULL;	/* Tell humums application not available now */
char   *loginprompt   = NULL;	/* unix-style login prompt (NULL for none)   */
word	apperrorlevel = 3;	/* Errorlevel to start application via batch */
EXTAPP *extapp	      = NULL;	/* External applications for modem/bbs/mail  */
char   *ringapp       = NULL;	/* Application to call before answering ring */
char   *mailpack      = NULL;	/* Ext.app to start at beginning of event    */
char   *mailprocess   = NULL;	/* Ext.app to start after mailsession	     */
#if OS2
char	*pipename = NULL;
HSNOOP	 hsnoop   = (HSNOOP) NULL;
HPIPE	 hpMCP	  = (HPIPE) NULL;
char	*MCPbase  = NULL;
word	 MCPtasks = 0;
#endif
char *download="";		/* path for download files		     */
struct _dial dial[20];		/* root of dial list			     */
struct _extrainfo extra;	/* the extra information		     */
char	     *xlatdash = NULL;	/* Translate first '-' in phonenumber to ?   */
DIALXLAT     *dialxlat = NULL;	/* Dial translation information 	     */
PREFIXSUFFIX *prefixsuffix = NULL;  /* Dial prefix/suffix commands	     */
PHONE	     *phone	 = NULL;/* Phone numbers (override and multiline)    */
OKDIAL	     *okdial	 = NULL;/* Patterns allowed to call, or not	     */
DIALBACK     *dialback	 = NULL;/* Dialback feature			     */
#if 0
RETURNCALL   *returncall = NULL;/* Returncall feature			     */
#endif
FTPMAIL      *ftpmail	 = NULL;/* List of FTP mail (client) entries	     */
SCRIPT	     *script	 = NULL;/* List of mailer/uucp/terminal scripts      */
MAILSCRIPT   *mailscript = NULL;/* List of when to use mailscript....	     */
TERMSCRIPT   *termscript = NULL;/* List of when to use termscript....	     */

/* For module GETNODE */
struct _nodeinfo ourbbs;	/* information about our BBS */
int    ourbbsflags=0;		/* what we want and what we don't want... */
struct _nodeinfo remote;	/* information about the other side */
DOMAIN	*domain  = NULL;	/* Domain / nodelist specifications  */
DOMPAT	*dompat  = NULL;	/* Domain replacement patterns	     */
DOMZONE *domzone = NULL;	/* Domain zone assignments	     */
boolean  nodomhold = false;	/* Don't use domain directories      */

/* For module MODEM.C */
struct _modem mdm;		/* information needed to make the modem work */
long init_timer  = 0L;		/* Modem re-init timer	 */
long waitforline = 0L;		/* Wait time before really wanting dialtone */
/*long rescan = 0L;*/		    /* Outbound rescan timer */
long scan_stamp = 0L;		/* Last outbound scan / semaphore timestamp */
char	mdmconnect[128];	/* complete modem connect string  */
char	errorfree[128]; 	/* is it an MNP connection?	  */
char	errorfree2[128];	/* Extra MNP/protocol information */
char	callerid[128];		/* Caller ID			  */
CLIP	clip;			/* Info derived from Caller ID	  */
boolean arqlink;		/* link has ARQ, MNP or V42	  */
boolean isdnlink;		/* link over ISDN		  */
boolean iplink; 		/* link over IP (Internet)	  */
int  ftb_user=0;		/* wanna start BBS? */

/* For modules SEALINK.C and XFER.C */
int ackless = 0;		/* true if ACKs not required	    */
int igncarr = 0;		/* ignore carrier status	    */
int xmodem  = 0;		/* plain xmodem wanted (no NAK EOT) */
int modem   = 0;		/* modem7 filename wanted	    */
int telink  = 0;		/* telink style block zero wanted   */
		     /* SEAlink mode select if !xmodem & !modem & !telink */
int sealink = 0;		/* other end uses sealink?	    */
int  found_zone  = 0;		/* zonenumber from packet  */
int  found_net	 = 0;		/* netnumber from packet   */
int  found_node  = 0;		/* nodenumber from packet  */
int  found_point = 0;		/* pointnumber from packet */
char found_pwd[8];		/* password from packet    */
int  kbtransfered;		/* Kbytes received/sent */

/* For system dependend module (SYS520ST.C) */
int loglevel;			/* minimum level of message for logging */
FILE *log=NULL; 		/* logfile */
FILE *cost=NULL;		/* cost-log */
char *capture=NULL;		/* terminal capture file */
int	com_handle;		/* comport file handle or port number */
boolean com_suspend = false;	/* suspend but don't close comport    */
dword	cur_speed;		/* speed we're working with           */
dword	line_speed;		/* actual line speed (with MNP/V42)   */
int  blank_secs = 0;		/* number of secs to wait before blanking */
long blank_time = 0L;		/* timer for blanks */
int  sema_secs	= 0;		/* number of seconds between semaphore scans */
long sema_timer = 0L;		/* timer for semaphore scan		     */

/* for MSESSION.C (mail session) */
int  mail_only; 		/* is this window mail only? */
char *banner=NULL;		/* banner for this BBS */
int pickup;			/* do we want to pickup mail? */
char *filepath="";		/* path for received files */
char *safepath = NULL;		/* path for received files (passworded) */
char *localpath = NULL; 	/* path for local files (fax msgs, etc) */
char *inbound;			/* current inbound (filepath or safepath) */
char *hold="";			/* path for mail we wanna send */
char *fax_indir = NULL; 	/* FAX inbound directory	    */
word  fax_format = 0;		/* FAX file format/naming	    */
word  fax_options = 0;		/* FAX options (off, lock, carrier) */
int  mail=0;			/* is this supposed to be a mail packet? */
int  caller=0;			/* we originated this call? */
char *made_request=NULL;	/* did we request anything? */
int  got_request=0;		/* did we get a file-request? */
/*char *ext_flag="NOFCIHWDAPRQ";*/  /* sorts of mail we've got & Request + Query */
char *ext_flag="PICDOFNAHWRQ";	/* sorts of mail we've got & Request + Query */
int got_msession;		/* got inbound mailsession */
int got_bundle; 		/* got .pkt file */
int got_arcmail;		/* got arcmail packet */
int got_mail;			/* got mail */
int got_fax;			/* got fax */
int (*wzgetfunc)(void), 	/* wazoo send/receive routines */
    (*wzsendfunc)(char *, char *, int, int, int);

/* for TCONF.C (configuration parser) */
char *logname=NULL;		/* name of the logfile			     */
struct _aka  aka[50];		/* addresses of this BBS		     */
int	     nakas = 0; 	/* number of addresses used		     */
PWD	    *pwd   = NULL;	/* root for passwords			     */
ALIAS	    *alias = NULL;	/* root for AKA aliasing		     */
int hotstart = 0;		/* startup with (mail) caller connected      */
int un_attended;		/* mailer mode, no local user		     */
int command_line_un;		/* unattended spec. on commandline	     */
int cmdlineterm = 0;		/* call terminal mode on commandline	     */
int cmdlineanswer = 0;		/* answer immediately on startup, no inits   */
int no_overdrive;		/* do not use ackless mode		     */
char	*reqpath = NULL;	/* file where REQUEST.CFG .IDX .DIR reside   */
char	*reqprog = NULL;	/* External (SRIF spec) request processor    */
int	 FTB_allowed;		/* set by BBS service, breaks NO_BBS event   */
char	*FTB_dos = NULL;	/* init by DOS serv, execute instead of BBS  */
SERVREQ *servreq = NULL;	/* Server requests			     */
SERVLST *servlst = NULL;	/* Server list (during mailsessions only)    */
TIMESYNC *timesync = NULL;	/* Time Synchronisation list		     */
long	  gmtdiff  = 0L;	/* difference from local time to GMT	     */
MAILHOUR *mailhour = NULL;	/* Zone Mail Hour definitions		     */
OUTBLIST *outblist = NULL;	/* List of stuff in outbound dirs	     */
#if 0
int sdmach = 0; 		/* Zmodem Special Delivery <tm> max.MACHs    */
dword machbps = 0;		/* above this speed,use limitmach not sdmach */
int limitmach = 0;		/* used if machbps && speed >= machbps	     */
#endif
long zmodem_txwindow;
dword hydra_minspeed, hydra_maxspeed;
char *hydra_nofdx, *hydra_fdx;
dword hydra_options;
long hydra_txwindow, hydra_rxwindow;
boolean hydra_debug = false;
boolean alloc_debug = false;
int emsi_adr = 50;		/* number of addresses to pass with EMSI     */
struct _emsi_ident emsi_ident;	/* EMSI extra field 'IDENT', nodelist info   */
struct _rem_adr rem_adr[50];	/* Remote address struct array		     */
int		num_rem_adr;	/* Number of remote addresses		     */
IEMSI_PROFILE  iemsi_profile;	/* IEMSI user profile			     */
IEMSI_PWD     *iemsi_pwd = NULL;/* IEMSI user passwords per node/tel#	     */
word	task = 0;		/* Task number (or 0 for single-line)	     */
boolean noevttask = false;	/* Don't use tasknumber for XENIA.EVT        */
char *flagdir = NULL;		/* Flag directory (config or Xenia homepath) */
char *bbspath = NULL;		/* Path to home directory of BBS program     */
word  bbstype = BBSTYPE_NONE;	/* Type of BBS program			     */

/* PTT telephone cost pulse counter - by Arjen G. Lentz, 2 Dec 1992	     */
/* Cost pulse connected to Ack line of printer port (can generate interrupt) */
	 word ptt_port; 	/* Printer port address (3BC,378,278,free)   */
	 word ptt_irq;		/* Printer hardware interrupt (7,5,free)     */
volatile word ptt_units;	/* The actual counter - VOLATILE!	     */
	 word ptt_cost; 	/* Cost per unit (calculation for log)	     */
	 char ptt_valuta[16];	/* Valuta string to use 		     */

/* for PERIOD.C (World time period scheduler) */
PERIOD *periods[MAX_PERIODS];	/* pointeres to periods */
byte	num_periods = 0;	/* number of periods	*/
/* for SCHED.C (scheduler) */
EVENT  *e_ptrs[128];		/* pointers to events	*/
int	num_events     = 0;	/* number of events	*/
int	cur_event      = -1;	/* active event 	*/
int	got_sched      = 0;	/* sched. read already	*/
int	noforce        = 0;	/* no forced events	*/
int	force	       = 0;	/* oposite of above	*/
int	max_connects   = 3;	/* max. connects	*/
int	max_noconnects = 35;	/* max. no connects	*/
int	redo_dynam     = 0;	/* redo dynamic events	*/
int	local_cost     = 0;	/* cost for local mail	*/

/* several messages */
char *msg_oabort     = "Aborted by operator";
char *msg_carrier    = "Carrier lost";
char *msg_timeout    = "TIMEOUT";
char *msg_fatimeout  = "Fatal TIMEOUT";
char *msg_manyerrors = "Too many errors";
char *msg_cancelled  = "Aborted by other side";
char *msg_noyoohoo   = "No YooHOO/2U2";
char *msg_success    = "Successful";
char *msg_badfname   = "Bad filename";
char *msg_complfname = "Filename complete";
char *msg_garbage    = "Retry %d: Garbage %02x";
char *msg_enquiry    = "Enquiry";
char *msg_datacrc    = "Data CRC error";
char *msg_endbatch   = "End of Batch";
char *msg_overdrive  = "Overdrive disengaged";

#endif

#ifndef XENIA_MAIN

/* For the main program */
extern WIN_IDX top_win, schedule_win, status_win, outbound_win,
	       modem_win, log_win, help_win, mailer_win, file_win, script_win;
extern struct _xenwin_colours xenwin_colours;
extern char   progname[];
extern char   *funkeys[13];
extern char   *homepath;
#if 0
extern char	*xenpoint_ver;
extern char	*xenpoint_reg;
#endif
extern char   *last_in;
extern char   *last_bbs;
extern char   *last_out;
extern char   *apptext;
extern char   *noapptext;
extern char   *loginprompt;
extern word    apperrorlevel;
extern EXTAPP *extapp;
extern char   *ringapp;
extern char   *mailpack;
extern char   *mailprocess;
#if OS2
extern char    *pipename;
extern HSNOOP	hsnoop;
extern HPIPE	hpMCP;
extern char    *MCPbase;
extern word	MCPtasks;
#endif
extern char *download;		/* path for downloaded files */
extern struct _dial dial[20];	/* root of dial list */
extern struct _extrainfo extra; /* the extra information */
extern char	    *xlatdash;
extern DIALXLAT     *dialxlat;
extern PREFIXSUFFIX *prefixsuffix;
extern PHONE	    *phone;
extern OKDIAL	    *okdial;
extern DIALBACK     *dialback;
#if 0
extern RETURNCALL   *returncall;
#endif
extern FTPMAIL	    *ftpmail;
extern SCRIPT	    *script;
extern MAILSCRIPT   *mailscript;
extern TERMSCRIPT   *termscript;

/* For module GETNODE */
extern struct _nodeinfo ourbbs; /* information about our BBS */
extern int    ourbbsflags;	/* what we want and what we don't want... */
extern struct _nodeinfo remote; /* information about the other side */
extern DOMAIN  *domain; 	/* Domain / nodelist specifications  */
extern DOMPAT  *dompat; 	/* Domain replacement patterns	     */
extern DOMZONE *domzone;	/* Domain zone assignments	     */
extern boolean	nodomhold;	/* Don't use domain directories      */

/* For module MODEM.C */
extern struct _modem mdm;
extern long init_timer;
extern long waitforline;
/*extern long rescan;*/
extern long scan_stamp;
extern char    mdmconnect[];
extern char    errorfree[];
extern char    errorfree2[];
extern char    callerid[];
extern CLIP    clip;
extern boolean arqlink;
extern boolean isdnlink;
extern boolean iplink;
extern int     ftb_user;

/* For modules SEALINK.C and XFER.C */
extern int  ackless;
extern int  igncarr;
extern int  xmodem;
extern int  modem;
extern int  telink;
extern int  sealink;
extern int  found_zone;
extern int  found_net;
extern int  found_node;
extern int  found_point;
extern char found_pwd[8];
extern int  kbtransfered;

/* For system dependend module (SYS520ST.C) */
extern int loglevel;
extern FILE *log;
extern FILE *cost;
extern char *capture;
extern int     com_handle;
extern boolean com_suspend;
extern dword   cur_speed;
extern dword   line_speed;
extern long blank_time;
extern int  blank_secs;
extern int  sema_secs;
extern long sema_timer;

/* for MSESSION.C (mail session) */
extern int mail_only;		/* window is mail only? */
extern char *banner;		/* banner for this BBS */
extern int pickup;		/* do we want to pickup mail? */
extern char *filepath;		/* path for attached files */
extern char *safepath;		/* path for attached files (passworded) */
extern char *localpath; 	/* path for local files (fax msgs, etc) */
extern char *inbound;		/* current inbound (filepath or safepath) */
extern char *hold;		/* path for mail we wanna send */
extern char *fax_indir;
extern word  fax_format;
extern word  fax_options;
extern int mail;		/* is this supposed to be a mail packet? */
extern int caller;		/* did we originate this call? */
extern char *made_request;	/* did we request anything? */
extern int  got_request;	/* did we get a file-request? */
extern char *ext_flag;		/* sorts of mail we've got */
extern int got_msession;
extern int got_bundle;
extern int got_arcmail;
extern int got_mail;
extern int got_fax;
extern int (*wzgetfunc)(void),	/* wazoo send/receive routines */
	   (*wzsendfunc)(char *, char *, int, int, int);

/* for TCONF.C (configuration parser) */
extern char *logname;		/* name of the logfile */
extern struct _aka  aka[];	/* addresses for this node */
extern int	    nakas;	/* number of addresses used */
extern PWD	   *pwd;
extern ALIAS	   *alias;
extern int hotstart;		/* startup with (mail) caller connected      */
extern int un_attended; 	/* mailer mode, no local user		     */
extern int command_line_un;	/* unattended spec. on commandline	     */
extern int cmdlineterm;
extern int cmdlineanswer;
extern int no_overdrive;	/* do not use ackless mode		     */
extern char *reqpath;
extern char *reqprog;
extern int	FTB_allowed;
extern char    *FTB_dos;
extern SERVREQ *servreq;
extern SERVLST *servlst;
extern TIMESYNC *timesync;
extern long	 gmtdiff;
extern MAILHOUR *mailhour;
extern OUTBLIST *outblist;
#if 0
extern int sdmach;		/* Zmodem Special Delivery <tm> max.MACHs    */
extern dword machbps;		/* above this speed,use limitmach not sdmach */
extern int limitmach;		/* used if machbps && speed >= machbps	     */
#endif
extern long zmodem_txwindow;
extern dword hydra_minspeed, hydra_maxspeed;
extern char *hydra_nofdx, *hydra_fdx;
extern dword hydra_options;
extern long hydra_txwindow, hydra_rxwindow;
extern boolean hydra_debug;
extern boolean alloc_debug;
extern int emsi_adr;
extern struct _emsi_ident emsi_ident;
extern struct _rem_adr rem_adr[];
extern int	       num_rem_adr;
extern IEMSI_PROFILE  iemsi_profile;
extern IEMSI_PWD     *iemsi_pwd;
extern word    task;
extern boolean noevttask;
extern char *flagdir;
extern char *bbspath;
extern word  bbstype;

extern		word ptt_port;
extern		word ptt_irq;
extern volatile word ptt_units;
extern		word ptt_cost;
extern		char ptt_valuta[];

/* for PERIOD.C (World time period scheduler) */
extern PERIOD *periods[];
extern byte    num_periods;
/* for SCHED.C (scheduler) */
extern EVENT *e_ptrs[];
extern int num_events;
extern int cur_event;
extern int got_sched;
extern int noforce;
extern int force;
extern int max_connects;
extern int max_noconnects;
extern int redo_dynam;
extern int local_cost;

/* several messages */
extern char *msg_oabort;
extern char *msg_carrier;
extern char *msg_timeout;
extern char *msg_fatimeout;
extern char *msg_manyerrors;
extern char *msg_cancelled;
extern char *msg_noyoohoo;
extern char *msg_success;
extern char *msg_badfname;
extern char *msg_complfname;
extern char *msg_garbage;
extern char *msg_enquiry;
extern char *msg_datacrc;
extern char *msg_endbatch;
extern char *msg_overdrive;

#endif

/*------------------------------------------------------------------------*/
/* YOOHOO<tm> CAPABILITY VALUES 					  */
/*------------------------------------------------------------------------*/
#define DIETIFNA   0x0001  /* Can do fast LoTek       0000 0000 0000 0001 */
#define FTB_USER   0x0002  /* Full Tilt boogie toggle 0000 0000 0000 0010 */
#define ZED_ZIPPER 0x0004  /* Can do Zmodem plain 1K  0000 0000 0000 0100 */
#define ZED_ZAPPER 0x0008  /* Can do Zmodem upto 8K   0000 0000 0000 1000 */
#define DOES_JANUS 0x0010  /* Can do JANUS fdx proto  0000 0000 0001 0000 */
#define DOES_HYDRA 0x0020  /* Can do HYDRA fdx proto  0000 0000 0010 0000 */
#define EMSI_SLK   0x0040  /* EMSI SLK, mask YooHoo!  0000 0000 0100 0000 */
#define EMSI_DZA   0x0080  /* EMSI DZA, mask YooHoo!  0000 0000 1000 0000 */
#define Bit_8	   0x0100  /* reserved by Opus	      0000 0001 0000 0000 */
#define Bit_9	   0x0200  /* reserved by Opus	      0000 0010 0000 0000 */
#define Bit_a	   0x0400  /* reserved by Opus	      0000 0100 0000 0000 */
#define Bit_b	   0x0800  /* reserved by Opus	      0000 1000 0000 0000 */
#define Bit_c	   0x1000  /* reserved by Opus	      0001 0000 0000 0000 */
#define Bit_d	   0x2000  /* reserved by Opus	      0010 0000 0000 0000 */
#define DO_DOMAIN  0x4000  /* system name has domain  0100 0000 0000 0000 */
#define WZ_FREQ    0x8000  /* WZ file req. ok	      1000 0000 0000 0000 */

/* Remote options (EMSI) ------------------------------------------------ */
#define EOPT_PRIMARY	0x0001	/* Send to primary address only 	  */
#define EOPT_HOLDALL	0x0002	/* Hold all traffic			  */
#define EOPT_HOLDECHO	0x0004	/* Hold echo (compressed) mail		  */
#define EOPT_REQFIRST	0x0008	/* Only REQ in 1st Hydra batch		  */
#define EOPT_BADAPPLE	0x4000	/* Only accept mail; but no send, no reqs */

/* Definitions for ourbbsflags, these control the way we 'live' --------- */
#define NO_SMART_MAIL	0x0001	/* Do we want to use YooHOO?		  */
#define NO_REQ_ON	0x0002	/* Don't allow requests on our money      */
#define NO_INIT 	0x0004	/* Do not initialise modem every 10 min   */
/*#define NO_EXT_REQ	  0x0008*/  /* Schedule doesn't allow extended reqs   */
#define NO_EMSI 	0x0010	/* Do we want to use EMSI?		  */
#define NO_COLLIDE	0x0020	/* No call collision detection		  */
#define NO_PWFORCE	0x0040	/* Don't do pw check when dialling out    */
#define NO_DIRSCAN	0x0080	/* Don't scan outbound dirs in this task  */
#define NO_SETDCB	0x0100	/* Don't set DCB info (OS/2 only)         */

/* Definitions for modem responses -------------------------------------- */
#define MDMRSP_BADCALL	  0x80
#define MDMRSP_OK	  0x40
#define MDMRSP_CONNECT	  0x20
#define MDMRSP_SET	  0x21	/* CONNECT + flag for SET */
#define MDMRSP_FAX	  0x22	/* CONNECT + flag for FAX */
#define MDMRSP_DATA	  0x24	/* DATA - USR connect ;-( */
#define MDMRSP_NOCONN	  0x10
#define MDMRSP_NODIAL	  0x08
#define MDMRSP_COLLIDE	  0x04
#define MDMRSP_RING	  0x02
#define MDMRSP_ERROR	  0x01
#define MDMRSP_IGNORE	     0

/* FAX options (fax_options) -------------------------------------------- */
#define FAXOPT_DISABLE	0x0001	/* Disable internal FAX routines	  */
#define FAXOPT_LOCK	0x0002	/* Also keep speed locked on FAX connects */
#define FAXOPT_DCD	0x0004	/* Modem provides DCD during FAX connects */
#define FAXOPT_NOSWAP	0x0008	/* direct bit order - don't do bitswap    */
#define FAXOPT_EOLPAD	0x0010	/* Add EOL padding at end of each page	  */
/* FAX format (fax format) ---------------------------------------------- */
#define FAXFMT_ZFAX	     0	/* ZyXEL ZFAX	.FAX compatible format	  */
#define FAXFMT_QFX	     1	/* QuickLink II .QFX compatible format	  */
#define FAXFMT_RAW	     2	/* Raw fax format			  */

/* old shit from nodelist.h - only viaboss,viahost,down used I think ---- */
#define VIABOSS 0x0004	/* POINT:    phone/speed/cost of boss/coordinator */
#define VIAHOST 0x0002	/* PVT,HOLD: phone/speed/cost of host/coordinator */
#define DOWN	0x0001	/* DOWN,KENL: not allowed to send mail		  */


/* end of xenia.h */
