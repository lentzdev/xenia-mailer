/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Based off The-Box Mailer (C) Jac Kersing
 * Outbound list (AGL 21 Mar 94)
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


#include "xenia.h"


#define QUE_FORMAT 2

static OUTBLIST *outb_free   = NULL;
static OUTBLIST *outb_line   = NULL;
static boolean	 outb_manual = false;
static boolean	 outb_busy   = false;


/* ------------------------------------------------------------------------- */
OUTBLIST *add_outentry (int zone, int net, int node, int point)
{
	OUTBLIST *op, **opp;

	for (opp = &outblist; (op = *opp) != NULL; opp = &op->next) {
	    if (op->zone  > zone ) break;
	    if (op->zone  < zone ) continue;
	    if (op->net   > net  ) break;
	    if (op->net   < net  ) continue;
	    if (op->node  > node ) break;
	    if (op->node  < node ) continue;
	    if (op->point > point) break;
	    if (op->point < point) continue;
	    return (op);
	}

	if (outb_free) {
	   op = outb_free;
	   outb_free = outb_free->next;
	}
	else if ((op = (OUTBLIST *) malloc(sizeof (OUTBLIST))) == NULL) {
	   message(6,"!Error allocating for outbound list");
	   return (NULL);
	}
	memset(op,0,sizeof (OUTBLIST));

	op->next = *opp;
	*opp = op;

	op->zone  = (short) zone;
	op->net   = (short) net;
	op->node  = (short) node;
	op->point = (short) point;

	return (op);
}/*add_outentry()*/


/* ------------------------------------------------------------------------- */
static boolean scan_outname (boolean tbstyle, char *outname, char *s,
			     int zone, int net, int node, int point)
{
	word type = 0;
	struct stat f;
	boolean scanlist, scanfile /*, scanquery*/ ;
	OUTBLIST *op;

	scanlist = scanfile = /*scanquery =*/ false;
	strupr(s);

	switch (*s++) { 				/* The-Box  Binkley  */
	       case 'N': if (!tbstyle) return (true);	/* N?	    not N??  */
	       case 'O':				/*	    OUT      */
	       case 'F': type = OUTB_NORMAL;	break;	/*	    FLO      */
	       case 'C': type = OUTB_CRASH;	break;	/* C?	    C??      */
	       case 'I': type = OUTB_IMMEDIATE; break;	/* I?	    I??      */
	       case 'H':				/*	    H??      */
	       case 'W': type = OUTB_HOLD;	break;	/* W?	    W??      */
	       case 'D': type = OUTB_DIRECT;	break;	/* D?	    D??      */
	       case 'A': type = OUTB_ALONG;	break;	/* A?	    A??      */
	       case 'P': type = OUTB_POLL;	break;	/* P?	    P??      */
	       case 'R': type = OUTB_REQUEST;	break;	/*	    REQ      */
	       case 'Q': type = OUTB_QUERY;		/*	    Q??      */
			 /*scanquery = true;*/
			 break;
	       case '$': break; 			/*	    $$?      */
	       default:  return (true); 		/* Ignore the rest   */
	}

	if (tbstyle) {					/* The-Box outbound  */
	   switch (*s) {
		  case 'M':				/* mail packet	     */
		  case 'A': scanfile = true;		/* arcmail	     */
			    break;
		  case 'F': scanlist = true;		/* file attach list  */
			    break;
		  case 'R': type |= OUTB_REQUEST;	/* request	     */
			    break;
		  default:  return (true);		/* Ignore the rest   */
	   }
	}
	else {						/* Binkley outbound  */
	   if	   (!strcmp(s,"UT")) scanfile = true;	/* mail packet	     */
	   else if (!strcmp(s,"LO")) scanlist = true;	/* file attach list  */
	   else if (!strcmp(s,"EQ")) scanfile = true;	/* request	     */
	   else if (s[0] == '$' && s[-1] == '$')
				     scanfile = true;	/* call progress     */
	   else if (!strcmp(s,"PL")) {			/* FreePoll<tm> sema */
	      if (!getstat(outname,&f) && time(NULL) > (f.st_mtime + 300))
		 unlink(outname);			/* Older than 5 mins */
	      return (true);
	   }
	   else return (true);				/* ignore the rest   */
	}

#if 0
	if (scanquery && !getstat(outname,&f)) {
	   if (time(NULL) > (f.st_mtime + 129600L)) {	/* Older than 36 hrs */
	      unlink(outname);				/* 1440 * 1.5 * 60   */
	      return (true);
	   }
	}
#endif

	op = add_outentry(zone,net,node,point);
	if (!op) return (false);

	if (scanfile) {
	   if (!getstat(outname,&f)) {
	      if      (type == OUTB_HOLD) op->waitbytes += f.st_size;
	      else if (type)		  op->outbytes	+= f.st_size;
	      if (!op->oldestitem ||
		  (type && f.st_mtime < op->oldestitem) ||
		  !op->types)
		 op->oldestitem = f.st_mtime;
	   }
	}
	else if (scanlist) {
	   char  buffer[256];
	   char *p;
	   FILE *fp;

	   if (!getstat(outname,&f) &&
	       (!op->oldestitem || f.st_mtime < op->oldestitem || !op->types))
	      op->oldestitem = f.st_mtime;

	   if ((fp = sfopen(outname,"rb",DENY_NONE)) != NULL) {
	      while (fgets(buffer,255,fp)) {
		    p = strtok(buffer," \t\r\n\032");
		    if (!p || strchr(";~",*p)) continue;
		    if (strchr("#.^",*p) && !*++p) continue;
		    if (!getstat(p,&f)) {
		       if (type & OUTB_HOLD) op->waitbytes += f.st_size;
		       else		     op->outbytes  += f.st_size;
		       if (is_arcmail(p,strlen(p) - 1) &&
			   (!op->oldestitem || f.st_mtime < op->oldestitem))
			  op->oldestitem = f.st_mtime;
		    }
	      }
	      fclose(fp);
	   }
	}

	op->types |= type;

	return (true);
}/*scan_outname()*/


