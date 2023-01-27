/*--------------------------------------------------------------------------*/
/*                  (C) Copyright 1994, Michael Buenter.                    */
/*                                                                          */
/*                32b OS/2 Snoop server library definitions                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#ifndef APIENTRY
#include <os2def.h>
#endif

#ifndef REALTHING
typedef SEL  HSNOOP;
typedef PSEL PHSNOOP;
#endif
typedef short   ( * APIENTRY PFNSN2) (short flag, char * str);


USHORT APIENTRY SnoopOpen2 (PSZ pszPipeName, PHSNOOP phSnoop,
                            PSZ pszAppName, PFNSN2 NotifyFunc);

USHORT APIENTRY SnoopWrite2 (HSNOOP hsn, PSZ pszMessage);

USHORT APIENTRY SnoopClose2 (HSNOOP hsn);
