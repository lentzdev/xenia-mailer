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


/* System dependent routines for IBM-PC: BC++2.0, TC++1.0, TC2.0 */
#include <time.h>
#include <stdarg.h>
#include <process.h>
#include "xenia.h"
#include "exec.h"
#include "aglcom.h"
extern far void key_release (void);


unsigned _stklen = 16 * 1024;


struct _fos_info {
       word	 strsize;	/* size of the structure in bytes     */
       byte	 majver;	/* FOSSIL spec driver conforms to     */
       byte	 minver;	/* rev level of this specific driver  */
       char far *ident; 	/* FAR pointer to ASCII ID string     */
       word	 ibufr; 	/* size of the input buffer (bytes)   */
       word	 ifree; 	/* number of bytes left in buffer     */
       word	 obufr; 	/* size of the output buffer (bytes)  */
       word	 ofree; 	/* number of bytes left in the buffer */
       byte	 swidth;	/* width of screen on this adapter    */
       byte	 sheight;	/* height of screen    "      "       */
       byte	 speed; 	/* ACTUAL speed, computer to modem    */
};

struct _fos_speedtable {
	dword speed;
	byte  bits;
};


extern int  X00    (union REGS *, union REGS *);
extern int  X00X   (union REGS *, union REGS *, struct SREGS *);
extern void BYPASS (void);
#define fossil_int(func) { _AH = func; _DX = com_handle; BYPASS(); /*geninterrupt(0x14);*/ }
#define FOS_SIGNATURE	0x1954
#define FOS_SETSPEED	0x00
#define FOS_GETSTATUS	0x03
#define FOS_INIT	0x04
/*#define FOS_DEINIT	  0x05*/
#define FOS_SETDTR	0x06
#define FOS_DUMP	0x09
#define FOS_PURGE	0x0a
#define FOS_PUTCNOWAIT	0x0b
#define FOS_SETFLOW	0x0f
#define FOS_XMITTER	0x10
#define FOS_BREAK	0x1a
#define FOS_NEWINIT	0x1c
/*#define FOS_NEWDEINIT   0x1d*/
#define FOS_EXTCONTROL	0x1e
#define FOS_EXTMODEM	0x1f
#define COM_MAXBUFLEN	1024

static boolean fossil;
static int     fos_initfunc;
static int     fos_maxfunc;
static struct _fos_info fos_info;
static byte   *com_inbuf = NULL, *com_outbuf = NULL;
static word    com_inwrite,  com_outwrite,
	       com_inread;
static struct _fos_speedtable fos_speedtable[] = {
	{   300UL, 0x43 },
	{  1200UL, 0x83 },
	{  2400UL, 0xA3 },
	{  4800UL, 0xC3 },
	{  9600UL, 0xE3 },
	{ 19200UL, 0x03 },
	{ 38400UL, 0x23 },
	{     0UL,    0 }
};
static struct _fos_speedtable extfos_speedtable[] = {
	{    300UL, 0x00 },
	{   1200UL, 0x04 },
	{   2400UL, 0x05 },
	{   4800UL, 0x06 },
	{   9600UL, 0x07 },
	{  19200UL, 0x08 },
	{  28800UL, 0x80 },
	{  38400UL, 0x81 },
	{  57600UL, 0x82 },
	{ 115200UL, 0x84 },
	{      0UL,    0 }
};


/* ------------------------------------------------------------------------- */
/* PTT telephone cost pulse counter - by Arjen G. Lentz, 2 Dec 1992	     */
/* Cost pulse connected to Ack line of printer port (can generate interrupt) */
/* Printer port address 3BC (mono/printer=LPT1), 378 (LPT1) or 278 (LPT2)    */
/* Hardware interrupt: IRQ 7 (LPT1) or IRQ 5 (LPT2)			     */

