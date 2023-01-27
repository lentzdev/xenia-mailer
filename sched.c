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


/* Scheduler routines */
#include "xenia.h"
#include "mail.h"
long lasttime;			/* last time time has been printed */


static long event_stamp = 0L;
static long sched_stamp = 0L;


char *start_time (EVENT *e, char *p)
{
   int i, j;

   if (sscanf (p, "%d:%d", &i, &j) != 2)
      {
      return NULL;
      }

   e->minute = (short) (i * 60 + j);
   if ((e->minute < 0) || (e->minute > (24*60)))
      {
      return (NULL);
      }

   p = skip_to_blank (p);

   return (p);
}

char *end_time (EVENT *e, char *p)
{
   int i, j, k;

   if (sscanf (p, "%d:%d", &i, &j) != 2)
      {
      return NULL;
      }

   k = i*60+j;
   if ((k > (24*60)) || (k < 0))
      {
      return (NULL);
      }

   if (k < e->minute)
      {
      fprint(stderr,"Event ending time wraps through midnight - not allowed\n");
      return (NULL);
      }

   e->length = (short) (k - e->minute);
   if (!e->length) {
      fprint(stderr,"Zero-length event - not allowed\n");
      return (NULL);
   }

   p = skip_to_blank (p);

   return (p);
}