/* ------------------------------------------------------------------------- */
static boolean scan_outdirs (char *outbase)
{
	char	 filespec[PATHLEN], buffer[PATHLEN];
	char	*outp, *pointp, *filep, *p;
	int	 i, dzone, zone, net, node, point;
	boolean  primary;

	sprint(buffer,"%s.*",outbase);
	for (outp = dfirst2(buffer); outp; outp = dnext2()) {
	    primary = false;
	    p = strrchr(outp,'.');
	    if (!p) {					/* no .000 extention */
	       if (strcmp(outbase,hold))
		  continue;				/* not primary, skip */
	       primary = true;				/* Primary dir	     */
	       dzone = aka[0].zone;			/* set default zone  */
	    }
	    else if (strlen(p) < 4 || !sscanf(p,".%03x",&dzone) || !dzone)
	       continue;

	    if (primary) sprint(buffer,"%s\\",outbase);
	    else	 sprint(buffer,"%s.%03x\\",outbase,(word) dzone);
	    if (stricmp(buffer,holdarea(dzone))) {
	       message(6,"!Orphan outbound dir for zone %d (%s)", dzone, buffer);
	       continue;
	    }
	    p = &buffer[strlen(buffer)];
	    sprint(filespec,"%s*.*",buffer);
	    for (filep = ffirst(filespec); filep; filep = fnext()) {
		i = strlen(filep);
		if (primary && i == 11) {
		   getadress(filep,&zone,&net,&node,&point); /* The-Box outbound  */
		   strcpy(p,filep);
		   if (!scan_outname(true,buffer,filep + 9,zone,net,node,0)) {
		      fnclose();
		      dnclose2();
		      return (false);
		   }
		}
		else if (i == 12) {			/* Binkley outbound  */
		   if (sscanf(filep,"%04x%04x",&net,&node) != 2) continue;
		   strcpy(p,filep);
		   if (!scan_outname(false,buffer,filep + 9,dzone,net,node,0)) {
		      fnclose();
		      dnclose2();
		      return (false);
		   }
		}
	    }
	    fnclose();

	    if (primary) sprint(buffer,"%s\\*.PNT",outbase);
	    else	 sprint(buffer,"%s.%03x\\*.PNT",outbase,(word) dzone);
	    for (pointp = dfirst(buffer); pointp; pointp = dnext()) {
		if (strlen(pointp) < 12) continue;
		if (sscanf(pointp,"%04x%04x",&net,&node) != 2) continue;

		if (primary) sprint(buffer,"%s\\%s\\",outbase,pointp);
		else	     sprint(buffer,"%s.%03x\\%s\\",outbase,(word) dzone,pointp);
		p = &buffer[strlen(buffer)];
		sprint(filespec,"%s*.*",buffer);
		for (filep = ffirst(filespec); filep; filep = fnext()) {
		    if (strlen(filep) == 12) {		/* Binkley outbound  */
		       if (!sscanf(filep,"%08x",&point)) continue;
		       if (!point) {
			  message(6,"!Orphan .0 entry for node %d:%d/%d (%s)", dzone, net, node, buffer);
			  continue;				/*AGL:25jun96*/
		       }
		       strcpy(p,filep);
		       if (!scan_outname(false,buffer,filep + 9,dzone,net,node,point)) {
			  fnclose();
			  dnclose();
			  dnclose2();
			  return (false);
		       }
		    }
		}
		fnclose();
	    }
	    dnclose();
	}
	dnclose2();

	return (true);
}/*scan_outdirs()*/


