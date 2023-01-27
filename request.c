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


#include "zmodem.h"
#include "request.h"
#include "mail.h"

extern char *mon[];

#define TECH_BARKREQ 0
#define TECH_SEALINK 1
#define TECH_ZMODEM  2

static int  proc_req();
static void prepfname(char *, char *);
static int  wildmatch(char FAR *, char FAR *);
static int  exactmatch(char FAR *, char FAR *);
static int  rsp_check(int,int);
static void process_service();
static int  qf_write(char FAR *fdir, char FAR *fname);
static void qm_write();
static void req_macro(char FAR *, int *, char FAR *);

       char   *timestr_tpl = "%02d %3s %02d  %02d:%02d:%02d";
static char   *int_tpl	   = "%d";
static char   *word_tpl    = "%u";
static char   *qfname = NULL, *qmname = NULL;
static FILE   *idxfp = NULL, *dirfp = NULL;
static FILE   *qffp = NULL, *qmfp = NULL, *reqlogfp = NULL;
static struct _reqhdr	   reqhdr;
static struct _reqidx	   reqidx;
static struct _reqdir	   reqdir;
static word    reqlevel;
static word    countlimit, sizelimit, timelimit;
static word    countorig,  sizeorig,  timeorig;
static word    counttotal;
static long    sizetotal;
static char    reqname[14], reqpwd[8];
static long    reqftime;
static char   *options;
static char FAR *rsp_text     = NULL;
static char FAR *rsp_fileinfo = NULL;
static char FAR *rsp_buffer   = NULL;
static int     rsp_pos = 0;
static int     rsp_code = 0;
static int     rsp_error = 0;	  /* zero while qmname ok (no write errors)  */
static int     rsp_noresp = 0;	  /* active if reqcfg says shouldn't respond */
static char   *rsp_logstr[] = { "-Request '%s' not found",
				"-'%s' nor other requests possible",
				"-'%s' nor other requests on us allowed",
				"-This event doesn't allow request '%s'",
				"-Requestlevel too low for '%s'",
				"-Requestlevel too high for '%s'",
				"-Password error for request '%s'",
				"-Count limit for request '%s' ",
				"-Size limit for request '%s' ",
				"-Timer limit for request '%s'",
				"-No older files for update request '%s'",
				"-No newer files for update request '%s'",
				"+File request '%s' processed",
				"+Old-file update request '%s' processed",
				"+New-file update request '%s' processed",
				"+Magic request '%s' processed",
				"+Service request '%s' processed",
				"-Error processing file request '%s'",
				"-Error processing old-update request '%s'",
				"-Error processing new-update request '%s'",
				"-Error processing magic file request '%s'",
				"-Error processing service request '%s'"};
static char *macro_key[] = {
     "OURNAME", "OURSYSOP", "OURADDRESS",
     "TASK", "PORT", "DEVICE", "HANDLE", "LOCKED", "SPEED",
     "RELIABLE", "CALLERID", "EVTTIME", "PROTOCOL",
     "OUTFILES", "OUTDATA", "CURTIME", "XENVERSION",
     "THEIRNAME", "THEIRSYSOP", "THEIRADDRESS", "REQLEVEL",
     "COUNTLIMIT", "SIZELIMIT", "TIMELIMIT",
     "COUNTTOTAL", "SIZETOTAL", "TIMETOTAL",
     "REQNAME", "REQUPDATE", "REQTIME", "REQPASSWORD", "REQOPTIONS",
     "FILENAME", "FILESIZE", "FILETIME", "XFERTIME",
     "FILEINFO",
     "TBVERSION",
     NULL};



static int req_open()		      /* 1=success (or already open), 0=fail */
{
	char  buffer[256];

	if (!reqpath)
	   return (0);
	if (idxfp)
	   return (1);

	alloc_msg("pre req init");

	sprint(buffer,"%sREQUEST.IDX",reqpath);
	if ((idxfp=sfopen(buffer,"rb",DENY_NONE))==NULL)
	   return (0);

	setvbuf(idxfp,NULL,_IOFBF,16384);

	fread(&reqhdr,sizeof (struct _reqhdr),1,idxfp);

	if (rsp_text) myfarfree(rsp_text);
	if (rsp_text) myfarfree(rsp_fileinfo);
	if (rsp_text) myfarfree(rsp_buffer);
	rsp_text = rsp_fileinfo = rsp_buffer = NULL;
	if (reqhdr.rsplen) {
	   if ((rsp_text     = (char FAR *)myfaralloc(reqhdr.rsplen)) == NULL ||
	       (rsp_fileinfo = (char FAR *)myfaralloc(MAXRSPTEXT+1))  == NULL ||
	       (rsp_buffer   = (char FAR *)myfaralloc(MAXRSPTEXT+1))  == NULL) {
	      message(6,"!REQ: Not enough memory for response processing!");
	      fclose(idxfp);
	      if (rsp_text) myfarfree(rsp_text);
	      if (rsp_buffer) myfarfree(rsp_buffer);
	      rsp_text = rsp_buffer = NULL;
	      return (0);
	   }

	   fseek(idxfp,(long) sizeof (struct _reqhdr) +
		       ((long) reqhdr.idxnum * sizeof (struct _reqidx)),SEEK_SET);

	   if (!myfarfread(rsp_text,reqhdr.rsplen,1,idxfp)) {
	      fclose(idxfp);
	      myfarfree(rsp_text);
	      myfarfree(rsp_fileinfo);
	      myfarfree(rsp_buffer);
	      rsp_text = rsp_fileinfo = rsp_buffer = NULL;
	      return (0);
	   }
	   rsp_error = 0;
	}

	sprint(buffer,"%sREQUEST.DIR",reqpath);
	if ((dirfp=sfopen(buffer,"rb",DENY_NONE))==NULL) {
	   fclose(idxfp);
	   return (0);
	}

	/* Security logic  -  getreqlevel(getnode.c)  time_to_noreq(sched.c) */
	reqlevel   = getreqlevel();
	countlimit = countorig = reqhdr.reqsec[reqlevel].countlimit;
	sizelimit  = sizeorig  = reqhdr.reqsec[reqlevel].sizelimit;
	timelimit  = timeorig  = time_to_noreq();
	counttotal = 0;
	sizetotal = 0L;
	message(2,"+REQ: level=%u  count=%u  size=%u  time=%u",
		reqlevel,countorig,sizeorig,timeorig);

#if 0
	if (rem_adr[0].point)
	   sprint(buffer,"%s%04.4x%04.4x.PNT", holdarea(rem_adr[0].zone),
			  (word) rem_adr[0].net, (word) rem_adr[0].node);
	else {
	   sprint(buffer, holdarea(rem_adr[0].zone));
	   p = &buffer[strlen(buffer) - 1];
	   if (*p == '\\') *p = '\0';
	}
	if (dfirst(buffer)) {
	   if (!qfname) {
	      sprint(buffer, "%s%04.4x%04.4x%s.QLO", holdarea(rem_adr[0].zone),
			     (word) rem_adr[0].net, (word) rem_adr[0].node,
			     holdpoint(rem_adr[0].point));
	      qfname = ctl_string(buffer);
	   }
	   if (rsp_text && !qmname) {
	      sprint(buffer, "%s%04.4x%04.4x%s.QUT", holdarea(rem_adr[0].zone),
			     (word) rem_adr[0].net, (word) rem_adr[0].node,
			     holdpoint(rem_adr[0].point));
	      qmname = ctl_string(buffer);
	   }
	}
	else
#endif
	{
	   if (!qfname) {
	      sprint(buffer, "%s\\XENIAQLO.%02x", hold, task);
	      qfname = ctl_string(buffer);
	   }
	   if (rsp_text && !qmname) {
	      sprint(buffer, "%s\\XENIAQUT.%02x", hold, task);
	      qmname = ctl_string(buffer);
	   }
	}

	sprint(buffer,"%sXENRQ%u.LOG",reqpath,task);
	if (fexist(buffer) &&
	    (reqlogfp = sfopen(buffer,"at",DENY_WRITE)) != NULL) {
	   int i = 0;

	   req_macro((char FAR *) buffer, &i, (char FAR *)
		     "\n* %CURTIME  %THEIRADDRESS  %THEIRNAME\n");
	   if (!fwrite(buffer,i,1,reqlogfp)) {
	      message(6,"!Error writing request logfile!");
	      fclose(reqlogfp);
	      reqlogfp = NULL;
	   }
	}

	alloc_msg("post req init");

	return (1);
}/*req_open()*/