parse_event (char *e_line)
{
   int i, j1, j2, j3;
   char *p;
   EVENT *e;

   /* If we already have a schedule, then forget it */
   if (got_sched)
      return (0);

   /* Skip blanks to get to tags */
   p = skip_blanks(e_line);

   /* reserve the space needed and initialise to 0 */
   e = (EVENT *) calloc (sizeof (EVENT), 1);
   e->days	= 0;
   e->wait_time = 120;
   e->max_conn	= 3;
   e->max_noco	= 35;

   /* Save tag. */
   if (toupper(*p)=='#')
   {
	/* This signals bahaviour change only, tag follows */
	e->behavior |= MAT_CBEHAV;
	++p;
   }
   if ( isalpha(*p) )
   {
      e->tag = (short) toupper(*p);
   }
   else
   {
      fprint(stderr,"'%s' has invalid tag\n",e_line);
      free(e);
      return (1);
   }
   
   /* If it is an exit with errorlevel then save errorlevel. */
   if (e->tag=='X')
   {
      ++p;
      if (isdigit(*p))
      {
	 sscanf(p,"%hd", &e->errlevel[0]);
	 if (e->errlevel[0]<4)
	 {
	    fprint(stderr,"'%s' has a bad Errorlevel code (less than 4)\n",e_line);
	    free(e);
	    return(1);
	 }
      }
      else
      {
	 fprint(stderr,"'%s' has a bad Errorlevel code\n",e_line);
	 free(e);
	 return(1);
      }
   }

   /* skip tag */
   p = skip_to_blank(p);

   /* Skip blanks to get to the days field */
   p = skip_blanks(p);

   /* Parse the days field */
   while ((*p) && (!isspace(*p)))
      {
      switch (toupper(*p))
	 {
	 case 'S':   /* Sunday or Saturday */
	    if (!strnicmp (p, "sun", 3))
	       {
	       e->days |= DAY_SUNDAY;
	       }
	    else if (!strnicmp (p, "sat", 3))
	       {
	       e->days |= DAY_SATURDAY;
	       }
	    else  /* Error condition */
	       {
	       goto err;
	       }
	    p += 3;
	    break;

	 case 'M':   /* Monday */
	    if (!strnicmp (p, "mon", 3))
	       {
	       e->days |= DAY_MONDAY;
	       }
	    else  /* Error condition */
	       {
	       goto err;
	       }
	    p += 3;
	    break;

	 case 'T':   /* Tuesday or Thursday */
	    if (!strnicmp (p, "tue", 3))
	       {
	       e->days |= DAY_TUESDAY;
	       }
	    else if (!strnicmp (p, "thu", 3))
	       {
	       e->days |= DAY_THURSDAY;
	       }
	    else  /* Error condition */
	       {
	       goto err;
	       }
	    p += 3;
	    break;

	 case 'W':   /* Wednesday, Week or Weekend */
	    if (!strnicmp (p, "wed", 3))
	       {
	       e->days |= DAY_WEDNESDAY;
	       p += 3;
	       }
	    else if (!strnicmp (p, "week", 4))
	       {
	       e->days |= DAY_WEEK;
	       p += 4;
	       }
	    else if (!strnicmp (p, "wkend", 5))
	       {
	       e->days |= DAY_WKEND;
	       p += 5;
	       }
	    else  /* Error condition */
	       {
	       goto err;
	       }
	    break;

	 case 'F':   /* Friday */
	    if (!strnicmp (p, "fri", 3))
	       {
	       e->days |= DAY_FRIDAY;
	       }
	    else  /* Error condition */
	       {
	       goto err;
	       }
	    p += 3;
	    break;

	 case 'A':   /* All */
	    if (!strnicmp (p, "all", 3))
	       {
	       e->days |= (DAY_WEEK|DAY_WKEND);
	       }
	    else  /* Error condition */
	       {
	       goto err;
	       }
	    p += 3;
	    break;

	 default:    /* Error condition */
	    goto err;
	 }

      if (*p == '+')
	 ++p;
      }

   /* Did we get something valid? */
   if (e->days == 0)
      {
      goto err;
      }

   /* Skip blanks to get to the start-time field */
   p = skip_blanks (p);

   /* Parse the start-time field */
   if ((p = start_time (e, p)) == NULL)
      {
	 fprint(stderr,"'%s' has an invalid START-TIME\n", e_line);
	 free (e);
	 return (1);
      }

   /* Give each event a default of 60 minutes */
   e->length = 60;

   /* While there are still things on the line */
   while (*p)
      {
      /* Skip blanks to get to the next field */
      p = skip_blanks (p);

      /* switch to find what thing is being parsed */
      switch (tolower(*p))
	 {
	 case '\0':  /* No more stuff */
	    break;
	 
	 case '0':   /* Digits must be an ending time */
	 case '1':
	 case '2':
	 case '3':
	 case '4':
	 case '5':
	 case '6':
	 case '7':
	 case '8':
	 case '9':
	    /* Parse ending time */
	    if ((p = end_time (e, p)) == NULL)
	       {
	       fprint(stderr,"'%s' has an invalid END-TIME\n", e_line);
	       free (e);
	       return (1);
	       }
	    break;

	 case ';':   /* Comment */
	    *p = '\0';
	    break;

	 case 'a':   /* Average wait */
	    ++p;
	    if (*p == '=')
	       {
	       ++p;
	       if (isdigit (*p))
		  {
		  i = atoi (p);
		  if ((i > 10000) || (i < 0))
		     {
		     fprint(stderr,"'%s' has bad AvgWait (higher than max of 10000 or less than min of 0)\n", e_line);
		     free (e);

		     return (1);
		     }
		  e->wait_time = (short) i;
		  p = skip_to_blank (p);
		  break;
		  }
	       }
	    fprint(stderr,"'%s' has a bad AvgWait code\n", e_line);
	    free (e);
	    return (1);
#if 0
	 case 'b':   /* BBS type event */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_BBS;
	    break;
#endif
	 case 'c':   /* #CM event */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_CM;
	    break;

	 case '#':   /* #mailhour event */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_MAILHOUR;
	    break;

	 case 'h':   /* high priority for crashmail */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_HIPRICM;
	    break;

	 case 'd':   /* Dynamic event */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_DYNAM;
	    break;

	 case 'e':   /* Exit Dynamic event */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_EXITDYNAM;
	    break;

/*	   case 'e':   /+ An errorlevel exit +/
	    ++p;
	    if (isdigit(*p))
	       {
	       i = *p - '0';
	       ++p;
	       if ((*p == '=') && (i <= 5) && (i > 0))
		  {
		  ++p;
		  if (isdigit (*p))
		     {
		     j = atoi (p);
		     e->errlevel[i-1] = j;
		     p = skip_to_blank (p);
		     break;
		     }
		  }
	       }
	    fprint(stderr,"'%s' has a bad Errorlevel code\n", e_line);
	    free (e);
	    return (1);
*/
	 case 'f':   /* Forced event */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_FORCED;
	    break;

	 case 'l':   /* Local only mail */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_LOCAL;
	    break;

	 case 'q':   /* QueueSize in KBs */
	    ++p;
	    j1 = 0;
	    i = sscanf (p, "=%d", &j1);
	    if (i < 1 || j1 < 0 || j1 > 32000) {
	       fprint(stderr,"'%s' has bad QueueSize code\n", e_line);
	       free(e);
	       return (1);
	    }
	    e->queue_size = (short) j1;
	    p = skip_to_blank (p);
	    break;

	 case 'm':   /* MailExit codes (mail[,arcmail[,fax]]) */
	    ++p;
	    j1 = j2 = j3 = 0;
	    i = sscanf (p, "=%d,%d,%d", &j1, &j2, &j3);
	    if (i < 1 || (j1 > 0 && j1 < 8) ||
		(i >= 2 && (j2 > 0 && j2 < 8)) ||
		(i >= 3 && (j3 > 0 && j3 < 8))) {
	       fprint(stderr,"'%s' has bad MailExit code(s)\n", e_line);
	       free(e);
	       return (1);
	    }
	    e->errlevel[1] = (short) j1;
	    e->errlevel[2] = (short) j2;
	    e->errlevel[3] = (short) j3;
	    p = skip_to_blank (p);
	    break;

	 case 'n':   /* No inbound requests */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_NOINREQ;
	    break;

	 case 'x':   /* No outbound requests */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_NOOUTREQ;
	    break;

#if JAC
	 case 'p':   /* disable phone */
	    p = skip_to_blank(p);
	    e->behavior |= MAT_NOPHONE;
	    break;
#endif

	 case 'r':   /* Receive only */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_NOOUT;
	    break;

	 case 's':   /* Send only */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_OUTONLY;
	    break;

	 case 't':   /* Tries */
	    ++p;
	    if (sscanf (p, "=%d,%d", &j1, &j2) != 2)
	       {
	       fprint(stderr,"'%s' has a bad number of Tries\n", e_line);
	       free(e);
	       return (1);
	       }
	    else
	       {
	       if ((j1 > 9) || (j1 < 0) || (j2 > 32000) || (j2 < 0))
		  {
		  fprint(stderr,"'%s' has a bad number of Tries\n", e_line);
		  free(e);
		  return (1);
		  }
	       e->max_conn = (short) j1;
	       e->max_noco = (short) j2;
	       }
	    p = skip_to_blank (p);
	    break;

	 case 'z':   /* Zero no-connect counters */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_ZERONOC;
	    break;

	 case '$':   /* Zero bad-connect counters */
	    p = skip_to_blank (p);
	    e->behavior |= MAT_ZEROCON;
	    break;

	 default:    /* Error condition */
	    fprint(stderr,"'%s' has something indecipherable\n", e_line);
	    free (e);
	    return (1);
	 }
      }

   /* Save it in the array  of pointers */
   e_ptrs[num_events++] = e;

#ifdef debug
	print ("\nEvent %d shows:\n",num_events-1);
	print ("  days = %04x\n", (word) e->days);
	print ("  minute = %d\n", (int)  e->minute);
	print ("  length = %d\n", (int)  e->length);
	for (i = 0; i < 5; i++)
	print ("  err[%d] = %d\n", i, (int) e->errlevel[i]);
#endif

   /* Return that everything is cool */
   return (0);

err:
   fprint(stderr,"'%s' has an invalid DAY field\n", e_line);
   free (e);
   return (1);
}


