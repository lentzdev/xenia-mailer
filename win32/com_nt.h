/*--------------------------------------------------------------------------*/

#if !defined( BAUD_300 )
/* Baud rate masks */

#define BAUD_300		300
#define BAUD_1200		1200
#define BAUD_2400		2400
#define BAUD_4800		4800
#define BAUD_9600		9600
#define BAUD_19200		19200
#define BAUD_38400		38400
#define BAUD_57600		57600
#define BAUD_115200		115200

#endif

#if !defined( _WINDOWS_ )

typedef unsigned long HANDLE ;

typedef struct _OVERLAPPED {
    DWORD   Internal;
    DWORD   InternalHigh;
    DWORD   Offset;
    DWORD   OffsetHigh;
    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;

#endif

typedef struct tagFOSSIL_PORTINFO
{
   ULONG       ulPortId ;
   HANDLE      hDevice ;
   OVERLAPPED  ov ;
   
} FOSSIL_PORTINFO, *PFOSSIL_PORTINFO ;

typedef unsigned short (*PFNU_II)( int, int ) ;
typedef unsigned short (*PFNU_H)( HANDLE ) ;
typedef short (*PFNS_HU)( HANDLE, ULONG ) ;
typedef short (*PFNS_HB)( HANDLE, byte ) ;
typedef void (*PFNV_H)( HANDLE ) ;
typedef int (*PFNI_H)( HANDLE ) ;
typedef int (*PFNI_HU)( HANDLE, ULONG ) ;
typedef int (*PFNI_I)( int ) ;
typedef void (*PFNV_HI)( HANDLE, int ) ;
typedef void (*PFNV_HU)( HANDLE, ULONG ) ;
typedef void (*PFNV_HPVUS)(HANDLE, void *, USHORT, short) ;
typedef short (*PFNS_H)( HANDLE ) ;

extern PFNU_II	  Cominit ;
extern PFNS_HU	  ComSetParms ;
extern PFNV_H	  ComDeInit ;
extern PFNS_H	  ComCarrier ;
extern PFNS_H	  ComInCount ;
extern PFNS_H	  ComOutCount ;
extern PFNS_H	  ComOutSpace ;
extern PFNV_H	  ComDTROff ;
extern PFNV_H	  ComDTROn ;
extern PFNV_H	  ComTXPurge ;
extern PFNV_H	  ComRXPurge ;
extern PFNV_H	  ComXONEnable ;
extern PFNV_H	  ComXONDisable ;
extern PFNS_HB	  ComPutc ;
extern PFNS_HB	  ComBufferByte ;
extern PFNV_HU	  ComTxWait ;
extern PFNI_HU	  ComRxWait ;
extern PFNU_H	  ComGetc ;
extern PFNS_H	  ComPeek ;
extern PFNV_HI	  ComBreak ;
extern PFNV_HPVUS ComWrite ;
extern PFNI_H	  ComGetFH ;
extern PFNI_H	  ComPause ;
extern PFNI_H	  ComResume ;
//extern PFNI_I     com_getc ;

extern void ComSelect( int ) ;
extern short KBHit (void);
extern short GetKBKey (void);

unsigned short stdCominit (int, int);
extern short stdComSetParms (HANDLE, ULONG);
extern void stdComDeInit (HANDLE);
extern short stdComCarrier (HANDLE);
extern short stdComInCount (HANDLE);
extern short stdComOutCount (HANDLE);
extern short stdComOutSpace (HANDLE);
extern void stdComDTROff (HANDLE);
extern void stdComDTROn (HANDLE);
extern void stdComTXPurge (HANDLE);
extern void stdComRXPurge (HANDLE);
extern void stdComXONEnable (HANDLE);
extern void stdComXONDisable (HANDLE);
extern short stdComPutc (HANDLE, byte);
extern short stdComBufferByte (HANDLE, byte);
extern void stdComTxWait (HANDLE, ULONG);
extern int stdComRxWait (HANDLE, ULONG);
extern unsigned short stdComGetc (HANDLE);
extern short stdComPeek (HANDLE);
extern void stdComBreak (HANDLE, int);
extern void stdComWrite (HANDLE, void *, USHORT, short);
extern int stdComGetFH (HANDLE);
extern int stdComPause (HANDLE);
extern int stdComResume (HANDLE);
extern int stdcom_getc( int ) ;

unsigned short wfCominit (int, int);
extern short wfComSetParms (HANDLE, ULONG);
extern void wfComDeInit (HANDLE);
extern short wfComCarrier (HANDLE);
extern short wfComInCount (HANDLE);
extern short wfComOutCount (HANDLE);
extern short wfComOutSpace (HANDLE);
extern void wfComDTROff (HANDLE);
extern void wfComDTROn (HANDLE);
extern void wfComTXPurge (HANDLE);
extern void wfComRXPurge (HANDLE);
extern void wfComXONEnable (HANDLE);
extern void wfComXONDisable (HANDLE);
extern short wfComPutc (HANDLE, byte);
extern short wfComBufferByte (HANDLE, byte);
extern void wfComTxWait (HANDLE, ULONG);
extern int wfComRxWait (HANDLE, ULONG);
extern unsigned short wfComGetc (HANDLE);
extern short wfComPeek (HANDLE);
extern void wfComBreak (HANDLE, int);
extern void wfComWrite (HANDLE, void *, USHORT, short);
extern int wfComGetFH (HANDLE);
extern int wfComPause (HANDLE);
extern int wfComResume (HANDLE);
extern int wfcom_getc( int ) ;

/* translate binkley fossil comm stuff to NT_ASYNC.C calls */

/*-----------------------------------------------*/
/* Service 0: SET BAUD(etc)			 */
/*-----------------------------------------------*/

#define MDM_ENABLE(b)		(ComSetParms (hcModem, b))

/*-----------------------------------------------*/
/* Service 1: SEND CHAR (wait)			 */
/*-----------------------------------------------*/

#define SENDBYTE(c)			(ComPutc (hcModem, c))

/*-----------------------------------------------*/
/* Service 2: GET CHAR (wait)			 */
/*-----------------------------------------------*/

#define MODEM_IN()			(ComGetc (hcModem))

/*-----------------------------------------------*/
/* Service 3: GET STATUS			 */
/*-----------------------------------------------*/

#define CARRIER 			(ComCarrier (hcModem))
#define CHAR_AVAIL()		(ComInCount (hcModem))
#define OUT_EMPTY()			(ComOutCount (hcModem) == 0)
#define OUT_FULL()			(ComOutSpace (hcModem) == 0)

/*-----------------------------------------------*/
/* Service 4: INIT/INSTALL			 */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 5: UNINSTALL 			 */
/*-----------------------------------------------*/

#define MDM_DISABLE()		(ComDeInit (hcModem))

/*-----------------------------------------------*/
/* Service 6: SET DTR				 */
/*-----------------------------------------------*/

#define LOWER_DTR()			(ComDTROff (hcModem))
#define RAISE_DTR()			(ComDTROn (hcModem))

/*-----------------------------------------------*/
/* Service 7: GET TIMER TICK PARMS		 */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 8: FLUSH OUTBOUND RING-BUFFER	 */
/*-----------------------------------------------*/

#define UNBUFFER_BYTES()	(ComTxWait (hcModem, 1L))

/*-----------------------------------------------*/
/* Service 9: NUKE OUTBOUND RING-BUFFER 	 */
/*-----------------------------------------------*/

#define CLEAR_OUTBOUND()	(ComTXPurge (hcModem))

/*-----------------------------------------------*/
/* Service a: NUKE INBOUND RING-BUFFER		 */
/*-----------------------------------------------*/

#define CLEAR_INBOUND() 	(ComRXPurge (hcModem))

/*-----------------------------------------------*/
/* Service b: SEND CHAR (no wait)		 */
/*-----------------------------------------------*/

#define BUFFER_BYTE(c)		(ComBufferByte (hcModem, c))

/*-----------------------------------------------*/
/* Service c: GET CHAR (nondestructive, no wait) */
/*-----------------------------------------------*/

#define PEEKBYTE()			(ComPeek (hcModem))

/*-----------------------------------------------*/
/* Service d: GET KEYBOARD STATUS		 */
/*-----------------------------------------------*/

#define KEYPRESS()			(KBHit ())

/*-----------------------------------------------*/
/* Service e: GET KEYBOARD CHARACTER (wait)	 */
/*-----------------------------------------------*/

#define READKB()			(GetKBKey ())
#define FOSSIL_CHAR()		(GetKBKey ())

/*-----------------------------------------------*/
/* Service f: SET/GET FLOW CONTROL STATUS	 */
/*-----------------------------------------------*/

#define XON_ENABLE()		(ComXONEnable (hcModem))
#define IN_XON_ENABLE()
#define XON_DISABLE()		(ComXONDisable (hcModem))
#define IN_XON_DISABLE()

/*-----------------------------------------------*/
/* Service 10: SET/GET CTL-BREAK CONTROLS	 */
/*	       Note that the "break" here refers */
/*	       to ^C and ^K rather than the	 */
/*	       tradition modem BREAK.		 */
/*-----------------------------------------------*/

#define _BRK_ENABLE()
#define _BRK_DISABLE()

/*-----------------------------------------------*/
/* Service 11: SET LOCAL VIDEO CURSOR POSITION	 */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 12: GET LOCAL VIDEO CURSOR POSITION	 */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 13: WRITE LOCAL ANSI CHARACTER	 */
/*-----------------------------------------------*/

#define WRITE_ANSI(c)		(putchar (c))

/*-----------------------------------------------*/
/* Service 14: WATCHDOG on/off			 */
/*-----------------------------------------------*/

#define FOSSIL_WATCHDOG(x)

/*-----------------------------------------------*/
/* Service 18: Write buffer, no wait		 */
/*-----------------------------------------------*/

#define SENDCHARS(buf, size, carcheck)	(ComWrite (hcModem, buf, (USHORT)size, (short)carcheck))

/*-----------------------------------------------*/
/* Service 1a: Break on/off			 */
/*-----------------------------------------------*/

#define do_break(on)		(ComBreak (hcModem, on))

#define hfComHandle			(ComGetFH (hcModem))

/* END OF FILE: com_nt.h */