void req_close()
{
	char buffer[256];

	alloc_msg("pre req close");

	if (rsp_text) {
	   myfarfree(rsp_text);
	   myfarfree(rsp_fileinfo);
	   myfarfree(rsp_buffer);
	   rsp_text = rsp_fileinfo = rsp_buffer = NULL;
	}
	if (idxfp) {
	   fclose(idxfp);
	   idxfp = NULL;
	}
	if (dirfp) {
	   fclose(dirfp);
	   dirfp = NULL;
	}
	if (reqlogfp) {
	   fclose(reqlogfp);
	   reqlogfp = NULL;
	}
	if (qffp) {
	   fclose(qffp);
	   qffp = NULL;
	}
	if (qfname) {
	   free(qfname);
	   qfname = NULL;
	}
	if (qmfp) {
	   fclose(qmfp);
	   qmfp = NULL;
	}
	if (qmname) {
	   free(qmname);
	   qmname = NULL;
	}

	sprint(buffer, "%s\\XENIAQLO.%02x", hold, task);
	if (ffirst(buffer)) unlink(buffer);
	sprint(buffer, "%s\\XENIAQUT.%02x", hold, task);
	if (ffirst(buffer)) unlink(buffer);

	rm_requests();	/* To get rid of servlst (server requests) */

	alloc_msg("post req close");
}/*req_close()*/


static int do_srif (int fsent, char *line, int tech)
{
	static FILE *fp = NULL;
	char  buffer[256],  srif[256], rspfile[256];
	char *p;
	int   i, res = XFER_OK;

	if (!fp) {
	   if (!line)
	      return (fsent);

	   sprint(buffer,"%s\\XENIAREQ.%02x",hold,task);
	   if ((fp = sfopen(buffer,"wt",DENY_ALL)) == NULL)
	      return (fsent);
	}

	if (line)
	   fputs(line,fp);

	if (line && tech != TECH_BARKREQ)
	   return (fsent);

	fclose(fp);

	/* --- Write SRIF format file ---- */
	sprint(srif,"%s\\XENIASRI.%02x",hold,task);
	if ((fp = sfopen(srif,"wt",DENY_ALL)) == NULL)
	   return (0);
	fprint(fp,"Sysop %s\n", remote.sysop);
	for (i = 0; i < num_rem_adr; i++)
	    fprint(fp,"AKA %d:%d/%d%s%s\n",
		       rem_adr[i].zone, rem_adr[i].net, rem_adr[i].node,
		       pointnr(rem_adr[i].point), zonedomname(rem_adr[i].zone));
	fprint(fp,"Baud %lu\n", cur_speed);
	fprint(fp,"Time %u\n", time_to_noreq());
	if (ourbbs.password[0])
	   fprint(fp,"Password %s\n", ourbbs.password);
	fprint(fp,"RequestList %s\\XENIAREQ.%02x\n",hold,task);
	fprint(fp,"ResponseList %s\\XENIARSP.%02x\n",hold,task);
	reqlevel = getreqlevel();
	fprint(fp,"RemoteStatus %sPROTECTED\n", reqlevel == 5 ? "" : "UN");
	fprint(fp,"SystemStatus %sLISTED\n", reqlevel >= 2 ? "" : "UN");
	fclose(fp);
	/* ---- */

	sprint(rspfile,"%s\\XENIARSP.%02x",hold,task);
	if (fexist(rspfile)) unlink(rspfile);

	message(3,"+Calling external request processor");

#if PC | OS2 | WINNT
	p = getenv("COMSPEC");
	sprint(buffer,"%s /c ",p ? p :
#if PC | WINNT
	       "COMMAND.COM");
#else/*OS2*/
	       "CMD.EXE");
#endif
#else/*ST*/
	buffer[0] = '\0';
#endif
	sprint(&buffer[strlen(buffer)],"%s %s", reqprog, srif);

	if (log != NULL) fclose(log);

	if (mdm.shareport)
	   com_suspend = true;
	sys_reset();
	(void) win_savestate();
	execute(buffer);
	(void) win_restorestate();
	sys_init();

	if (fexist(srif)) unlink(srif);
	sprint(buffer,"%s\\XENIAREQ.%02x",hold,task);
	if (fexist(buffer)) unlink(buffer);

	message(1,"+Returned from external request processor");

	if (log != NULL) {
	   if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
	      message(6,"!Can't re-open logfile!");
	}

	if ((fp = sfopen(rspfile,"rt",DENY_ALL)) == NULL)
	   message(6,"!Can't open request response file!");
	else {
	   while (carrier() && fgets(buffer,255,fp)) {
		 if ((p = strchr(buffer,'\n')) != NULL) *p = '\0';
		 if (!buffer[0]) continue;

		 if (tech != TECH_ZMODEM)
		    com_purge();
		 res = (*wzsendfunc)(&buffer[1], NULL,
			 (int) (buffer[0] == '+' ? NOTHING_AFTER : DELETE_AFTER),
			 fsent++, DO_WAZOO);
		 if (buffer[0] == '-' && fexist(&buffer[1]))
		    unlink(&buffer[1]);
		 if (res == XFER_ABORT) break;
	   }

	   fclose(fp);
	}

	if (fexist(rspfile)) unlink(rspfile);
	fp = NULL;

	if (res != XFER_ABORT && tech == TECH_BARKREQ)
	   end_batch();

	return ((res != XFER_ABORT) ? fsent : -1);
}/*do_srif()*/


