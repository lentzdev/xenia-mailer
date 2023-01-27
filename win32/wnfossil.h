//---------------------------------------------------------------------------
//
//  Module:   fossil.h
//
//  Description:
//     Public header file
//
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1994-1996 Bryan A. Woodruff.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#if !defined( _WNFOSCTL_ )

#include <pshpack1.h>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

// I/O Control Support... definitions borrowed from NTDDK.H

#if !defined( CTL_CODE )
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define FILE_DEVICE_UNKNOWN 0x00000022

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#endif

#define IOCTL_FOSSIL_BASE            FILE_DEVICE_UNKNOWN
#define IOCTL_FOSSIL_RESERVED        CTL_CODE(IOCTL_FOSSIL_BASE, 0x000, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_FOSSIL_GET_VERSION     CTL_CODE(IOCTL_FOSSIL_BASE, 0x001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_FOSSIL_ACTIVATE_PORT   CTL_CODE(IOCTL_FOSSIL_BASE, 0x002, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_FOSSIL_DEACTIVATE_PORT CTL_CODE(IOCTL_FOSSIL_BASE, 0x003, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_FOSSIL_CONTROL         CTL_CODE(IOCTL_FOSSIL_BASE, 0x004, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)
#define IOCTL_FOSSIL_GETPORTINFO     CTL_CODE(IOCTL_FOSSIL_BASE, 0x005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_FOSSIL_GETPORTSTATUS   CTL_CODE(IOCTL_FOSSIL_BASE, 0x006, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_FOSSIL_SETPARAMS       CTL_CODE(IOCTL_FOSSIL_BASE, 0x007, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_FOSSIL_BLOCKXMIT       CTL_CODE(IOCTL_FOSSIL_BASE, 0x008, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_FOSSIL_BLOCKRECV       CTL_CODE(IOCTL_FOSSIL_BASE, 0x009, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct
{
   WORD   wSignature ;
   BYTE   bVerMajor ;
   BYTE   bVerMinor ;
   ULONG  cbReceiveBuffer ;
   ULONG  cbReceiveFree ;
   ULONG  cbTransmitBuffer ;
   ULONG  cbTransmitFree ;

} FOSSIL_INFORMATION, *PFOSSIL_INFORMATION ;

typedef struct
{
   ULONG  ulPortId ;
   ULONG  ulControl ;
   ULONG  ulParam ;

} FOSSIL_CONTROL, *PFOSSIL_CONTROL ;

#define FOSSIL_CONTROL_DTR               0x0001

#define FOSSIL_CONTROL_FLOWCTL           0x0002
#define FOSSIL_FLOWCTLF_INX              0x01
#define FOSSIL_FLOWCTLF_RTSCTS           0x02
#define FOSSIL_FLOWCTLF_OUTX             0x08

#define FOSSIL_CONTROL_BREAK             0x0003

#define FOSSIL_CONTROL_PURGE             0x0004
#define FOSSIL_PURGE_TRANSMIT            0x00
#define FOSSIL_PURGE_RECEIVE             0x01

#define FOSSIL_STATUSF_INQ_DATA          0x0100
#define FOSSIL_STATUSF_OUTQ_ROOM         0x2000
#define FOSSIL_STATUSF_OUTQ_EMPTY        0x4000
#define FOSSIL_STATUSF_TIMEOUT           0x8000
#define FOSSIL_STATUSF_MS_CTS_ON         0x0010
#define FOSSIL_STATUSF_MS_DSR_ON         0x0020
#define FOSSIL_STATUSF_MS_RING_ON        0x0040
#define FOSSIL_STATUSF_MS_RLSD_ON        0x0080

typedef struct
{
   ULONG  ulPortId ;
   ULONG  BaudRate ;
   ULONG  ByteSize ;
   ULONG  StopBits ;
   ULONG  Parity ;

} FOSSIL_SETPARAMS, *PFOSSIL_SETPARAMS ;

typedef struct
{
   ULONG  ulPortId ;
   PCHAR  pchBuffer ;
   ULONG  cbBuffer ;

} FOSSIL_BUFFER, *PFOSSIL_BUFFER ;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#include <poppack.h>

#endif

// API support

#define FOSSIL_API_GET_VERSION    0x0000
#define FOSSIL_API_UNLOCK_PORT    0x0001
#define FOSSIL_API_LOCK_PORT      0x0002

//---------------------------------------------------------------------------
//  End of File: fossil.h
//---------------------------------------------------------------------------

