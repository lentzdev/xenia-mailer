/*--------------------------------------------------------------------------*/
/*  (C) Copyright 1987-93, Bit Bucket Software Co., a Delaware Corporation. */
/*                                                                          */
/*               This header file was written by Bill Andrus                */
/*          OS/2 Snoop server library definitions for BinkleyTerm           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#ifndef APIENTRY
#include <os2def.h>
#endif

#ifndef CALLBACK
#define CALLBACK _Far16 _Pascal
#endif

typedef SEL  HSNOOP;
typedef PSEL PHSNOOP;
typedef short   ( * CALLBACK PFNSN) (short flag, char * _Seg16 _Far16 str);


USHORT CALLBACK SnoopOpen (PSZ _Seg16 pszPipeName,
   PHSNOOP _Seg16 phSnoop,
   PSZ _Seg16 pszAppName,
   PFNSN NotifyFunc);

USHORT CALLBACK SnoopWrite (HSNOOP hsn, PSZ _Seg16 pszMessage);

USHORT CALLBACK SnoopClose (HSNOOP hsn);
