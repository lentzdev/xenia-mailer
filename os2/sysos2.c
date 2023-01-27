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
#define INCL_BASE
#define INCL_DOSDEVIOCTL
#define INCL_DOSNMPIPES
#define INCL_DOSMODULEMGR
#define INCL_NOPMAPI			  /* BC/2 define		     */
#define INCL_NOPM			  /* used in other compilers	     */
#include <os2.h>
#include "xenia.h"
#define ASYNC_SETEXTBITRATE  0x0043
#define ASYNC_GETEXTBITRATE  0x0063
typedef struct _LINECTRL {
     BYTE bDataBits;
     BYTE bParity;
     BYTE bStopBits;
} LINECTRL;
typedef struct _EXTBITRATECONTROL {
     ULONG dwBitRate;
     BYTE  bFraction;
} EXTBITRATECONTROL;
#include "comcapi.h"


static	HCOMM	hc = NULL;
static	DCBINFO old_dcb, dcb;
static	boolean restore_dcb = false;
boolean shutdown = false;

#define COM_MAXBUFLEN	2048
static	byte   *com_inbuf = NULL, *com_outbuf = NULL;
static	word	com_inwrite,  com_outwrite,
		com_inread;

/* ------------------------------------------------------------------------- */
static HMODULE hmod_snsrv_32dll = 0UL;
static HMODULE hmod_mcp32dll	= 0UL;
static HMODULE hmod_comcapidll	= 0UL;

USHORT APIENTRY (*dllSnoopOpen2)  (PSZ pszPipeName, PHSNOOP phSnoop, PSZ pszAppName, PFNSN2 NotifyFunc);
USHORT APIENTRY (*dllSnoopWrite2) (HSNOOP hsn, PSZ pszMessage);
USHORT APIENTRY (*dllSnoopClose2) (HSNOOP hsn);

int APIENTRY (*dllMcpSendMsg) (HPIPE hp, USHORT usType, BYTE *pbMsg, USHORT cbMsg);

USHORT COMMAPI (*dllComOpen)	    (PSZ PortName, HCOMM *pHcomm, USHORT RxBufSize, USHORT TxBufSize);
USHORT COMMAPI (*dllComClose)	    (HCOMM hcomm);
USHORT COMMAPI (*dllComIsOnline)    (HCOMM hcomm);
USHORT COMMAPI (*dllComWrite)	    (HCOMM hc, PVOID pvBuf, USHORT cbBuf);
USHORT COMMAPI (*dllComRead)	    (HCOMM hc, PVOID pvBuf, USHORT cbBuf, PUSHORT pcbBytesRead);
USHORT COMMAPI (*dllComPutc)	    (HCOMM hc, SHORT c);
USHORT COMMAPI (*dllComOutCount)    (HCOMM hc);
USHORT COMMAPI (*dllComOutSpace)    (HCOMM hc);
USHORT COMMAPI (*dllComPurge)	    (HCOMM hc,	SHORT fsWhich);
USHORT COMMAPI (*dllComPause)	    (HCOMM hc);
USHORT COMMAPI (*dllComResume)	    (HCOMM hc);
HFILE  COMMAPI (*dllComGetFH)	    (HCOMM hc);
USHORT COMMAPI (*dllComGetDCB)	    (HCOMM hc, PDCBINFO pdbc);
USHORT COMMAPI (*dllComSetDCB)	    (HCOMM hc, PDCBINFO pdcb);
USHORT COMMAPI (*dllComSetBaudRate) (HCOMM hcomm, LONG bps, CHAR parity, USHORT databits, USHORT stopbits);
USHORT COMMAPI (*dllComDTR)	    (HCOMM hc, BOOL OnOff);
USHORT COMMAPI (*dllComXON)	    (HCOMM hc, BOOL OnOff);
USHORT COMMAPI (*dllComBreak)	    (HCOMM hc, BOOL OnOff);

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
}


void com_flow (byte flags)
{
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
}


void com_putblock (byte *s, word len)
{
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
}


