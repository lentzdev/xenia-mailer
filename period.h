/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * period.h  World time period scheduler - general global definitions
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


#ifndef __PERIOD_DEF_
#define __PERIOD_DEF_
#include "2types.h"


#define MAX_PERIODS 128

#define PER_NUM_YEAR	35	/* 1995-2029 */
#define PER_NUM_MONTH	12	/* 1-12      */
#define PER_NUM_MDAY	31	/* 1-31      */
#define PER_NUM_WDAY	 7	/* 1-7	     */
#define PER_NUM_HOUR	25	/* 0-24 (!!) */
#define PER_NUM_MIN	60	/* 0-59      */
#define PER_YEAR	(0)
#define PER_MONTH	(PER_YEAR  + PER_NUM_YEAR )
#define PER_MDAY	(PER_MONTH + PER_NUM_MONTH)
#define PER_WDAY	(PER_MDAY  + PER_NUM_MDAY )
#define PER_HOUR	(PER_WDAY  + PER_NUM_WDAY )
#define PER_MIN 	(PER_HOUR  + PER_NUM_HOUR )
#define PER_BIT_SIZE	(PER_MIN   + PER_NUM_MIN  )
#define PER_BYTE_SIZE	((PER_BIT_SIZE / 8) + ((PER_BIT_SIZE % 8) ? 1 : 0))
#define PER_MASK_DWORDS ((MAX_PERIODS / 32) + ((MAX_PERIODS % 32) ? 1 : 0))

#define PER_GETBIT(base,type,item) (base[(type + (item)) / 8] &  (1 << ((type + (item)) % 8)))
#define PER_SETBIT(base,type,item) (base[(type + (item)) / 8] |= (1 << ((type + (item)) % 8)))
#define PER_SETALL(base,type,num) { for (i = 0; i < num; i++) PER_SETBIT(base,type,i); }

#define PER_MASK_GETBIT(basep,item) ((basep)->mask[item / 32] &   (1 << (item % 32)))
#define PER_MASK_SETBIT(basep,item) ((basep)->mask[item / 32] |=  (1 << (item % 32)))
#define PER_MASK_CLRBIT(basep,item) ((basep)->mask[item / 32] &= ~(1 << (item % 32)))


typedef struct {			/* Internal period structure	     */
	char  tag[9];			/* Name tag of this period	     */
	byte  flags;			/* Period status flags - see PER_xxx */
	byte  defbegin[PER_BYTE_SIZE],	/* Definition of begin of period     */
	      defend[PER_BYTE_SIZE];	/* Definition of end of period	     */
	dword length;			/* Alternate: define end by length   */
	long  begin,			/* Calculated begin of period	     */
	      end;			/* Calculated end of period	     */
} PERIOD;


#define PER_LOCAL	0x01		/* Date specified in Local or GMT    */
#define PER_ACTIVE	0x02		/* Period is currently active	     */
#define PER_NOMORE	0x04		/* No starting date in future found  */
#define PER_BEGUN	0x08		/* Period begun, not all chores done */
#define PER_ENDED	0x10		/* Period ended, not all chores done */

#define SEC_MIN 	60		/* Number of seconds in one minute   */
#define SEC_HOUR	(60 * SEC_MIN)	/* Number of seconds in one hour     */
#define SEC_DAY 	(24L * SEC_HOUR)/* Number of seconds in one day      */


typedef struct {			/* Period bitmasks		     */
	dword mask[PER_MASK_DWORDS];	/* one bit for each period; 1=active */
} PERIOD_MASK;


#endif/*__PERIOD_DEF_*/


/* end of period.h */
