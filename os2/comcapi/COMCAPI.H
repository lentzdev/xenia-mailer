/*
 *  Header file for COMCAPI.DLL,
 *                  A high performance, multithreaded COM and CAPI DLL
 *                  for OS/2 2.x communications
 *
 *  Copyright (C) 1994, Michael Buenter.  All rights reserved.
 *
 */

/* NB:  To create a "comcapi.lib" to link with,  do this:

      IMPLIB comcapi.lib comcapi.dll

  (assuming comcapi.dll is in the current directory

 */

/*
 * include os2.h or os2def.h before this file.
 *
 */

#ifndef APIENTRY
#include <os2def.h>
#include <bsedos.h>
#endif

#ifdef REALTHING
typedef CSTUFF * HCOMM;
#else
typedef VOID   * HCOMM;
extern HCOMM hcModem;
#endif
#define COMMAPI  APIENTRY

/*
 * open comm device.  return 0 if successful,  OS/2 error if not.
 *
 * This function (or ComHRegister()) must be called before any
 * other Com() function.
 *
 * Do not use this function if the port handle has been inherited from
 * another process (use ComHRegister() instead).
 *
 * The comm handle (placed in pHcomm) is not a file handle,  and should
 * not be used by Dos() functions.  If the file handle to the port is
 * needed (to pass to a child process),  use the ComGetFH() function.
 *
 * If RxBufSize and/or TxBufSize are/is zero,  a default size is used.
 *

  When using COM0x.SYS,  the default buffer size is the same as the
  buffer size used by COM0x.SYS.  For Tx, this is the optimum size
  to keep COM0x.SYS busy.  A buffer larger than this will not gain
  any extra performance.  Therefore, the common urge to make the
  transmit buffer large will not have the desired effect.  If there
  was some way to make the REAL transmit buffer (the one in
  COM0x.SYS) larger,  performance gains WOULD be seen,  but there is
  no way to do this unless you have the source code for COM0x.SYS.
  If you DO have the source (in the DDK), and you increase the
  buffer sizes, use 0 for the XxBufSize and ComOpen will use your
  larger buffers (ComOpen/Register uses the standard IOCTL call to
  query the buffer sizes).

  RxBufSize, on the otherhand, does not affect performance of any
  functions in the Com() module.  To avoid overflows however,  you
  should set this to at least the size of the COM0x.SYS Rx buffer,
  or use 0 so the default size if selected.  When the Rx buffer gets
  full,  the speaker will sound (DosBeep(200, 10)).  There are two
  solutions to this problem:  Make the Rx buffer larger (preferred),
  or raise the priority of the thread calling ComRead()/ComGetc().

  All threads created by comm.dll run at the default priority. Since
  com0?.sys is interrupt driven,  Reading/Writing to it at a high
  priorty does not improve anything.  If the Rx thread is starved
  for such a long time that com0?.sys buffer gets full,  it means
  some other process on your computer is badly behaved (IT is
  problably running at Time Critical class when it shouldn't be).
  There are four ways to solve this (in order of preference):

  1) Insert DosSleep(1L) in the polling loops that are guilty (this
  assumes you have source code), and/or have it run at NORMAL
  priority class.

  2) decrease MAXWAIT in config.sys.

  3) Prior to calling ComOpen/ComHRegister, raise the priorty of the
  current thread (or of the whole process).  The Rx/Tx threads will
  inherhit this priority.

  4) set PRIORITY=ABSOLUTE in config.sys.


  FYI:  OS/2 1.2 uses a default size of 1024 bytes for Rx, and 128
        bytes for Tx.

 *
 *
 */
USHORT COMMAPI ComOpen(PSZ PortName,
                       HCOMM *pHcomm,      /* pointer to var for Comm handle */
                       USHORT RxBufSize,   /* desired size of Receive queue*/
                       USHORT TxBufSize);  /* desired size of Transmit queue */

/*
 * ComHRegister():  Register a comm handle opened by another process.
 *
 * This function (or ComOpen()) must be called before any
 * other Com() function.
 */
USHORT COMMAPI ComHRegister(HFILE hf,
                       HCOMM *pHcomm,      /* pointer to var for Comm handle */
                       USHORT RxBufSize,   /* desired size of receive queue*/
                       USHORT TxBufSize);  /* desired size of xmit queue */


