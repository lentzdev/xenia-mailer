#ifndef __NTCOMM_H_DEFINED
#define __NTCOMM_H_DEFINED

#include "pwin.h"
#include "comqueue.h"

#define COMMAPI  __stdcall  /* Standard NT API calling convention */
/*#define DEFAULT_COMM_MASK   (EV_ERR | EV_RLSD | EV_RXFLAG)*/
#define DEFAULT_COMM_MASK   (EV_ERR | EV_RLSD)

typedef struct
{
  HANDLE h;                 /* Handle of the physical com port file */

  COMQUEUE cqTx;            /* Transmit queue */
  COMQUEUE cqRx;            /* Receive queue */

  HANDLE hRx, hTx, hMn;     /* Handles for read and write threads */
  HANDLE hevTx, hevRx;      /* Semaphores for the tx/rx threads */
  HANDLE hevTxDone;         /* Pending transmit has completed */
  HANDLE hevRxWait, hevTxWait;  /* Waiting for input/output buf to clear */
  HANDLE hevRxPause, hevTxPause;  /* Stop transmitter for compause/resume */
  HANDLE hevRxDone;         /* Pending receive has completed */
  HANDLE hevMonDone;        /* Pending monitor has completed */

  BOOL fDCD;                /* Current status of DCD */
  volatile BOOL fDie;       /* True if we are trying to kill threads */
  DWORD dwCtrlC;            /* How many ^C's have we received from user? */
  volatile DWORD cThreads;  /* Number of active threads */

  COMMTIMEOUTS ct;          /* Timeout values */
} *HCOMM;

#define COMM_PURGE_RX 1
#define COMM_PURGE_TX 2
#define COMM_PURGE_ALL  (COMM_PURGE_RX | COMM_PURGE_TX)

BOOL COMMAPI ComOpenHandle(HANDLE hfComm, HCOMM *phc, DWORD dwRxBuf, DWORD dwTxBuf);
BOOL COMMAPI ComOpen(LPTSTR pszDevice, HCOMM *phc, DWORD dwRxBuf, DWORD dwTxBuf);
BOOL COMMAPI ComClose(HCOMM hc);
USHORT COMMAPI ComIsOnline(HCOMM hc);
BOOL COMMAPI ComWrite(HCOMM hc, PVOID pvBuf, DWORD dwCount);
BOOL COMMAPI ComRead(HCOMM hc, PVOID pvBuf, DWORD dwBytesToRead, PDWORD pdwBytesRead);
int COMMAPI ComGetc(HCOMM hc);
int COMMAPI ComPeek(HCOMM hc);
BOOL COMMAPI ComPutc(HCOMM hc, int c);
BOOL COMMAPI ComRxWait(HCOMM hc, DWORD dwTimeOut);
BOOL COMMAPI ComTxWait(HCOMM hc, DWORD dwTimeOut);
DWORD COMMAPI ComInCount(HCOMM hc);
DWORD COMMAPI ComOutCount(HCOMM hc);
DWORD COMMAPI ComOutSpace(HCOMM hc);
BOOL COMMAPI ComPurge(HCOMM hc, DWORD fBuffer);
HANDLE COMMAPI ComGetHandle(HCOMM hc);
BOOL COMMAPI ComGetDCB(HCOMM hc, LPDCB pdcb);
USHORT COMMAPI ComSetDCB(HCOMM hc, LPDCB pdcb);
BOOL COMMAPI ComSetBaudRate(HCOMM hc, DWORD dwBps, BYTE bParity, BYTE bDataBits, BYTE bStopBits);
BOOL COMMAPI ComPause(HCOMM hc);
BOOL COMMAPI ComResume(HCOMM hc);
BOOL COMMAPI ComWatchDog(HCOMM hc, BOOL fEnable, DWORD ulTimeOut);

#endif /* __NTCOMM_H_DEFINED */