int process_req_files(int fsent)
{
	FILE *fp;
	char req_tpl[256], reqfile[256];
	char buffer[256];
	char *p;
	int   c, got;
	int  res=0;
	SERVLST *l;

	if (remote.options & EOPT_BADAPPLE) {
	   message(1,"=Ignoring inbound requests");
	   rm_requests();
	   return (fsent);
	}

	if (servlst) {
#if 0
	   if (!qfname) {
	      sprint(buffer, "%s%04.4x%04.4x%s.QLO", holdarea(rem_adr[0].zone),
			     (word) rem_adr[0].net, (word) rem_adr[0].node,
			     holdpoint(rem_adr[0].point));
	      qfname = ctl_string(buffer);
	      createhold(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point);
	   }
#endif
	   if (!qfname) {
	      sprint(buffer, "%s\\XENIAQLO.%02x", hold, task);
	      qfname = ctl_string(buffer);
	   }

	   for (l = servlst; l; l = l->next) {
	       if (!l->pathname) continue;

	       message(3,"+Calling server %s", l->servptr->description);

#if PC | OS2 | WINNT
	       p = getenv("COMSPEC");
	       sprint(buffer,"%s /c ",p ? p :
#if PC | WINNT
		      "COMMAND.COM");
#else/*OS2*/
		      "CMD.EXE");
#endif
#else/*ST*/
	       buffer[0] = '\0';
#endif
	       sprint(&buffer[strlen(buffer)],"%s %s %u %s",
		      l->servptr->progname, l->pathname, task, qfname);

	       if (log != NULL) fclose(log);
	       if (qffp != NULL) fclose(qffp);

	       if (mdm.shareport)
		  com_suspend = true;
	       sys_reset();
	       (void) win_savestate();
	       execute(buffer);
	       (void) win_restorestate();
	       sys_init();

	       message(1,"+Returned from server");

	       if (log != NULL) {
		  if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
		     message(6,"!Can't re-open logfile!");
	       }
	       if (qffp != NULL) {
		  if ((qffp = sfopen(qfname, "r+b", DENY_ALL)) == NULL)
		     message(6,"!Can't re-open request outbound list!");
	       }
	   }
	}

	sprint(req_tpl,"%sXENREQ%02x.*",inbound,task);

	while (((p = ffirst(req_tpl)) != NULL) && (res != -1)) {
	      fnclose();
	      sprint(reqfile,"%s%s",inbound,p);
	      if ((fp = sfopen(reqfile,"rb",DENY_ALL)) == NULL)
		 continue;
	      do {
		 got = 0;
		 while ((c = getc(fp)) != EOF && !ferror(fp) && got < 128) {
		       if (c == '\032')
			  break;
		       if (c == '\r' || c == '\n') {
			  if (got) break;
		       }
		       else
			 buffer[got++] = c;
		 }
		 if (!got || ferror(fp)) break;
		 if (buffer[0] == ';') continue;
		 buffer[got++] = '\n';
		 buffer[got]   = '\0';

		 if (reqprog)
		    res = do_srif(fsent,buffer,
				  (wzsendfunc == send_Zmodem || wzsendfunc == send_hydra) ?
				  TECH_ZMODEM : TECH_SEALINK);
		 else
		    res = do_reqline(buffer);
	      } while (res != -1);
	      fclose(fp);
	      unlink(reqfile);
	}
	fnclose();

	if (reqprog)
	   res = do_srif(fsent,NULL,
			 (wzsendfunc == send_Zmodem || wzsendfunc == send_hydra) ?
			  TECH_ZMODEM : TECH_SEALINK);

#if 0
	if (!carrier())
	   qm_finish();
	else
#endif
	   res = qf_send(fsent,
			 (wzsendfunc == send_Zmodem || wzsendfunc == send_hydra) ?
			 TECH_ZMODEM : TECH_SEALINK);

	req_close();		      /* not earlier, qf_send() needs qfname */

	return (res);
}/*process_req_files()*/


int do_reqline(char *reqline)
{
	char *p, *q;
	int res;

	for (p = reqline + strlen(reqline) - 1;
	     (p >= reqline) && (*p < ' ');
	     *p-- = '\0');
	p = skip_blanks(reqline);
	if (*(q = skip_to_blank(p))) {
	   *q++='\0';
	   q = skip_blanks(q);
	}
	if (!*reqline) return 0;
	if (strlen(p) > 12)
	   p[12] = '\0';
	strcpy(reqname,p);
	if (!reqname[0]) return (0);

	*reqpwd = '\0';
	reqftime = 0L;

	if (*q == '!') {
	   p = q + 1;
	   if (*(q = skip_to_blank(p))) {
	      *q++ = '\0';
	      q = skip_blanks(q);
	   }
	   if (strlen(p) > 6)
	      p[6] = '\0';
	   strcpy(reqpwd,p);
	}

	if ((*q == '+' || *q == '-') && isdigit(*(q + 1))) {
	   res = (*q == '+');
	   q++;
	   reqftime = res ? atol(q) : -atol(q);
	   if (*(q = skip_to_blank(p))) {
	      *q++ = '\0';
	      q = skip_blanks(q);
	   }
	}

	options = q;
	res = proc_req();

	return (res);
}/*do_reqline()*/


void rm_requests()
{
	char req_tpl[128], buffer[128];
	char *p;
	SERVLST *l, *lnext;

	sprint(req_tpl,"%sXENREQ%02x.*",filepath,task);
	while ((p = ffirst(req_tpl)) != NULL) {
	      fnclose();
	      sprint(buffer,"%s%s",filepath,p);
	      unlink(buffer);
	}
	fnclose();

	sprint(req_tpl,"%sXENREQ%02x.*",safepath,task);
	while ((p = ffirst(req_tpl)) != NULL) {
	      fnclose();
	      sprint(buffer,"%s%s",safepath,p);
	      unlink(buffer);
	}
	fnclose();

	for (l = servlst; l; l = lnext) {
	    lnext = l->next;
	    if (l->pathname) free(l->pathname);
	    free(l);
	}
	servlst = NULL;
}/*rm_requests()*/