/* ------------------------------------------------------------------------- */
boolean nlflag_isonline (int zone, char *flags, long curgmt)
{
	MAILHOUR *mp;
	boolean online;
	long midnight, start, end;
	char buffer[256];
	char *p;

	midnight = curgmt - (curgmt % 86400L);

	for (mp = mailhour; mp; mp = mp->next) {
	    if (zone == mp->zone && period_active(&mp->per_mask))
	       return (true);
	}

	strcpy(buffer,flags);

	online = false;
	for (p = strtok(buffer,","); p && !online; p = strtok(NULL,",")) {
	    if (!stricmp(p,"CM")) {
	       online = true;
	       continue;
	    }

	    if (p[0] == 'U' && !*++p)
	       continue;

	    if (strlen(p) == 3 && p[0] == 'T' && isalpha(p[1]) && isalpha(p[2])) {
	       start = midnight + (((islower(p[1]) ? 30L : 0L) + ((toupper(p[1]) - 'A') * 60L)) * 60L);
	       end   = midnight + (((islower(p[2]) ? 30L : 0L) + ((toupper(p[2]) - 'A') * 60L)) * 60L);
	       if (start > curgmt) {
		  start -= 86400L;
		  end	-= 86400L;
	       }
	       if (end < start)
		  end += 86400L;
	       if (curgmt >= start && curgmt <= end)
		  online = true;
	       continue;
	    }

	    while (strlen(p) >= 3 && (p[0] == '#' || p[0] == '!') &&
		   isdigit(p[1]) && isdigit(p[2])) {
		  start = midnight + (atoi(&p[1]) * 3600L);
		  end	= start + 3600L;
		  if (start > curgmt) {
		     start -= 86400L;
		     end   -= 86400L;
		  }
		  if (end < start)
		     end += 86400L;
		  if (curgmt >= start && curgmt <= end) {
		     online = true;
		     break;
		  }
		  p += 3;
	    }
	}

	return (online);
}/*nlflag_isonline()*/


