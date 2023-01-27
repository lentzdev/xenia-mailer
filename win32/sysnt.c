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


#include <time.h>
#include <stdarg.h>
#include <process.h>
#include <signal.h>
#include <malloc.h>
#include <sys\utime.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef FAR
#undef FAR
#endif
#include "xenia.h"
#include "com_nt.h"
#include "wnfossil.h"


#define INBUF_SIZE  8192L
#define OUTBUF_SIZE 10240L

static HANDLE	  hcModem	= 0;		/* comm handle		  */
static HANDLE	  hRead 	= 0;		/* Read event handle	  */
static HANDLE	  hWrite	= 0;		/* Write event handle	  */
static OVERLAPPED ovRead;			/* Read overlapped stuct  */
static OVERLAPPED ovWrite;			/* Write overlapped stuct */
static DWORD	  FailSafeTimer = 6000;
static BOOL	  fWriteWait	= FALSE;	/* Now waiting for write  */

static short	  fDCD; 			/* Current carrier detect */
static HANDLE	  hDCD; 			/* Associated event	  */

static short	  fRXAvailable; 	    /* We think there's a character  */
static HANDLE	  hRXAvailable; 	    /* Associated event 	     */

static short	  fTXEmpty;		    /* We think transmitter's silent */

static ULONG	  SavedRate = 0;	    /* Last set baud rate	     */

DWORD FAR PASCAL stdComWatchProc (LPVOID nothing);
HANDLE hWatchThread = 0;		    /* Watch comm events      */
DWORD dwWatchThread = 0;		    /* Thread identifier      */
CRITICAL_SECTION csWatchThread; /* Critical section for watched events */

boolean shutdown = false;

#define COM_MAXBUFLEN	2048
static	byte   *com_inbuf = NULL, *com_outbuf = NULL;
static	word	com_inwrite,  com_outwrite,
		com_inread;

/* ------------------------------------------------------------------------- */
/* PTT telephone cost pulse counter - by Arjen G. Lentz, 2 Dec 1992	     */
void ptt_enable  (void) {}
void ptt_disable (void) {}
void ptt_reset	 (void) {}


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
#if 0
    if (hmod_comcapidll)
       dllComDTR(hc,flag ? TRUE : FALSE);
    else {
       MODEMSTATUS ms;
       USHORT SIOerror;
       ULONG  parmlen, datalen;

       if (flag) {
	  ms.fbModemOn	= DTR_ON;
	  ms.fbModemOff = 0xff;
       }
       else {
	  ms.fbModemOn	= 0;
	  ms.fbModemOff = DTR_OFF;
       }

       parmlen = sizeof (ms);
       datalen = sizeof (SIOerror);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETMODEMCTRL,
		   &ms, parmlen, &parmlen,
		   &SIOerror, datalen, &datalen);
    }
#endif
}


void com_flow (byte flags)
{
#if 0
    if (hmod_comcapidll)
       dllComXON(hc,(flags & 0x09) ? TRUE : FALSE);
    else {
       ULONG parmlen;

       if (flags & 0x09)
	  dcb.fbFlowReplace |= MODE_AUTO_TRANSMIT | MODE_AUTO_RECEIVE;
       else
	  dcb.fbFlowReplace &= ~(MODE_AUTO_TRANSMIT | MODE_AUTO_RECEIVE);

       parmlen = sizeof (dcb);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETDCBINFO,
		   &dcb, parmlen, &parmlen,
		   NULL, 0, NULL);
    }
#endif
}


void com_disk (boolean diskaccess)
{
    diskaccess = diskaccess;
}


void setspeed (dword speed, boolean force)	     /* set speed of comport */
{
    char buf[128];

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

#if 0
    if (mdm.lock && !force)
       speed = mdm.speed;
    speed /= mdm.divisor;

    if (hmod_comcapidll)
       dllComSetBaudRate(hc,(LONG) speed,'N',(USHORT) 8,(USHORT) 1);
    else {
       LINECTRL linectrl;
       EXTBITRATECONTROL extbitratecontrol;
       ULONG parmlen;

       linectrl.bDataBits = 8;
       linectrl.bParity   = 0;
       linectrl.bStopBits = 1;

       parmlen = sizeof (linectrl);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETLINECTRL,
		   &linectrl, parmlen, &parmlen,
		   NULL, 0, NULL);

       extbitratecontrol.dwBitRate = speed;
       extbitratecontrol.bFraction = 0;

       parmlen = sizeof (extbitratecontrol);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETEXTBITRATE,
		   &extbitratecontrol, parmlen, &parmlen,
		   NULL, 0, NULL);
    }