void process_bark_req(char *req)
{
	char *p, *q;

	wzsendfunc = send_SEA;

	if (reqprog) {
	   char buffer[256];

	   p=skip_blanks(req);
	   if (*(q=skip_to_blank(p))) {
	      *q++='\0';
	      q=skip_blanks(q);
	   }
	   if (!*p)
	      return;
	   strcpy(buffer,p);

	   if (*q) {
	      reqftime = atol(q);
	      q = skip_to_blank(q);
	      q = skip_blanks(q);
	      options = "";
	      if (*q) {
		 if ((p = skip_blanks(skip_to_blank(q))) != NULL)
		    options = p;
		 *skip_to_blank(q) = '\0';
		 sprint(&buffer[strlen(buffer)]," !%s", q);
	      }

	      if (reqftime)
		 sprint(&buffer[strlen(buffer)]," %c%ld",
			 reqftime > 0L ? '+' : '-', labs(reqftime));
	      if (*options)
		 sprint(&buffer[strlen(buffer)]," %s", options);
	   }

	   strcat(buffer,"\n");
	   do_srif(0, buffer, TECH_BARKREQ);
	}
	else {
	   p=skip_blanks(req);
	   if (*(q=skip_to_blank(p))) {
	      *q++='\0';
	      q=skip_blanks(q);
	   }
	   if (!*p)
	      return;
	   if (strlen(p)>12)
	      p[12] = '\0';
	   strcpy(reqname,p);

	   *reqpwd = '\0';
	   reqftime = 0L;
	   options = "";

	   if (*q) {
	      reqftime = atol(q);
	      q = skip_to_blank(q);
	      q = skip_blanks(q);
	      if (*q) {
		 if ((p = skip_blanks(skip_to_blank(q))) != NULL)
		    options = p;
		 *skip_to_blank(q) = '\0';
		 if (strlen(q)>6)
		    q[6] = '\0';
		 strcpy(reqpwd,q);
	      }
	   }

	   proc_req();
	   qf_send(0,TECH_BARKREQ);
	}
}/*process_bark_req()*/


static int proc_req()
{
	int dirpos;
	int exact=0, hit=0, hits=0, magic=0, service=0;
	int i;
	char reqprep[12];

	if (!carrier()) return (0);

	if (*reqname=='.') {	    /* Dutchie's trick - only check services */
	   service = 1;
	   strcpy(reqname,reqname+1);	 /* ugly tricky - remove leading dot */
	}
	strupr(reqname);
	strupr(reqpwd);
	prepfname(reqprep,reqname);
	reqname[11 + ((strchr(reqname,'.') != NULL) ? 1 : 0)] = '\0';

	if (!req_open()) {
	   rsp_code = RSP_NOREQUESTS;
	   rsp_noresp = 0;
	   hits = -1;
	   goto done;
	}

	rsp_pos = 0;
	rsp_fileinfo[rsp_pos] = '\0';

	if (caller && (ourbbsflags & NO_REQ_ON)) {
	   rsp_code = RSP_NOREQONUS;
	   rsp_noresp = 0;
	   hits = -1;
	   goto done;
	}

	rsp_code = RSP_NOTFOUND;
	rsp_noresp = 0;
	fseek(idxfp,(long) sizeof (struct _reqhdr),SEEK_SET);

	for (i=0; i < reqhdr.idxnum; i++) {
	    if (rsp_check(fread(&reqidx,sizeof (struct _reqidx),1,idxfp) != 1,RSP_ERRFILE)) {
	       hits = -1;
	       goto done;
	    }

	    if (!service && reqidx.flags&REQ_MAGIC &&
		exactmatch((char FAR *) reqidx.filespec,(char FAR *) reqname)) {
	       exact = hit = magic = 1;
	       if (rsp_check((e_ptrs[cur_event]->behavior & MAT_NOINREQ) && !(reqidx.flags&REQ_NOEVENT),RSP_NOREQEVENT) ||
		   rsp_check(reqlevel < reqidx.minlevel,RSP_LOLEVEL) ||
		   rsp_check(reqlevel > reqidx.maxlevel,RSP_HILEVEL) ||
		   rsp_check(*((char FAR *) reqidx.password) && !exactmatch((char FAR *) reqidx.password,(char FAR *) reqpwd),RSP_PASSWORD))
		  continue;
	       if (!magic && !(reqidx.flags&REQ_NOCOUNT)) {
		  if (rsp_check(!countlimit,RSP_COUNT))
		     continue;
		  countlimit--;
	       }
	       rsp_check(1,RSP_MAGIC);
	       fseek(dirfp,reqidx.offset,SEEK_SET);
	       for (dirpos=reqidx.number; dirpos>0; dirpos--) {
		   if (rsp_check(fread(&reqdir,sizeof (struct _reqdir),1,dirfp) != 1 ||
		       !qf_write((char FAR *) reqidx.params,(char FAR *) reqdir.fname),RSP_ERRMAGIC)) {
		      hits = -1;
		      goto done;
		   }
	       }
	       hits++;
	       continue;
	    }

	    if (magic) break;

	    if (reqidx.flags&REQ_SERVICE &&
		exactmatch((char FAR *) reqidx.filespec,(char FAR *) reqname)) {
	       exact = hit = 1;
	       if (rsp_check((e_ptrs[cur_event]->behavior & MAT_NOINREQ) && !(reqidx.flags&REQ_NOEVENT),RSP_NOREQEVENT) ||
		   rsp_check(reqlevel < reqidx.minlevel,RSP_LOLEVEL) ||
		   rsp_check(reqlevel > reqidx.maxlevel,RSP_HILEVEL) ||
		   rsp_check(*((char FAR *) reqidx.password) && !exactmatch((char FAR *) reqidx.password,(char FAR *) reqpwd),RSP_PASSWORD))
		  continue;
	       if (!(reqidx.flags&REQ_NOCOUNT)) {
		  if (rsp_check(!countlimit,RSP_COUNT))
		     continue;
		  countlimit--;
	       }
	       rsp_check(1,RSP_SERVICE);
	       process_service();
	       hits++;
	       break;
	    }

	    if (service) continue;

	    if (reqidx.flags & (REQ_MAGIC | REQ_SERVICE))
	       continue;

	    if (reqidx.flags&REQ_NOWILDS &&
		(strchr(reqprep,'?') || strchr(reqprep,'*')))
	       continue;

	    if (reqidx.flags&REQ_EXACT) {
	       if (!exactmatch((char FAR *) reqidx.filespec,(char FAR *) reqprep))
		  continue;
	       exact = 1;
	    }
	    else {
	       if (!wildmatch((char FAR *) reqidx.filespec,(char FAR *) reqprep))
		  continue;
	    }

	    fseek(dirfp,reqidx.offset,SEEK_SET);
	    for (dirpos=reqidx.number; dirpos>0; dirpos--) {
		if (rsp_check(fread(&reqdir,sizeof (struct _reqdir),1,dirfp) != 1,RSP_ERRFILE)) {
		   hits = -1;
		   goto done;
		}
		if (wildmatch((char FAR *) reqdir.fname,(char FAR *) reqprep)) {
		   hit++;
		   if (rsp_check((e_ptrs[cur_event]->behavior & MAT_NOINREQ) && !(reqidx.flags&REQ_NOEVENT),RSP_NOREQEVENT) ||
		       rsp_check(reqlevel < reqidx.minlevel,RSP_LOLEVEL) ||
		       rsp_check(reqlevel > reqidx.maxlevel,RSP_HILEVEL) ||
		       rsp_check(*((char FAR *) reqidx.password) && !exactmatch((char FAR *) reqidx.password,(char FAR *) reqpwd),RSP_PASSWORD))
		      continue;
		   if (rsp_check((reqftime<0 && reqdir.fstamp>=(-reqftime)),RSP_NOLOUPDATE) ||
		       rsp_check((reqftime>0 && reqdir.fstamp<=reqftime),RSP_NOHIUPDATE))
		       continue;
		   if (!(reqidx.flags&REQ_NOCOUNT) &&
		       rsp_check(!countlimit,RSP_COUNT))
		      continue;
		   if (!(reqidx.flags&REQ_NOSIZE) &&
		       rsp_check(((sizelimit-(reqdir.fsize/1024))<0),RSP_SIZE))
		      continue;
		   if (!(reqidx.flags&REQ_NOTIMER)) {
		      if (rsp_check(((int) timelimit - (int) ((((reqdir.fsize/128)*1340)/line_speed)/60) < 0),RSP_TIMER))
			 continue;
		      timelimit -= (word) ((((reqdir.fsize / 128) * 1340) / line_speed) / 60);
		   }
		   if (!(reqidx.flags&REQ_NOSIZE))
		      sizelimit -= (word) ((int) (reqdir.fsize / (long) 1024));
		   if (!(reqidx.flags&REQ_NOCOUNT))
		      countlimit--;
		   if (!rsp_check(reqftime<0,RSP_LOUPDATE) &&
		       !rsp_check(reqftime>0,RSP_HIUPDATE))
		      rsp_check(1,RSP_FILE);
		   hits++;
		   if (!qf_write((char FAR *) reqidx.params,(char FAR *) reqdir.fname)) {
		      if (rsp_code < RSP_ERRFILE)
			 rsp_code += 5;
		      hits = -1;
		      goto done;
		   }
		}
	    }

	    if ((reqidx.flags&REQ_BRKEXACT && exact) ||
		(reqidx.flags&REQ_BRKHIT && hit))
	       break;
	}

done:	message(2,rsp_logstr[rsp_code],reqname);
	if (rsp_text && !rsp_noresp)
	   qm_write();

	return (hits);
}/*process_req()*/
  
  
static void prepfname(char *dst, char *src)  /* PREPARE filename for wildmatch */
{ 
	int i, delim, fixed;
  
	for (delim = 8, i = 0; *src && i < 11; src++) {
	    if (*src=='.') {
	       if (i > 8)
		  i = 8;
	       else
		  while (i < 8)
			dst[i++] = ' ';
	       delim = 11;
	    }
	    else if (*src=='*') {
	       while (i < delim)
		     dst[i++] = '?';
	    }
	    else
	       dst[i++] = *src;
	}/*for*/
  
	while (i < 11)
	      dst[i++] = ' ';
	dst[i] = '\0';

	if (strchr(dst,'?')) {
	   for (i = fixed = 0; i < 8 && dst[i] != ' '; i++)
	       if (dst[i] != '?') fixed++;
	   if (fixed < 3)
	      strcpy(dst,"           ");
	}
}/*prepfname()*/
  
  
static int wildmatch(char FAR *s1, char FAR *s2)
			/* wildcard compare two PREPARED filespecs */
{
	while (*s1) {
	      if (*s1!='?' && *s2!='?' && *s1!=*s2)
		 return (0);
	      s1++;
	      s2++;
	}

	return (1);
}/*wildmatch()*/


