#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <mem.h>
#include <string.h>
#include "mcp.h"


static struct _cstat cstat;
word cur_ch    = -1;
word num_task  = 0;
word *tasks    = NULL;
long scan_time = NORM_SCAN_TIME;
HPIPE hpMCP;


/* ------------------------------------------------------------------------- */
/* Send a ping to the server every two minutes to show that we're awake */

void mcp_sleep (void)
{
    static long time_ping = 0L;

    if (hpMCP && timeup(time_ping)) {
       McpSendMsg(hpMCP, PMSG_PING, NULL, 0);

       /* Two minute timeout */
       time_ping=timerset(2 * 60 * 100);
    }
}


/* ------------------------------------------------------------------------- */
void SendVideoDump(void)
{
    VIO_DUMP_HDR *pdh;
    int   i, size;
    byte *buf, *bufp;

    if ((buf = bufp = malloc(size = win->s_width * win->s_height * 2 + sizeof *pdh)) == NULL) {
       logit(mem_none);
       return;
    }

    pdh = (VIO_DUMP_HDR *)bufp;

    pdh->bHeight  = (BYTE) win->s_height;
    pdh->bWidth   = (BYTE) win->s_width;
    pdh->bCurRow  = (BYTE) win->row;
    pdh->bCurCol  = (BYTE) win->col;
    pdh->bCurAttr = (BYTE) curattr;

    bufp += sizeof *pdh;

    for (i=0; i < win->s_height; i++) {
	memmove(bufp, win->rowtable[i], win->s_width*2);
	bufp += win->s_width*2;
    }

    /* Send the screen dump to the client */

    McpSendMsg(hpMCP, PMSG_VIO_DUMP, buf, size);

    free(buf);
}


/* ------------------------------------------------------------------------- */
/* Set our current activity status.  This sends a message to MCP and	  *
 * sets our status in one of its internal fields.			  */

void ChatSetStatus (int avail, char *status)
{
    if (!hpMCP)
       return;

    cstat.avail = avail;
    strcpy(cstat.username, usrname);
    strcpy(cstat.status, status);

    McpSendMsg(hpMCP, PMSG_SET_STATUS, (BYTE *) &cstat, sizeof cstat);
}


/* ------------------------------------------------------------------------- */
/* Send an interprocess message directly to another Maximus node.  This   *
 * does not use the PipeSendMsg function for speed reasons; by directly   *
 * filling in the message type, we only need to shift the buffer	  *
 * around in memory once, not twice.					  */

int ChatSendMsg (byte dest_tid, int type, int len, char *msg)
{
    struct _cdat *pcd;
    BYTE    *buf;
    OS2UINT  rc, cbWritten;

    if (!hpMCP)
       return (-1);

    if ((buf = malloc(len + sizeof (USHORT) + sizeof (*pcd))) == NULL) {
       logit(mem_none);
       return (-1);
    }

    *(USHORT *) buf = PMSG_MAX_SEND_MSG;

    pcd = (struct _cdat *) (buf + sizeof (USHORT));
    pcd->tid	  = task_num;
    pcd->dest_tid = dest_tid;
    pcd->type	  = type;
    pcd->len	  = len;

    memmove(buf + sizeof (USHORT) + sizeof (*pcd), msg, len);

    if ((rc = DosWrite(hpMCP, buf, len + sizeof (USHORT) + sizeof (*pcd),
		       &cbWritten)) != 0) {
       logit("!SYS%04d: DosWrite MCP 2", rc);
       return (-1);
    }

    free(buf);

    return (0);
}


/* ------------------------------------------------------------------------- */
/* Begin an IPC session with MCP */

int ChatOpenMCP (void)
{
    char szPipe[PATHLEN];
    OS2UINT rc, usAction;
    int  fFirst;
    int  fOpened;
    byte tid = task_num;
    long lTryTime;

    /* IPC already open */

    if (hpMCP)
       return (-1);

    /* The place for us to connect is the "\maximus" side off the root
     * MCP "path".
     */

    if (!*szMcpPipe)
       strcpy(szMcpPipe, "\\pipe\\maximus\\mcp");

    strcpy(szPipe, szMcpPipe);
    strcat(szPipe, "\\maximus");

    fFirst = TRUE;

    /* Try to start MCP for up to five seconds */

    lTryTime = timerset(500);

    do {
       fOpened = FALSE;

       rc = DosOpen(szPipe, &hpMCP, &usAction, 0L, FILE_NORMAL,
		    OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
		    OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE |
		    OPEN_FLAGS_NOINHERIT, NULL);

       if (rc == 0)
	  fOpened = TRUE;
       else {
	  if (!fFirst)
	     DosSleep(100L);
	  else {
	     extern HCOMM hcModem;
	     RESULTCODES rcd;
	     ULONG  ulState;
	     HFILE  hf;
	     char   szFailObj[PATHLEN];
	     char   szMcpString[PATHLEN];
	     char  *psz;

	     logit(log_starting_mcp);

	     fFirst = FALSE;

	     strcpy(szMcpString, "mcp.exe");
	     psz = szMcpString + strlen(szMcpString) + 1;

	     sprintf(psz, ". %s %d server\0", szMcpPipe, prm.mcp_sessions);

	     /* Add an extra nul at the end as per convention */

	     psz[strlen(psz)] = 0;


	     /* Now ensure that MCP doesn't inherit our modem handle */

	     if (!local) {
		hf = ComGetFH(hcModem);

		DosQueryFHState(hf, &ulState);
		ulState &= 0x7f88;
		DosSetFHState(hf, ulState | OPEN_FLAGS_NOINHERIT);
	     }

	     /* Try to start MCP as a detached process */

	     rc = DosExecPgm(szFailObj,
			     PATHLEN,
			     EXEC_BACKGROUND,
			     szMcpString,
			     NULL,
			     &rcd,
			     szMcpString);

	     /* Restore the modem handle back to its original settings */

	     if (!local)
		DosSetFHandState(hf, ulState);

	     /* If we can't start MCP server, put a msg in the log */

	     if (rc) {
		logit(log_mcp_err_2, rc);
		break;
	     }
	  }
       }
    } while (!fOpened && !timeup(lTryTime));

    if (fOpened)
       McpSendMsg(hpMCP, PMSG_HELLO, &tid, 1);
    else {
       hpMCP = 0;
       logit(log_no_mcp, szMcpPipe);
    }

    return (0);
}