#define TIMER_INT   0x1c		/* INT 1C = User timer (18.2x/sec)   */
#define SPACE_TICKS 7			/* One PTT pulse per 7 timer ticks   */
enum lpt_ofs { LPT_DATA,		/* LPT port offset of data latch     */
	       LPT_STATUS,		/* LPT port offset of status latch   */
	       LPT_CONTROL		/* LPT port offset of control latch  */
};
#define LPT_ACKMASK 0x40		/* Ack (LPT pin 10) bit 6 of status  */
#define LPT_IRQMASK 0x10		/* Interrupt enable bit 4 of control */

static word ptt_pic;				/* 8259 PIC base    (20,A0)  */
static void interrupt (*oldlpthandler)(void);	/* Old LPT handler	     */
static void interrupt (*oldtimerhandler)(void); /* Old user timer handler    */
static volatile word ptt_unitspace;		/* Countdown pulse delay     */
static boolean ptt_active = false, ptt_save = false;


static void interrupt costunit_handler (void)
{
	if (!ptt_unitspace) {
	   ptt_units++;
	   ptt_unitspace = SPACE_TICKS;
	}
	outportb(ptt_pic + PIC_CMD, PIC_EOI);
}/*costunit_handler()*/


static void interrupt timer_handler (void)
{
	if (ptt_unitspace)
	   ptt_unitspace--;
	oldtimerhandler();
}/*timer_handler()*/


void ptt_enable (void)
{
	if (ptt_cost && !ptt_active) {
	   ptt_pic = (ptt_irq > 7) ? PIC2_PORTBASE : PIC1_PORTBASE;
	   oldtimerhandler = getvect(TIMER_INT);
	   setvect(TIMER_INT, timer_handler);
	   outportb(ptt_port + LPT_DATA, 0xff);
	   outportb(ptt_port + LPT_CONTROL,
		    inportb(ptt_port + LPT_CONTROL) | LPT_IRQMASK);
	   oldlpthandler = getvect(((ptt_irq > 7) ? PIC2_VECTBASE : PIC1_VECTBASE) + (ptt_irq & 0x07));
	   setvect(((ptt_irq > 7) ? PIC2_VECTBASE : PIC1_VECTBASE) + (ptt_irq & 0x07),
		   costunit_handler);
	   outportb(ptt_pic + PIC_CTL,
		    inportb(ptt_pic + PIC_CTL) & ~(1 << (ptt_irq & 0x07)));
	   ptt_active = true;
	}
}/*ptt_enable()*/


void ptt_disable (void)
{
	if (ptt_active) {
	   outportb(ptt_pic + PIC_CTL,
		    inportb(ptt_pic + PIC_CTL) | (1 << (ptt_irq & 0x07)));
	   setvect(((ptt_irq > 7) ? PIC2_VECTBASE : PIC1_VECTBASE) + (ptt_irq & 0x07),
		   oldlpthandler);
	   outportb(ptt_port + LPT_CONTROL,
		    inportb(ptt_port + LPT_CONTROL) & ~LPT_IRQMASK);
	   outportb(ptt_port + LPT_DATA, 0);
	   setvect(TIMER_INT, oldtimerhandler);
	   ptt_active = false;
	}
}/*ptt_disable()*/


void ptt_reset (void)
{
	ptt_units = ptt_unitspace = 0;
}/*ptt_reset()*/


/* ------------------------------------------------------------------------- */


/* These routines implement a simple multiple alarm system.
   The routines allow setting any number of alarms, and then
   checking if any one of them has expired.
*/


#define TENTHS_PER_HOUR    36000L
#define TENTHS_PER_MINUTE    600L
#define TENTHS_PER_SECOND     10L


/* This routine returns a timer variable based on the MS-DOS
   time.  The variable returned is a long which corresponds to
   the MS-DOS time at which the timer will expire.  The variable
   need never be used once set, but to find if the timer has in
   fact expired, it will be necessary to call the timeup function.
   The expire time 't' is in tenths of a second.
*/