static int exactmatch(char FAR *s1, char FAR *s2)
{
	while (*s1 && *s1==*s2) {
	      s1++;
	      s2++;
	}

	return (*s1 == *s2);
}


static int rsp_check(int rsp_comp,int rsp_type)
{
	if (rsp_comp) {
	   if (rsp_code < rsp_type) {
	      rsp_code = rsp_type;
	      rsp_noresp = reqidx.flags&REQ_NORESP;
	   }
	   return (1);
	}
	else
	   return (0);
}/*rsp_check()*/


static void process_service()
{
	char cmdline[256];
	int  i;

	i = 0;
	req_macro((char FAR *) cmdline,&i,reqidx.params);

	if (!strncmp(cmdline,"BBS",3) && (cmdline[3]=='\0' || cmdline[3]==' ')) {
	   if (!mail_only) FTB_allowed = 1;
	   return;
	}
	if (!strncmp(cmdline,"SYSOP-BBS",9) && (cmdline[9]=='\0' || cmdline[9]==' ')) {
	   FTB_allowed = 1;
	   return;
	}
	if (!strncmp(cmdline,"DOWN",4) && (cmdline[4]=='\0' || cmdline[4]==' ')) {
	   cur_event = 0;
	   return;
	}
	if (!strncmp(cmdline,"DOS",3) && (cmdline[3]=='\0' || cmdline[3]==' ')) {
	   FTB_dos = ctl_string(&cmdline[3]);
	   return;
	}

	if (log != NULL) fclose(log);
	if (qffp != NULL) fclose(qffp);

	if (mdm.shareport)
	   com_suspend = true;
	sys_reset();
	(void) win_savestate();
#if (PC || OS2 || WINNT)
	{ char cmdtemp[256];
	  char *p = getenv("COMSPEC");

#if PC | WINNT
	  sprint(cmdtemp,"%s /c %s",(p != NULL) ? p : "COMMAND.COM",cmdline);
#else
	  sprint(cmdtemp,"%s /c %s",(p != NULL) ? p : "CMD.EXE",cmdline);
#endif
	  strcpy(cmdline,cmdtemp);
	}
#endif
	execute(cmdline);     /* Want shell-ok check here for RSP_ERRSERVICE */
	(void) win_restorestate();
	sys_init();

	if (log != NULL) {
	   if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL) {
	      message(6,"!Can't re-open logfile!");
	      rsp_code = RSP_ERRSERVICE;
	   }
	}
	if (qffp != NULL) {
	   if ((qffp = sfopen(qfname, "r+b", DENY_ALL)) == NULL) {
	      message(6,"!Can't re-open request outbound list!");
	      rsp_code = RSP_ERRSERVICE;
	   }
	}
}/*process_service()*/


