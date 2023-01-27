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


#include <stdarg.h>
#include "xenia.h"


extern long lasttime;


/* CRC routines */

word crc_update(word crc,word crc_char)
{
	 register long x;
	 register long temp;
	 register int i;

	 x=(long)crc;
	 x= x<<8;		/* Schuif CRC omhoog */
	 temp= crc_char;
	 temp &=0x0FFL;
	 x = x|temp;		 /* Zet char onderin */

	 for(i=0;i<8;i++) {
		 x = x << 1;
		 if(x & 0x01000000L)  x^=0x01102100L;
		 }
	 temp= x>>8;
	 return( (word)temp );
}


word crc_finish(word crc) /* end the CRC */
{
	 word temp;

	 temp = crc_update(crc,0);
	 temp = crc_update(temp,0);
	 return((word) (temp & 0xFFFF));
}


/* Feed with year>0, month 0..11 (!!), day 1..31 */
int dayofweek (int year, int month, int day)
{
	dword df1;

	df1 = 365L * year + 31L * month + day + 1;
	if (month > 1) {
	   df1 -= (4 * (month + 1) + 23 ) / 10;
	   year++;
	}
	df1 += (year - 1) / 4;

	df1 -= (year - 1) / 100 - (year - 1) / 400;	      /* friday == 1 */
	df1 += 5;

#if 0
/* Y2K prob: from 1 Mar 2000 the weekday calculation screwed up.
   Solved 30 Jul 98 by Bernhard Kuemel (2:313/37.42@fidonet) Moedling Austria
   "While mathematically equivalent, due to integer division effects it
    is not possible to condense (or obfuscate) the code in this way."
   This is the old faulty code he referred to:
*/
	df1 -= (3 * ((year - 1) / 100) + 1) / 4;	      /* friday == 1 */
	df1 += 4;
#endif

	return ((int) (df1 % 7));			   /* 0=Sun .. 6=Fri */
}


void tdelay(int tme)			/* wait a specific time */
{
	long t;

	t = timerset((word) tme);
	while (!timeup(t)) win_idle();
}


void hangup (void)			/* disconnect the other side! */
{
	int fd;

	com_dump();

	mdm_puts(mdm.hangup);

	if (carrier()) {
	   long t;

	   dtr_out(0);
	   tdelay(50);
	   mdm_puts(mdm.hangup);

	   for (t = timerset(50); carrier() && !timeup(t); win_idle());

	   if (carrier()) {
	      dtr_out(0);
	      tdelay(50);

	      if (carrier()) {
		 message(6,"!TE/modem does not disconnect");
		 mdm_puts(mdm.hangup);
		 dtr_out(0);
		 tdelay(50);

		 if (carrier()) {
		    print("\n\07\07\07\07\07\07\07\07  DISCONNECT TE/MODEM PLEASE!\n\07\07\07\07");
		    mdm_puts(mdm.hangup);
		    dtr_out(0);
		    tdelay(50);

		    if (carrier()) {
		       char buffer[256];

		       flag_session(false);

		       sprint(buffer,"%sXMHANGUP.%u",flagdir,task);
		       if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0)
			  dos_close(fd);

		       errl_exit(1,false,"TE/modem carrier still high!");
		    }
		 }
	      }
	   }
	}

	dtr_out(1);			/* raise DTR */
	com_purge();

	if ((fd = dup(fileno(log))) >= 0)
	   dos_close(fd);
}/*hangup()*/


void alarm()					   /* Wake the operator !!!! */
{
	while (!win_keyscan()) {
	      if (!carrier())
		 return;
	      (void) win_bell();
	      tdelay(9);
	}
	win_keygetc();
}


int keyabort()				     /* local <ESC> pressed? */
{
	long t;

	if (lasttime != (t = time(NULL))) {
	   lasttime = t;
	   win_xyprint(top_win,win_maxhor - 24,1,"$aD $D $aM $Y $t");
#if OS2
	   mcp_sleep();
#endif
	}

	return (win_keycheck(Esc));
}