long timerset (word t)
{
   long l;
   struct dostime_t tim;

   _dos_gettime(&tim);					 /* Get the DOS time */
   l = tim.hour   * TENTHS_PER_HOUR   +
       tim.minute * TENTHS_PER_MINUTE + 	     /* Figure out the hundredths of */
       tim.second * TENTHS_PER_SECOND;		     /* a second so far this day     */
   l += (long) (tim.hsecond / 10);
   l += t;					   /* Add in the timer value */
   return (l);					/* Return the alarm off time */
}

/* This routine returns a 1 if the passed timer variable corresponds
   to a timer which has expired, or 0 otherwise.
*/

int timeup (long t)
{
   long l;

   l = timerset(0);			   /* Get current time in hundredths */
	    /* If current is less then set by more than max int, then adjust */
   if (l < (t - 4 * TENTHS_PER_HOUR))
      l += 24 * TENTHS_PER_HOUR;
	    /* Return whether the current is greater than the timer variable */
   return ((l - t) >= 0L);
}


int rts_out (int x)		      /* Set RTS according to the value of x */
{
	return (x);
}


void dtr_out (byte flag)
{
	if (fossil) {
	   _AL = flag;
	   fossil_int(FOS_SETDTR);
	}
	else
	   agl_setdtr(flag ? true : false);
}


void com_flow (byte flags)
{
	if (fossil) {
	   _AL = flags;
	   fossil_int(FOS_SETFLOW);
	}
	else {
	   agl_setflow(((flags & 0x02) ? FLOW_HARD : FLOW_NONE) |
		       ((flags & 0x09) ? FLOW_SOFT : FLOW_NONE));
	}
}


void com_disk (boolean diskaccess)
{
	if (!mdm.slowdisk) return;

	if (fossil) {
	   if (fos_maxfunc >= FOS_EXTMODEM &&
	       (!diskaccess || !remote.capabilities || carrier())) {
	      _AL = 0;
	      fossil_int(FOS_EXTMODEM);
	      _AL = 1;
	      if (diskaccess) _BL &= ~0x02;
	      else	      _BL |= 0x02;
	      fossil_int(FOS_EXTMODEM);

#if 0
	      _AL = diskaccess ? 0x02 : 0x00;
	      fossil_int(FOS_XMITTER);
#endif
	   }
	}
	else if (!diskaccess || !remote.capabilities || carrier())
	   agl_rxdisable(diskaccess);
}


void setspeed (dword speed, boolean force)	     /* set speed of comport */
{
	char buf[256];

	if (!speed) speed = cur_speed;

	win_setattrib(modem_win,xenwin_colours.modem.template);
	win_setrange(modem_win,2,2,windows[modem_win].win_horlen - 1,11);
	win_clreol(modem_win);
	sprint(buf,"%lu%s",speed,errorfree);
	buf[windows[modem_win].cur_horlen] = '\0';
	win_puts(modem_win,buf);
	win_setrange(modem_win,2,4,windows[modem_win].win_horlen - 1,11);
	win_setpos(modem_win,1,8);
	win_setattrib(modem_win,xenwin_colours.modem.normal);

	cur_speed = line_speed = speed;

	if (!hotstart) {
	   register int i;

	   if (mdm.lock && !force)
	      speed = mdm.speed;
	   speed /= mdm.divisor;

	   if (fossil) {
	      for (i = 0; fos_speedtable[i].speed; i++) {
		  if (speed == fos_speedtable[i].speed) {
		     _AL = fos_speedtable[i].bits;
		     fossil_int(FOS_SETSPEED);
		     break;
		  }
	      }

	      for (i = 0; extfos_speedtable[i].speed; i++) {
		  if (speed == extfos_speedtable[i].speed) {
		     union REGS regs;

		     regs.h.ah = FOS_EXTCONTROL;
		     regs.h.al = 0;			/* no break   */
		     regs.h.bh = 0;			/* no parity  */
		     regs.h.bl = 0;			/* 1 stopbit  */
		     regs.h.ch = 3;			/* 8 databits */
		     regs.h.cl = extfos_speedtable[i].bits;
		     regs.x.dx = com_handle;
		     X00(&regs,&regs);
		     break;
		  }
	      }
	   }
	   else {
	      agl_setspeed(speed);
	      agl_setparity(PAR_NONE,8,1);
	   }
	}
}


