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


#include <stdarg.h>
#include "xenia.h"
#include "mail.h"
#if OS2 || WINNT
extern boolean shutdown;
#endif


static long period_nextchange;
int ptemp = -1;


handle_inbound_mail(int forced)
{
   if (forced) goto get_now;

inloop:
   if (!com_scan())			/* Any action from modem? */
      return (0);			/* No, nothing to do	  */

   mail_only = 0;

get_now:
   if (got_caller(forced) < 256)
      goto inloop;

   /*if (carrier() || !strncmp(mdmconnect,"+FCO",4) || strstr(mdmconnect,"FAX"))*/

   {
      long btime;

      got_msession = 0;
      btime = time(NULL) - ((isdnlink || iplink) ? 1L : 10L);
      alloc_msg("pre recv session");
      recv_session();	   /* if we have DCD or FAX do a mail or BBS session */
      alloc_msg("post recv session");
      com_dump();
      hangup();
      rm_requests();
      statusmsg(NULL);
      if (got_msession) {
	 btime = time(NULL) - btime;
	 message(6,"+Incoming session for %ld:%02ld min.",
		   btime / 60L, btime % 60L);

	 mdm_aftercall();
      }
      if (got_arcmail || got_bundle || got_mail || got_fax)
	 receive_exit ();

      if (e_ptrs[cur_event]->behavior & MAT_EXITDYNAM)
	 errl_exit(0,false,"Processed incoming call (Exit Dynamic)");

      init_modem();
   }

   return (1);
}


void start_unatt()
{
   static boolean firsttime = true;

   fprint(log,"\n");
   if (task) message(6,"*%s%s : begin (task=%u)", progname, WATCOM ? " (Watcom)" : "", task);
   else      message(6,"*%s%s : begin", progname, WATCOM ? " (Watcom)" : "");

   if (firsttime) {
      period_read();
      firsttime = false;
   }

   period_nextchange = period_calculate();

   find_event();    /* See if we should exit before initializing the modem   */
		    /* (and therefore possibly letting a call sneak through) */
   un_attended = 1;

   if (!cur_event)
      event0();
}


extern long lasttime;
extern int polled_last;