char *get_str(char *str, int len)      /* get string from keyboard */
{
	int i;			       /* counter */
	word c; 		       /* character typed by user */

	i=0;
	for(;;) 		       /* number of chars to process */
	{
		c = win_keygetc();
		if (c > 254) continue;
		if (c == Enter) break;
		if (c == Esc) {
		   if (i) {
		      do win_print(log_win,"\b \b");
		      while (--i);
		   }
		   win_puts(log_win,"<aborted>");
		   break;
		}
		if (c == BS || c == Ctrl_BS)
		{
			if (i)
			{
			    --i;	     /* delete char */
			    win_print(log_win,"\b \b");
			}
			continue;
		}
		win_putc(log_win,c);
		str[i++] = (char) c;
		if (i>len-2)
		{
		    --i;
		    (void) win_bell();
		}
	}
	win_putc(log_win,'\n');
	str[i]='\0';
	return (str);
}


static WIN_IDX wputcwin;

static void wputc(char c)		   /* output function of win_print */
{
	win_putc(wputcwin,c);
}


void print(char *fmt, ...)	      /* display local - in log window */
{
	va_list arg_ptr;

	va_start(arg_ptr,fmt);
	wputcwin = log_win;
	doprint(wputc, fmt, arg_ptr);
	va_end(arg_ptr);
}


void win_print(WIN_IDX win,char *fmt,...) /* print formatted in specified window */
{
	va_list arg_ptr;

	va_start(arg_ptr,fmt);

	wputcwin = win;
	doprint(wputc, fmt, arg_ptr);
	va_end(arg_ptr);
}


void win_xyprint(WIN_IDX win, byte horpos, byte verpos, char *fmt,...)
{	 /* print formatted in specified window at specified cursor position */
	va_list arg_ptr;

	if (win_setpos(win,horpos,verpos)) {
	   wputcwin = win;
	   va_start(arg_ptr,fmt);
	   doprint(wputc, fmt, arg_ptr);
	   va_end(arg_ptr);
	}
}


void splitfn(char *path, char *filename, char *file)
{
	char *p,*q;

	for (p=file;*p;p++) ;
	while (p!=file && *p!=':' && *p!='\\' && *p!='/') --p;
	if (*p==':' || *p=='\\' || *p=='/') ++p;/* begin */
	q=file;
	while (q!=p) *path++=*q++;		/* copy path */
	*path='\0';
	strcpy(filename,p);
}


void maketnam(char *name, char ch, char *out) /* make temp. filename */
{
	char temp[40],t;

	splitfn(out,temp,name);
	t=strlen(out);
	out[t]=ch;
	out[t+1]=0;
	strcat(out,temp);
}


void invent_pktname(char *str)
{
    static long lastt = 0L;
    long t = time(NULL);

    if (t == lastt) t++;
    sprint(str,"%08lx.pkt",t);
    lastt = t;
}


static char *suffixes[8] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA", NULL
};

void invent_arcname(char *str)
{
    struct dosdate_t d;

    _dos_getdate(&d);
    sprint(str,"%04x%04x.%s0",
	       (word) (ourbbs.net - remote.net),
	       (word) (ourbbs.node - remote.node),
	       suffixes[dayofweek(d.year,d.month-1,d.day)]);
}


void throughput(int opt, dword btes)
{
    static dword started = 0L;

    if (!opt)
       started	= time(NULL);
    else if (started) {
       dword elapsed, cps/*, percentage*/;

       elapsed	= time(NULL) - started;
       if (btes < 1024L || elapsed < 60L || btes < com_outfull())
	  return;
       cps = (btes - com_outfull()) / elapsed;
#if 0
       percentage  = (cps * 1000L) / line_speed;
       if (opt == 2) {
	  win_xyprint(file_win,31,6,"%lu (%lu%%)",cps,percentage);
	  win_clreol(file_win);
       }
       else
	  message(2,"+CPS: %lu (%lu bytes), %d:%02d min.  Efficiency: %lu%%",
		  cps,btes, (int) (((int)elapsed)/60),
		  (int) (((int)elapsed)%60), percentage);
#else
       if (opt == 2) {
	  win_xyprint(file_win,31,6,"%lu",cps);
	  win_clreol(file_win);
       }
       else
	  message(2,"+CPS: %lu (%lu bytes), %d:%02d min.",
		  cps,btes, (int) (((int)elapsed)/60),
		  (int) (((int)elapsed)%60));
#endif
    }
}/*throughput()*/


void send_can()
{
    static char abortstr[] = { 24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0 };

    com_dump();
    com_putblock((byte *) abortstr,(word) strlen(abortstr));
    com_flush();
}


