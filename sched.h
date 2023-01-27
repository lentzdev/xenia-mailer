/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Based off The-Box Mailer (C) Jac Kersing
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


/* Scheduler definitions and structures */

/* Definitions for day of the week */
#define DAY_SUNDAY	0x01
#define DAY_MONDAY	0x02
#define DAY_TUESDAY	0x04
#define DAY_WEDNESDAY	0x08
#define DAY_THURSDAY	0x10
#define DAY_FRIDAY	0x20
#define DAY_SATURDAY	0x40
#define DAY_UNUSED	0x80

#define DAY_WEEK  (DAY_MONDAY|DAY_TUESDAY|DAY_WEDNESDAY|DAY_THURSDAY|DAY_FRIDAY)
#define DAY_WKEND (DAY_SUNDAY|DAY_SATURDAY)

/* Definitions for matrix behavior */
#define MAT_CM		0x0001			/* Crash		     */
#define MAT_DYNAM	0x0002			/* Dynamic		     */
//#define MAT_BBS	  0x0004		  /* BBS		       */
#define MAT_NOINREQ	0x0008			/* No inbound Requests	     */
#define MAT_OUTONLY	0x0010			/* Send only		     */
#define MAT_NOOUT	0x0020			/* Receive only 	     */
#define MAT_FORCED	0x0040			/* Forced Event 	     */
#define MAT_LOCAL	0x0080			/* Local mail only	     */
#define MAT_SKIP	0x0100			/* Event already executed    */
#define MAT_CBEHAV	0x0200			/* Change behaviour only     */
#define MAT_EXITDYNAM	0x0400			/* Exit (0) on Dynamic end   */
#define MAT_ZERONOC	0x0800			/* Reset noconnect counters  */
#define MAT_MAILHOUR	0x1000			/* Mailhour (send to non-CM) */
#define MAT_HIPRICM	0x2000			/* Hi priority for crashmail */
#define MAT_ZEROCON	0x4000			/* Reset connect counters    */
#define MAT_NOOUTREQ	0x8000			/* No outbound requests      */

typedef struct _event {
   short tag;		/* Tag of event, Z for behavior */
   short days;		/* Bit field for which days to execute */
   short minute;	/* Minutes after midnight to start event */
   short length;	/* Number of minutes event runs */
   short errlevel[5];	/* Errorlevel exits */
   short last_ran;	/* Day of month last ran */
   short behavior;	/* Behavior mask */
   short wait_time;	/* Average number of seconds to wait between dials */
   short max_conn;	/* Max. number of connected calls    */
   short max_noco;	/* Max. number of not conected calls */
   short queue_size;	/* Required minimum amount of waiting data to call */
   short extra[1];	/* Extra space for later */
} EVENT;


/* end of sched.h */