void unattended()
{
	char buffer[PATHLEN];
	int  bzone, bnet, bnode, bpoint;
	int  i, m;
	word key;
	int  more_mail;
	long t;

	srand((int) time(NULL));	 /* Init the random number generator */

	if (noforce && un_attended) {		   /* Turn off forced events */
	   find_event();
	   noforce = 0;
	}

	if (redo_dynam) {
	   for (i = 0; i < num_events; i++)
	       e_ptrs[i]->behavior &= ~MAT_SKIP;
	   redo_dynam = 0;
	}

	cur_event = 0;
	if (un_attended)
	   start_unatt();
	else {
	   period_read();
	   period_nextchange = period_calculate();
	   event0();
	}

	rm_requests();			   /* get rid of pending WZ-requests */
	waitforline = time(NULL) + 60;

	if (sema_secs)
	   sema_timer = timerset(1);

	if (polled_last == 1 || (e_ptrs[cur_event]->behavior & MAT_EXITDYNAM))
	   t = timerset(1);
	else
	   t = 0;

mail_top:
	m = 1;

	more_mail = 1;			  /* As long as we don't press a key */

esc_only:
	while (!win_keyscan()) {
	      flag_shutdown();

	      if (period_nextchange && time(NULL) >= period_nextchange)
		 period_nextchange = period_calculate();
	      if (un_attended)
		 find_event();

	      if (m)				   /* Show that we are ready */
		message(-1,"+Event %d - Ready",cur_event);

	      /* If we haven't gotten anything in 10 minutes, re-init modem */
	      if (timeup(init_timer)) {
		 if (ourbbsflags & NO_INIT)
		    init_timer = timerset(6000);
		 else if (init_modem())
		    errl_exit(1,false,NULL);
	      }

#if 0
	      if (rescan && timeup(rescan)) {
		 sprint(buffer,"%sXMRESCAN.FLG",flagdir);
		 if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0) dos_close(fd);
		 rescan = 0L;
		 scan_outbound();
	      }
#endif

	      m = 0;

			      /* set up the amount of time to wait at random */
	      if (!t)
		 t = random_time(e_ptrs[cur_event]->wait_time);

	      while (!timeup(t) && !timeup(init_timer) &&
		     /*(!rescan || !timeup(rescan)) &&*/
		     !win_keyscan() && m == 0) {

		    if (period_nextchange && time(NULL) >= period_nextchange)
		       period_nextchange = period_calculate();
		    if (un_attended)
		       find_event();
		    else if (lasttime != time(NULL)) {
		       lasttime = time(NULL);
		       win_xyprint(top_win,win_maxhor - 24,1,"$aD $D $aM $Y $t");
#if OS2 /*&& WATCOM*/
		       mcp_sleep();
#endif
		    }

		    if (mdm.port != -1)
		       m = handle_inbound_mail(0);		/* any call? */

#if OS2 || WINNT
		    if ((sema_secs && timeup(sema_timer)) || shutdown)
#else
		    if (sema_secs && timeup(sema_timer))
#endif
		       flag_shutdown();

		    if (blank_time != 0L && blank_secs != 0 &&
			(int) ((time(NULL) - blank_time) > blank_secs)) {
		       (void) win_blank();
		       blank_time = 0L;
		    }

#if OS2
		    DosSleep(250);
#else
		    win_idle();
#endif
	      }

immed_call:   flag_shutdown();

	      if (win_keyscan())	/* If we have pressed a key, get out */
		 break;

	      t = 0;

	      if (period_nextchange && time(NULL) >= period_nextchange)
		 period_nextchange = period_calculate();
	      if (un_attended)
		 find_event();

	      if (polled_last != -1) {
		 mailout(-1);
		 show_outbound(No_Key);
		 if (got_arcmail || got_bundle || got_mail || got_fax)
		    receive_exit();
	      }

	      if ((e_ptrs[cur_event]->behavior & MAT_OUTONLY) &&
		  (e_ptrs[cur_event]->behavior & MAT_EXITDYNAM) && polled_last == -1 &&
		  (cur_event <= 0 || !more_mail || (e_ptrs[cur_event]->behavior & MAT_NOOUT))) {
		 errl_exit(0,false,"No more calls to make (Exit Dynamic)");
	      }

	      if (cur_event <= 0 || (e_ptrs[cur_event]->behavior & MAT_NOOUT))
		 continue;

	      sprint(buffer,"%sXMRESCAN.BSY",flagdir);
	      if (fexist(buffer))
		 continue;

	      if ((more_mail = xmit_next(&bzone,&bnet,&bnode,&bpoint)) != 0) {
		 flag_session(true);
		 if (flag_set(bzone,bnet,bnode,bpoint)) {
		    m = do_mail (bzone, bnet, bnode, bpoint);

		    if (m == 2) {		      /* bad with carrier    */
		       OUTBLIST *op = add_outentry(bzone,bnet,bnode,bpoint);
		       FILE *fp;
		       char buffer[PATHLEN];
		       char c;

		       if (op) {
			  if	  (op->types & OUTB_IMMEDIATE) c = 'I';
			  else if (op->types & OUTB_CRASH)     c = 'C';
			  else if (op->types & OUTB_POLL)      c = 'P';
			  else if (op->types & OUTB_DIRECT)    c = 'D';
			  else if (op->types & OUTB_NORMAL)    c = 'F';
			  else				       c = 0;

			  if (c) {
			     message(6,"*Regenerating call reason");
			     sprint(buffer, "%s%04.4x%04.4x%s.%cLO",
					    holdarea(bzone),
					    (word) bnet, (word) bnode,
					    holdpoint(bpoint), (short) c);
			     createhold(bzone,bnet,bnode,bpoint);
			     if ((fp = sfopen(buffer,"at",DENY_WRITE)) == NULL)
				message(6,"!Error opening %s",buffer);
			     else
				fclose(fp);
			  }
		       }

		       bad_call(bzone,bnet,bnode,bpoint, 1);
		    }
		    else if (m == 0)		      /* bad without carrier */
		       bad_call(bzone,bnet,bnode,bpoint, 2);
		    else if (m == 1) {		      /* okidoki, delete     */
		       bad_call(bzone,bnet,bnode,bpoint,-1);
		       for (i = 0; i < num_rem_adr; i++)
			   bad_call(rem_adr[i].zone,rem_adr[i].net,rem_adr[i].node,rem_adr[i].point,-1);
		    }
		    show_outbound(No_Key);
		 }
		 flag_session(false);

		 if (ptemp >= 0) {
		    pickup = ptemp;
		    ptemp = -1;
		 }

		 /* If we did some processing */
		 if (m) {
		    if (got_arcmail || got_bundle || got_mail || got_fax)
		       receive_exit();
		 }
	      }

	      if (!more_mail) {       /* No more mail to do, was it dynamic? */
		 if (e_ptrs[cur_event]->behavior & MAT_DYNAM) {
		    e_ptrs[cur_event]->behavior |= MAT_SKIP;
		    message(3,"*End of Dynamic Event %d", cur_event);
		    m = 1;
		 }
	      }
	}/*while(!win_keyscan())*/

	if (!win_keyscan()) {
	   /* Be serious, there had to be a key pressed or we wouldn't be here */
	   /* I know it sounds silly, but ^C will sometimes do crap like this  */
	}
	else {
	   (void) win_unblank();
	   key = win_keyscan();
	   if (key < 256 && (key == 'c' || key == 'C')) {
	      win_keygetc();
	      win_keypush(Alt_C);
	   }
	   else if (key == Alt_C || key == Ctrl_C) {
	      sprint(buffer,"%sXMRESCAN.FLG",flagdir);
	      if (fexist(buffer)) unlink(buffer);
#if 0
	      if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0) dos_close(fd);
	      /*rescan = 0L;*/
#endif
	      scan_outbound();
	   }