#if 0 /*WATCOM*/
static short CALLBACK PFNSN notifyfunc (short error, char * _Seg16 _Far16 str)
#else
static short APIENTRY notifyfunc (short error, char *str)
#endif
{
    if (!error) message(0," %s", str);
    else	message(6,"!SYS%04u : %s", error, str);
    return(0);
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
    char obj[9];
    APIRET rc;
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
	  if (hmod_comcapidll) {
	     if (strncmp(mdm.comdev,"CAPI",4))
		dllComResume(hc);
	  }
	  else {
	     ULONG parmlen;

	     parmlen = sizeof (dcb);
	     DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETDCBINFO,
			 &dcb, parmlen, &parmlen,
			 NULL, 0, NULL);
	  }

	  com_suspend = false;
       }
       else {
	  if (!strncmp(mdm.comdev,"CAPI",4)) {
	     if (DosLoadModule((PSZ) obj, sizeof (obj), (PSZ) "COMCAPI", &hmod_comcapidll) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComOpen"	  ,(PFN *) &dllComOpen	     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComClose"	  ,(PFN *) &dllComClose      ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComIsOnline"   ,(PFN *) &dllComIsOnline   ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComWrite"	  ,(PFN *) &dllComWrite      ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComRead"	  ,(PFN *) &dllComRead	     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComPutc"	  ,(PFN *) &dllComPutc	     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComOutCount"   ,(PFN *) &dllComOutCount   ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComOutSpace"   ,(PFN *) &dllComOutSpace   ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComPurge"	  ,(PFN *) &dllComPurge      ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComPause"	  ,(PFN *) &dllComPause      ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComResume"	  ,(PFN *) &dllComResume     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComGetFH"	  ,(PFN *) &dllComGetFH      ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComGetDCB"	  ,(PFN *) &dllComGetDCB     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComSetDCB"	  ,(PFN *) &dllComSetDCB     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComSetBaudRate",(PFN *) &dllComSetBaudRate) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComDTR"	  ,(PFN *) &dllComDTR	     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComXON"	  ,(PFN *) &dllComXON	     ) ||
		 DosQueryProcAddr(hmod_comcapidll,0,(PSZ) "ComBreak"	  ,(PFN *) &dllComBreak      )) {
		message(6,"-Can't load/query COMCAPI.DLL");
		if (hmod_comcapidll) {
		   DosFreeModule(hmod_comcapidll);
		   hmod_comcapidll = 0UL;
		}
	     }
	  }

	  win_getpos(log_win,&horpos,&verpos);
	  if (horpos == 1 && verpos == 1)
	     print("COM routines: %s\r",hmod_comcapidll ? "COMCAPI" : "internal OS/2");
	  if (hmod_comcapidll)
	     rc = dllComOpen((PSZ) mdm.comdev,&hc,4096,4096);
	  else {
	     ULONG action;

	     rc = DosOpen((PSZ) mdm.comdev, (PHFILE) &com_handle, &action, 0UL,
			  FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
			  OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,
			  NULL);
	  }

	  if (rc) {
	     win_deinit();
	     fprint(stderr,"Xenia can't open %s (error %lu)\n", mdm.comdev, rc);
	     exit(1);
	  }

	  if (hmod_comcapidll)
	     com_handle = dllComGetFH(hc);

	  if ((com_inbuf  = (byte *) malloc(COM_MAXBUFLEN + 16)) == NULL ||
	      (com_outbuf = (byte *) malloc(COM_MAXBUFLEN + 16)) == NULL) {
	     win_deinit();
	     fprint(stderr,"Can't allocate internal comm buffers\n");
	     exit (1);
	  }

	  if (hmod_comcapidll) {
	     if (!cmdlineanswer && !dllComGetDCB(hc,&old_dcb)) {
		restore_dcb = true;
		memmove(&dcb,&old_dcb,sizeof (DCBINFO));

		dcb.fbCtlHndShake &= ~MODE_DTR_HANDSHAKE;
		dcb.fbCtlHndShake |= MODE_DTR_CONTROL;

		dllComSetDCB(hc,&dcb);
	     }
	  }
	  else {
	     ULONG parmlen, datalen;

	     memset(&old_dcb,0,sizeof (DCBINFO));
	     datalen = sizeof (old_dcb);
	     DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_GETDCBINFO,
			 NULL, 0, NULL,
			 &old_dcb, datalen, &datalen);

	     restore_dcb = true;
	     memmove(&dcb,&old_dcb,sizeof (DCBINFO));

	     dcb.usWriteTimeout = 100;
	     dcb.usReadTimeout	= 10;

	     if (ourbbsflags & NO_SETDCB) {
		dcb.fbTimeout &= ~(MODE_NOWAIT_READ_TIMEOUT | MODE_NO_WRITE_TIMEOUT);
		dcb.fbTimeout |= MODE_WAIT_READ_TIMEOUT;
	     }
	     else {
		dcb.fbCtlHndShake &= ~(MODE_DTR_HANDSHAKE | MODE_DSR_HANDSHAKE | MODE_DCD_HANDSHAKE | MODE_DSR_SENSITIVITY);
		dcb.fbCtlHndShake |= MODE_DTR_CONTROL;
		dcb.fbFlowReplace &= ~(MODE_RTS_CONTROL | MODE_ERROR_CHAR | MODE_NULL_STRIPPING | MODE_BREAK_CHAR);
		dcb.fbFlowReplace |= 0x20;
		dcb.fbTimeout &= ~(MODE_NOWAIT_READ_TIMEOUT | MODE_NO_WRITE_TIMEOUT);
		dcb.fbTimeout |= 0x80 | 0x40 | 0x10 | MODE_WAIT_READ_TIMEOUT;
		dcb.bErrorReplacementChar = 0;
		dcb.bBreakReplacementChar = 0;
		dcb.bXONChar  = 0x11;
		dcb.bXOFFChar = 0x13;

		if (mdm.handshake & 0x02) {
		   dcb.fbCtlHndShake |= MODE_CTS_HANDSHAKE;
		   dcb.fbFlowReplace |= MODE_RTS_HANDSHAKE;
		}
	     }

	     parmlen = sizeof (dcb);
	     DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETDCBINFO,
			 &dcb, parmlen, &parmlen,
			 NULL, 0, NULL);