void find_event ()
{
   char buffer[256];
   char flagbuf[30];
   struct dostime_t t;
   struct dosdate_t d;
   long tim;
   int cur_day;
   int our_time;
   char *p;
   int i;
   int left;
   static int lastleft=-1;
   int prev_event;

   /* Get the current time in minutes */
   _dos_getdate(&d);
   _dos_gettime(&t);
   our_time = t.hour * 60 + t.minute;

   tim = time(NULL);
   /* update the time on display */
   if (lasttime != tim)
   {
	lasttime = tim;
	win_xyprint(top_win,win_maxhor - 24,1,"$aD $D $aM $Y $t");
#if OS2 /*&& WATCOM*/
	mcp_sleep();
#endif
   }

   /* Get the current day of the week */
   cur_day= dayofweek(d.year,d.month-1,d.day);
   cur_day= 1 << cur_day;

   prev_event = cur_event;
   cur_event = 0;

   /* Go through the events from top to bottom */
   for (i = 1; i < num_events; i++)
   {
      /* check start-time first */
      if (our_time >= e_ptrs[i]->minute)
      {
	 /* does is run today? */
	 if (cur_day & e_ptrs[i]->days)
	 {
	    /* not after end-time or is it forced? */
	    if (((our_time-e_ptrs[i]->minute) < e_ptrs[i]->length) ||
		((our_time == e_ptrs[i]->minute) && (e_ptrs[i]->length == 0)) ||
		((e_ptrs[i]->behavior & MAT_FORCED) && (e_ptrs[i]->last_ran != d.day)))
	    {
	       /* Are we not supposed to force old events */
	       if ( (!force) &&
		    ((our_time-e_ptrs[i]->minute) > e_ptrs[i]->length) &&
		    (noforce))
	       {
		  e_ptrs[i]->last_ran = d.day;
		  continue;
	       }

	       left= e_ptrs[i]->minute + e_ptrs[i]->length - our_time;
	       if (lastleft != left || i != prev_event)
	       {
		   win_xyprint(schedule_win,8,3,"%02d:%02d",
			       (our_time + left) / 60,(our_time + left) % 60);
		   lastleft= left;
	       }

	       if (e_ptrs[i]->last_ran != d.day)
	       {
		  message(3,"*Starting Event %d", i);

		  /* Mark that this one is running */
		  e_ptrs[i]->last_ran = d.day;

		  /* reset the flags so it won't be skipped if it's dynamic */
		  e_ptrs[i]->behavior &= ~MAT_SKIP;

		  /* Write out the schedule */
		  period_write();
		  write_sched();

		  if (e_ptrs[i]->behavior & (MAT_ZERONOC | MAT_ZEROCON)) {
		     OUTBLIST *op;
		     boolean changed;

		     message(3,"+Clearing call counters");
		     for (op = outblist; op; op = op->next) {
			 bad_call(op->zone,op->net,op->node,op->point,0);
			 changed = false;
			 if ((e_ptrs[i]->behavior & MAT_ZERONOC) && op->noco) {
			    op->noco = 0;
			    changed = true;
			 }
			 if ((e_ptrs[i]->behavior & MAT_ZEROCON) && op->conn) {
			    op->conn = 0;
			    changed = true;
			 }
			 if (changed)
			    bad_call(op->zone,op->net,op->node,op->point,3);
		     }
		  }

		  /* If we are supposed to exit, then do it */
		  if (e_ptrs[i]->errlevel[0])
		     errl_exit(e_ptrs[i]->errlevel[0],false,"Exit at start of event");

		  /* Starting new mail schedule or just change behaviour? */
		  if ( (!(e_ptrs[i]->behavior & MAT_CBEHAV)) &&
		       (e_ptrs[i]->tag!='X') && (e_ptrs[i]->tag!='Z')) {
		     if (mailpack) {
			message(3,"*Start packing mail for event %d (tag %c)",
				  i, (char) e_ptrs[i]->tag);
			mdm_busy();
			if (log != NULL) fclose (log);
			sys_reset();
			(void) win_savestate();
			sprint(buffer,"%s %u %c",
				      mailpack, task, (char) e_ptrs[i]->tag);
			execute(buffer);
			(void) win_restorestate();
			sys_init();
			if (log != NULL) {
			   if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
			      message(6,"!Can't re-open logfile!");
			}
			init_modem();
			waitforline = time(NULL) + 60;
			message(3,"*Returned from mailpack");
		     }
		     else
			errl_exit(4,false,"Start packing mail for event %d",i);
		  }

		  win_keypush(Ctrl_C);
	       }
	       else
	       {
		  /* Don't do events that have been exited already */
		  if (e_ptrs[i]->behavior & MAT_SKIP)
		     continue;
	       }

	       cur_event = i;

	       max_connects   = e_ptrs[i]->max_conn;
	       max_noconnects = e_ptrs[i]->max_noco;

	       i= e_ptrs[i]->behavior;

	       if (cur_event != prev_event)
	       {
		  win_xyprint(schedule_win,8,2,"%d",cur_event);
		  win_clreol(schedule_win);
		  p = flagbuf;
		  if (i & MAT_CM       ) *p++ = 'C';
		  if (i & MAT_HIPRICM  ) *p++ = 'H';
		  if (i & MAT_MAILHOUR ) *p++ = '#';
		  if (i & MAT_DYNAM    ) *p++ = 'D';
		  if (i & MAT_EXITDYNAM) *p++ = 'E';
		  { EXTAPP *ep;
		    for (ep = extapp; ep; ep = ep->next) {
			if ((ep->flags & APP_BBS) && period_active(&ep->per_mask)) {
			   *p++ = 'B';
			   break;
			}
		    }
		  }
		  if (i & MAT_NOINREQ  ) *p++ = 'N';
		  if (i & MAT_NOOUTREQ ) *p++ = 'X';
		  if (i & MAT_NOOUT    ) *p++ = 'R';
		  if (i & MAT_OUTONLY  ) *p++ = 'S';
		  if (i & MAT_LOCAL    ) *p++ = 'L';
		  *p = '\0';
		  if (e_ptrs[cur_event]->queue_size)
		     sprint(p,"Q%d", (int) e_ptrs[cur_event]->queue_size);
		  if (flagbuf[0])
		     win_print(schedule_win,"/%s",flagbuf);
		  win_xyprint(schedule_win,8,4,"%d  ",(int) e_ptrs[cur_event]->wait_time);
		  win_clreol(schedule_win);
	       }

#if JAC
		   if (cur_event==0)
		   {
			phone_out(0);
		   }
		   else
		   {
			phone_out(e_ptrs[cur_event]->behavior & MAT_NOPHONE);
		   }
#endif

	       break;
	    }
	 }
      }
   }

   if (sched_stamp < (tim - (tim % 86400L)))
      write_sched();

#ifdef debug
	print("\nCurrent event is event %d\n", cur_event);
	if (cur_event > 0)
	{
		print ("Event shows:\n");
		print ("  days = %04x\n", (word) e_ptrs[cur_event]->days);
		print ("  minute = %d\n", (int)  e_ptrs[cur_event]->minute);
		print ("  length = %d\n", (int)  e_ptrs[cur_event]->length);
		for (i = 0; i < 5; i++)
		print ("  err[%d] = %d\n", i, (int) e_ptrs[cur_event]->errlevel[i]);
	}
#endif
}