static int qf_write(char FAR *fdir, char FAR *fname)
{
	char addline[256], buffer[256];
	boolean found;
	char FAR *p;
	char *q;
	int i;

	if (qffp==NULL) {
	   if ((qffp = sfopen(qfname,"r+b",DENY_ALL)) == NULL &&
	       (qffp = sfopen(qfname,"w+b",DENY_ALL)) == NULL)
	      return (0);
	}

	q = addline;
	for (p=fdir; *p; *q++ = *p++);
	for (p=fname, i=8; *p!=' ' && i; *q++ = *p++, i--);
	if (fname[8] != ' ') {
	   *q++ = '.';

	   for (p=&fname[8]; *p && *p!=' '; *q++ = *p++);
	}
	*q = '\0';

	fseek(qffp,0L,SEEK_SET);
	found = false;
	while (fgets(buffer,255,qffp)) {
	      if ((q = strpbrk(buffer,"\r\n\032")) != NULL) *q = '\0';
	      if (!stricmp(buffer,addline)) {
		 found = true;
		 break;
	      }
	}

	if (!found) {
	   fseek(qffp,0L,SEEK_END);
	   fprint(qffp,"%s\r\n",addline);
	}

	counttotal++;
	sizetotal += reqdir.fsize;
	if (rsp_text && !rsp_error && reqhdr.rspofs_fileinfo != 0xffff)
	   req_macro(rsp_fileinfo,&rsp_pos,&rsp_text[reqhdr.rspofs_fileinfo]);
	if (reqlogfp) {
	   char buffer[50];
	   i = 0;
	   req_macro((char FAR *) buffer, &i, (char FAR *)
		     "  %FILENAME  %FILESIZE\n");
	   if (!fwrite(buffer,i,1,reqlogfp)) {
	      message(6,"!Error writing request logfile!");
	      fclose(reqlogfp);
	      reqlogfp = NULL;
	   }
	}

	return (1);
}/*qf_write()*/


char *qm_finish()
{
	if (rsp_text && qmfp && !rsp_error) {
	   int i = 0;

	   if (reqhdr.rspofs_footer != 0xffff)
	      req_macro(rsp_buffer,&i,&rsp_text[reqhdr.rspofs_footer]);
	   rsp_buffer[i++] = '\0';			   /* end of message */
	   rsp_buffer[i++] = 0; 			   /* end of packet  */
	   rsp_buffer[i++] = 0;
	   fseek(qmfp,0L,SEEK_END);
	   if (!myfarfwrite(rsp_buffer,i,1,qmfp)) {
	      fclose(qmfp);
	      qmfp = NULL;
	      if (fexist(qmname))
		 unlink(qmname);
	      rsp_error = 1;
	      return (NULL);
	   }
	   else {
	      fclose(qmfp);
	      qmfp = NULL;
	      rsp_error = 0;
	      return (qmname);
	   }
	}

	return (NULL);
}/*qm_finish()*/


char *qf_read(int firsttime)
{
	static char buffer[128];

	if (qffp == NULL)
	   return (NULL);

	if (firsttime)
	   fseek(qffp,0L,SEEK_SET);

	if (fgets(buffer,127,qffp))
	   return (buffer);

	fclose(qffp);
	qffp = NULL;
	unlink (qfname);

	return (NULL);
}/*qf_read()*/


int qf_send(int fsent,int tech)
{
	char  buffer[256];
	char *fname, *alias;
	long  current, last_start;
	int   res = XFER_OK;
	int   doafter;

	if (rsp_text && qmfp && !rsp_error && qm_finish()) {
	   invent_pktname(buffer);
	   res = (*wzsendfunc)(qmname,buffer,(int) DELETE_AFTER,fsent++,DO_WAZOO);
	   if (res == XFER_ABORT) goto err;
	}

	if (qffp!=NULL) {
	   fseek(qffp,0L,SEEK_SET);
	   current = 0L;
	   while (last_start = current, (carrier() && fgets(buffer,127,qffp))) {
		 current = ftell(qffp);

		 fname = strtok(buffer," \t\r\n\032");
		 if (!fname) continue;

		 if (*fname=='#') {
		    fname++;
		    doafter = TRUNC_AFTER;
		 }
		 else if (*fname=='^') {
		    fname++;
		    doafter = DELETE_AFTER;
		 }
		 else if (*fname == ';' || *fname == '~')
		    continue;
		 else
		    doafter = NOTHING_AFTER;

		 alias = strtok(NULL," \t\r\n\032");

		 if (*fname) {
		    if (tech != TECH_ZMODEM)
		       com_purge();
		    res = (*wzsendfunc)(fname,alias,(int) doafter,fsent++,DO_WAZOO);
		    if (res == XFER_ABORT) break;

		    fseek(qffp,last_start,SEEK_SET);
		    fprint(qffp,"~");
		    fflush(qffp);
		    rewind(qffp);
		    fseek(qffp,current,SEEK_SET);
		 }
	   }

	   fclose(qffp);
	   qffp = NULL;
	   if (carrier() && res != XFER_ABORT && qfname) unlink(qfname);
	}

err:	if (res != XFER_ABORT && tech == TECH_BARKREQ)
	   end_batch();

	return ((res != XFER_ABORT) ? fsent : -1);
}/*qf_send()*/