#if 0
	     memset(&dcb,0,sizeof (DCBINFO));
	     datalen = sizeof (dcb);
	     if (!DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_GETDCBINFO,
			      NULL, 0, NULL,
			      &dcb, datalen, &datalen)) {
		message(0,">us*Timeout Read=%u Write=%u", (word) dcb.usWriteTimeout, (word) dcb.usReadTimeout);
		message(0,">fbCtlHndShake         %02x", (word) dcb.fbCtlHndShake);
		message(0,">fbFlowReplace         %02x", (word) dcb.fbFlowReplace);
		message(0,">fbTimeout             %02x", (word) dcb.fbTimeout);
		message(0,">bErrorReplacementChar %u", (word) dcb.bErrorReplacementChar);
		message(0,">bBreakReplacementChar %u", (word) dcb.bBreakReplacementChar);
		message(0,">bXONChar=%u bXOFFChar=%u", (word) dcb.bXONChar, (word) dcb.bXOFFChar);
	     }
#endif
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

    if (pipename) {
       if (DosLoadModule((PSZ) obj, sizeof (obj), (PSZ) "SNSRV_32", &hmod_snsrv_32dll) ||
	   DosQueryProcAddr(hmod_snsrv_32dll,0,(PSZ) "SnoopOpen2",(PFN *) &dllSnoopOpen2) ||
	   DosQueryProcAddr(hmod_snsrv_32dll,0,(PSZ) "SnoopWrite2",(PFN *) &dllSnoopWrite2) ||
	   DosQueryProcAddr(hmod_snsrv_32dll,0,(PSZ) "SnoopClose2",(PFN *) &dllSnoopClose2)) {
	  message(6,"-Can't load/query SNSRV_32.DLL");
	  if (hmod_snsrv_32dll) {
	     DosFreeModule(hmod_snsrv_32dll);
	     hmod_snsrv_32dll = 0UL;
	  }
       }
    }

    if (pipename && hmod_snsrv_32dll) {
       char buf[128];

       sprint(buf,"%s - Task %u", progname, task);
#if 0 /*WATCOM*/
       rc = SnoopOpen((PSZ _Seg16) pipename, (PHSNOOP _Seg16) &hsnoop,
		       (PSZ _Seg16) buf, (PFNSN) notifyfunc);
#else
       rc = dllSnoopOpen2((PSZ) pipename, &hsnoop, (PSZ) buf, (PFNSN2) notifyfunc);
#endif
       if (rc) message(6,"-Can't init snoopserver for pipe %s (error %lu)",pipename,rc);
    }

    if (MCPbase && !hpMCP) {
       if (DosLoadModule((PSZ) obj, sizeof (obj), (PSZ) "MCP32", &hmod_mcp32dll) ||
	   DosQueryProcAddr(hmod_mcp32dll,0,(PSZ) "McpSendMsg",(PFN *) &dllMcpSendMsg)) {
	  message(6,"-Can't load/query MCP32.DLL");
	  if (hmod_mcp32dll) {
	     DosFreeModule(hmod_mcp32dll);
	     hmod_mcp32dll = 0UL;
	  }
       }
    }

    if (MCPbase && !hpMCP && hmod_mcp32dll) {
       char   szPipe[PATHLEN];
       ULONG  usAction;
       int    fFirst;
       int    fOpened;
       byte   tid = task;
       long   lTryTime;

       sprint(szPipe,"%s\\maximus", MCPbase);

       fFirst = TRUE;

       lTryTime = timerset(50);

       do {
	  fOpened = FALSE;

	  rc = DosOpen((PSZ) szPipe, &hpMCP, &usAction, 0L, FILE_NORMAL,
		       OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
		       OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE |
		       OPEN_FLAGS_NOINHERIT, NULL);

	  if (rc == 0)
	     fOpened = TRUE;
	  else {
	     if (!fFirst)
		DosSleep(100L);
	     else {
		RESULTCODES rcd;
		ULONG  ulState;
		char   szFailObj[PATHLEN];
		char   szMcpString[PATHLEN];
		char  *psz;

		message(0,"+Starting Master Control Program");

		fFirst = FALSE;

		strcpy(szMcpString, "mcp.exe");
		psz = szMcpString + strlen(szMcpString) + 1;

		sprint(psz, ". %s %d server", MCPbase, MCPtasks);

		if (mdm.port != -1 && strncmp(mdm.comdev,"CAPI",4)) {
		   DosQueryFHState(com_handle, &ulState);
		   ulState &= 0x7f88;
		   DosSetFHState(com_handle, ulState | OPEN_FLAGS_NOINHERIT);
		}

		rc = DosExecPgm(szFailObj,
				PATHLEN,
				EXEC_BACKGROUND,
				(PSZ) szMcpString,
				NULL,
				&rcd,
				(PSZ) szMcpString);

		if (mdm.port != -1 && strncmp(mdm.comdev,"CAPI",4))
		   DosSetFHandState(com_handle, ulState);

		if (rc) {
		   message(6,"-Can't start Master Control Program (%lu)",rc);
		   break;
		}
	     }
	  }
       } while (!fOpened && !timeup(lTryTime));

       if (fOpened)
	  dllMcpSendMsg(hpMCP, PMSG_HELLO, &tid, 1);
       else {
	  hpMCP = 0;
	  message(6,"-Couldn't open MCP pipe '%s'", szPipe);
       }
    }
}


