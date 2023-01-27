/*--------------------------------------------------------------------------*/
/*                  (C) Copyright 1994, Michael Buenter.                    */
/*                                                                          */
/*                    32b OS/2 Snoop server thunk layer                     */
/*--------------------------------------------------------------------------*/

#define INCL_NOPM
#include <os2.h>
#include <string.h>
#include "snserver.h"
#include "snsrv_32.h"


PFNSN2 CallBackFunc;


short CALLBACK CallBack (short flag, char * _Seg16 _Far16 str)
{
   return CallBackFunc (flag, str);
}


USHORT APIENTRY SnoopOpen2 (PSZ pszPipeName, PHSNOOP phSnoop,
                            PSZ pszAppName, PFNSN2 NotifyFunc)
{
   CHAR pipeStr[100], appStr[100];
   strcpy (pipeStr, pszPipeName);
   strcpy (appStr, pszAppName);
   CallBackFunc = NotifyFunc;

   return SnoopOpen (pipeStr, phSnoop, appStr, CallBack);
}


USHORT APIENTRY SnoopWrite2 (HSNOOP hsn, PSZ pszMessage)
{
   CHAR msgStr[255];
   strcpy (msgStr, pszMessage);

   return SnoopWrite (hsn, msgStr);
}

USHORT APIENTRY SnoopClose2 (HSNOOP hsn)
{
    return SnoopClose (hsn);
}