is_arcmail (char *p, int n)
{
   int i;
   char c[256];

   strcpy (c, p);
   strupr (c);

   if (n < 11 || !isalnum(c[n]))
      return (0);

   for (i = n - 11; i < n - 3; i++) {
       if ((!isdigit(c[i])) && ((c[i] > 'F') || (c[i] < 'A')))
	  return (0);
   }

   for (i = 0; i < 7; i++) {
      if (strnicmp (&c[n-2], suffixes[i], 2) == 0)
	 break;
   }
   if (i >= 7)
      return (0);

   return (1);
}


int fexist (char *filename)
{
   /*struct stat f;*/
   register int i;

   com_disk(true);
   i = access(filename,0);
   /*i = getstat(filename,&f);*/
   com_disk(false);

   /*return ((i != -1) ? 1 : 0);*/
   return ((i == 0) ? 1 : 0);
}


word ztoi(char *str,int len)
{
	word temp;

	temp=0;
	while (*str && len-- && isalnum(*str))
	{
		temp *= (word) 36;
		temp += (word) (isdigit(*str) ? *str-'0' : toupper(*str)-'A'+10);
		++str;
	}
	return temp;
}


getadress(char *str, int *zone, int *net, int *node, int *point)
{
	*zone  = ztoi(str   , 2);
	*net   = ztoi(str +2, 3);
	*node  = ztoi(str +5, 3);
	*point = 0;

	return (0);
}


