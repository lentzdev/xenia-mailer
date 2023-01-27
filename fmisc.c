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


#include "xenia.h"


static char xfer_log[PATHLEN + 1];
static boolean xfer_logged;
static boolean xfer_isreq;
static char xfer_pathname[PATHLEN + 1];
static char xfer_real[NAMELEN],
	    xfer_temp[NAMELEN];
static long xfer_fsize,
	    xfer_ftime;


void unique_name(char *pathname)
{
	static char *suffix = ".000";
	register char *p;
	register int   n;
	int len;
	int isarc;

	if (fexist(pathname)) {
	   p = pathname;
	   while (*p && *p != '.') p++;
	   for (n = 0; n < 4; n++) {
	       if (!*p) {
		  *p	 = suffix[n];
		  *(++p) = '\0';
	       }
	       else
		  p++;
	   }

	   len = ((int) strlen(pathname)) - 1;
	   isarc = is_arcmail(pathname,len);

	   while (fexist(pathname)) {
		 p = pathname + len;
		 if (!isdigit(*p) && (!isarc || !isalpha(*p)))
		    *p = '0';
		 else {
		    for (n = 3; n--; ) {
			if (isarc) {
			   if (!isalnum(*p)) *p = '0';
			   if (isdigit(*p)) {
			      if (++(*p) <= '9') break;
			      *p = 'A';
			      break;
			   }
			   else
			      if (toupper(++(*p)) <= 'Z') break;
			}
			else {
			   if (!isdigit(*p)) *p = '0';
			   if (++(*p) <= '9') break;
			}
			*(p--) = '0';
		    }/*for*/
		 }
	   }/*while(exist)*/
	}/*if(exist)*/
}/*unique_name()*/


char *xfer_init (char *fname, long fsize, long ftime)
{
	char  linebuf[LINELEN + 1];
	char  bad_real[NAMELEN],
	      bad_temp[NAMELEN];
	long  bad_fsize,
	      bad_ftime;
	struct stat f;
	char *p;
	FILE *fp;

	strcpy(xfer_real,fname);
	xfer_fsize = fsize;
	xfer_ftime = ftime;

	if (remote.capabilities) {
	   int n = ((int) strlen(xfer_real)) - 1;

	   if (n > 3 && !stricmp(&xfer_real[n-3],".REQ")) {
	      strcpy(xfer_pathname,inbound);
	      xfer_isreq = true;
	      goto doreq;
	   }
	}
	xfer_isreq = false;

	strcpy(xfer_pathname,remote.capabilities ? inbound : download);
	strcat(xfer_pathname,xfer_real);

	if (!getstat(xfer_pathname,&f) &&
	    xfer_fsize == f.st_size && xfer_ftime == f.st_mtime)
	   return (NULL);				/* already have file */

	strcpy(xfer_log,remote.capabilities ? inbound : download);
	strcat(xfer_log,"BAD-XFER.LOG");

	if ((fp = sfopen(xfer_log,"rt",DENY_WRITE)) != NULL) {
	   while (fgets(linebuf,LINELEN,fp)) {
		 if (sscanf(linebuf,"%s %s %ld %lo",
			    bad_real, bad_temp, &bad_fsize, &bad_ftime) < 4)
		    continue;
		 if (!stricmp(xfer_real,bad_real) &&
		     xfer_fsize == bad_fsize && xfer_ftime == bad_ftime) {
		    strcpy(xfer_pathname,remote.capabilities ? inbound : download);
		    strcat(xfer_pathname,bad_temp);
		    if (fexist(xfer_pathname)) {
		       fclose(fp);
		       strcpy(xfer_temp,bad_temp);

		       xfer_logged = true;
		       return (xfer_pathname);
		    }
		 }
	   }

	   fclose(fp);
	}

	strcpy(xfer_pathname,remote.capabilities ? inbound : download);
doreq:	p = xfer_pathname + ((int) strlen(xfer_pathname));
	sprint(p,"XENBAD%02x.000",task);
	unique_name(xfer_pathname);
	strcpy(xfer_temp,p);

	xfer_logged = false;
	return (xfer_pathname);
}/*xfer_init()*/