void read_sched (void)
{
   char temp1[PATHLEN], temp2[PATHLEN];
   EVENT *sptr;
   struct stat buffer1, buffer2;
   FILE *f;
   long tim;
   int i;
   int size;

   sprint(temp1,"%sXENIA%u.SCD",homepath,task);
   sprint(temp2,"%sXENIA%u.EVT",homepath,task);
   if (!fexist(temp2)) {
      noevttask = true;
      sprint(temp2,"%sXENIA.EVT",homepath);
   }

   if (!getstat(temp2,&buffer2))
      event_stamp = buffer2.st_mtime;

   if (getstat(temp1, &buffer1) ||
       (f = sfopen (temp1, "rb", DENY_WRITE)) == NULL)
      return;

   sched_stamp = buffer1.st_mtime;

   if (buffer1.st_size < sizeof(struct _extrainfo))
      goto fini;
   tim = time(NULL);
   if (extra.calls != 0xffff && sched_stamp >= (tim - (tim % 86400L)))
      fread ((char *) &extra, sizeof(struct _extrainfo), 1, f);
   else {
      fseek(f,(long) sizeof(struct _extrainfo), SEEK_SET);
      extra.calls=0;
   }

   if (!event_stamp)
      goto fini;

   if (event_stamp > time(NULL)) {
      event_stamp = time(NULL);
      setstamp(temp2,event_stamp);
      sched_stamp = 0L;
   }

   if (sched_stamp < event_stamp) {   /* earlier date? */
/*	fprint(stderr,"Get schedule from EVENT file.");*/
      noforce = 1;
      goto fini;
   }

   size = ((int) buffer1.st_size) - (int) sizeof(struct _extrainfo);
   if (size <= 0 || (sptr = (EVENT *) malloc (size)) == NULL)
      goto fini;

   fread(sptr,size,1,f);
   got_sched = 1;

   num_events = ((int) (size / sizeof (EVENT))) + 1;
   for (i = 1; i < num_events; i++)
       e_ptrs[i] = sptr++;

fini:
   fclose (f);

   /*fprint(stderr,"Schedule read");*/
   return;
}


