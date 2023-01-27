/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 *
 * This file is part of Xenia Mailer.
 * Xenia Mailer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


/*
   Arjen's Zmodem compression code should be here. But as you can see, it's not.
   This is because I was dumb and left it out when publishing the source code.
   When I find the CD again on which it is backed up, I'll see if it's readable.
   Yep, we now know that CDs don't last forever. Who would've thunk :)

   What did the code do?
    - Modified Lempel-Ziv-Welch (LZW), compression of repeated strings
    - Run Length Encoding (RLE), compression of repeated characters
    - or nada, if the compression didn't do any good (ARC/ZIP/ARJ files)

   Basically, it was yet another way to help people save money. When
   communicating via modems over the public telephone network (PSTN/ISDN) in
   that era, most calls tended to cost AND be timed. People communicated, and
   that adds up like you wouldn't believe. Internationally, but also locally.
   So anything that could make things faster (higher modem speeds or compression),
   and more reliable (fewer dropped connections, and definitely no hangs), were
   key. Calls were automatic/unattended, and made at night (lower tariffs).

   This was also the reason I re-implemented Zmodem from scratch. All other
   implementations were based off the late Chuck Forsberg's original code, and
   there were problems somewhere in the protocol. I could tell because we could
   spot race conditions, and various patches around that didn't quite cut it.
   So I actually wrote up state tables for Zmodem - yea, they'd never been done!
   (and sorry: it was on paper, 3 decades ago on another continent... I think
    they may also be lost.)
   As it turned out, there were race conditions in the protocol, it could get
   stuck in infinite loops and undefined conditions. So I amended the protocol
   with consistent retry counters, a braindead timeout (just in case), and a new
   implementation that followed the state table structure for easier reading.
   The results were marvellous, well worth the effort.
   Have a look at zsend.c and zrecv.c, and compare with Chuck's rzsz code.
   Unfortunately, Chuck and I never communicated (my bad: arrogant teenager)
   and for that I am truly sorry.

   The HYDRA bi-directional file transfer protocol is strongly influenced by
   Zmodem (with the fixes), and also proved to be very fast and reliable.
   HYDRA was valuable again in saving money (lots of it), because modems could
   simultaneously transmit data in both directions, yet Zmodem would only
   transfer uni-directionally.

   Around this time, the web also came along, more people got onto the Internet,
   the rest is mostly history. TCP/IP over modems was not super efficient, and
   neither were the transfer protocols, but if you had a multi-tasking operating
   system you could upload and download stuff at the same time anyway!
   OS/2 (Warp) was my fave at the time, followed by Linux (Slackware).


           -- Arjen Lentz, 27 Feb 2023
*/


/* end of zcomp.c */