#endif
}


void com_putblock (byte *s, word len)
{
#if 0
    int i;

    if (com_outwrite)
       com_bufflush();

    if (!len) return;

    if (hmod_comcapidll) {
       USHORT bytesfree;

       i = 6000U;  /* 6000 x 10ms = 60 secs */
       do {
	  bytesfree = dllComOutSpace(hc);
	  if (bytesfree >= len) {
	     dllComWrite(hc,s,(USHORT) len);
	     break;
	  }
	  DosSleep(10);
       } while (carrier() && --i > 0);
    }
    else {
       ULONG BytesWritten;

       i = 60;
       do {
	  BytesWritten = 0;
	  DosWrite(com_handle,s,len,&BytesWritten);
	  s   += BytesWritten;
	  len -= (word) BytesWritten;
       } while (len && carrier() && --i > 0);
    }
#endif
}


void sys_down (int sig)
{
	sig = sig;
	shutdown = true;
	signal(SIGTERM,SIG_DFL);
}/*sys_down()*/


void sys_init(void)	    /* all system dependent init should be done here */
{
    static char *newprompt = NULL;
    char buffer[300];
    byte horpos, verpos;
    char *p;

    signal(SIGINT,SIG_IGN);
    signal(SIGTERM,sys_down);

    if (!newprompt) {
       sprint(buffer,"PROMPT=[%s Shell]$$_", progname);
       if ((p=getenv("PROMPT")) != NULL) strcat(buffer,p);
       else				 strcat(buffer,"$p$g");
       newprompt = strdup(buffer);
    }
    putenv(newprompt);

    if (mdm.port != -1) {
       if (com_suspend) {
#if 0
	  if (mdm.winfossil) {
	  }
	  else {
	     DCB dcb;
	     BOOL rc;

	     SetupComm(hcModem, INBUF_SIZE, OUTBUF_SIZE);
	     if (rc = GetCommState(hcModem, &dcb)) {
		dcb.fBinary	      = 1;
		dcb.fParity	      = 0;
		dcb.fOutxCtsFlow      = 1;
		dcb.fOutxDsrFlow      = 0;
		dcb.fDtrControl       = DTR_CONTROL_ENABLE;
		dcb.fDsrSensitivity   = 0;
		dcb.fTXContinueOnXoff = 0;
		dcb.fOutX = dcb.fInX  = 0;
		dcb.fErrorChar	      = 0;
		dcb.fNull	      = 0;
		dcb.fRtsControl       = RTS_CONTROL_HANDSHAKE;
		dcb.fAbortOnError     = 0;
		dcb.XoffLim	      = INBUF_SIZE - 512;
		dcb.XonLim	      = 512;

		SetCommState(hcModem, &dcb);
		if (SavedRate)
		   stdComSetParms(hcModem, SavedRate);
		EscapeCommFunction(hcModem, SETDTR);
	     }

	     if (hDCD == NULL) {
		hDCD = CreateEvent(NULL, TRUE, FALSE, NULL);
		//if (hDCD == NULL)
	     }

	     fRXAvailable = 1;
	     if (hRXAvailable == NULL) {
		hRXAvailable = CreateEvent(NULL, TRUE, FALSE, NULL);
		// if (hRXAvailable == NULL)
	     }

	     fTXEmpty = 1;

	     if (hWatchThread == NULL) {
		InitializeCriticalSection(&csWatchThread);
		hWatchThread = CreateThread(NULL, 2048, stdComWatchProc,
					    NULL, 0, &dwWatchThread);
		// if (hWatchThread == NULL)
	     }
	  }
#endif

	  com_suspend = false;
       }
       else {
	  win_getpos(log_win,&horpos,&verpos);
	  if (horpos == 1 && verpos == 1)
	     print("COM routines: %s\r", mdm.winfossil ? "WinFOSSIL" : "internal Win95/NT");

#if 0
	  if (mdm.winfossil) {
	  }
	  else {
	     DCB dcb;
	     BOOL rc;
	     SECURITY_ATTRIBUTES sa;
	     COMMTIMEOUTS pct;			/* Common to read stuff */

	     sa.nLength 	     = sizeof (sa);
	     sa.lpSecurityDescriptor = NULL;
	     sa.bInheritHandle	     = TRUE;

	     hcModem = CreateFile(mdm.comdev, GENERIC_READ | GENERIC_WRITE,
				  FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
				  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	     if (hcModem == (HANDLE) 0xFFFFFFFFUL) {
		win_deinit();
		fprint(stderr,"Xenia can't open %s (error %lu)\n", mdm.comdev, GetLastError());
		exit(1);
	     }

	     SetupComm(hcModem, INBUF_SIZE, OUTBUF_SIZE);
	     if (rc = GetCommState(hcModem, &dcb)) {
		dcb.fBinary	      = 1;
		dcb.fParity	      = 0;
		dcb.fOutxCtsFlow      = 1;
		dcb.fOutxDsrFlow      = 0;
		dcb.fDtrControl       = DTR_CONTROL_ENABLE;
		dcb.fDsrSensitivity   = 0;
		dcb.fTXContinueOnXoff = 0;
		dcb.fOutX = dcb.fInX  = 0;
		dcb.fErrorChar	      = 0;
		dcb.fNull	      = 0;
		dcb.fRtsControl       = RTS_CONTROL_HANDSHAKE;
		dcb.fAbortOnError     = 0;
		dcb.XoffLim	      = INBUF_SIZE - 512;
		dcb.XonLim	      = 512;

		SetCommState(hcModem, &dcb);
		EscapeCommFunction(hcModem, SETDTR);
	     }

	     hRead = CreateEvent(NULL, TRUE, TRUE, NULL);
	     //if (hRead == NULL)
	     memset(&ovRead, 0, sizeof (OVERLAPPED));
	     ovRead.hEvent = hRead;

	     hWrite = CreateEvent(NULL, TRUE, TRUE, NULL);
	     //if (hWrite == NULL)
	     memset(&ovWrite, 0, sizeof (OVERLAPPED));
	     ovWrite.hEvent = hWrite;
	     fWriteWait = FALSE;

	     GetCommTimeouts(hcModem, &pct);
	     pct.ReadIntervalTimeout	     = 10;
	     pct.ReadTotalTimeoutMultiplier  = 0;
	     pct.ReadTotalTimeoutConstant    = 500;
	     pct.WriteTotalTimeoutMultiplier = 0;
	     pct.WriteTotalTimeoutConstant   = 60000;
	     SetCommTimeouts(hcModem, &pct);

	     fDCD = 0;
	     hDCD = CreateEvent(NULL, TRUE, FALSE, NULL);
	     //if (hDCD == NULL)

	     fRXAvailable = 1;
	     hRXAvailable = CreateEvent (NULL, TRUE, FALSE, NULL);
	     //if (hRXAvailable == NULL)

	     fTXEmpty = 1;

	     InitializeCriticalSection(&csWatchThread);
	     hWatchThread = CreateThread(NULL, 2048, stdComWatchProc,
					 NULL, 0, &dwWatchThread);
	     //if (hWatchThread == NULL)

	     com_handle = hcModem;
	  }
#endif

	  if ((com_inbuf  = (byte *) malloc(COM_MAXBUFLEN + 16)) == NULL ||
	      (com_outbuf = (byte *) malloc(COM_MAXBUFLEN + 16)) == NULL) {
	     win_deinit();
	     fprint(stderr,"Can't allocate internal comm buffers\n");
	     exit (1);
	  }
       }

       com_flow(mdm.handshake);
       dtr_out(1);

       com_inread = com_inwrite = com_outwrite = 0;
    }

    win_setattrib(modem_win,xenwin_colours.modem.border);
    win_line(modem_win,1,1,windows[modem_win].win_horlen - 1,1,LINE_SINGLE);
    win_setattrib(modem_win,xenwin_colours.modem.title);
    win_setrange(modem_win,1,1,windows[modem_win].win_horlen - 1,1);
    win_xyputs(modem_win,2,1,mdm.comdev);
    win_setrange(modem_win,2,4,windows[modem_win].win_horlen - 1,11);
    win_setpos(modem_win,1,8);
    win_setattrib(modem_win,xenwin_colours.modem.normal);
}


void sys_reset(void)				/* same as above for de-init */
{
    if (mdm.port != -1) {
       if (com_suspend) {
#if 0
	  if (mdm.winfossil) {
	  }
	  else {
	     DCB  dcb;
	     BOOL rc;

	     FlushFileBuffers(hcModem);
	     com_purge();

	     TerminateThread(hWatchThread, 1);
	     hWatchThread = NULL;

	     CloseHandle(hDCD);
	     hDCD = NULL;

	     CloseHandle(hRXAvailable);
	     hRXAvailable = NULL;

	     DeleteCriticalSection(&csWatchThread);

	     if (rc = GetCommState(hcModem, &dcb)) {
		dcb.fBinary	      = 1;
		dcb.fParity	      = 0;
		dcb.fOutxCtsFlow      = 1;
		dcb.fOutxDsrFlow      = 0;
		dcb.fDtrControl       = DTR_CONTROL_ENABLE;
		dcb.fDsrSensitivity   = 0;
		dcb.fTXContinueOnXoff = 0;
		dcb.fOutX = dcb.fInX  = 0;
		dcb.fErrorChar	      = 0;
		dcb.fNull	      = 0;
		dcb.fRtsControl       = RTS_CONTROL_HANDSHAKE;
		dcb.fAbortOnError     = 0;

		SetCommState(hcModem, &dcb);
	     }
	  }
#endif
       }
       else {
#if 0
	  if (mdm.winfossil) {
	  }
	  else {
	     DWORD dwEventMask = 0;

	     if (hcModem) {
		TerminateThread(hWatchThread, 1);
		DeleteCriticalSection(&csWatchThread);
		CloseHandle(hDCD);
		CloseHandle(hRXAvailable);
		com_dump();
		com_purge();
		CloseHandle(hcModem);
		CloseHandle(hRead);
		CloseHandle(hWrite);
		hcModem = 0;
	     }
	  }
#endif

	  if (com_inbuf) free(com_inbuf);
	  com_inbuf = NULL;
	  if (com_outbuf) free(com_outbuf);
	  com_outbuf = NULL;

	  signal(SIGTERM,SIG_DFL);
       }
    }
}


int com_outfull (void)		/* Return the amount in fossil output buffer */
{				/* Ie. remaining no. bytes to be transmitted */
#if 0
    if (hmod_comcapidll)
       return (dllComOutCount(hc));
    else {
       RXQUEUE rxq;
       ULONG   datalen;

       datalen = sizeof (rxq);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_GETOUTQUECOUNT,
		   NULL, 0, NULL,
		   &rxq, datalen, &datalen);
       return (rxq.cch);
    }
#endif
}