static void qm_write()
{
	FILE *fp;
	char buffer[128];
	int i;

	if (rsp_error) return;

	if (qmfp == NULL) {
	   struct _pkthdr  pkthdr;
	   struct _pktmsgs pktmsg;
	   struct dosdate_t d;
	   struct dostime_t t;
	   boolean appending = false;
	   int zone, net, node, point;

	   if (fexist(qmname)) appending = true;
	   qmfp = sfopen(qmname,appending ? "r+b" : "w+b",DENY_ALL);
	   if (qmfp == NULL) {
	      rsp_error = 1;
	      return;
	   }

	   memset(&pkthdr,0,sizeof (struct _pkthdr));
	   memset(&pktmsg,0,sizeof (struct _pktmsgs));
	   _dos_getdate(&d);
	   _dos_gettime(&t);

	   zone = rem_adr[0].zone;
	   net	= rem_adr[0].net;
	   if (remote.nodeflags & VIAHOST)
	      node = point = 0;
	   else {
	      node = rem_adr[0].node;
	      if (remote.nodeflags & VIABOSS)
		 point = 0;
	      else
		 point = rem_adr[0].point;
	   }

	   if (appending) {
	      word pktend;

	      fseek(qmfp,-2L,SEEK_END);
	      if (!fread(&pktend,sizeof (word),1,qmfp))
		 appending = false;
	      else if (pktend != 0)
		 fseek(qmfp,0L,SEEK_END);
	      else
		 fseek(qmfp,-2L,SEEK_END);
	   }

	   if (!appending) {
	      pkthdr.ph_yr	= (short) inteli(d.year);
	      pkthdr.ph_mo	= (short) (inteli(d.month) - 1);
	      pkthdr.ph_dy	= inteli(d.day);
	      pkthdr.ph_hr	= inteli(t.hour);
	      pkthdr.ph_mn	= inteli(t.minute);
	      pkthdr.ph_sc	= inteli(t.second);
	      pkthdr.ph_rate	= (short) inteli(cur_speed < 65535UL ? (word) cur_speed : 9600);
	      pkthdr.ph_ver	= inteli(2);
	      pkthdr.ph_prodh	= 0;
	      pkthdr.ph_prodl	= isXENIA;
	      pkthdr.ph_revh	= XEN_MAJ;
	      pkthdr.ph_revl	= XEN_MIN;
	      pkthdr.ph_cwd	= inteli(1);  /* CapWord: can handle type 2+ */
	      pkthdr.ph_cwdcopy = inteli(256);/* CapWordCopy = byteswapped   */
	      strncpy(pkthdr.ph_pwd,ourbbs.password,8);
	      pkthdr.ph_ozone1	= pkthdr.ph_ozone2 = (short) inteli(ourbbs.zone);
	      pkthdr.ph_dzone1	= pkthdr.ph_dzone2 = (short) inteli(zone);
	      pkthdr.ph_onet	= (short) inteli(ourbbs.net);
	      pkthdr.ph_auxnet	= inteli(0);
#if 0	      /* Enable this for FSC-0048 instead of FSC-0039 */
	      if (ourbbs.point) {
		 pkthdr.ph_onet   = inteli(-1);
		 pkthdr.ph_auxnet = inteli(ourbbs.net);
	      }
#endif
	      pkthdr.ph_dnet	= (short) inteli(net);
	      pkthdr.ph_onode	= (short) inteli(ourbbs.node);
	      pkthdr.ph_dnode	= (short) inteli(node);
	      pkthdr.ph_opoint	= (short) inteli(ourbbs.point);
	      pkthdr.ph_dpoint	= (short) inteli(point);
	      /* *(long *) pkthdr.ph_spec = intell(xenkey->serial); */

	      fseek(qmfp,0L,SEEK_SET);
	      if (!fwrite(&pkthdr,sizeof (struct _pkthdr),1,qmfp)) {
		 fclose(qmfp);
		 rsp_error = 1;
		 return;
	      }
	   }

	   pktmsg.pm_ver    = inteli(2);
	   pktmsg.pm_attr   = inteli(1);       /* message flagged private */
	   pktmsg.pm_cost   = 0;
	   pktmsg.pm_onet   = (short) inteli(ourbbs.net);
	   pktmsg.pm_dnet   = (short) inteli(net);
	   pktmsg.pm_onode  = (short) inteli(ourbbs.node);
	   pktmsg.pm_dnode  = (short) inteli(node);

	   if (!fwrite(&pktmsg,sizeof (struct _pktmsgs),1,qmfp)) {
	      fclose(qmfp);
	      rsp_error = 1;
	      return;
	   }

	   sprint(buffer,timestr_tpl,
		  d.day, mon[d.month - 1], d.year % 100,
		  t.hour, t.minute, t.second);
	   fwrite(buffer,20,1,qmfp);
	   strcpy(buffer,*remote.sysop ? remote.sysop : "Sysop");
	   fwrite(buffer,strlen(buffer)+1,1,qmfp);
	   sprint(buffer,"%s%s",PRGNAME,XENIA_SFX);
	   fwrite(buffer,strlen(buffer)+1,1,qmfp);
	   strcpy(buffer,"Request response");
	   fwrite(buffer,strlen(buffer)+1,1,qmfp);

	   if (*zonedomabbrev(ourbbs.zone) && *zonedomabbrev(zone) &&
	       strcmp(zonedomabbrev(ourbbs.zone),zonedomabbrev(zone))) {
	      fprint(qmfp,"\001DOMAIN %s %d:%d/%d",
			  zonedomabbrev(zone),
			  zone,net,node);
	      fprint(qmfp," %s %d:%d/%d\r\n",
			  zonedomabbrev(ourbbs.zone),
			  ourbbs.zone,ourbbs.net,ourbbs.node);
	   }
	   if (ourbbs.zone != rem_adr[0].zone)
	      fprint(qmfp,"\001INTL %d:%d/%d %d:%d/%d\r\n",
			  zone,net,node,
			  ourbbs.zone,ourbbs.net,ourbbs.node);
	   if (ourbbs.point)
	      fprint(qmfp,"\001FMPT %d\r\n",ourbbs.point);
	   if (rem_adr[0].point)
	      fprint(qmfp,"\001TOPT %d\r\n",point);

	   fprint(qmfp,"\001MSGID: %d:%d/%d%s%s",
		       ourbbs.zone,ourbbs.net,ourbbs.node,
		       pointnr(ourbbs.point),zonedomabbrev(ourbbs.zone));
	   fprint(qmfp," %08lx\r\n",time(NULL));

	   fprint(qmfp,"\001PID: Xenia%s %s",
		       XENIA_SFX, VERSION);
	   fprint(qmfp," OpenSource\r\n");

	   if (reqhdr.rspofs_header != 0xffff) {
	      i = 0;
	      req_macro(rsp_buffer,&i,&rsp_text[reqhdr.rspofs_header]);
	      fseek(qmfp,0L,SEEK_END);
	      if (!myfarfwrite(rsp_buffer,i,1,qmfp)) {
		 fclose(qmfp);
		 rsp_error = 1;
		 return;
	      }
	   }
	}

	if (reqhdr.rsp_codeinfo[rsp_code].rspofs_code != 0xffff &&
	    reqlevel >= reqhdr.rsp_codeinfo[rsp_code].minlevel &&
	    reqlevel <= reqhdr.rsp_codeinfo[rsp_code].maxlevel) {
	   i = 0;
	   req_macro(rsp_buffer,&i,&rsp_text[reqhdr.rsp_codeinfo[rsp_code].rspofs_code]);
	   fseek(qmfp,0L,SEEK_END);
	   if (!myfarfwrite(rsp_buffer,i,1,qmfp)) {
	      fclose(qmfp);
	      rsp_error = 1;
	      return;
	   }
	}

	sprint(buffer,"%sXENREQ%02x.OUT",reqpath,task);
	if (fexist(buffer)) {
	   if ((fp = sfopen(buffer,"rt",DENY_ALL)) != NULL) {
	      fseek(qmfp,0L,SEEK_END);
	      while ((i = getc(fp)) != EOF) {
		    if (i == 26) continue;
		    if (i == '\n') putc('\r',qmfp);
		    if (putc(i,qmfp) == EOF) {
		       fclose(fp);
		       unlink(buffer);
		       fclose(qmfp);
		       rsp_error = 1;
		       return;
		    }
	      }
	      fclose(fp);
	   }
	   unlink(buffer);
	}
}/*qm_write()*/


