/*--------------------------------------------------------------------------*/
/*                  (C) Copyright 1994, Michael Buenter.                    */
/*                                                                          */
/*                    32b OS/2 Snoop server thunk layer test                */
/*--------------------------------------------------------------------------*/

#define INCL_NOPM
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include "snsrv_32.h"


static short APIENTRY mesgfunc (short error, char * mesg)
{
   if (!error)
   {
      printf ("+%s", mesg);
   }
   else
      printf ("!SYS%04hd : %s", error, mesg);
   return (0);
}

int main (int argc, char *argv[])
{
   static HSNOOP hsnoop;

   SnoopOpen2 ("\\pipe\\line1", &hsnoop, "Test Program", mesgfunc);
   if (hsnoop != (HSNOOP) 0L)
   {
      (void) SnoopWrite2 (hsnoop, "This is a simple test!");
      (void) SnoopWrite2 (hsnoop, "!This is a simple test!");
      SnoopClose2 (hsnoop);
   }

}