int carrier(void)				    /* Return carrier status */
{
#if 0
    if (hmod_comcapidll)
       return (dllComIsOnline(hc));
    else {
       BYTE  flags;
       ULONG datalen;

       datalen = sizeof (flags);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_GETMODEMINPUT,
		   NULL, 0, NULL,
		   &flags, datalen, &datalen);
       return (flags & DCD_ON);
    }
#endif
}


void com_flush(void)		  /* wait till all characters have been sent */
{
#if 0
    int i = 3000;  /* 3000 x 10ms = 30 secs */

    if (com_outwrite)
       com_bufflush();

    do {
       if (!com_outfull())
	  break;
       DosSleep(10);
    } while (carrier() && --i > 0);
#endif
}


void com_putbyte (byte c)
{
#if 0
    int i;

    if (com_outwrite)
       com_bufflush();

    if (hmod_comcapidll) {
       i = 1000;  /* 1000 x 10ms = 10 secs */
       do {
	  if (dllComOutSpace(hc) >= 1) {
	     dllComPutc(hc,c);
	     break;
	  }
	  DosSleep(10);
       } while (--i > 0);
    }
    else {
       ULONG BytesWritten;

       i = 10;
       do {
	  BytesWritten = 0;
	  DosWrite(com_handle,&c,1,&BytesWritten);
       } while (BytesWritten != 1 && --i > 0);
    }
#endif
}