/* ComClose():  Close and/or DeRegister comm handle.
 *
 * This function will not DosClose() the handle if the handle was not
 * opened by this process with ComOpen() (ie: If ComHRegister() was used,
 * ComClose() will not DosClose() the handle.  ComClose() must still be
 * called though).
 */
USHORT COMMAPI ComClose(HCOMM hcomm);


/*
 * return TRUE if the device (comx, named pipe, etc) is considered
 * in an "Online" state.
 */
USHORT COMMAPI ComIsOnline(HCOMM hcomm);

/*
 * ComWrite(): Place many bytes in output queue.
 *
 * This function does not return until all of the bytes have been
 * placed in the output queue,  or an error occurs.
 *
 * If the device becomes offline while this function is waiting for
 * queue space, and the watchdog feature is enabled (see
 * ComWatchDog()),  this function returns before completion.
 *
 */
USHORT COMMAPI ComWrite(
     HCOMM hc,                /* file handle             */
     PVOID pvBuf,            /* pointer to buffer         */
     USHORT cbBuf);                     /* number of bytes to write      */


/*
 * ComRead(): Read many bytes from input queue.
 *
 * This function will copy bytes from the input queue to pvBuf,
 * until the input queue is empty, or 'cbBuf' bytes have been copied.
 *
 *
 */
USHORT COMMAPI ComRead(
     HCOMM hc,                /* file handle               */
     PVOID pvBuf,            /* pointer to buffer           */
     USHORT cbBuf,            /* maximum bytes to read       */
     PUSHORT pcbBytesRead);             /* pointer to variable receiving
                                         * byte count */

/*
 * ComGetc() : return next byte in input queue,  or -1 if there is none.
 */
SHORT COMMAPI ComGetc(HCOMM hc);

/*
 * ComPeek():  the same as ComGetc(),  but the character is left in the
 * input queue.
 */
SHORT COMMAPI ComPeek(HCOMM hc);

/*
 * ComPutc():  Place one byte in output queue.    This function does not
 * return until the byte is placed in the output queue, or an error occurs.
 *
 * If the device becomes offline while this function is waiting for
 * queue space, and the watchdog feature is enabled (see
 * ComWatchDog()),  this function returns before completion.
 */
USHORT COMMAPI ComPutc(HCOMM hc, SHORT c);

/*
 * ComRxWait():  Wait for something to be placed into the input queue.
 *
 * If lTimeOut is -1L,    wait forever.  Otherwise, lTimeOut is the
 * number of milliseconds to wait (or 0) before returning a timeout
 * error (ERROR_SEM_TIMEOUT).
 *
 * If the device becomes offline while this function is waiting, and
 * the watchdog feature is enabled (see ComWatchDog()),  this
 * function returns before completion.
 *
 */
USHORT COMMAPI ComRxWait(HCOMM hc, LONG lTimeOut);

/*
 * ComTxWait():  Wait for output queue to empty.
 *
 * If lTimeOut is -1L,    wait forever.  Otherwise, lTimeOut is the
 * number of milliseconds to wait (or 0) before returning a timeout
 * error (ERROR_SEM_TIMEOUT).
 *
 * If the device becomes offline while this function is waiting, and
 * the watchdog feature is enabled (see ComWatchDog()),  this
 * function returns before completion.
 */
USHORT COMMAPI ComTxWait(HCOMM hc, LONG lTimeOut);


/*
 * return number of bytes in input queue.
 *
 */
USHORT COMMAPI ComInCount(HCOMM hc);

/*
 * return number of bytes in output queue.
 *
 */
USHORT COMMAPI ComOutCount(HCOMM hc);

/*
 * return space in output queue.
 *
 */
USHORT COMMAPI ComOutSpace(HCOMM hc);


#define COMM_PURGE_RX 1
#define COMM_PURGE_TX 2
#define COMM_PURGE_ALL    (COMM_PURGE_RX | COMM_PURGE_TX)
/*
 * ComPurge():    Purge (discard) i/o queue(s).
 *
 *  Where fsWhich is a combination of :
 *         COMM_PURGE_RX    Purge input queue.
 *
 *         COMM_PURGE_TX    Purge output queue.
 */
USHORT COMMAPI ComPurge(HCOMM hc,  SHORT fsWhich);