static void req_macro(char FAR *dst,int *ofs, char FAR *src)
{
	char s[256];
	char *p;
	struct dosdate_t d;
	struct dostime_t t;
	struct tm *tt;
	int macro;
	int i;

	while (*src) {
	      if (*ofs == MAXRSPTEXT) break;
	      if (*src == '%' && *++src != '%') {
		 for (macro = 0; macro_key[macro]; macro++) {
		     for (i = 0, p = macro_key[macro];
			  *p && src[i] == *p; p++, i++);
		     if (!*p) break;
		 }
		 if (!macro_key[macro])
		    continue;
		 src += i;

		 switch (macro) {
/* OURNAME	     */ case  0: strcpy(s,ourbbs.name); break;
/* OURSYSOP	     */ case  1: strcpy(s,ourbbs.sysop); break;
/* OURADDRESS	     */ case  2: sprint(s,"%d:%d/%d%s%s", ourbbs.zone, ourbbs.net,
					  ourbbs.node, pointnr(ourbbs.point),
					  zonedomabbrev(ourbbs.zone));
				 break;
/* TASK 	     */ case  3: sprint(s,"%u",task); break;
/* PORT 	     */ case  4: sprint(s,int_tpl,mdm.port); break;
/* DEVICE	     */ case  5: sprint(s,"%s",mdm.comdev); break;
/* HANDLE	     */ case  6: sprint(s,int_tpl,com_handle); break;
/* LOCKED	     */ case  7: sprint(s,"%lu",mdm.lock ? mdm.speed : cur_speed); break;
/* SPEED	     */ case  8: sprint(s,"%lu",cur_speed); break;
/* RELIABLE	     */ case  9: strcpy(s,errorfree[0] ? errorfree : "/NONE"); break;
/* CALLERID	     */ case 10: strcpy(s,callerid[0] ? callerid : "N/A"); break;
/* EVTTIME	     */ case 11: sprint(s,int_tpl,time_to_next()); break;
/* PROTOCOL	     */ case 12:
#ifndef NOHYDRA
				 if (wzsendfunc==send_hydra)  strcpy(s,"HYDRA");
				 else
#endif
				 if (wzsendfunc==send_Zmodem) strcpy(s,"ZMODEM");
				 else			      strcpy(s,"OTHER");
				 break;
/* OUTFILES	     */ case 13: strcpy(s,qfname); break;
/* OUTDATA	     */ case 14: sprint(s,"%sXENREQ%02x.OUT",reqpath,task);
				 if (fexist(s)) unlink(s);
				 break;
/* CURTIME	     */ case 15: _dos_getdate(&d); _dos_gettime(&t);
				 sprint(s,timestr_tpl,
					d.day,mon[d.month-1],d.year % 100,
					t.hour,t.minute,t.second);
				 break;
/* XENVERSION	     */ case 16: sprint(s,"%s %s", VERSION, XENIA_OS);
				 break;
/* THEIRNAME	     */ case 17: strcpy(s,*remote.name ? remote.name : "Unknown system name"); break;
/* THEIRSYSOP	     */ case 18: strcpy(s,*remote.sysop ? remote.sysop : "Sysop"); break;
/* THEIRADDRESS      */ case 19: sprint(s,"%d:%d/%d%s%s", rem_adr[0].zone, rem_adr[0].net,
					  rem_adr[0].node, pointnr(rem_adr[0].point),
					  zonedomabbrev(rem_adr[0].zone));
				 break;
/* REQLEVEL	     */ case 20: sprint(s,word_tpl,reqlevel); break;
/* COUNTLIMIT	     */ case 21: sprint(s,word_tpl,countorig); break;
/* SIZELIMIT	     */ case 22: sprint(s,word_tpl,sizeorig); break;
/* TIMELIMIT	     */ case 23: sprint(s,word_tpl,timeorig); break;
/* COUNTTOTAL	     */ case 24: sprint(s,word_tpl,counttotal); break;
/* SIZETOTAL	     */ case 25: sprint(s,"%ld",(long) (sizetotal / 1024L)); break;
/* TIMETOTAL	     */ case 26: sprint(s,word_tpl,(word) ((((sizetotal / 128) *
					  1340) / line_speed) / 60));
				 break;
/* REQNAME	     */ case 27: strcpy(s,reqname); break;
/* REQUPDATE	     */ case 28: sprint(s,"%ld",reqftime); break;
/* REQTIME	     */ case 29: tt = localtime((time_t *) &reqftime);
				 sprint(s,"%2d %3s %02d  %02d:%02d",
					tt->tm_mday, mon[tt->tm_mon],
					tt->tm_year%100, tt->tm_hour, tt->tm_min);
				 break;
/* REQPASSWORD	     */ case 30: strcpy(s,reqpwd); break;
/* REQOPTIONS	     */ case 31: strcpy(s,options); break;
/* FILENAME	     */ case 32: sprint(s,"%-8.8s %-3s",
					reqdir.fname,reqdir.fname+8);
				 break;
/* FILESIZE	     */ case 33: sprint(s,"%8ld",reqdir.fsize); break;
/* FILETIME	     */ case 34: tt = localtime((time_t *) &reqdir.fstamp);
				 sprint(s,"%2d %3s %02d  %02d:%02d",
					tt->tm_mday, mon[tt->tm_mon],
					tt->tm_year%100, tt->tm_hour, tt->tm_min);
				 break;
/* XFERTIME	     */ case 35: sprint(s,int_tpl,(int) ((((reqdir.fsize / 128) *
					  1340) / line_speed) / 60));
				 break;
/* FILEINFO	     */ case 36: if (!rsp_text ||
				     reqhdr.rspofs_fileinfo == 0xffff ||
				     dst == rsp_fileinfo) {	  /* nesting */
				    s[0] = '\0';
				    break;
				 }
				 for (i = 0;
				      rsp_fileinfo[i] && *ofs < MAXRSPTEXT;
				      dst[(*ofs)++] = rsp_fileinfo[i++]);
				 goto next;
/* TBVERSION	     */ case 37: sprint(s,"%s %s", VERSION, XENIA_OS);
				 break;
			default: s[0] = '\0'; break;
		 }
		 for (p = s; *p && ((*ofs) < MAXRSPTEXT); dst[(*ofs)++] = *p++);
	      }
	      else
		 dst[(*ofs)++] = *(src++);
next:;	}

	dst[*ofs] = '\0';
}/*req_macro()*/


/* end of request.c */