#if 0
	   else if (key == Alt_D) {
	      win_keygetc();
	      setspeed(mdm.speed < 19200UL ? 9600UL : 19200UL,true);
	      if (mdm_command("AT+FCLASS=2|``ATX1D|",
			      MDMRSP_CONNECT|MDMRSP_NOCONN|MDMRSP_BADCALL|MDMRSP_NODIAL|MDMRSP_RING,
			      750, 600) > 256) {
		 caller = 1;
		 fax_transmit();
		 init_modem();
	      }

	      blank_time = time(NULL);
	      goto esc_only;
	   }
#endif
	   else if (key == Home || key == End || key == Up ||
		    key == Down || key == PgUp || key == PgDn) {
	      win_keygetc();
	      show_outbound(key);
	      blank_time = time(NULL);
	      goto esc_only;
	   }
#if JAC
	   else if (key == 'P') {
	      win_keygetc();
	      phone_out(0);
	      message(-1,"+Phone enabled");
	      blank_time = time(NULL);
	      goto esc_only;
	   }
	   else if (key == 'O') {
	      win_keygetc();
	      phone_out(1);
	      message(-1,"+Phone disabled");
	      blank_time = time(NULL);
	      goto esc_only;
	   }
#endif			   

	   key = call_mailermenu();
	   blank_time = time(NULL);
	   switch (key) {
		  case Alt_O:
		       if (polled_last != -1) {
			  polled_last = -1;
			  win_xyputs(schedule_win,8,5,"0  ");
		       }
		       if (e_ptrs[cur_event]->behavior & MAT_EXITDYNAM) {
			  m = 0;
			  more_mail = 1;
			  goto immed_call;
		       }
		       break;

		  case Alt_P:
		       if (mdm.port == -1) break;
		       mailout(1);
		       statusmsg(NULL);
		       m = 1;
		       if (got_arcmail || got_bundle || got_mail || got_fax)
			  receive_exit();
		       break;

		  case Alt_R:
		       if (!un_attended) break;
		       message(3,"*Operator restarted events.");
		       for (i = 1; i < num_events; i++) {
			   /* Don't redo forced events */
			   if (!(e_ptrs[i]->behavior & MAT_FORCED))
			      e_ptrs[i]->behavior &= ~MAT_SKIP;
		       }
		       goto mail_top;

		  case Alt_Q:
		       if (cur_event > 0) {
			  message(3,"*Operator terminated event.");
			  e_ptrs[cur_event]->behavior |= MAT_SKIP;
		       }
		       goto mail_top;

		  case Alt_C:
		  case Ctrl_C:
		       if (mdm.port == -1) break;
		       m = 0;
		       more_mail = 1;
		       if ((cur_event > 0 && !(e_ptrs[cur_event]->behavior & MAT_NOOUT)) ||
			   polled_last != -1 && key != Ctrl_C)
			  message(0,"+Immediate call requested");
		       goto immed_call;

		  case Alt_A:
		       if (mdm.port == -1) break;
		       message(3,"+Forced answering phone");
		       statusmsg("Forced answering phone");
		       handle_inbound_mail(1);
		       statusmsg(NULL);
		       goto immed_call;

#if 0
		  case Alt_Z:
		       show_help(1);
		       break;
#endif

		  case Alt_I:
		       if (mdm.port == -1) break;
		       mailout(0);
		       if (got_arcmail || got_bundle || got_mail || got_fax)
			  receive_exit();
		       break;

		  case Alt_W: 
		       if (un_attended) break;
		       e_ptrs[0]->behavior ^= MAT_OUTONLY;
		       if (!(e_ptrs[0]->behavior & MAT_OUTONLY))
			  message(3,"*Accepting calls");
		       else
			  message(3,"*Ignoring calls");
		       event0();
		       break;

		  case Alt_G:					/* Get files */
		       makelist(0);
		       break;

		  case Alt_S:				       /* Send files */
		       makelist(1);
		       break;

		  case Alt_M:  /* don't use, prevent trouble with term Alt-M */
		       break;

		  case Alt_U:
		       if (!un_attended)
			  start_unatt();
		       else {
			  period_nextchange = 0L;
			  cur_event = 0;
			  un_attended = 0;
			  event0();
			  message(6,"*%s : end", progname);
		       }
		       goto mail_top;

		  case Alt_B:
		       (void) win_blank();
		       blank_time = 0L;
		       break;

		  default:	 /* invalid key or just returned from menu system */
		       break;
	   }
	}

	goto esc_only;
}