void sys_reset(void)				/* same as above for de-init */
{
    if (hsnoop != (HSNOOP) NULL) {
#if 0 /*WATCOM*/
       SnoopClose(hsnoop);
#else
       dllSnoopClose2(hsnoop);
#endif
       hsnoop = (HSNOOP) NULL;
    }
    if (hmod_snsrv_32dll) {
       DosFreeModule(hmod_snsrv_32dll);
       hmod_snsrv_32dll = 0UL;
    }

    if (hpMCP) {
       dllMcpSendMsg(hpMCP, PMSG_EOT, NULL, 0);
       DosClose(hpMCP);
       hpMCP = (HPIPE) NULL;
    }
    if (hmod_mcp32dll) {
       DosFreeModule(hmod_mcp32dll);
       hmod_mcp32dll = 0UL;
    }

    if (mdm.port != -1) {
       if (hmod_comcapidll && !strncmp(mdm.comdev,"CAPI",4))
	  com_suspend = true;

       if (com_suspend) {
	  if (hmod_comcapidll && strncmp(mdm.comdev,"CAPI",4))
	     dllComPause(hc);
       }
       else {
	  if (hmod_comcapidll) {
	     if (restore_dcb)
		dllComSetDCB(hc,(PDCBINFO) &old_dcb);

	     dllComClose(hc);
	     hc = NULL;
	     DosFreeModule(hmod_comcapidll);
	     hmod_comcapidll = 0UL;
	  }
	  else {
	     if (restore_dcb) {
		ULONG parmlen;

		parmlen = sizeof (old_dcb);
		DosDevIOCtl(com_handle, IOCTL_ASYNC, ASYNC_SETDCBINFO,
			    &old_dcb, parmlen, &parmlen,
			    NULL, 0, NULL);
	     }

	     DosClose(com_handle);
	  }

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
}


int carrier(void)				    /* Return carrier status */
{
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
}  


void com_flush(void)		  /* wait till all characters have been sent */
{
    int i = 3000;  /* 3000 x 10ms = 30 secs */

    if (com_outwrite)
       com_bufflush();

    do {
       if (!com_outfull())
	  break;
       DosSleep(10);
    } while (carrier() && --i > 0);
}


void com_putbyte (byte c)
{
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
}


void com_dump (void)
{
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
}


int com_scan(void)			     /* Check for characters waiting */
{
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
}


int com_getc(int timeout)
{
    if ((com_inread >= com_inwrite) && !com_scan()) {
       long t = timerset((word) timeout);

       while (!com_scan())
	     if (timeup(t)) return (EOF);
    }

    return (com_inbuf[com_inread++]);
}


int com_getbyte(void)
{
    return (((com_inread < com_inwrite) || com_scan()) ?
	    com_inbuf[com_inread++] : EOF);
}


void com_break(void)
{
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
    int   drive;
    ULONG Buff[5];
    long  bytes;

    if (drivepath == NULL)
       return (0);

    memset(Buff,0,sizeof Buff);

    if (drivepath[0] && drivepath[1] == ':' && isalpha(drivepath[0]))
       drive = (toupper(drivepath[0]) - '@');
    else
       drive = 0;

    DosQueryFSInfo(drive, FSIL_ALLOC, Buff, sizeof (Buff));

    bytes = (long) (Buff[1] * Buff[3] * Buff[4]);
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


#if 0
void SetLongName (char *fname, char *longfname)
{
    char  TfPath[512];
    HFILE hFile;
    ULONG Action;

    strcpy(TfPath,download);
    strcat(TfPath,longfname);

    switch (DosOpen((PUCHAR) TfPath,&hFile,&Action,0,FILE_NORMAL,OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,OPEN_FLAGS_FAIL_ON_ERROR|OPEN_SHARE_DENYNONE,NULL)) {
	   case NO_ERROR:
		DosClose(hFile);
		break;

	   case ERROR_FILENAME_EXCED_RANGE:
		return;
    }

    strcpy(fname,longfname);
}
#endif


int execute (char *cmd)
{
    int res;

    if (!cmd || !cmd[0]) {
       cmd = getenv("COMSPEC");
       if (!cmd) cmd = "CMD.EXE";
    }
    if (hmod_comcapidll && hc && strncmp(mdm.comdev,"CAPI",4))
       dllComPause(hc);
    res = system(cmd);
    if (hmod_comcapidll && hc && strncmp(mdm.comdev,"CAPI",4))
       dllComResume(hc);

    /*rescan = 0L;*/

    return (res);
}


int shell(char *cmd)
{
    return (execute(cmd));
}


#if 0
void set_idle (boolean flag)
{
	DosSetPriority(PRTYS_THREAD,
		       flag ? PRTYC_IDLETIME : PRTYC_REGULAR,
		       PRTYD_MAXIMUM,
		       0UL);
}
#endif


void mcp_sleep (void)
{
	static long time_ping = 0L;

	if (hpMCP && timeup(time_ping)) {
	   dllMcpSendMsg(hpMCP, PMSG_PING, NULL, 0);

	   time_ping = timerset(600);  /* One minute timer */
	}
}


/* end of sysos2.c */