static near word fos_getstatus (void)
{
	fossil_int(FOS_GETSTATUS);
	return (_AX);
}


static near int get_fos_info (void)
{
	union REGS regs;
	struct SREGS sregs;

	fos_info.strsize = 0;

	regs.x.ax = 0x1b00;
	regs.x.cx = sizeof (struct _fos_info);
	regs.x.dx = com_handle;
	segread(&sregs);
	sregs.es = FP_SEG((byte far *) &fos_info);
	regs.x.di = FP_OFF((byte far *) &fos_info);
	X00X(&regs,&regs,&sregs);

	return (fos_info.strsize == sizeof (struct _fos_info));
}


static near int com_fillinbuf (void)
{
	if (fossil) {
	   struct SREGS sregs;
	   union  REGS	regs;

	   if (!(fos_getstatus() & 0x0100))
	      return (0);

	   regs.x.ax = 0x1800;
	   regs.x.dx = com_handle;
	   regs.x.cx = COM_MAXBUFLEN;
	   segread(&sregs);
	   sregs.es  = FP_SEG(com_inbuf);
	   regs.x.di = FP_OFF(com_inbuf);
	   X00X(&regs,&regs,&sregs);
	   com_inwrite = regs.x.ax;
	}
	else {
	   com_inwrite = agl_rxblock(com_inbuf,COM_MAXBUFLEN);
	}

	com_inread = 0;

	return (com_inwrite);
}


void com_putblock (byte *s, word len)
{
	long t;

	if (com_outwrite)
	   com_bufflush();

	if (!len) return;

	t = timerset(600);

	if (fossil) {
	   struct SREGS sregs;
	   union  REGS	regs;

	   do {
	      regs.x.ax = 0x1900;
	      regs.x.dx = com_handle;
	      regs.x.cx = len;
	      segread(&sregs);
	      sregs.es	= FP_SEG(s);
	      regs.x.di = FP_OFF(s);
	      X00X(&regs,&regs,&sregs);
	      len -= regs.x.ax;
	      s += regs.x.ax;
	   } while (len && carrier() && !timeup(t));
	}
	else {
	   do {
	      word n;

	      n = agl_txblock((byte far *) s,len);
	      len -= n;
	      s += n;
	   } while (len && carrier() && !timeup(t));
	}
}


boolean com_setport (word comport, word adr, word irq)
{
	if (comport < 1 || comport > COM_MAXPORTS || irq > 15)
	   return (false);

	agl_portinfo[comport - 1].portbase = adr;
	agl_portinfo[comport - 1].irq	   = irq;
	return (true);
}


static near boolean fos_init (void)
{
	union REGS regs;

	regs.h.ah = fos_initfunc = FOS_NEWINIT;
	regs.x.dx = com_handle;
	X00(&regs,&regs);
	if (regs.x.ax != FOS_SIGNATURE || regs.h.bh < 5 || regs.h.bl < 0x1b) {
	   regs.h.ah = fos_initfunc = FOS_INIT;
	   regs.x.dx = com_handle;
	   X00(&regs,&regs);
	}
	if (regs.x.ax != FOS_SIGNATURE || regs.h.bh < 5 || regs.h.bl < 0x1b)
	   return (false);
	fos_maxfunc = regs.h.bl;
	return (true);
}


static int critical_handler (void)
{
	return (3);	/* FAIL */
}/*critical_handler()*/