/* ------------------------------------------------------------------------- */
/* Terminate the IPC session */

void ChatCloseMCP(void)
{
    if (hpMCP) {
       McpSendMsg(hpMCP, PMSG_EOT, NULL, 0);
       DosClose(hpMCP);
       hpMCP = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* This function is called to dispatch messages of types other		  *
 * than PMSG_MAX_SEND_MSG.						  */

int usStrokes;
byte cbStrokeBuf[MAX_BUF_STROKE];


static void near ChatDispatchMsg (byte *pbMsg, USHORT cbMsg)
{
    USHORT usType = *(USHORT *) pbMsg;
    USHORT usAdd;

    if (!hpMCP)
       return;

    switch (usType) {
	   case RPMSG_HAPPY_DAGGER:
		logit("!MCP-initiated shutdown");
		ChatSetStatus(FALSE, "Terminating...");
		quit(ERROR_KEXIT);
		break;

	   case RPMSG_MONITOR:
		mcp_video = !!pbMsg[2];

		/* If we just enabled video monitoring, send a dump of our *
		 * video buffer.					   */

		if (mcp_video)
		   SendVideoDump();
		break;

		/* If the user hit ^c... */

	   case RPMSG_CTRLC:
	   case RPMSG_BREAK:
		brk_trapped++;
		break;

		/* Add characters to our local keyboard buffer */

	   case RPMSG_KEY:
		cbMsg -= 2;
		usAdd = min(MAX_BUF_STROKE-usStrokes, cbMsg);

		memmove(cbStrokeBuf + usStrokes, pbMsg + sizeof (USHORT), usAdd);

		usStrokes += usAdd;
		break;
    }
}


/* ------------------------------------------------------------------------- */
/* Retrieve a message from the MCP server */

static int near ChatGetMsg (byte *tid, int *type, int *len, char *msg, int maxlen)
{
    AVAILDATA ad;
    OS2UINT   fsState;
    OS2UINT   rc, usGot;
    BYTE      buf;

    if (!hpMCP)
       return (-1);

    if ((rc = DosPeekNmPipe(hpMCP, &buf, 1, &usGot, &ad, &fsState)) != 0)
       return (-1);

    /* No messages available */

    if (!usGot)
       return (1);

    if (ad.cbmessage > maxlen) {
       logit("!MCP message truncated (%d)", ad.cbmessage);
       ad.cbmessage = maxlen;
    }

    if ((rc = DosRead(hpMCP, msg, ad.cbmessage, &usGot)) != 0)
       logit("!SYS%04d: MCP DosRead", rc);
    else if (usGot != ad.cbmessage)
       logit("!Could not read all MCP data from pipe %d/%d", usGot, ad.cbmessage);
    else {
       struct _cdat *pcd;

       if (*(USHORT *)msg == RPMSG_GOT_MSG) {
	  /* Extract the header data */

	  pcd = (struct _cdat *) (msg + sizeof (USHORT));
	  *tid	= pcd->tid;
	  *type = pcd->type;
	  *len	= pcd->len;

	  /* Now shift the message back to hide the header data */

	  memmove(msg, msg + sizeof (USHORT) + sizeof *pcd, min(ad.cbmessage, *len));

	  return 0;
       }
       else
	  ChatDispatchMsg(msg, ad.cbmessage);
    }

    return (1);
}


/* ------------------------------------------------------------------------- */
/* These two functions save and restore the current CHAT availability and   *
 * status.								    */

void ChatSaveStatus (struct _css *css)
{
    css->avail = cstat.avail;
    strcpy(css->status, cstat.status);
}


/* ------------------------------------------------------------------------- */
void ChatRestoreStatus (struct _css *css)
{
    ChatSetStatus(css->avail, css->status);
}


/* ------------------------------------------------------------------------- */
void Check_For_Message (char *s1, char *s2)
{
    int redo, type, len;
    time_t now;
    byte tid;
    char *msg;
    static long last_check    = 0L;
    static long last_received = 0x7fffffffL;

    now = time(NULL);

    // If we received a msg within last 20 seconds, poll continuously
    if (hpMCP && fLoggedOn && task_num &&
	(now >= last_check + scan_time || now <= last_received + 20)) {
       last_check = time(NULL);

       redo = FALSE;

       if ((msg = malloc(PATHLEN)) != NULL) {
	  while (ChatGetMsg(&tid, &type, &len, msg, PATHLEN) == 0) {
		last_received = now;
		ChatHandleMessage(tid, type, len, msg, &redo);
	  }

	  free(msg);
       }

       if (redo) {
	  if (s1 && s2)
	     Printf(ss, s1, s2);

	  vbuf_flush();
       }
  }
}