void write_sched (void)
{
   extern char *mon[];
   char temp1[PATHLEN], temp2[PATHLEN];
   struct stat st;
   long tim;
   FILE *f;
   int i;

   sprint(temp1,"%sXENIA%u.SCD",homepath,task);
   sprint(temp2,"%sXENIA%.0u.EVT",homepath,noevttask ? 0 : task);

   tim = time(NULL);

   if (sched_stamp < (tim - (tim % 86400L))) {
      struct tm *tt;

      if (sched_stamp) {
	 tt = localtime((time_t *) &sched_stamp);
	 message(6,"%Calls %02d %3s %04d: In=%u  BBS=%u  Mail=%u  Out=%u",
		   tt->tm_mday, mon[tt->tm_mon], tt->tm_year + 1900,
		   extra.calls, extra.human, extra.mailer, extra.called);
	 message(6,"%Moved %02d %3s %04d: Mail=%u (%u KB)  Files=%u (%u KB)",
		   tt->tm_mday, mon[tt->tm_mon], tt->tm_year + 1900,
		   extra.nrmail, extra.kbmail, extra.nrfiles, extra.kbfiles);
      }

      memset(&extra,0,sizeof (struct _extrainfo));
      show_stats();
   }

   if ((f = sfopen(temp1,"wb",DENY_ALL)) == NULL)
      return;

   fwrite((char *) &extra, sizeof(struct _extrainfo), 1, f);

   for (i = 1; i < num_events; i++) {
       /* If it is skipped, but not dynamic, reset it */
       if ((e_ptrs[i]->behavior & MAT_SKIP))
	  e_ptrs[i]->behavior &= ~MAT_SKIP;

       /* Write this one out */
       fwrite(e_ptrs[i],sizeof (EVENT),1,f);
   }

   fclose(f);

   sched_stamp = tim;
   setstamp(temp1, sched_stamp);

   if (!getstat(temp2,&st)) {
      if (st.st_mtime != event_stamp) {
	 event_stamp = sched_stamp + 2;
	 setstamp(temp2, event_stamp);
      }
   }

   return;
}