/* ------------------------------------------------------------------------- */
void scan_outbound (void)
{
	char	  buffer[PATHLEN];
	char	  bsyfile[PATHLEN];
	int	  fd;
	FILE	 *fp;
	OUTBLIST  newob;
	OUTBLIST *op;
	DOMAIN	 *domp;
	int	  zone, net, node, point;
	struct stat f;

	sprint(buffer,"%sXENIABSY.%02x",flagdir,task);
	if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT,DENY_WRITE)) < 0)
	   return;
	dos_close(fd);
	sprint(bsyfile,"%sXMRESCAN.BSY",flagdir);
	if (!getstat(bsyfile,&f) && (f.st_mtime < (time(NULL) - 600)))
	   unlink(bsyfile);		/* delete if the BSY is old crap ;-) */
	if (rename(buffer,bsyfile)) {
	   unlink(buffer);
	   outb_busy = true;
	   return;
	}

	statusmsg("Reading outbound directories");

	alloc_msg("pre clear outbound");

	if (outblist) { 			      /* Clear outbound list */
	   for (op = outblist; op->next; op = op->next);
	   op->next = outb_free;
	   outb_free = outblist;
	   outblist = NULL;
	}

	alloc_msg("post clear outbound");

	sprint(buffer,"%sXMRESCAN.FLG",flagdir);
	if (!(ourbbsflags & NO_DIRSCAN) && !fexist(buffer)) {
	   if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0) dos_close(fd);
	}
	if (!getstat(buffer,&f))
	   scan_stamp = f.st_mtime;
	sprint(buffer,"%sXMRESCAN.QUE",flagdir);
	if (!getstat(buffer,&f) && f.st_mtime == scan_stamp &&
	    /*scan_stamp > (time(NULL) - 1800L) &&*/
	    (fp = sfopen(buffer,"rb",DENY_ALL)) != NULL) {
	   if (getc(fp) != QUE_FORMAT)
	      message(6,"-Incompatible XMRESCAN.QUE file in use!");
	   else {
	      while (fread(&newob,sizeof (OUTBLIST),1,fp) == 1) {
		    op = add_outentry(newob.zone,newob.net,newob.node,newob.point);
		    op->types	    = newob.types;
		    op->status	    = newob.status;
		    op->waitbytes   = newob.waitbytes;
		    op->outbytes    = newob.outbytes;
		    op->oldestitem  = newob.oldestitem;
		    op->noco	    = newob.noco;
		    op->conn	    = newob.conn;
		    op->lastattempt = newob.lastattempt;
	      }
	   }
	   fclose(fp);
	}

	if (!outblist) {
	   if (ourbbsflags & NO_DIRSCAN) {
	      scan_stamp = 0;
	      outb_busy = true;
	      goto fini;
	   }

	   if (!scan_outdirs(hold))		      /* Scan primary domain */
	      goto fini;

	   if (!nodomhold) {			      /* Do we have domains? */
	      for (domp = domain; domp; domp = domp->next) {
		  if (domp->hold && strcmp(domp->hold,hold)) {
		     if (!scan_outdirs(domp->hold))   /* Scan other domains  */
			goto fini;
		  }
	      }
	   }

	   if ((fp = sfopen(buffer,"wb",DENY_ALL)) == NULL)
	      message(6,"-Error creating file %s",buffer);
	   else {
	      putc(QUE_FORMAT,fp);
	      for (op = outblist; op; op = op->next)
		  fwrite(op,sizeof (OUTBLIST),1,fp);
	      fclose(fp);
	   }

	   setstamp(buffer,scan_stamp);
	}

#if 0
{ FILE *fp;
  char buffer[PATHLEN];
	sprint(buffer,"%soutb%u.dmp",homepath,task);
	if ((fp = fopen(buffer,"wt")) != NULL) {
	   for (op = outblist; op; op = op->next) {
	       sprint(buffer,"%d:%d/%d%s",
		      (int) op->zone, (int) op->net, (int) op->node, pointnr(op->point));
	       fprint(fp,"%-20s  %c%c%c%c%c%c%c%c  %8lu  %8lu  %1z,%1z  %08lx\n",
			 buffer,
			 (op->types & OUTB_NORMAL   ) ? 'N' : '-',
			 (op->types & OUTB_CRASH    ) ? 'C' : '-',
			 (op->types & OUTB_IMMEDIATE) ? 'I' : '-',
			 (op->types & OUTB_HOLD     ) ? 'H' : '-',
			 (op->types & OUTB_DIRECT   ) ? 'D' : '-',
			 (op->types & OUTB_ALONG    ) ? 'A' : '-',
			 (op->types & OUTB_POLL     ) ? 'P' : '-',
			 (op->types & OUTB_REQUEST  ) ? 'R' : '-',
			 op->waitbytes, op->outbytes,
			 op->conn, op->noco, op->lastattempt);
	   }
	   fclose(fp);
	}
}
#endif

	outb_busy = false;
fini:	alloc_msg("post scan outbound");
	outb_line = outblist;
	outb_manual = false;
	unlink(bsyfile);
	xmit_next(&zone,&net,&node,&point);
	/*rescan = timerset(6000);*/
	statusmsg(NULL);
}/*scan_outbound()*/