void receive_exit()
{
   if (got_fax && cur_event > 0 && e_ptrs[cur_event]->errlevel[3])
      errl_exit(e_ptrs[cur_event]->errlevel[3],false,"Received FAX");

   if (got_arcmail && cur_event > 0 && e_ptrs[cur_event]->errlevel[2])
      errl_exit(e_ptrs[cur_event]->errlevel[2],false,"Received ARCmail");

   if ((got_mail || got_bundle) && cur_event > 0 && e_ptrs[cur_event]->errlevel[1]) {
      errl_exit(e_ptrs[cur_event]->errlevel[1],false,"Received mail");
   }

   if (cur_event > 0 &&
       (got_arcmail || got_bundle || got_mail || got_fax) &&
       !e_ptrs[cur_event]->errlevel[1] &&
       !e_ptrs[cur_event]->errlevel[2] &&
       !e_ptrs[cur_event]->errlevel[3]) {
      int fd;
      char buffer[128];

      if (mailprocess) {
	 message(3,"*Calling mailprocess");
	 mdm_busy();
	 if (log != NULL) fclose (log);
	 sys_reset();
	 (void) win_savestate();
	 sprint(buffer,"%s %u %s",
		       mailprocess, task,
		       got_fax ? "FAX" : (got_arcmail ? "ARCMAIL" : "MAIL"));
	 execute(buffer);
	 (void) win_restorestate();
	 sys_init();
	 if (log != NULL) {
	    if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
	       message(6,"!Can't re-open logfile!");
	 }
	 init_modem();
	 waitforline = time(NULL) + 60;
	 message(3,"*Returned from mailprocess");
      }
      else {
	 sprint(buffer,"%sXMMAILIN.%u",flagdir,task);
	 if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT,DENY_NONE)) >= 0) {
	    dos_close(fd);
	    message(3,"@Set flag for mail processing task");
	 }
      }
   }

   got_arcmail = got_bundle = got_mail = got_fax = 0;
}