void com_bufbyte (byte c)
{
#if 0
    com_outbuf[com_outwrite++] = c;
    if (com_outwrite >= COM_MAXBUFLEN) {
       register word len = com_outwrite;

       com_outwrite = 0;
       com_putblock(com_outbuf,len);
    }
#endif
}


void com_bufflush (void)
{
#if 0
    if (com_outwrite) {
       register word len = com_outwrite;

       com_outwrite = 0;
       com_putblock(com_outbuf,len);
    }
#endif
}


void com_purge (void)
{
#if 0
    if (hmod_comcapidll)
       dllComPurge(hc,COMM_PURGE_RX);
    else {
       BYTE  bCmdInfo;
       ULONG parmlen;

       bCmdInfo = 0;
       parmlen = sizeof (bCmdInfo);
       DosDevIOCtl(com_handle, IOCTL_GENERAL, DEV_FLUSHINPUT,
		   &bCmdInfo, parmlen, &parmlen,
		   NULL, 0, NULL);
    }

    com_inread = com_inwrite = 0;
#endif
}


void com_dump (void)
{
#if 0
    if (hmod_comcapidll)
       dllComPurge(hc,COMM_PURGE_TX);
    else {
       BYTE  bCmdInfo;
       ULONG parmlen;

       bCmdInfo = 0;
       parmlen = sizeof (bCmdInfo);
       DosDevIOCtl(com_handle, IOCTL_GENERAL, DEV_FLUSHOUTPUT,
		   &bCmdInfo, parmlen, &parmlen,
		   NULL, 0, NULL);
    }

    com_outwrite = 0;
#endif
}