/*
 * ComPause():
 *
 * This function causes the Rx and Tx threads to end, after all
 * Tx activity has ended.  Any bytes still in the Rx queue are
 * purged.  After calling this function,  it is safe for other
 * processes to use the comm port file handle (see ComGetFH()).
 *
 */
USHORT COMMAPI ComPause(HCOMM hc);

/*
 * ComResume():
 *
 * This function creates new Rx and Tx threads,  and resets the state
 * of the comm port to what is was when ComPause() was called.
 *
 * It is an error to call this function without calling ComPause().
 *
 */
USHORT COMMAPI ComResume(HCOMM hc);

/*
 *
 * ComGetFH():  get the file handle (HFILE) the comm module is
 * using.
 *
 * If the comm routines were initialized with ComHRegister(),  the
 * handle returned by this function will be the same handle passed
 * to ComHRegister().
 *
 * NOTE: Programs must not use this handle with any file system
 * calls (DosRead, DosWrite, DosClose, etc).  It may be used with
 * DosDevIOCTL() to query device dependant info.
 *
 * The intended use for this function is so that one process can
 * pass the file handle to another process.  Don't forget to
 * call ComPause() before spawning another process that uses the comm
 * port.
 */
HFILE COMMAPI ComGetFH(HCOMM hc);

/*
 * ComWatchDog():  Enable/disable the "online" watchdog.
 *
 * When the watchdog is enabled,  and the state of the comm handle
 * is no longer "online" (in the case of a modem,  the carrier has dropped),
 * any Com functions that are blocked (ComRxWait, ComTxWait) become
 * unblocked,  within the period specified by the "Seconds" paramater.
 *
 * It is up to the application to notice that the device is no longer
 * online,  after the blocked Com function returns.
 *
 * Obviously,  one must remember to disable the watchdog feature between
 * phone calls.
 *
 * When a device is first initialized by the comm moduel (via ComOpen() or
 * ComHRegister()) the watchdog feature is disabled.
 *
 * The "Seconds" paramater is ignored unless "OnOff" is TRUE.
 *
 * If "OnOff" is TRUE,  and "Seconds" is 0,  the current watchdog period
 * is used.  When first initialized,  the watchdog period is 3 seconds.
 */
USHORT COMMAPI ComWatchDog(HCOMM hc, BOOL OnOff, USHORT Seconds);

/*******************************************************************
 *                                   *
 * Thus far, all functions listed are applicable to many       *
 * device types,  including named pipes.  The 3 functions          *
 * that follow, however, are specific to COM0x.SYS (rs232c ports). *
 *                                   *
 *******************************************************************/

/*
 * Get/Set DBC.  See os/2 docs on "DCBINFO", used in a
 * DosIOCTL(ASYNC_GETDCBINFO) call (ioctl category 1, function 73h)
 *
 *
 * NOTE: ComSetDCB() ignores bits 0, 1 and 2 of the "fbTimeout"
 * field of the DCBINFO structure. (if you are using the IBM toolkit,
 * this is called "flags3" of the DCB).  These bits control the type
 * of read timeout processing, and write timeout processing that the
 * com01.sys device driver uses.  The Com() api insists that
 * "write infinite timeout processing" NOT be enabled,  and
 * "Wait-for-something read timeout processing" be in effect, so this
 * function overrides the above mentioned bits to ensure this.
 *
 */
#ifdef IOCTL_ASYNC
USHORT COMMAPI ComGetDCB(HCOMM hc, PDCBINFO pdbc);
USHORT COMMAPI ComSetDCB(HCOMM hc, PDCBINFO pdcb);
#endif

USHORT COMMAPI ComSetBaudRate(HCOMM hcomm,
     LONG bps,
     CHAR parity,
     USHORT databits,
     USHORT stopbits);


/* functions added by Michael Buenter 94-01-10 */

/* force transmitter to go */
USHORT COMMAPI ComKick (HCOMM hc);

/* set DTR on/off */
USHORT COMMAPI ComDTR (HCOMM hc, BOOL OnOff);

/* set XON on/off */
USHORT COMMAPI ComXON (HCOMM hc, BOOL OnOff);

/* set Break on/off */
USHORT COMMAPI ComBreak (HCOMM hc, BOOL OnOff);

/* get Version Info */
USHORT COMMAPI ComGetVersion (PSZ versionStr);