boolean xfer_bad (void)
{
	struct stat f;
	FILE *fp;
	int   n;

	if (xfer_logged)			/* Already a logged bad-xfer */
	   return (true);

	n = ((int) strlen(xfer_real)) - 1;
	if (xfer_isreq || (n > 3 && !stricmp(&xfer_real[n-3],".PKT"))) {
	   xfer_del();
	   return (false);			/* don't recover .PKT / .REQ */
	}

	getstat(xfer_pathname,&f);
	if (f.st_size < 1024L) {			 /* not worth saving */
	   xfer_del();
	   return (false);
	}

	if ((fp = sfopen(xfer_log,"at",DENY_WRITE)) != NULL) {
	   fprint(fp,"%s %s %ld %lo",
		     xfer_real, xfer_temp, xfer_fsize, xfer_ftime);
	   if (remote.capabilities && num_rem_adr)
	      fprint(fp," %d:%d/%d%s%s",
			rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
			pointnr(rem_adr[0].point),
			zonedomabbrev(rem_adr[0].zone));
	   fprint(fp,"\n");
	   fclose(fp);

	   return (true);			      /* bad-xfer logged now */
	}

	xfer_del();
	return (false); 			    /* Couldn't log bad-xfer */
}/*xfer_bad()*/


char *xfer_okay (void)
{
	static char new_pathname[PATHLEN + 1];
	char *p;
	SERVREQ *srp;

	if (xfer_isreq) {
	   sprint(xfer_real,"XENREQ%02x.000",task);
	   strcpy(new_pathname,inbound);
	}
	else
	   strcpy(new_pathname,remote.capabilities ? inbound : download);

	p = new_pathname + ((int) strlen(new_pathname));   /* start of fname */
	strcat(new_pathname,xfer_real); 		   /* add real fname */
	unique_name(new_pathname);			   /* make it unique */
	if (rename(xfer_pathname,new_pathname))       /* rename temp to real */
	   message(6,"!Could not rename '%s' to '%s'",xfer_pathname,new_pathname);
	else if (caller || !(ourbbs.capabilities & NO_REQ_ON)) {
	   for (srp = servreq; srp; srp = srp->next) {
	       if (patimat(xfer_real,srp->filepat)) {
		  SERVLST *slp, **slpp;

		  for (slpp = &servlst; (slp = *slpp) != NULL; slpp = &slp->next);
		  if ((slp = (SERVLST *) calloc(1,sizeof (SERVREQ))) == NULL)
		     message(6,"!Could not allocate server list");
		  else {
		     slp->pathname = ctl_string(new_pathname);
		     slp->servptr  = srp;
		     slp->next	   = *slpp;
		     *slpp = slp;
		  }
	       }
	   }
	}

	if (xfer_ftime)
	   setstamp(new_pathname,xfer_ftime);		    /* set timestamp */

	if (xfer_logged)			 /* delete from bad-xfer log */
	   xfer_del();

	return (stricmp(p,xfer_real) ? p : NULL);	      /* dup rename? */
}


void xfer_del (void)
{
	char  new_log[PATHLEN + 1];
	char  linebuf[LINELEN + 1];
	char  bad_real[NAMELEN],
	      bad_temp[NAMELEN];
	long  bad_fsize,
	      bad_ftime;
	FILE *fp, *new_fp;
	boolean left;

	if (fexist(xfer_pathname))
	   unlink(xfer_pathname);

	if (!xfer_isreq && (fp = sfopen(xfer_log, "rt", DENY_WRITE)) != NULL) {
	   sprint(new_log,"%sXENBAD%02x.$$$",
			  remote.capabilities ? inbound : download,task);
	   if ((new_fp = sfopen(new_log, "wt", DENY_ALL)) != NULL) {
	      left = false;
	      while (fgets(linebuf,LINELEN,fp)) {
		    if (sscanf(linebuf,"%s %s %ld %lo",
			       bad_real, bad_temp, &bad_fsize, &bad_ftime) < 4)
		       continue;
		    if (stricmp(xfer_real,bad_real) ||
			stricmp(xfer_temp,bad_temp) ||
			xfer_fsize != bad_fsize || xfer_ftime != bad_ftime) {
		       fputs(linebuf,new_fp);
		       left = true;
		    }
	      }
	      fclose(fp);
	      fclose(new_fp);
	      unlink(xfer_log);
	      if (left) rename(new_log,xfer_log);
	      else	unlink(new_log);
	   }
	   else
	      fclose(fp);
	}
}/*xfer_del()*/


/* end of fmisc.c */
