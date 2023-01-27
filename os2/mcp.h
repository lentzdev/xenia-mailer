#if 0
#ifdef __WATCOMC__
  #ifdef __FLAT__
    #pragma library("..\lib\mcp32.lib");
  #else
    #pragma library("..\lib\mcp.lib");
  #endif
#endif
#endif

#define MCP                           /* Make sure that MCP is defined */
#define PIPE_MSG_SIZE         16384   /* Up to 16k IPC messages */
#define MCP_OUT_BUF_MAX       512     /* Send 512 bytes at a time */

#ifndef CALLBACK
#ifdef __WATCOM__
  #define CALLBACK far __syscall
#else
  #define CALLBACK APIENTRY
#endif
#endif

/* Table of handler functions */

typedef struct
{
  USHORT dispatch_code;
  int (CALLBACK *f)(PVOID);
} DISPATCH;


typedef struct
{
  BYTE bHeight;
  BYTE bWidth;
  BYTE bCurRow;
  BYTE bCurCol;
  BYTE bCurAttr;
} VIO_DUMP_HDR;



/* Messages sent to MCP by Maximus: */

#define PMSG_HELLO              0x00    /* Task on-line */
#define PMSG_EOT                0x01    /* End of session */
#define PMSG_SET_STATUS         0x02    /* Set IPC status of this node */
#define PMSG_MAX_SEND_MSG       0x03    /* Send inter-node message */
#define PMSG_QUERY_ACTIVE       0x04    /* Query status of task number x */
#define PMSG_QUERY_TASKINFO     0x04    /* Query status of task number x */
#define PMSG_QUERY_ACTIVE_NUM   0x05    /* Query total no. of active tasks */
#define PMSG_QUERY_TASKLIST     0x05    /* Query total no. of active tasks */
#define PMSG_PING               0x06    /* We are alive */
#define PMSG_VIO                0x07    /* Video I/O */
#define PMSG_VIO_DUMP           0x08    /* Video screen buffer dump */
#define PMSG_LOG                0x09    /* Logging information */

/* Messages sent to Maximus by MCP: */

#define RPMSG_GOT_MSG           0x00    /* Msg for you from other Max node */
#define RPMSG_MONITOR           0x01    /* Begin/end monitoring */
#define RPMSG_HAPPY_DAGGER      0x02    /* Terminate session */
#define RPMSG_CTRLC             0x03    /* Got ^c from client */
#define RPMSG_BREAK             0x04    /* Got ^brk from client */
#define RPMSG_KEY               0x05    /* Keyboard I/O */

/* Messages sent by client to MCP */

#define CLMSG_BEGINMONITOR      0x00    /* Start monitoring a Max task */
#define CLMSG_ENDMONITOR        0x01    /* End monitoring a Max task */
#define CLMSG_BEGINOVR          0x02    /* Start sending overview info */
#define CLMSG_ENDOVR            0x03    /* Stop sending overview info */
#define CLMSG_BYEBYE            0x04    /* Disconnect from client */
#define CLMSG_KEY               0x05    /* Keyboard input */
#define CLMSG_START             0x06    /* Tell Max task to begin */
#define CLMSG_HAPPY_DAGGER      0x07    /* Tell Max task to end */
#define CLMSG_MAX_SEND_MSG      0x08    /* Send a msg to a Max task */

/* Messages send by MCP to client */

#define RCMSG_ACK               0x00    /* Okay */
#define RCMSG_BADNODE           0x01    /* Unknown node */
#define RCMSG_NORESOURCES       0x02    /* Not enough resources for req */
#define RCMSG_IO_MONITOR        0x03    /* I/O from monitored session */
#define RCMSG_CIAO_BABY         0x04    /* MCP terminating connection */
#define RCMSG_IO_MONITOR_DUMP   0x05    /* Incoming screen dump */
#define RCMSG_OVERVIEW          0x06    /* Overview information */
#define RCMSG_PING              0x07    /* Got ping from node */
#define RCMSG_LOG               0x08    /* Got logging info from node */

int APIENTRY McpGetMsg(HPIPE hp, PVOID pv, USHORT *pusSize, USHORT usMaxSize);
int APIENTRY McpPeekMsg(HPIPE hp, PVOID pv, USHORT *pusSize, USHORT usMaxSize);
int APIENTRY McpDispatchMsg(PVOID pv, USHORT usSize, DISPATCH *dt, PVOID pvParm);
int APIENTRY McpSendMsg(HPIPE hp, USHORT usType, BYTE *pbMsg, USHORT cbMsg);
int APIENTRY McpOpenPipe(char *pszPipeName, HPIPE *php);
void APIENTRY McpClosePipe(HPIPE hp);

