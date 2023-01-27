/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Indexing system on raw ASCII nodelist (St.Louis/FTS-0005 format) 10 Sep 1993
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


#include "2types.h"


#define NL_IDSTRING		   "NLidx"
#define NL_MAJOR		   (1)
#define NL_MINOR		   (0)
#define NL_REVISION		   ((NL_MAJOR << 8) | NL_MINOR)
#define NL_REVMAJOR(revision)	   (((revision) >> 8) & 0x00ff)
#define NL_REVMINOR(revision)	   ((revision) & 0x00ff)

#define NL_MAXFILES		   (256)
#define NL_ENTFILE(fileofs)	   (((int) ((fileofs) >> 24)) & 0x00ff)
#define NL_ENTOFS(fileofs)	   ((fileofs) & 0x00ffffffL)
#define NL_ENTRY(fileidx,fileofs)  ((((FILE_OFS) (fileidx)) << 24) | (fileofs))

enum { NL_NORM, NL_PVT, NL_HOLD, NL_DOWN, NL_HUB, NL_NC, NL_RC, NL_ZC };
#define NL_TYPEMASK(flags)	   ((flags) & 0x07)
#define NL_HASPOINTS		   (0x8000)		/* System has points */

#define NL_FPERIODIC		   (0x0001)		/* Periodic raw file */


/* HEADER NODE ... NET ... [POINT ... BOSS ...] NAME ... FILE ... ZONE ... */
typedef struct _nlheader NLHEADER;
typedef struct _nlfile	 NLFILE;
typedef struct _nlzone	 NLZONE;
typedef struct _nlnet	 NLNET;
typedef struct _nlnode	 NLNODE;
typedef struct _nlboss	 NLBOSS;
typedef struct _nlpoint  NLPOINT;


struct _nlheader {		/* ------- Header record at start of index   */
	char	 idstring[5];		/* Nodelist index ID string	     */
	char	 creator[40];		/* Name/ver index creating program   */
	word	 revision;		/* Index format revision major/minor */
	long	 timestamp;		/* UNIX timestamp of this index file */
	word	 files; 		/* Number of file records	     */
	FILE_OFS file_ofs;		/* Offset of file records in index   */
	word	 name_size;		/* Total size of name string block   */
	FILE_OFS name_ofs;		/* Offset of name strings in index   */
	word	 zones; 		/* Number of zone records	     */
	FILE_OFS zone_ofs;		/* Offset of zone records in index   */
	word	 bosses;		/* Number of boss records	     */
	FILE_OFS boss_ofs;		/* Offset of boss records in index   */
};

struct _nlfile {		/* ------- File record --------------------- */
	long	timestamp;		/* UNIX timestamp of this raw file   */
	word	file_flags;		/* Information flags		     */
	word	name_ofs;		/* Ofs in name string block to fname */
};

struct _nlzone {		/* ------- Zone record --------------------- */
	word	 zone;			/* Zone number			     */
	word	 flags; 		/* Information flags		     */
	FILE_OFS entry; 		/* File number and offset to entry   */
	word	 nets;			/* Number of nets in this zone	     */
	FILE_OFS net_ofs;		/* Offset of net records in index    */
};

struct _nlnet { 		/* ------- Net record ---------------------- */
	word	 net;			/* Net number			     */
	word	 flags; 		/* Information flags		     */
	FILE_OFS entry; 		/* File number and offset to entry   */
	word	 nodes; 		/* Number of nodes in this net	     */
	FILE_OFS node_ofs;		/* Offset of node records in index   */
};

struct _nlnode {		/* ------- Node record --------------------- */
	word	 node;			/* Node number			     */
	word	 flags; 		/* Information flags		     */
	FILE_OFS entry; 		/* File number and offset to entry   */
};

struct _nlboss {		/* ------- Boss record --------------------- */
	word	 zone, net, node;	/* Node number			     */
	word	 flags; 		/* Information flags		     */
	FILE_OFS entry; 		/* File number and offset to entry   */
	word	 points;		/* Number of points under this node  */
	FILE_OFS point_ofs;		/* Offset of point records in index  */
};

struct _nlpoint {		/* ------- Point record -------------------- */
	word	 point; 		/* Point number 		     */
	word	 flags; 		/* Information flags		     */
	FILE_OFS entry; 		/* File number and offset to entry   */
};


/* end of nodeidx.h */