time_to_next ()
{
   return (1440);
#if 0
   int cur_day;
   struct dostime_t t;
   struct dosdate_t d;
   int our_time;
   int i;
   int time_to;
   int guess;
   int nmin;

   /* Get the current time in minutes */
   _dos_getdate(&d);
   _dos_gettime(&t);
   our_time = t.hour * 60 + t.minute;

   /* Get the current day of the week */
   cur_day= dayofweek(d.year,d.month-1,d.day);

   /* Make the month 0 based */

   cur_day = 1 << cur_day;

   /* A ridiculous number */
   time_to = 3000;

   /* Go through the events from top to bottom */
   for (i = 1; i < num_events; i++)
      {
      /* If it is the current event, skip it */
      if (cur_event == i)
	 continue;

      /* If it is a BBS event, skip it */
      if (e_ptrs[i]->behavior & MAT_BBS)
	 continue;

      /* If it was already run today, skip it */
      if (e_ptrs[i]->last_ran == d.day)
	 continue;

      /* If it doesn't happen today, skip it */
      if (!(e_ptrs[i]->days & cur_day))
	 continue;

      /* If it is earlier than now, skip it unless it is forced */
      if (e_ptrs[i]->minute < our_time)
	 {
	 if (!(e_ptrs[i]->behavior & MAT_FORCED))
	    {
	    continue;
	    }

	 /* Hmm, found a forced event that has not executed yet */
	 /* Give the guy 2 minutes and call it quits */
	 guess = 2;
	 }
      else
	 {
	 /* Calculate how far it is from now */
	 guess = e_ptrs[i]->minute - our_time;
	 }   

      /* If less than closest so far, keep it */
      if (time_to > guess)
	 time_to = guess;
      }

   /* If we still have nothing, then do it again, starting at midnight */
   if (time_to >= 1441)
      {
      /* Calculate here to midnight */
      nmin = 1440 - our_time;

      /* Go to midnight */
      our_time = 0;

      /* Go to the next possible day */
      cur_day = cur_day << 1;
      if (cur_day > DAY_SATURDAY)
	 cur_day = DAY_SUNDAY;

      /* Go through the events from top to bottom */
      for (i = 1; i < num_events; i++)
	 {
	 /* If it is a BBS event, skip it */
	 if (e_ptrs[i]->behavior & MAT_BBS)
	    continue;

	 /* If it doesn't happen today, skip it */
	 if (!(e_ptrs[i]->days & cur_day))
	    continue;

	 /* Calculate how far it is from now */
	 guess = e_ptrs[i]->minute + nmin;

	 /* If less than closest so far, keep it */
	 if (time_to > guess)
	    time_to = guess;
	 }
      }

   if (time_to > 1440)
      time_to = 1440;

   if (time_to < 1)
      time_to = 1;

   return (time_to);
#endif
}