char *mon[12]  = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
char *wkday[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static void conv(dword getal,char *p, int base,int unsign,int blank)
{		      /* Local function of doprint - convert number to ascii */
	char buf[80], *t = &buf[79], sign=getal&0x80000000L && !unsign;
	char ch;
	*t = '\0';
	if (!blank || getal != 0) {
	   if (sign) getal = -getal;
	   do {
	      ch=getal % base;
	      *--t = (ch < 10) ? ch+'0' : ch-10+'a';
	      getal /= base;
	   } while (getal);
	   if (sign) *--t='-';
	}
	strcpy(p,t);
}


static void iprint(void (*outp)(char c),char *fmt,...)
{				/* needed for internal print	*/
	va_list arg_ptr;

	va_start(arg_ptr,fmt);
	doprint(outp, fmt, arg_ptr);
	va_end(arg_ptr);
}


void cdecl doprint(void (*outp)(char c),char *fmt,va_list args)
{			     /* Print internal format	     */
	register int  width, precision;
	register char *p,*pos,left,dot,pad;
	int	 uselong;
	char	 buf[256];

 p = fmt-1;
 while (*++p) {
  if (*p=='%') {
   p++;
   uselong=width=precision=0;
   pad = ' ';
   if (*p=='%') { (*outp)('%'); continue; }
   left = *p=='-';
   if ( left ) p++;
   if ( *p=='0' ) pad='0';
   if (*p == '*') {
      width = va_arg(args,int);
      p++;
   }
   else
      while ( isdigit(*p) ) width = width * 10 + *p++ - '0';
   dot = *p=='.';
   if ( dot ) {
      p++;
      if (*p == '*') {
	 precision = va_arg(args,int);
	 p++;
      }
      else
	 while ( isdigit(*p) ) precision = precision * 10 + *p++ - '0';
   }
   uselong= *p=='l';
   if (uselong) p++;
   switch (*p) {
    case 'D': uselong++;
    case 'd': conv(uselong ? va_arg(args,long) : (long) va_arg(args,int),
		   buf, 10, false, (dot && !precision));
	      break;
    case 'U': uselong++;
    case 'u': conv(uselong ? va_arg(args,dword) : (dword) va_arg(args,word),
		   buf, 10, true, (dot && !precision));
	      break;
    case 'X': uselong++;
    case 'x': conv(uselong ? va_arg(args,dword) : (dword) va_arg(args,word),
		   buf, 0x10, true, (dot && !precision));
	      break;
    case 'O': uselong++;
    case 'o': conv(uselong ? va_arg(args,dword) : (dword) va_arg(args,word),
		   buf, 8, true, (dot && !precision));
	      break;
    case 'Z': uselong++;
    case 'z': conv(uselong ? va_arg(args,dword) : (dword)va_arg(args,word),
		   buf, 36, true, (dot && !precision));
	      break;
    case 'c': buf[0] = (char) va_arg(args,short);
	      buf[1] = 0;
	      if (dot) buf[precision] = '\0';
	      break;
    case 's': strncpy(buf,va_arg(args,char *),255);
	      buf[255] = '\0';
	      if (dot) buf[precision] = '\0';
	      break;
   }
   if (dot && precision)
      buf[precision] = '\0';
   width -= (int) strlen(buf);
   if (!left) while (width-->0) (*outp)(pad);	/* left padding 	*/
   pos=buf;
   while (*pos) (*outp)( *pos++ );		/* buffer		*/
   if (left) while (width-->0) (*outp)(pad);	/* right padding	*/
   continue;
  }

  if (*p=='$') {
     struct dosdate_t d;
     struct dostime_t t;

     if (*++p=='$') { (*outp)('$'); continue; }

     _dos_getdate(&d);
     _dos_gettime(&t);

     uselong= *p=='a';
     if (uselong) p++;
     switch (*p) {
	    case 't' : iprint(outp, "%02d:%02d:%02d",
				    t.hour, t.minute, t.second);
		       break;
	    case 'd' : iprint(outp, "%02d-%02d-%02d",		  /*Not used!*/
				    d.day, d.month, d.year % 100);
		       break;
	    case 'D' : if (uselong) iprint(outp, "%s", wkday[dayofweek(d.year,d.month-1,d.day)]);
		       else	    iprint(outp, "%02d", d.day);
		       break;
	    case 'M' : if (uselong) iprint(outp, "%03s", mon[d.month-1]);
		       else	    iprint(outp, "%02d", d.month);
		       break;
	    case 'y' : iprint(outp, "%02d", d.year % 100); break; /*Not used!*/
	    case 'Y' : iprint(outp, "%4d", d.year); break;
   }
  }
  else (*outp)(*p);
 }
}


static char *prtstr[2]; 		/* Must be array: Time/Date!	*/
static int prtcnt = -1; 		/* points to current buffer	*/
static FILE *fp;			/* pointer for fprint		*/

static void putSTR(char c)		/* Append char to string	*/
{
	*prtstr[prtcnt]++ = c;
}


static void putFILE(char c)
{
	fputc(c,fp);
}


void cdecl fmprint(char *buf,char *fmt,void *args) /* print format to string spec! */
{
	prtstr[++prtcnt] = buf;
	doprint(putSTR, fmt, args);
	*prtstr[prtcnt--] = 0;		/* add string terminator	*/
}


void sprint(char *buf, char *fmt,...)
{
	va_list arg_ptr;

	va_start(arg_ptr,fmt);

	prtstr[++prtcnt] = buf;
	doprint(putSTR, fmt, arg_ptr);
	*prtstr[prtcnt--] = 0;

	va_end(arg_ptr);
}


void fprint(FILE *f, char *fmt, ...)
{
	va_list arg_ptr;

	va_start(arg_ptr,fmt);

	fp=f;
	doprint(putFILE, fmt, arg_ptr);

	va_end(arg_ptr);
}


void win_cyprint (WIN_IDX win, byte verpos, char *fmt, ...)
{
	char buf[200];
	va_list arg_ptr;

	va_start(arg_ptr,fmt);
	prtstr[++prtcnt] = buf;
	doprint(putSTR,fmt,arg_ptr);
	*prtstr[prtcnt--] = 0;
	va_end(arg_ptr);

	win_xyputs(win,1 + ((windows[win].cur_horlen - strlen(buf)) / 2),verpos,buf);
}/*win_cyprint()*/


void message(int level, char *fmt,...)	/* display a message to screen */
{					/* and logfile		       */
    char buf[320];
    va_list arg_ptr;
    long t;

    if (lasttime != (t = time(NULL))) {
       lasttime = t;
       win_xyprint(top_win,win_maxhor - 24,1,"$aD $D $aM $Y $t");
#if OS2
       mcp_sleep();
#endif
    }

    va_start(arg_ptr,fmt);

    sprint(buf,"%c $D $aM $t XEN  ", (short) *fmt);
    fmprint(&buf[23], &fmt[1], arg_ptr);
    va_end(arg_ptr);

    if (level>=loglevel && log) {
       com_disk(true);
       fprint(log, "%s\n", buf);
       fflush(log);
       com_disk(false);
    }

    blank_time=time(NULL);
    if (buf[0] == '!')
       win_setattrib(log_win, windows[log_win].win_top ?
			      xenwin_colours.log.hotnormal : xenwin_colours.term.hotnormal);
    print("\r\n%s\r",buf);
    if (buf[0] == '!')
       win_setattrib(log_win, windows[log_win].win_top ?
			      xenwin_colours.log.normal : xenwin_colours.term.normal);

#if OS2
    if (hsnoop != (HSNOOP)NULL) {
       extern USHORT APIENTRY (*dllSnoopWrite2) (HSNOOP hsn, PSZ pszMessage);
       buf[254] = '\0';

#if 0 /*WATCOM*/
       SnoopWrite(hsnoop, (PSZ _Seg16) buf);
#else
       dllSnoopWrite2(hsnoop, (PSZ) buf);
#endif
    }
#endif
}


/* ------------------------------------------------------------------------- */
static char *prodcode[] = {
/* 00*/  "Fido",		  "Rover",		   "SEAdog",		    "WinDog",
/* 04*/  "Slick-150",		  "Opus",		   "Dutchie",		    "WPL_Library",
/* 08*/  "Tabby",		  "SWMail",		   "Wolf-68k",		    "QMM",
/* 0C*/  "FrontDoor",		  "GOmail",		   "FFGate",		    "FileMgr",
/* 10*/  "FIDZERCP",		  "MailMan",		   "OOPS",		    "GS-Point",
/* 14*/  "BGMail",		  "ComMotion/2",	   "OurBBS_Fidomailer",     "FidoPcb",
/* 18*/  "WimpLink",		  "BinkScan",		   "D'Bridge",		    "BinkleyTerm",
/* 1C*/  "Yankee",		  "uuGate",		   "Daisy",		    "Polar_Bear",
/* 20*/  "The-Box",		  "STARgate/2", 	   "TMail",		    "TCOMMail",
/* 24*/  "GIGO",		  "RBBSMail",		   "Apple-Netmail",	    "Chameleon",
/* 28*/  "Majik_Board", 	  "QM", 		   "Point_And_Click",	    "Aurora_Three_Bundler",
/* 2C*/  "FourDog",		  "MSG-PACK",		   "AMAX",		    "Domain_Communication_System",
/* 30*/  "LesRobot",		  "Rose",		   "Paragon",		    "BinkleyTerm/oMMM/ST",
/* 34*/  "StarNet",		  "ZzyZx",		   "QEcho",		    "BOOM",
/* 38*/  "PBBS",		  "TrapDoor",		   "Welmat",		    "NetGate",
/* 3C*/  "Odie",		  "Quick_Gimme",	   "dbLink",		    "TosScan",
/* 40*/  "Beagle",		  "Igor",		   "TIMS",		    "Phoenix",
/* 44*/  "FrontDoor_APX",	  "XRS",		   "Juliet_Mail_System",    "Jabberwocky",
/* 48*/  "XST", 		  "MailStorm",		   "BIX-Mail",		    "IMAIL",
/* 4C*/  "FTNGate",		  "RealMail",		   "Lora-CBIS", 	    "TDCS",
/* 50*/  "InterMail",		  "RFD",		   "Yuppie!",		    "EMMA",
/* 54*/  "QBoxMail",		  "Number_4",		   "Number_5",		    "GSBBS",
/* 58*/  "Merlin",		  "TPCS",		   "Raid",		    "Outpost",
/* 5C*/  "Nizze",		  "Armadillo",		   "rfmail",		    "Msgtoss",
/* 60*/  "InfoTex",		  "GEcho",		   "CDEhost",		    "Pktize",
/* 64*/  "PC-RAIN",		  "Truffle",		   "Foozle",		    "White_Pointer",
/* 68*/  "GateWorks",		  "Portal_of_Power",	   "MacWoof",		    "Mosaic",
/* 6C*/  "TPBEcho",		  "HandyMail",		   "EchoSmith", 	    "FileHost",
/* 70*/  "SFTS",		  "Benjamin",		   "RiBBS",		    "MP",
/* 74*/  "Ping",		  "Door2Europe",	   "SWIFT",		    "WMAIL",
/* 78*/  "RATS",		  "Harry_the_Dirty_Dog",   "Maximus-CBCS",	    "SwifEcho",
/* 7C*/  "GCChost",		  "RPX-Mail",		   "Tosser",		    "TCL",
/* 80*/  "MsgTrack",		  "FMail",		   "Scantoss",		    "Point_Manager",
/* 84*/  "IMBINK",		  "Simplex",		   "UMTP",		    "Indaba",
/* 88*/  "Echomail_Engine",	  "DragonMail", 	   "Prox",		    "Tick",
/* 8C*/  "RA-Echo",		  "TrapToss",		   "Babel",		    "UMS",
/* 90*/  "RWMail",		  "WildMail",		   "AlMAIL",		    "XCS",
/* 94*/  "Fone-Link",		  "Dogfight",		   "Ascan",		    "FastMail",
/* 98*/  "DoorMan",		  "PhaedoZap",		   "SCREAM",		    "MoonMail",
/* 9C*/  "Backdoor",		  "MailLink",		   "Mail_Manager",	    "Black_Star",
/* A0*/  "Bermuda",		  "PT", 		   "UltiMail",		    "GMD",
/* A4*/  "FreeMail",		  "Meliora",		   "Foodo",		    "MSBBS",
/* A8*/  "Boston_BBS",		  "XenoMail",		   "XenoLink",		    "ObjectMatrix",
/* AC*/  "Milquetoast", 	  "PipBase",		   "EzyMail",		    "FastEcho",
/* B0*/  "IOS", 		  "Communique", 	   "PointMail", 	    "Harvey's_Robot",
/* B4*/  "2daPoint",		  "CommLink",		   "fronttoss", 	    "SysopPoint",
/* B8*/  "PTMAIL",		  "MHS",		   "DLGMail",		    "GatePrep",
/* BC*/  "Spoint",		  "TurboMail",		   "FXMAIL",		    "NextBBS",
/* C0*/  "EchoToss",		  "SilverBox",		   "MBMail",		    "SkyFreq",
/* C4*/  "ProMailer",		  "Mega_Mail",		   "YaBom",		    "TachEcho",
/* C8*/  "XAP", 		  "EZMAIL",		   "Arc-Binkley",	    "Roser",
/* CC*/  "UU2", 		  "NMS",		   "BBCSCAN",		    "XBBS",
/* D0*/  "LoTek_Vzrul", 	  "Private_Point_Project", "NoSnail",		    "SmlNet",
/* D4*/  "STIR",		  "RiscBBS",		   "Hercules",		    "AMPRGATE",
/* D8*/  "BinkEMSI",		  "EditMsg",		   "Roof",		    "QwkPkt",
/* DC*/  "MARISCAN",		  "NewsFlash",		   "Paradise",		    "DogMatic-ACB",
/* E0*/  "T-Mail",		  "JetMail",		   "MainDoor",		    "Starnet_Products",
/* E4*/  "BMB", 		  "BNP",		   "MailMaster",	    "Mail_Manager_+Plus+",
/* E8*/  "BloufGate",		  "CrossPoint", 	   "DeltaEcho", 	    "ALLFIX",
/* EC*/  "NetWay",		  "MARSmail",		   "ITRACK",		    "GateUtil",
/* F0*/  "Bert",		  "Techno",		   "AutoMail",		    "April",
/* F4*/  "Amanda",		  "NmFwd",		   "FileScan",		    "FredMail",
/* F8*/  "TP_Kom",		  "FidoZerb",		   "!!MessageBase",	    "EMFido",
/* FC*/  "GS-Toss",		  "QWKDoor",		   "No_product_id_allocated","16-bit_product_id",
/*100*/  "Reservered",		  "The_Brake!", 	   "Zeus_BBS",		    "XenoPhobe-Mailer",
/*104*/  "BinkleyTerm/ST",	  "Terminate",		   "TeleMail",		    "CMBBS",
/*108*/  "Shuttle",		  "Quater",		   "Windo"
};
#define LASTFTSCPROD	0x10A			/* Source: file FTSCPROD.DOC */
/* ------------------------------------------------------------------------- */


void showprod(word prod,word pmaj,word pmin)
{
    char buffer[80];

    if (prod == 0x05)
       sprint(buffer,"Opus %u.%02u (%02x)", pmaj,(pmin == 48) ? 0 : pmin,prod);
    else if (prod <= LASTFTSCPROD)
       sprint(buffer,"%s %u.%02u (%02x)", prodcode[prod],pmaj,pmin,prod);
    else
       sprint(buffer,"Unknown program %u.%02u (%02x)", pmaj,pmin,prod);

     message(2,"=Mailer: %s",buffer);
     win_xyputs(mailer_win,10,4,buffer);
}


void statusmsg(char *fmt,...)
{
	static char lastmsg[80] = "";
	char buffer[256];

	if (fmt == NULL)
	   sprint(buffer,mdm.port == -1 ? "Waiting for event" : "Waiting for call or event");
	else {
	   va_list arg_ptr;

	   va_start(arg_ptr,fmt);
	   fmprint(buffer,fmt,arg_ptr);
	   va_end(arg_ptr);
	}
	buffer[79] = '\0';

	win_xyputs(status_win,1,1,buffer);
	win_clreol(status_win);

#if OS2
	if (hpMCP) {
	   extern int APIENTRY (*dllMcpSendMsg)(HPIPE hp, USHORT usType, BYTE *pbMsg, USHORT cbMsg);

	   struct _cstat { short avail;
			   char  username[36];
			   char  status[80];
	   } cstat;

	   memset(&cstat,0,sizeof (struct _cstat));
	   sprint(cstat.username,"%s %s",PRGNAME,XENIA_OS);
	   strcpy(cstat.status,buffer);

	   dllMcpSendMsg(hpMCP, PMSG_SET_STATUS,
		      (BYTE *) &cstat, sizeof (struct _cstat));
	}
#endif

	if (strcmp(lastmsg,buffer) && bbstype == BBSTYPE_MAXIMUS) {
	   struct _max_ipc { short avail;
			     char  username[36];
			     char  status[80];
			     word  msgs_waiting;
			     dword next_msgofs;
			     dword new_msgofs;
	   } max_ipc;
	   int fd;

	   memset(&max_ipc,0,sizeof (struct _max_ipc));
	   sprint(max_ipc.username,"%s %s",PRGNAME,XENIA_OS);
	   strcpy(max_ipc.status,buffer);

	   sprint(buffer,"%sIPC%02x.BBS",flagdir,(word) task);
	   if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_ALL)) >= 0) {
	      dos_write(fd,&max_ipc,sizeof (struct _max_ipc));
	      dos_close(fd);
	   }
	}

	strcpy(lastmsg,buffer);
}


