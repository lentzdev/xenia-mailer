Xenia Mailer - FidoNet Technology Mailer
Copyright (C) 1987-2001 Arjen G. Lentz
Portions based off The-Box Mailer (C) Jac Kersing

> Xenia Mailer is free software; you can redistribute it and/or
> modify it under the terms of the GNU General Public License
> as published by the Free Software Foundation; either version 2
> of the License, or (at your option) any later version.
> 
> This program is distributed in the hope that it will be useful,
> but WITHOUT ANY WARRANTY; without even the implied warranty of
> MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
> GNU General Public License for more details.
> 
> You should have received a copy of the GNU General Public License
> along with this program; if not, write to the Free Software
> Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



Hi everybody, these are the main Xenia mailer sources. It's a multi-platform
FidoNet-technology mailer and will currently compile under DOS and OS/2.
Jac's original code also compiled for Atari ST, and it still might.

> Jac Kersing wrote The-Box Mailer for Atari ST (and later DOS) in the 80s,
> I think even before BinkleyTerm (with sourcecode) came out. So that was a
> giant effort, no other code to refer to, only (sometimes sketchy) specs.
> While many aspects of Xenia Mailer are new, refactored or replaced, Jac
> deserves all the credit for the original architecture and making it buzz!
> We remain friends to this day.

The DOS stuff is clean for BC 3.1, you may be able to use BC 5 which is now
free for download from borland.com, but this has not been tested.
The DOS make utility used is NDMAKE 4.3 or 4.5, which is very unix-alike; don't
be using the crappy Borland make tool unless you want to rewrite the makefile.

Under OS/2 either BC/2 1.0 or Watcom C can be used, that's all stable.
Watcom offers some additional trickery, for instance the FTPMAIL facility.



Unzip this archive into a xenia\ dir under your (Xenia) development root.
To compile Xenia, you'll need to have the AGLWIN, AGLCOM and AGL_CRC packages
installed (also on https://github.com/lentzdev/)
To run Xenia, you'll need Xenia configuration files and Xenia tools.

If you're using Borland C, check TURBOC.CFG and TLINK.CFG in bc\bin\
TURBO.CFG should have lines like

	-Iu:\bc\include
	-Iu:\bc\include\sys
	-Iu:\bc\aglwin
	-Lu:\bc\lib
	-Lu:\bc\aglwin

TLINK.CFG should contain something like

	/Lu:\bc\lib;u:\bc\aglwin;


In the xenia\ directory, you will find the following stuff:

- README.md     This file
- CHANGES       The Xenia CHANGES file
- LICENSE.md    The GNU General Public License

- MAKEFILE      Generate XENIA.EXE (DOS) using Borland C, TASM and TLINK.

The make utility used is NDMAKE 4.3 or 4.5, which is very unix-alike; don't be
using the crappy Borland make tool unless you want to rewrite the makefile.

- os2/          OS/2 specific stuff. Stable.
- os2/comcapi   COMCAPI.DLL from Michael Buenter
- os2/snsrv32   Snoop server from Michael Buenter
- os2/maxdev30  MAXDEV30.ZIP (Maximus BBS 3.0 dev kit) from Lanius Corp.

All used with permission, of course.

- win32/        Start of win32 port, works without com routines so the basic
                code seems ok. But of course you need to make winnt.c tick ;)

All the DOS dependent stuff:

- exec.h        PD code by Thomas Wagner for a memory swapping spawn under DOS
- exec.c
- compat.h
- spawn.asm
- checkpat.asm  Fast wildcard pattern handling
- checkpat.h

- X00CL.OBJ     Code by Ray Gwinn to make a quicker call into X00 fossil.
- syspc.c       DOS dependent stuff for Xenia. There is some #if/#ifdef stuff
                going on in other sourcefiles too, check xenia.h for details.
                Anyway, the main thing resides here. Use as basis for porting.


The rest of Xenia Mailer:

- xenia.c      Main file - just gets things started (and terminal mode)
- xenia.h      Main include file
- proto.h      Contains all function prototype for clean ANSI compiles

- tconf.c      Configuration

- get.c        Generating manual file attachments/requests (non-windowed)
- mailer.c     Mailer - here's where the main loop is
- misc.c       Misc functions - logging, internal print, etc
- dos_file.c   Portable file I/O
- tmenu.c      Menu functions
- window.c     Window functions
- modem.c      Modem routines
- script.c     Scripting handler

- sched.h      The old but faithful event scheduler
- sched.c

- period.h     The infamous period scheduler ;)
- period.c

- nodeidx.h    Include for nodelist indexing (NODEIDX tool to create indexes)
- getnode.c    Nodelist/index read access with caching
- outblist.c   Managing the outbound directories

- mail.h       Contains defs for mail exchange (FidoNet packet formats etc)
- bbs.c        Code when starting a BBS application (spawn/exec/swap etc)
- msession.c   Mail session routines, main & incoming (mostly)
- tmailer.c    Mailer routines, outbound
- wzsend.c     WaZoo session routines (send)
- fmisc.c      Sorts out temp names for receive, recovery (Zmodem/Hydra)
- sealink.c    SEAlink protocol
- xfer.c       Modem7 filename transmit/receive routines
- cdog.c       SEAdog style file requests

- emsi.h       EMSI handshake
- emsi.c

- hydra.h      Xenia's Hydra file transfer protocol
- hydra.c

- zmodem.h     Xenia's Zmodem - this is one I (AGL) built, not a port.

This is relevant because of the improved stability/portability.

- zrecv.c      Receive code
- zsend.c      Transmit code
- zmisc.c      Stuff used by both recv and send
- zvars.c      Variables
- zcomp.c      Read this file for detail on what's not there (sorry!)

- faxproto.h   FAX receive
- faxrecv.c

- request.h    The file/service request handler (REQCOMP tool to create index)
- request.c

- md5.h        MD5 code (used for EMSI shared secret password exchange)
- md5.c



The main thing you need to do for a port is work on a syslinux.c (based
on syspc.c), creating non-blocking comport I/O (just to get a start).
See the AGLWIN package and the README.md there for ideas on how to deal with
the video/keyboard stuff.


    -- 20 May 2001, 27 January 2023
    Arjen G. Lentz