void errl_exit (int n, boolean online, char *fmt, ...)
{
   char  buf[128];
   char  fname[PATHLEN];
   FILE *fp;
   int	 count;
   va_list arg_ptr;

#if (PC || ATARIST)
   n %= 256;
#endif

   if (fmt) {
      va_start(arg_ptr,fmt);
      fmprint(buf,fmt,arg_ptr);
      va_end(arg_ptr);
      message(6,"*%s",buf);
   }

   message(6,"*%s : end (%d)", progname,n);

   if (online) {
      if (mdm.shareport)
	 com_suspend = true;
   }
   else {
      if (n == 0)
	 reset_modem();
      else if (n != 1)
	 mdm_busy();
   }

   statusmsg("External application");
   sys_reset();
   win_deinit();
   if (log) fclose(log);		/* close logfile(s) */
   if (cost) fclose(cost);
   period_write();
   write_sched();
#if JAC
   phone_out(0);
#endif

   sprint(fname,"%sXENIA%u.NVR",homepath,task);
   if ((fp = sfopen(fname,"wt",DENY_WRITE)) != NULL) {	       /* NVRAM file */
      struct stat f;

      if (polled_last != -1) {
	 for (count = 0; count < 20; count++) {
	     if (dial[count].zone != -2 && dial[count].zone != -3)
		fprint(fp,"POLL %d:%d/%d%s\n",
			  dial[count].zone, dial[count].net, dial[count].node,
			  pointnr(dial[count].point));
	 }
      }
      fprint(fp,"STATS_CALLS %u %u %u %u\n",
		extra.calls, extra.human, extra.mailer, extra.called);
      fprint(fp,"STATS_MOVED %u %u %u %u\n",
		extra.nrmail, extra.kbmail, extra.nrfiles, extra.kbfiles);
      if (last_in ) fprint(fp,"LAST_IN  %s\n", last_in);
      if (last_bbs) fprint(fp,"LAST_BBS %s\n", last_bbs);
      if (last_out) fprint(fp,"LAST_OUT %s\n", last_out);
      fclose(fp);

      if (!getstat(fname,&f) && !f.st_size)
	 unlink(fname);
   }

   if (n) {
      fprint(stderr,"Exit - errorlevel %d",n);
      if (fmt) fprint(stderr," (%s)",buf);
      fprint(stderr,"\n");
   }
   else
      fprint(stderr,"Thank you for using %s. See you again soon!\n",PRGNAME);

   exit (n);				/* and finaly get out.. */
}


long random_time (int x)
{
   int i;

   if (x == 0)
      x = 20;
   else if (x > 60 && (e_ptrs[cur_event]->behavior & MAT_OUTONLY))
      x /= 3;

   /* Number of seconds to delay is random based on x +/- 50% */
   i = (rand () % (x + 1)) + (x / 2);
   if (i < 0) i = x < 120 ? x : 120;
   if (i < 5) i = 5;

   win_setpos(schedule_win,8,4);
   win_clreol(schedule_win);
   win_print(schedule_win,"%d secs (%d)",(int) e_ptrs[cur_event]->wait_time,i);

   return (timerset((word) (i * 10)));
}