/* ------------------------------------------------------------------------- */
xmit_next (int *zone, int *net, int *node, int *point)
{
   OUTBLIST *op, *bestop;
   PHONE    *pp;
   char      naddress[80];
   char      buffer[PATHLEN];
   boolean   override, iscm, hipricm;
   long      curgmt;
   char     *p, *q;
   int	     i;
   extern int ptemp;  /* declared in mailer.c */

   sprint(buffer,"%sXMRESCAN.BSY",flagdir);
   if (fexist(buffer)) {
      outb_busy = true;
      show_outbound(No_Key);
      return (0);
   }

   statusmsg("Scanning outbound list");

   curgmt = time(NULL) + gmtdiff;
   bestop = NULL;
   for (op = outblist; op; op = op->next) {
       if (op->status != OUTB_DONE)
	  op->status = OUTB_OTHER;

       remote.zone  = *zone  = op->zone;
       remote.net   = *net   = op->net;
       remote.node  = *node  = op->node;
       remote.point = *point = op->point;

       if (!nodetoinfo(&remote)) {
	  op->status = OUTB_UNKNOWN;
	  continue;
       }
       if (remote.nodeflags & VIAHOST)
	  remote.node = remote.point = 0;
       else if (remote.nodeflags & VIABOSS)
	  remote.point = 0;

       for (i = 0; i < emsi_adr; i++) {
	   if (aka[i].zone == remote.zone && aka[i].net == remote.net &&
	       aka[i].node == remote.node && aka[i].point == remote.point)
	      break;
       }
       if (i < emsi_adr) {
	  op->status = OUTB_OKDIAL;
	  continue;
       }

       if (op->types & (OUTB_CRASH | OUTB_IMMEDIATE | OUTB_POLL))
	  iscm = true;
       else
	  iscm = false;

       if (!iscm) {
	  if (!(op->types & (OUTB_NORMAL | OUTB_DIRECT)))
	     continue;
	  if (e_ptrs[cur_event]->behavior & MAT_CM) {
	     op->status = OUTB_CM;
	     continue;
	  }
       }

       if (iscm && (e_ptrs[cur_event]->behavior & MAT_HIPRICM))
	  hipricm = true;
       else
	  hipricm = false;

       if (!hipricm && (e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
	  op->status = OUTB_NOOUT;
	  continue;
       }

       /* If this is supposed to be only local, then get out if it isn't */
       if (!iscm && (e_ptrs[cur_event]->behavior & MAT_LOCAL) &&
	   remote.cost > local_cost) {
	  op->status = OUTB_LOCAL;
	  continue;
       }

       sprint(naddress,"%d:%d/%d%s",remote.zone,remote.net,remote.node,pointnr(remote.point));

       /* Not mailhour: only allowed to send to systems with CM flag */
       if (!hipricm && !(e_ptrs[cur_event]->behavior & MAT_MAILHOUR)) {
	  override = false;
	  for (pp = phone; pp; pp = pp->next) {
	      if (remote.zone == pp->zone && remote.net   == pp->net &&
		  remote.node == pp->node && remote.point == pp->point) {
		 override = true;

		 if ((p = pp->nodeflags) == NULL) {
		    if (strpbrk(pp->number,":/")) {
		       int tzone, tnet, tnode, tpoint;

		       getaddress(pp->number,&tzone,&tnet,&tnode,&tpoint);
		       nlidx_getphone(tzone,tnet,tnode,tpoint,NULL,&p);
		    }
		    else
		       nlidx_getphone(remote.zone,remote.net,remote.node,remote.point,NULL,&p);
		 }

		 if (p && nlflag_isonline(remote.zone,p,curgmt))
		    break;
	      }
	  }
	  if (!pp) {
	     if (override) {
		op->status = OUTB_MAILHOUR;
		continue;
	     }
	     if (nlidx_getphone(remote.zone,remote.net,remote.node,remote.point,NULL,&p) == NULL ||
		 !nlflag_isonline(remote.zone,p,curgmt)) {
		op->status = OUTB_MAILHOUR;
		continue;
	     }
	  }
       }

       if (okdial) {
	  OKDIAL *dp;

	  override = false;
	  dp = NULL;
	  for (pp = phone; pp && !dp; pp = pp->next) {
	      if (remote.zone == pp->zone && remote.net   == pp->net &&
		  remote.node == pp->node && remote.point == pp->point) {
		 override = true;

		 q = pp->number;
		 p = pp->nodeflags;

		 if (strpbrk(pp->number,":/")) {
		    int tzone, tnet, tnode, tpoint;

		    getaddress(pp->number,&tzone,&tnet,&tnode,&tpoint);
		    if (!p) nlidx_getphone(tzone,tnet,tnode,tpoint,&q,&p);
		 }
		 else if (!p)
		    nlidx_getphone(remote.zone,remote.net,remote.node,remote.point,NULL,&p);

		 for (dp = okdial; dp; dp = dp->next) {
		     if (patimat(naddress,dp->pattern) ||
			 patimat(pp->number,dp->pattern) ||
			 (p && patimat(p,dp->pattern)))
			break;
		 }
	      }
	  }
	  if (!dp && !override) {
	     nlidx_getphone(remote.zone,remote.net,remote.node,remote.point,&p,&q);
	     for (dp = okdial; dp; dp = dp->next) {
		 if (patimat(naddress,dp->pattern) ||
		     (p && patimat(p,dp->pattern)) ||
		     (q && patimat(q,dp->pattern)))
		    break;
	     }
	  }

	  if (!dp || !dp->allowed) {
	     op->status = OUTB_OKDIAL;
	     continue;
	  }
       }

       /* See if we spent too much calling him already */
       if (bad_call(op->zone,op->net,op->node,op->point,0) || !op->types) {
	  if (op->status == OUTB_WHOKNOWS)
	     continue;
	  if (op->types) {
	     if (op->conn >= max_connects || op->noco >= max_noconnects)
		op->status = OUTB_TOOBAD;
	  }
	  else {
	     if (op->status != OUTB_DONE) op->status = OUTB_WHOKNOWS;
	  }
	  continue;
       }

       if (op->noco > 0) op->status = OUTB_TRIED;
       else		 op->status = OUTB_WILLCALL;

       if (!iscm && ((op->outbytes / 1024) < e_ptrs[cur_event]->queue_size)) {
	  op->status = OUTB_TOOSMALL;
	  continue;
       }

       if (!flag_test(remote.zone,remote.net,remote.node,remote.point))
	  continue;

       if (outb_manual && op == outb_line) {
	  bestop = op;
	  break;
       }

       if (!bestop || op->lastattempt < bestop->lastattempt)
	  bestop = op;
   }

#if 0
{ FILE *fp;
  char buffer[PATHLEN];
	sprint(buffer,"%soutb%u.dmp",homepath,task);
	if ((fp = fopen(buffer,"wt")) != NULL) {
	   for (op = outblist; op; op = op->next) {
	       sprint(buffer,"%d:%d/%d%s",
		      (int) op->zone, (int) op->net, (int) op->node, pointnr(op->point));
	       fprint(fp,"%-20s  %c%c%c%c%c%c%c%c  %8lu  %8lu  %1z,%1z  %08lx\n",
			 buffer,
			 (op->types & OUTB_NORMAL   ) ? 'N' : '-',
			 (op->types & OUTB_CRASH    ) ? 'C' : '-',
			 (op->types & OUTB_IMMEDIATE) ? 'I' : '-',
			 (op->types & OUTB_HOLD     ) ? 'H' : '-',
			 (op->types & OUTB_DIRECT   ) ? 'D' : '-',
			 (op->types & OUTB_ALONG    ) ? 'A' : '-',
			 (op->types & OUTB_POLL     ) ? 'P' : '-',
			 (op->types & OUTB_REQUEST  ) ? 'R' : '-',
			 (op->types & OUTB_QUERY    ) ? 'Q' : '-',
			 op->waitbytes, op->outbytes,
			 op->conn, op->noco, op->lastattempt);
	   }
	   fclose(fp);
	}
}
#endif

   /* If we got out, then we have no more to do (or found something) */
   outb_line = bestop;
   outb_manual = false;
   show_outbound(No_Key);
   statusmsg(NULL);

   if (bestop) {
      *zone  = bestop->zone;
      *net   = bestop->net;
      *node  = bestop->node;
      *point = bestop->point;

      if (bestop->types & OUTB_POLL) { /* we've found a poll or pickup! */
	 ptemp = pickup;
	 pickup = 1;
      }
      return (1);	/* we found something to call */
   }

   return (0);	/* nothing found */
}/*xmit_next()*/


/* ------------------------------------------------------------------------- */
void show_outbound (word keycode)
{
	static char status_chars[] = "-!x*+rcl#d<$?";
	char naddress[80];
	OUTBLIST *op;
	long curtime = time(NULL);
	int  top, total, i;
	word depth = windows[outbound_win].cur_verlen;

	if (outb_busy) {
	   win_cls(outbound_win);
	   win_cyprint(outbound_win,depth / 2, "Rescan busy");
	   return;
	}
	if (!outblist) {
	   if (!keycode) {
	      win_cls(outbound_win);
	      win_cyprint(outbound_win,depth / 2, "Outbound empty");
	   }
	   return;
	}

	curtime -= (curtime % 86400L);

	for (op = outblist, total = top = i = 0; op; op = op->next, total++)
	    if (outb_line == op) top = i = total;

	switch (keycode) {
	       case Home: top = 0;
			  break;
	       case Up:   if (top) top--;
			  break;
	       case Down: if (top < (total - 1)) top++;
			  break;
	       case PgUp: if (top >= depth) top -= depth;
			  else		    top = 0;
			  break;
	       case PgDn: if (total > depth && top < (total - depth)) {
			     top += depth;
			     break;
			  }
			  /* fallthrough to end */
	       case End:  top = (word) (total ? (total - 1) : 0);
			  break;
	       default:   break;
	}

	outb_line = outblist;
	for (i = 0; i != top; i++, outb_line = outb_line->next);

	if (keycode)
	   outb_manual = true;

	for (op = outb_line, i = 1; i <= depth; i++) {
	    win_setpos(outbound_win,1,i);
	    if (op) {
	       static char amount_chars[] = "bKMGT";
	       word amount_index;
	       dword amount = op->waitbytes + op->outbytes,
		     old_amount;
	       char namount[10], tempstr[10];
	       word nage;

	       sprint(naddress,"%d:%d/%d%s",
			       (int) op->zone, (int) op->net, (int) op->node,
			       pointnr(op->point));

	       for (amount_index = 0;
		    amount >= 1024L && amount_chars[amount_index];
		    amount_index++) {
		   old_amount = amount;
		   amount /= 1024LU;
	       }

	       if (!amount_chars[amount_index]) {
		  strcpy(namount,"Huge");
	       }
	       else if (amount > 999LU) {
		  sprint(namount,".%2lu%c", (dword) ((amount * 25LU) / 256LU),
					     amount_chars[amount_index + 1]);
	       }
	       else if (amount < 10 && amount_index) {
		  sprint(tempstr,"%02u", (word) ((old_amount * 5LU) / 512LU));
		  sprint(namount,"%c.%c%c", tempstr[0], tempstr[1],
					    amount_chars[amount_index]);
	       }
	       else if (amount > 0) {
		  sprint(namount,"%3lu%c", amount,
					   amount_chars[amount_index]);
	       }
	       else
		  namount[0] = '\0';

	       if (op->oldestitem)
		  nage = (word) ((curtime - (op->oldestitem - (op->oldestitem % 86400L))) / 86400L);
	       else
		  nage = 0;
	       sprint(tempstr,"%u", nage < 999 ? nage : 999);
	       sprint(naddress, "%-13.13s %4s %3s",
				naddress, namount,
				nage < 1 ? "new" : (nage < 365 ? tempstr : "old"));

	       win_print(outbound_win,"%-22.22s %c%c%c%c%c%c%c%c%c %c",
			 naddress,
			 (op->types & OUTB_NORMAL   ) ? 'N' : '-',
			 (op->types & OUTB_CRASH    ) ? 'C' : '-',
			 (op->types & OUTB_IMMEDIATE) ? 'I' : '-',
			 (op->types & OUTB_HOLD     ) ? 'H' : '-',
			 (op->types & OUTB_DIRECT   ) ? 'D' : '-',
			 (op->types & OUTB_ALONG    ) ? 'A' : '-',
			 (op->types & OUTB_POLL     ) ? 'P' : '-',
			 (op->types & OUTB_REQUEST  ) ? 'R' : '-',
			 (op->types & OUTB_QUERY    ) ? 'Q' : '-',
			 status_chars[op->status]);
	       op = op->next;
	    }
	    else
	       win_clreol(outbound_win);
	}
}/*show_outbound()*/


/* ------------------------------------------------------------------------- */
dword check_kbmail (boolean iscaller, int zone, int net, int node, int point)
{
	OUTBLIST *op;

	for (op = outblist; op; op = op->next) {
	    if (op->zone  > zone ) break;
	    if (op->zone  < zone ) continue;
	    if (op->net   > net  ) break;
	    if (op->net   < net  ) continue;
	    if (op->node  > node ) break;
	    if (op->node  < node ) continue;
	    if (op->point > point) break;
	    if (op->point < point) continue;

	    if (iscaller) return (op->outbytes);
	    else	  return (op->waitbytes + op->outbytes);
	}

	return (0UL);
}/*check_kbmail()*/


/* outblist.c */