void sys_init(void)	    /* all system dependent init should be done here */
{
	static char *newprompt = NULL;
	char buffer[300];
	char *p;

	harderr(critical_handler);

	if (!newprompt) {
	   sprint(buffer,"PROMPT=[%s Shell]$$_", progname);
	   if ((p=getenv("PROMPT")) != NULL) strcat(buffer,p);
	   else 			     strcat(buffer,"$p$g");
	   newprompt = strdup(buffer);
	}
	putenv(newprompt);

	if (mdm.port != -1) {
	   if (!(com_inbuf  = malloc(COM_MAXBUFLEN + 16)) ||
	       !(com_outbuf = malloc(COM_MAXBUFLEN + 16))) {
	      win_deinit();
	      fprint(stderr,"Can't allocate internal comm buffers\n");
	      exit (1);
	   }

	   fossil = true;
	   if (!mdm.port) {
	      if (hotstart) {
		 win_deinit();
		 fprint(stderr,"Can't auto-detect TE/modem comport in HotStart mode\n");
		 exit (1);
	      }

	      for (com_handle = 0; com_handle < COM_MAXPORTS && (!agl_uart(com_handle) || !agl_modem(com_handle)); com_handle++);
	      if (com_handle < COM_MAXPORTS) {
		 mdm.port = com_handle + 1;
		 message(0,"+Found TE/modem on COM%d",mdm.port);
		 sprint(buffer,"COM%d",mdm.port);
		 mdm.comdev = ctl_string(buffer);
	      }
	      else {
		 win_deinit();
		 fprint(stderr,"Unable to auto-detect TE/modem comport\n");
		 exit (1);
	      }
	   }

	   com_handle = mdm.port - 1;
	   fossil = true;
	   if (mdm.nofossil || (!hotstart && !com_suspend && !fos_init())) {
	      com_handle %= COM_MAXPORTS;
	      if (agl_uart(com_handle) == NS16550)
		 message(6,"!Warning: COM%d has 16550 UART (faulty FIFOs)",mdm.port);
	      if (!agl_open(com_handle)) {
		 win_deinit();
		 fprint(stderr,"Xenia can't find/open COM%d\n",
			       mdm.port,
			       agl_portinfo[com_handle].portbase, agl_portinfo[com_handle].irq);
		 exit (1);
	      }
	      mdm.nofossil = true;
	      fossil = false;

	      /* FIFOs are set allowed by async routines: default at startup */
	      if (agl_havefifo(com_handle) && mdm.fifo)
		 agl_setfifo(mdm.fifo);
	      else
		 agl_usefifo(false);	   /* Disallow FIFO usage */

	      agl_setdcd(mdm.dcdmask);
	   }
	   else if (!get_fos_info() || fos_info.majver < 5) {
norev5:       fossil_int(fos_initfunc + 1);
	      win_deinit();
	      fprint(stderr,"Xenia requires a revision 5 FOSSIL driver (funcs upto 1B)\n");
	      exit (1);
	   }

	   com_suspend = false;
	}

	win_setattrib(modem_win,xenwin_colours.modem.border);
	win_line(modem_win,1,1,windows[modem_win].win_horlen - 1,1,LINE_SINGLE);
	win_setattrib(modem_win,xenwin_colours.modem.title);
	win_setrange(modem_win,1,1,windows[modem_win].win_horlen - 1,1);
	win_xyputs(modem_win,2,1,mdm.comdev);
	win_setrange(modem_win,2,4,windows[modem_win].win_horlen - 1,11);
	win_setpos(modem_win,1,8);
	win_setattrib(modem_win,xenwin_colours.modem.normal);

	if (mdm.port != -1) {
	   if (fossil) {
	      byte horpos, verpos;

	      win_getpos(log_win,&horpos,&verpos);
	      if (fos_info.ident != NULL && fos_info.ident[0] &&
		  horpos == 1 && verpos == 1)
		 print("FOSSIL: %s\r", fos_info.ident);
	   }

	   if (fossil) {
	      if (fos_info.obufr < 4095)
		 message(6,"!Warning: FOSSIL transmit buffer size %u, should be >= 4096",fos_info.obufr);
#if 0
	      if (fos_info.ibufr < 4095)
		 message(6,"!Warning: FOSSIL receive buffer size %u, should be >= 4096",fos_info.ibufr);
#endif
	   }

	   com_flow(mdm.handshake);
	   com_inread = com_inwrite = com_outwrite = 0;

	   if (ptt_save) {
	      ptt_enable();
	      ptt_save = false;
	   }
	}

	atexit(key_release);
	atexit(ptt_disable);
	atexit(sys_reset);
}