void filemsg(char *fmt,...)
{
	va_list arg_ptr;

	va_start(arg_ptr,fmt);

	win_setattrib(file_win,xenwin_colours.file.hotnormal);
	win_setpos(file_win,14,3);
	wputcwin = file_win;
	doprint(wputc, fmt, arg_ptr);
	va_end(arg_ptr);
	win_clreol(file_win);
	win_setattrib(file_win,xenwin_colours.file.normal);
}/*filemsg()*/


#ifndef NOHYDRA
char *h_revdate (long revstamp)
{
	static char  buf[12];
	struct tm   *t;

	t = localtime((time_t *) &revstamp);
	sprint(buf, "%02d %s %d",
		    t->tm_mday, mon[t->tm_mon], t->tm_year + 1900);

	return (buf);
}/*h_revdate()*/


void hydramsg (char *fmt, ...)
{
	va_list arg_ptr;

	va_start(arg_ptr,fmt);

	win_setattrib(file_win,xenwin_colours.file.hotnormal);
	win_setpos(file_win,13,2);
	wputcwin = file_win;
	doprint(wputc, fmt, arg_ptr);
	va_end(arg_ptr);
	win_clreol(file_win);
	win_setattrib(file_win,xenwin_colours.file.normal);
}/*hydramsg()*/
#endif