bad_call (int bzone, int bnet, int bnode, int bpoint, int rwd)
{
   char *c;
   word con, noc;
   char fname[PATHLEN];
   struct stat f;
   int fd = -1;
   OUTBLIST *op = add_outentry(bzone,bnet,bnode,bpoint);

   sprint(fname, "%s%04.4x%04.4x%s.$$$$?",
		  holdarea(bzone), (word) bnet, (word) bnode, holdpoint(bpoint));

   con = noc = 0;

   c = ffirst(fname);
   fnclose();
   if (c && isdigit(c[11]))
      con = (word) (c[11] - '0');

   sprint(fname, "%s%04.4x%04.4x%s.$$$$%01u",
		 holdarea(bzone), (word) bnet, (word) bnode, holdpoint(bpoint),
		 con);

   if (c && (fd = dos_sopen(fname,O_RDONLY,DENY_NONE)) >= 0) {
      dos_read(fd,&noc,sizeof (word));
      dos_close(fd);
   }

   if (rwd > 0) {	      /* We are writing out an unsuccessful attempt  */
      if (c) unlink(fname);

      if (rwd == 3) {	      /* We are clearing either or both counters     */
	 con = op->conn;
	 noc = op->noco;
      }
      if (rwd == 2) {	      /* We had an unsuccesful no carrier attempt    */
	 noc++;
      }
      else if (max_connects) {		/* Unsuccessful attempt with carrier */
	 if (++con > 9) con = 9;
	 sprint(fname, "%s%04.4x%04.4x.$$$$%01u",
		       holdarea(bzone), (word) bnet, (word) bnode,
		       con);
      }

      if (op) {
	 op->conn = con;
	 op->noco = noc;
	 op->lastattempt = time(NULL);
      }

      createhold(bzone,bnet,bnode,bpoint);
      if ((fd = dos_sopen(fname,O_WRONLY|O_CREAT,DENY_WRITE)) < 0)
	 message(6,"!Can't create call count file %s",fname);
      else {
	 dos_write(fd,&noc,sizeof (word));
	 dos_close(fd);
      }
   }
   else if (rwd == 0) {       /* We are reading a bad call status	     */
      if (op) {
	 if (!c && (op->conn || op->noco || op->lastattempt)) {
	    op->status = OUTB_WHOKNOWS;
	    return (1);
	 }
	 else if (!getstat(fname,&f))
	    op->lastattempt = f.st_mtime;
	 op->conn = (word) con;
	 op->noco = (word) noc;
      }

			      /* We've had too many connects with carrier?   */
      if ((max_connects > 0 && con >= max_connects) ||
	  (max_noconnects > 0 && noc >= max_noconnects))
	 return (1);
   }
   else {		      /* We have to delete call status		     */
      if (c) unlink (fname);

      if (op) {
	 op->conn = 0;
	 op->noco = 0;
	 op->lastattempt = 0L;
	 op->types &= ~(OUTB_NORMAL | OUTB_CRASH | OUTB_IMMEDIATE | OUTB_DIRECT | OUTB_POLL);
	 op->status = OUTB_DONE;
      }
   }

   return (0);
}

char *holdarea (int uzone)
{
	static char buf[128];
	DOMZONE *zp;

	buf[0] = '\0';

	for (zp = domzone; zp; zp = zp->next) {
	    if (uzone == zp->zone) {
	       strcpy(buf,zp->domptr->hold);
	       break;
	    }
	}

	if (!buf[0]) {
	   char    adr[20];
	   DOMPAT *dpp;

	   sprint(adr,"%d:",uzone);
	   for (dpp = dompat; dpp; dpp = dpp->next) {
	       if (patimat(adr,dpp->pattern)) {
		  strcpy(buf,dpp->domptr->hold);
		  break;
	       }
	   }
	}

	if (!buf[0])
	   strcpy(buf,hold);

	if (buf[0]) {
	   if (uzone != aka[0].zone)	/* Not zone of primary address	     */
	      sprint(&buf[strlen(buf)],".%03x", (word) uzone); /* Add zone suffix   */
	   strcat(buf,"\\");
	}

	return (buf);
}

char *holdpoint (int point)
{
	static char buf[14];

	if (point) sprint(buf,".PNT\\%08.8x",(word) point);
	else	   buf[0] = '\0';

	return (buf);
}


boolean createhold (int zone, int net, int node, int point)
{
	char buffer[PATHLEN];
	DOMZONE *zp;

	for (zp = domzone; zp; zp = zp->next) {
	    if (zone == zp->zone) {
	       strcpy(buffer,zp->domptr->hold);
	       break;
	    }
	}
	if (!zp) strcpy(buffer,hold);	/* Not found in zone->domain list    */
	if (buffer[0]) {
	   if (zone != aka[0].zone)	/* Not zone of primary address	     */
	      sprint(&buffer[strlen(buffer)],".%03x", (word) zone); /* Add zone suffix	 */

	   if (!dfirst(buffer) && mkdir(buffer))
	      return (false);
	   fnclose();

	   strcat(buffer,"\\");
	}

	if (point) {
	   sprint(&buffer[strlen(buffer)], "%04.4x%04.4x.PNT",
					   (word) net, (word) node);

	   if (!dfirst(buffer) && mkdir(buffer))
	      return (false);
	   fnclose();
	}

	return (true);
}