void sys_reset(void)				/* same as above for de-init */
{
	if (mdm.port != -1) {
	   ptt_save = ptt_active;
	   ptt_disable();

	   if (fossil) {
	      if (!hotstart && !com_suspend)
		 fossil_int(fos_initfunc + 1);
	   }
	   else
	      agl_close(false);

	   if (com_inbuf) free(com_inbuf);
	   com_inbuf = NULL;
	   if (com_outbuf) free(com_outbuf);
	   com_outbuf = NULL;
	}
}


int com_outfull (void)		/* Return the amount in fossil output buffer */
{				/* Ie. remaining no. bytes to be transmitted */
	if (fossil) {
	   get_fos_info();
	   return (fos_info.obufr - fos_info.ofree);
	}
	else
	   return (agl_txfill());
}


int carrier(void)				    /* Return carrier status */
{
	if (fossil) {
	   fossil_int(FOS_GETSTATUS);
	   return (_AX & mdm.dcdmask);
	}
	else
	   return (agl_carrier());	 /* DCD mask already set in sys_init */
}  


#if 0
int cts(void)						/* return CTS status */
{
	if (fossil) {
	   fossil_int(FOS_GETSTATUS);
	   return (_AX & 0x0010);
	}
	else
	   return (agl_getmsr() & MSR_CTS);
}
#endif


#if 0
int com_empty(void)
{
	if (fossil) {
	   fossil_int(FOS_GETSTATUS);
	   return (!(_AX & 0x4000));
	}
	else
	   return (!agl_txempty());
}
#endif


void com_flush(void)		  /* wait till all characters have been sent */
{
	long t;

	if (com_outwrite)
	   com_bufflush();

	t = timerset(300);

	if (fossil) {
	   while (!(fos_getstatus() & 0x4000) && carrier() && !timeup(t))
		 win_idle();
	}
	else {
	   while (!agl_txempty() && carrier() && !timeup(t))
		 win_idle();
	}
}


void com_putbyte (byte c)
{
	if (com_outwrite)
	   com_bufflush();

	if (fossil) {
	   _AL = c;
	   fossil_int(FOS_PUTCNOWAIT);
	   if (!_AX) {
	      long t = timerset(100);

	      do {
		 _AL = c;
		 fossil_int(FOS_PUTCNOWAIT);
	      } while (!_AX && !timeup(t));
	   }
	}
	else {
	   if (!agl_txbyte(c)) {
	      long t = timerset(100);

	      while (!agl_txbyte(c) && !timeup(t));
	   }
	}
}


void com_bufbyte (byte c)
{
	com_outbuf[com_outwrite++] = c;
	if (com_outwrite >= COM_MAXBUFLEN) {
	   register word len = com_outwrite;

	   com_outwrite = 0;
	   com_putblock(com_outbuf,len);
	}
}


void com_bufflush (void)
{
	if (com_outwrite) {
	   register word len = com_outwrite;

	   com_outwrite = 0;
	   com_putblock(com_outbuf,len);
	}
}


void com_purge (void)
{
	if (fossil) {
	   fossil_int(FOS_PURGE);
	}
	else
	   agl_rxclear();

	com_inread = com_inwrite = 0;
}


void com_dump (void)
{
	if (fossil) {
	   fossil_int(FOS_DUMP);
	}
	else
	   agl_txclear();

	com_outwrite = 0;
}


int com_scan(void)			     /* Check for characters waiting */
{
	return ((com_inread < com_inwrite) || com_fillinbuf());
}


int com_getc(int timeout)
{
	if (!com_scan()) {
	   long t = timerset(timeout);

	   while (!com_fillinbuf())
		 if (timeup(t)) return (EOF);
	}

	return (com_inbuf[com_inread++]);
}


int com_getbyte(void)
{
	return (((com_inread < com_inwrite) || com_fillinbuf()) ?
		com_inbuf[com_inread++] : EOF);
}