void xfertime(long length)
{
				 /* +/- \361 */
	win_xyprint(file_win,31,5,"%d:%02d min.",
		    (int)(((length/128L)*1340L)/line_speed)/60,
		    (int)(((length/128L)*1340L)/line_speed)%60);
	win_clreol(file_win);
}


void last_set (char **s, char *str)
{
	struct tm *tt;
	long tim;
	char buf[80];

	tim = time(NULL);
	tt = localtime((time_t *) &tim);

	sprint(buf, "%-18.18s %02d:%02d", str, tt->tm_hour, tt->tm_min);

	if (*s) free(*s);
	*s = strdup(buf);
}/*last_set()*/


boolean patimat (char *raw, char *pat)	     /* Pattern matching - recursive */
{				      /* **** Case INSENSITIVE version! **** */
	if (!*pat)
	   return (*raw ? false : true);

	if (*pat == '*') {
	   if (!*++pat)
	      return (true);
	   do if ((inl_tolower(*raw) == inl_tolower(*pat) || *pat == '?') &&
		  patimat(raw + 1,pat + 1))
		 return (true);
	   while (*raw++);
	}
	else if (*raw && (*pat == '?' || inl_tolower(*pat) == inl_tolower(*raw)))
	   return (patimat(++raw,++pat));

	return (false);
}/*patimat()*/


/* end of misc.c */