word time_to_noreq (void)
{
   int cur_day;
   struct dostime_t t;
   struct dosdate_t d;
   int our_time;
   int i;
   int time_to;
   int guess;
   int nmin;

   /* Get the current time in minutes */
   _dos_getdate(&d);
   _dos_gettime(&t);
   our_time = t.hour * 60 + t.minute;

   /* Get the current day of the week */
   cur_day= dayofweek(d.year,d.month-1,d.day);

   /* Make the month 0 based */

   cur_day = 1 << cur_day;

   /* A ridiculous number */
   time_to = 3000;

   /* Go through the events from top to bottom */
   for (i = 1; i < num_events; i++)
      {
      /* If it is the current event, skip it */
      if (cur_event == i)
	 continue;

      /* If it is a no-real-change event, skip it */
      if (((e_ptrs[i]->behavior & MAT_CBEHAV) || (e_ptrs[i]->tag=='Z'))
	  && !(e_ptrs[i]->behavior & MAT_NOINREQ))
	 continue;

      /* If it was already run today, skip it */
      if (e_ptrs[i]->last_ran == d.day)
	 continue;

      /* If it doesn't happen today, skip it */
      if (!(e_ptrs[i]->days & cur_day))
	 continue;

      /* If it is earlier than now, skip it unless it is forced */
      if (e_ptrs[i]->minute < our_time)
	 {
	 if (!(e_ptrs[i]->behavior & MAT_FORCED))
	    {
	    continue;
	    }

	 /* Hmm, found a forced event that has not executed yet */
	 /* Give the guy 2 minutes and call it quits */
	 guess = 2;
	 }
      else
	 {
	 /* Calculate how far it is from now */
	 guess = e_ptrs[i]->minute - our_time;
	 }   

      /* If less than closest so far, keep it */
      if (time_to > guess)
	 time_to = guess;
      }

   /* If we still have nothing, then do it again, starting at midnight */
   if (time_to >= 1441)
      {
      /* Calculate here to midnight */
      nmin = 1440 - our_time;

      /* Go to midnight */
      our_time = 0;

      /* Go to the next possible day */
      cur_day = cur_day << 1;
      if (cur_day > DAY_SATURDAY)
	 cur_day = DAY_SUNDAY;

      /* Go through the events from top to bottom */
      for (i = 1; i < num_events; i++)
	 {
	 /* If it is a no-real-change event, skip it */
	 if (((e_ptrs[i]->behavior & MAT_CBEHAV) || (e_ptrs[i]->tag=='Z'))
	     && !(e_ptrs[i]->behavior & MAT_NOINREQ))
	    continue;

	 /* If it doesn't happen today, skip it */
	 if (!(e_ptrs[i]->days & cur_day))
	    continue;

	 /* Calculate how far it is from now */
	 guess = e_ptrs[i]->minute + nmin;

	 /* If less than closest so far, keep it */
	 if (time_to > guess)
	    time_to = guess;
	 }
      }

   if (time_to > 1440)
      time_to = 1440;

   if (time_to < 1)
      time_to = 1;

   return ((word) time_to);
}


/* end of sched.c */