void com_break(void)
{
	com_flush();

	if (fossil) {
	   long t = timerset(10);

	   _AL = 1;
	   fossil_int(FOS_BREAK);
	   while (!timeup(t));
	   _AL = 0;
	   fossil_int(FOS_BREAK);
	}
	else
	   agl_break(1000);
}


void setstamp(char *name,long tim)		    /* Set time/date of file */
{
	int fd;
	struct tm *t;
	union REGS regs;

	if ((fd = dos_sopen(name,O_WRONLY,DENY_NONE)) < 0)
	   return;

	if (tim == 0L)
	   time(&tim);
	t = localtime((time_t *) &tim);

	regs.x.cx = (t->tm_hour << 11) |
		    (t->tm_min << 5) |
		    (t->tm_sec/2);
	regs.x.dx = ((t->tm_year-80) << 9) |
		    ((t->tm_mon+1) << 5) |
		    (t->tm_mday);
	regs.x.bx = fd;
	regs.x.ax = 0x5701;	/* DOS int 21 fn 57 sub 1 */
	intdos(&regs,&regs);	/* Set a file's date/time */

	close(fd);
}


int getstat (char *pathname, struct stat *statbuf)
{
    struct find_t ffdta;
    struct tm	  t;
    unsigned res;

    res = _dos_findfirst(pathname,_A_NORMAL,&ffdta);
    if (!res) {
       t.tm_sec   = ((ffdta.wr_time & 0x001f) << 1);	     /* seconds after the minute -- [0,61] */
       t.tm_min   = ((ffdta.wr_time & 0x07e0) >> 5);	     /* minutes after the hour	 -- [0,59] */
       t.tm_hour  = ((ffdta.wr_time & 0xf800) >> 11);	     /* hours after midnight	 -- [0,23] */
       t.tm_mday  = (ffdta.wr_date & 0x001f);		     /* day of the month	 -- [1,31] */
       t.tm_mon   = ((ffdta.wr_date & 0x01e0) >> 5) - 1;     /* months since January	 -- [0,11] */
       t.tm_year  = (((ffdta.wr_date & 0xfe00) >> 9) + 80);  /* years since 1900		   */
       t.tm_isdst = 0;
       statbuf->st_atime = statbuf->st_mtime = statbuf->st_ctime = mktime(&t);
       statbuf->st_size  = ffdta.size;
    }

#if WATCOM
     _dos_findclose(&ffdta);
#else
     while (!_dos_findnext(&ffdta));
     _dos_findnext(&ffdta);
#endif

     return (res ? -1 : 0);
}


long freespace(char *drivepath)    /* get free diskspace for specified drive */
{
	union REGS regs;
	long	   bytes;

	if (drivepath[0] && drivepath[1] == ':' && isalpha(drivepath[0]))
	   regs.h.dl = (toupper(drivepath[0]) - 64);
	else
	   regs.h.dl = 0;
	regs.x.ax = 0x3600;
	intdos(&regs,&regs);

	bytes = (long)regs.x.cx * (long)regs.x.ax * (long)regs.x.bx;
	return (bytes < 0L ? 0x7fffffffL : bytes);
}


static struct find_t ffblk,
		     dfblk,
		     dfblk2;

char *ffirst (char *name)
{
    return (_dos_findfirst(name,_A_NORMAL,&ffblk) ? NULL : ffblk.name);
}/*ffirst()*/


char *fnext (void)
{
    return (_dos_findnext(&ffblk) ? NULL : ffblk.name);
}/*fnext()*/


void fnclose (void)
{
#if WATCOM
    _dos_findclose(&ffblk);
#else
    while (fnext());
    /*fnext();*/
#endif
}/*fnclose()*/


char *dfirst (char *filespec)
{
    if (_dos_findfirst(filespec,_A_SUBDIR,&dfblk))
       return (NULL);
    if (!(dfblk.attrib & _A_SUBDIR) || !strcmp(dfblk.name,"."))
       return (dnext());

    return (dfblk.name);
}/*dfirst()*/