char *pointnr (int point)
{
	static char buf[8];

	if (point) sprint(buf,".%d",point);
	else	   buf[0] = '\0';

	return (buf);
}

char *zonedomabbrev (int zone)
{
	static char buf[10];  /* @ + 8chars + NUL = 10 exactly */
	DOMZONE *zp;

	buf[0] = '\0';

	for (zp = domzone; zp; zp = zp->next) {
	    if (zone == zp->zone) {
	       sprint(buf,"@%s",zp->domptr->abbrev);
	       strlwr(buf);
	       break;
	    }
	}

	if (!buf[0]) {
	   char    adr[20];
	   DOMPAT *dpp;

	   sprint(adr,"%d:",zone);
	   for (dpp = dompat; dpp; dpp = dpp->next) {
	       if (patimat(adr,dpp->pattern)) {
		  sprint(buf,"@%s",dpp->domptr->abbrev);
		  strlwr(buf);
		  break;
	       }
	   }
	}

	return (buf);
}


char *zonedomname (int zone)
{
	static char buf[40];
	DOMZONE *zp;

	buf[0] = '\0';

	for (zp = domzone; zp; zp = zp->next) {
	    if (zone == zp->zone) {
	       sprint(buf,"@%s",zp->domptr->domname);
	       strlwr(buf);
	       break;
	    }
	}

	if (!buf[0]) {
	   char    adr[20];
	   DOMPAT *dpp;

	   sprint(adr,"%d:",zone);
	   for (dpp = dompat; dpp; dpp = dpp->next) {
	       if (patimat(adr,dpp->pattern)) {
		  sprint(buf,"@%s",dpp->domptr->domname);
		  strlwr(buf);
		  break;
	       }
	   }
	}

	return (buf);
}


void flag_shutdown (void)
{
	static boolean newmail = false;
	char	buffer[PATHLEN];
	int	erl = 0;
	struct	stat f;
	boolean flag;

	if (sema_secs)
	   sema_timer = timerset((word) sema_secs);

	sprint(buffer,"%sXMEXIT.%u",flagdir,task);
	if (fexist(buffer)) {
	   FILE *fp;
	   char line[10];

	   if ((fp = fopen(buffer,"rt")) == NULL)
	      goto noshut;
	   fgets(line,9,fp);
	   fclose(fp);
	   unlink(buffer);
	   erl = atoi(line);
#if OS2 || WINNT
	   shutdown = true;
	}
noshut: if (shutdown) {
#endif
	   errl_exit(erl,false,"Shutdown requested");
	}
#if !(OS2 || WINNT)
noshut: ;
#endif


#if 0
	if (rescan) {
	   sprint(buffer,"%sXMRESCAN.FLG",flagdir);
	   if (!getstat(buffer,&f) && scan_stamp != f.st_mtime) {
	      rescan = 0L;
	      scan_stamp = f.st_mtime;
	   }
	}
	if (!rescan)
	   scan_outbound();
#endif
	sprint(buffer,"%sXMRESCAN.FLG",flagdir);
	if (!getstat(buffer,&f) && f.st_mtime < (time(NULL) - 1800L)) {
	   unlink(buffer);
	   scan_stamp = 0L;
	}
	if (!fexist(buffer)) {		/* sema doesn't exist? */
	   scan_stamp = 0L;
	   scan_outbound();
	}
	else if (scan_stamp != f.st_mtime)
	   scan_outbound();

	sprint(buffer,"%sNEWMAIL.FLG",flagdir);
	flag = fexist(buffer) ? true : false;
	if (newmail != flag) {
	   newmail = flag;
	   win_setpos(status_win,1,2);
	   if (newmail) {
	      win_setattrib(status_win,xenwin_colours.status.hotnormal);
	      win_print(status_win,"NOTE: NEW MAIL ARRIVED");
	      win_setattrib(status_win,xenwin_colours.status.normal);
	      (void) win_unblank();
	      blank_time = time(NULL);
	   }
	   win_clreol(status_win);
	}
}/*flag_shutdown()*/


/* end of mailer.c */