int com_scan(void)			     /* Check for characters waiting */
{
#if 0
    if (com_inread < com_inwrite)
       return (1);

    if (hmod_comcapidll)
       dllComRead(hc,com_inbuf,COM_MAXBUFLEN,&com_inwrite);
    else {
       APIRET rc;
       ULONG BytesRead;

       com_inwrite = 0;
       rc = DosRead(com_handle,com_inbuf,COM_MAXBUFLEN,&BytesRead);
       if (!rc || rc == ERROR_MORE_DATA)
	  com_inwrite = (word) BytesRead;
    }

    com_inread = 0;

    return (com_inwrite);
#endif
}


int com_getc(int timeout)
{
#if 0
    if ((com_inread >= com_inwrite) && !com_scan()) {
       long t = timerset((word) timeout);

       while (!com_scan())
	     if (timeup(t)) return (EOF);
    }

    return (com_inbuf[com_inread++]);
#endif
}


int com_getbyte(void)
{
#if 0
    return (((com_inread < com_inwrite) || com_scan()) ?
	    com_inbuf[com_inread++] : EOF);
#endif
}


void com_break(void)
{
#if 0
    com_flush();

    if (hmod_comcapidll) {
       dllComBreak(hc,TRUE);
       DosSleep(1000);
       dllComBreak(hc,FALSE);
    }
    else {
       USHORT SIOerror;
       ULONG  datalen;

       datalen = sizeof (SIOerror);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETBREAKON,
		   NULL, 0, NULL,
		   &SIOerror, datalen, &datalen);

       DosSleep(1000);

       datalen = sizeof (SIOerror);
       DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETBREAKOFF,
		   NULL, 0, NULL,
		   &SIOerror, datalen, &datalen);
    }
#endif
}


void setstamp(char *name,long tim)		    /* Set time/date of file */
{
    struct utimbuf FileTime;

    FileTime.actime=FileTime.modtime=tim;

    utime(name,&FileTime);
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


long freespace (char *drivepath)   /* get free diskspace for specified drive */
{
    unsigned driveno;
    struct _diskfree_t diskinfo;
    long stat;

    if (drivepath[0] != '\0' && drivepath[1] == ':') {
       driveno = (unsigned) (islower(*drivepath) ? toupper(*drivepath) : *drivepath);
       driveno = (unsigned) (driveno - 'A' + 1);
    }
    else
       driveno = 0;			/* Default drive */

    if (_getdiskfree(driveno, &diskinfo) != 0)
       return (0L);

    stat = diskinfo.avail_clusters *
	   diskinfo.sectors_per_cluster *
	   diskinfo.bytes_per_sector;

    return (stat < 0L ? 0x7fffffffL : stat);
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


void MakeShortName (char *Name, char *ShortName)
{
    int r1 = 0,
	r2 = 0,
	r3 = 0;

    if (strchr(Name,'.') == NULL) {
       strncpy(ShortName,Name,8);
       ShortName[8] = 0;
    }
    else {
       while (r1 < 8 && Name[r1] != '.' && Name[r1] != '\0')
	     ShortName[r2++] = Name[r1++];

       ShortName[r2++] = '.';

       r1 = strlen(Name) - 1;

       ShortName[r2] = '\0';

       while (r1 > 0 && Name[r1] != '.' && Name[r1] != '\0' && r3 < 3) {
	     r1--;
	     r3++;
       }

       r1++;

       strcat(ShortName,&Name[r1]);
    }

    strlwr(ShortName);
}


int execute (char *cmd)
{
    int res;

    if (!cmd || !cmd[0]) {
       cmd = getenv("COMSPEC");
       if (!cmd) cmd = "CMD.EXE";
    }
#if 0
    if (hmod_comcapidll && hc && strncmp(mdm.comdev,"CAPI",4))
       dllComPause(hc);
#endif
    res = system(cmd);
#if 0
    if (hmod_comcapidll && hc && strncmp(mdm.comdev,"CAPI",4))
       dllComResume(hc);
#endif

    return (res);
}


int shell(char *cmd)
{
    return (execute(cmd));
}


/* end of sysnt.c */