char *dnext (void)
{
    do {
       if (_dos_findnext(&dfblk))
	  return (NULL);
    } while (!(dfblk.attrib & _A_SUBDIR) || !strcmp(dfblk.name,"."));

    return (dfblk.name);
}/*dnext()*/


void dnclose (void)
{
#if WATCOM
    _dos_findclose(&dfblk);
#else
    while (dnext());
    /*dnext();*/
#endif
}/*dnclose()*/


char *dfirst2 (char *filespec)
{
    if (_dos_findfirst(filespec,_A_SUBDIR,&dfblk2))
       return (NULL);
    if (!(dfblk2.attrib & _A_SUBDIR) || !strcmp(dfblk2.name,"."))
       return (dnext2());

    return (dfblk2.name);
}/*dfirst2()*/


char *dnext2 (void)
{
    do {
       if (_dos_findnext(&dfblk2))
	  return (NULL);
    } while (!(dfblk2.attrib & _A_SUBDIR) || !strcmp(dfblk2.name,"."));

    return (dfblk2.name);
}/*dnext2()*/


void dnclose2 (void)
{
#if WATCOM
    _dos_findclose(&dfblk2);
#else
    while (dnext2());
    /*dnext2();*/
#endif
}/*dnclose2()*/


#if 0
int rename (const char *oldname, const char *newname)
{
	struct SREGS sregs;
	union  REGS  regs;
	char	 cwd[129],
		 frompath[129], fromname[20], *old = (char *) oldname,
		 topath[129],	toname[20], *new = (char *) newname;
	char	*p;
	boolean  changed = false;

	splitfn(frompath,fromname,old);
	splitfn(topath,toname,new);

	if (frompath[0] && !stricmp(frompath,topath)) {
	   getcwd(cwd,128);
	   strupr(cwd);

	   changed = true;

	   strupr(frompath);
	   if (strlen(frompath) >= 2) {
	      p = &frompath[strlen(frompath)] - 1;
	      if (p[0] == '\\' && p[-1] != ':')
		 p[0] = '\0';
	   }
	   if (strlen(frompath) >= 2 && isalpha(frompath[0]) && frompath[1] == ':')
	      setdisk(frompath[0] - 'A');
	   chdir(frompath);
	   old = fromname;
	   new = toname;
	}

	regs.h.ah = 0x56;
	segread(&sregs);
	sregs.ds  = FP_SEG(old);
	regs.x.dx = FP_OFF(old);
	sregs.es  = FP_SEG(new);
	regs.x.di = FP_OFF(new);
	intdosx(&regs,&regs,&sregs);

	if (changed) {
	   setdisk(cwd[0] - 'A');
	   chdir(cwd);
	}

	return (regs.x.cflag ? -1 : 0);
}/*rename()*/
#endif


int execute (char *cmd)
{
	char xfn[256], pars[128];
	char cwd[129];
	int res;
	boolean ptt_wasactive = ptt_active;

	if (cmd) {
	   char *p;

	   strcpy(xfn,cmd);
	   if ((p = strchr(xfn,' ')) != NULL) {
	      *p++ = '\0';
	      strcpy(pars,p);
	   }
	   else
	      pars[0] = '\0';
	}
	else
	   xfn[0] = pars[0] = '\0';

	getcwd(cwd,128);
	strupr(cwd);

	if (!fossil)
	   agl_close(false);
	if (ptt_wasactive) ptt_disable();
	res = do_exec(xfn, pars,
		      USE_ALL | XMS_FIRST | HIDE_FILE | CHECK_NET,
		      0xffff, environ);
	if (res == 0x0101)
	   res = do_exec(xfn, pars, USE_EMS | USE_XMS | XMS_FIRST, 0, environ);
	if (ptt_wasactive) ptt_enable();
	if (!fossil)
	   agl_open(com_handle);

	setdisk(*cwd - 'A');
	chdir(cwd);

	/*rescan = 0L;*/

	return (res);
}


int shell(char *cmd)
{
	return (execute(cmd));
}


/* end of syspc.c */
