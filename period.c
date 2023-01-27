/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * period.c  World time period scheduler - general global functions
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


#define PER_FORMAT  1


/* ------------------------------------------------------------------------- */
extern char *wkday[];
extern char *mon[];
static byte  mdays[12]	= { 31, 28, 31, 30, 31, 30,
			    31, 31, 30, 31, 30, 31 };
static const char *separator = ",|";
static PERIOD_MASK active_mask;


/* ------------------------------------------------------------------------- */
/* leapyears: 1972 76 80 84 88 92 96 2000 2004 2008 2012 2016 2020 2024 2028 */
/* Mon 01 Jan 1980 00:00 - number of seconds since 01 Jan 1970 00:00 */
#define DAY1980 (10 * 365 + 2)	/* 10 years since 1970, including 2 leapdays */
#define SEC1980 (86400L * (long) DAY1980)  /* secs per day * days since 1970 */
#define DAY1995 (25 * 365 + 6)	/* 25 years since 1970, including 6 leapdays */
#define SEC1995 (86400L * (long) DAY1995)  /* secs per day * days since 1970 */
#define DAY2030 (60 * 365 + 15) /* 60 years since 1970, including 15 leapdays*/
#define SEC2030 (86400L * (long) DAY2030)  /* secs per day * days since 1970 */
#define BEGINYEAR 1995
#define ENDYEAR 2029   /* More exact 31 Dec 2029 24:00 -=> 01 Jan 2030 00:00 */



/* ------------------------------------------------------------------------- */
static int period_strwild (char *s)
{
	return (!strcmp(s,"*") || !stricmp(s,"All") || !stricmp(s,"Any"));
}/*period_strwild()*/


/* ------------------------------------------------------------------------- */
static int period_findstr (char *s, char *table[], int num_items)
{
	int i;

	for (i = 0; i < num_items; i++) 	 /* Check every day or month */
	    if (!stricmp(s,table[i])) return (i);
	return (-1);
}/*period_findstr()*/


/* ------------------------------------------------------------------------- */
static void unixtoperiod (long t, word *year, byte *month, byte *day,
				  byte *hour, byte *min)
{
	t -= SEC1980;					/* Start in 1980     */
	t /= 60;					/* Time in minutes   */
	*min = t % 60;					/* Store minutes     */
	t /= 60;					/* Time in hours     */
	*year = (word) (1980 + ((int) ((t / (1461L * 24L)) << 2)));
	t %= 1461L * 24L;
	if (t > 366 * 24) {
	   t -= 366 * 24;
	   (*year)++;
	   *year += (word) (t / (365 * 24));
	   t %= 365 * 24;
	}
	*hour = t % 24;
	t /= 24;					/* Time in days      */
	t++;
	if (((*year) & 3) == 0) {
	   if (t > 60)
	      t--;
	   else if (t == 60) {
	      *month = 1;
	      *day = 29;
	      return;
	   }
	}
	for (*month = 0; mdays[*month] < t; (*month)++)
	    t -= mdays[*month];
	*day = t;
}/*unixtoperiod()*/


/* ------------------------------------------------------------------------- */
static long periodtounix (word year, byte month, byte day, byte hour, byte min)
{
	long t;
	register word i, days;
	word hours;

	t = SEC1980;				/* Convert 1980 to 1970 base */
	i = (word) (year - 1980);		/* years since 1980 leapyear */
	t += (i >> 2) * (1461L * 24L * 60L * 60L);
	t += (i & 3) * (24L * 60L * 60L * 365L);
	if (i & 3)
	   t += 24L * 3600L;
	days = 0;
	i = month;					    /* Add in months */
	while (i > 0)
	      days += mdays[--i];
	days += (word) (day - 1);
	if ((month > 1) && ((year & 3) == 0))
	   days++;				   /* Currently in leap year */
	hours = (word) (days * 24 + hour);		       /* Find hours */
	t += hours * 3600L;
	t += 60L * min; 				      /* + 0 seconds */

	return (t);
}/*periodtounix()*/


/* ------------------------------------------------------------------------- */
static long period_next (byte base[], long offset)  /* calculate next date/time fit */
{
	word year;
	byte month, day, hour, min;
	byte numdays;

	unixtoperiod(offset ? offset : SEC1995,&year,&month,&day,&hour,&min);

	while (year <= ENDYEAR) {
	      if (PER_GETBIT(base,PER_YEAR,year - BEGINYEAR)) {
		 while (month < 12) {
		       if (PER_GETBIT(base,PER_MONTH,month)) {
			  numdays = mdays[month];
			  if ((month == 1) && !(year % 4))
			     numdays++;
			  while (day <= numdays) {
				if (PER_GETBIT(base,PER_MDAY,day - 1)) {
				   if (PER_GETBIT(base,PER_WDAY,dayofweek(year,month,day))) {
				      while (hour <= 24) {
					    if (PER_GETBIT(base,PER_HOUR,hour)) {
					       while (min < 60) {
						     if (PER_GETBIT(base,PER_MIN,min))
							return (periodtounix(year,month,day,hour,min));
						     min++;
					       }/*while(min)*/
					    }/*if(hour)*/
					    hour++;
					    min = 0;
				      }/*while(hour)*/
				   }/*if(wday)*/
				}/*if(day)*/
				day++;
				hour = min = 0;
			  }/*while(day)*/
		       }/*if(month)*/
		       month++;
		       day = 1;
		       hour = min = 0;
		 }/*while(month)*/
	      }/*if(year)*/
	      year++;
	      month = 0;
	      day = 1;
	      hour = min = 0;
	}/*if(endyear)*/

	return (0L);			/* no more valid dates in the future */
}/*period_next()*/


/* ------------------------------------------------------------------------- */
int period_parsetime (char *av[], byte base[])		/* parse user time   */
{      /* No. parms MUST be 5 -> Fri 13 Oct 1989 13:00	(* field = wildcard) */
	char buf1[128], buf2[128];
	char c;
	int i, j, step;
	register char *p;

	memset(base,0,PER_BYTE_SIZE);			/* reset period def  */

	for (p = strtok(av[0],separator); p; p = strtok(NULL,separator)) {
	    if (period_strwild(p)) {
	       PER_SETALL(base,PER_WDAY,PER_NUM_WDAY);
	    }
	    else if (!stricmp(p,"Week")) {
	       j = period_findstr("Fri",wkday,PER_NUM_WDAY);
	       for (i = period_findstr("Mon",wkday,PER_NUM_WDAY); i <= j; i++)
		   PER_SETBIT(base,PER_WDAY,i);
	    }
	    else if (!stricmp(p,"WkEnd")) {
	       PER_SETBIT(base,PER_WDAY,period_findstr("Sat",wkday,PER_NUM_WDAY));
	       PER_SETBIT(base,PER_WDAY,period_findstr("Sun",wkday,PER_NUM_WDAY));
	    }
	    else {
	       buf1[0] = buf2[0] = '\0';
	       c = 0;
	       sscanf(p,"%[a-zA-Z]%c%s",buf1,&c,buf2);
	       if ((i = j = period_findstr(buf1,wkday,PER_NUM_WDAY)) < 0)
		  return (0);
	       step = 1;
	       if (c && buf2[0]) {
		  if (c == '-') {
		     if ((j = period_findstr(buf2,wkday,PER_NUM_WDAY)) < 0)
			return (0);
		  }
		  else if (c == '+') {
		     if ((step = atoi(buf2)) < 1)
			return (0);
		     j = PER_NUM_WDAY;
		  }
		  else
		     return (0);
	       }
	       for ( ; i <= j; i += step)
		   PER_SETBIT(base,PER_WDAY,i);
	    }
	}

	for (p = strtok(av[1],separator); p; p = strtok(NULL,separator)) {
	    if (period_strwild(p)) {
	       PER_SETALL(base,PER_MDAY,PER_NUM_MDAY);
	    }
	    else {
	       i = j = 0;
	       c = 0;
	       sscanf(p,"%d%c%d",&i,&c,&j);
	       if (i < 1 || i > PER_NUM_MDAY)
		  return (1);
	       step = 1;
	       if (c) {
		  if (c == '-') {
		     if (j < 1 || j > PER_NUM_MDAY)
			return (1);
		  }
		  else if (c == '+') {
		     step = j;
		     if (step < 1)
			return (1);
		     j = PER_NUM_MDAY;
		  }
		  else
		     return (1);
	       }
	       else
		  j = i;
	       for (i--, j--; i <= j; i += step)
		   PER_SETBIT(base,PER_MDAY,i);
	    }
	}

	for (p = strtok(av[2],separator); p; p = strtok(NULL,separator)) {
	    if (period_strwild(p)) {
	       PER_SETALL(base,PER_MONTH,PER_NUM_MONTH);
	    }
	    else {
	       buf1[0] = buf2[0] = '\0';
	       c = 0;
	       sscanf(p,"%[a-zA-Z]%c%s",buf1,&c,buf2);
	       if ((i = j = period_findstr(buf1,mon,PER_NUM_MONTH)) < 0)
		  return (2);
	       step = 1;
	       if (c && buf2[0]) {
		  if (c == '-') {
		     if ((j = period_findstr(buf2,mon,PER_NUM_MONTH)) < 0)
			return (2);
		  }
		  else if (c == '+') {
		     if ((step = atoi(buf2)) < 1)
			return (2);
		     j = PER_NUM_MONTH;
		  }
		  else
		     return (2);
	       }
	       for ( ; i <= j; i += step)
		   PER_SETBIT(base,PER_MONTH,i);
	    }
	}

	for (p = strtok(av[3],separator); p; p = strtok(NULL,separator)) {
	    if (period_strwild(p)) {
	       PER_SETALL(base,PER_YEAR,PER_NUM_YEAR);
	    }
	    else {
	       i = j = 0;
	       c = 0;
	       sscanf(p,"%d%c%d",&i,&c,&j);
	       if (i < BEGINYEAR || i > ENDYEAR)
		  return (3);
	       step = 1;
	       if (c) {
		  if (c == '-') {
		     if (j < BEGINYEAR || j > ENDYEAR)
			return (3);
		  }
		  else if (c == '+') {
		     step = j;
		     if (step < 1)
			return (3);
		     j = ENDYEAR;
		  }
		  else
		     return (3);
	       }
	       else
		  j = i;
	       for (i -= BEGINYEAR, j -= BEGINYEAR; i <= j; i += step)
		   PER_SETBIT(base,PER_YEAR,i);
	    }
	}

	if ((av[4] = strtok(av[4],":")) == NULL ||
	    (av[5] = strtok(NULL,":")) == NULL)
	   return (6);

	for (p = strtok(av[4],separator); p; p = strtok(NULL,separator)) {
	    if (period_strwild(p)) {
	       PER_SETALL(base,PER_HOUR,PER_NUM_HOUR);
	    }
	    else {
	       i = j = -1;
	       c = 0;
	       sscanf(p,"%d%c%d",&i,&c,&j);
	       if (i < 0 || i > 24)
		  return (4);
	       step = 1;
	       if (c) {
		  if (c == '-') {
		     if (j < 0 || j > 24)
			return (4);
		  }
		  else if (c == '+') {
		     step = j;
		     if (step < 1)
			return (4);
		     j = 24;
		  }
		  else
		     return (4);
	       }
	       else
		  j = i;
	       for ( ; i <= j; i += step)
		   PER_SETBIT(base,PER_HOUR,i);
	    }
	}

	for (p = strtok(av[5],separator); p; p = strtok(NULL,separator)) {
	    if (period_strwild(p)) {
	       PER_SETALL(base,PER_MIN,PER_NUM_MIN);
	    }
	    else {
	       i = j = -1;
	       c = 0;
	       sscanf(p,"%d%c%d",&i,&c,&j);
	       if (i < 0 || i > 59)
		  return (5);
	       step = 1;
	       if (c) {
		  if (c == '-') {
		     if (j < 0 || j > 59)
			return (5);
		  }
		  else if (c == '+') {
		     step = j;
		     if (step < 1)
			return (5);
		     j = 59;
		  }
		  else
		     return (5);
	       }
	       else
		  j = i;
	       for ( ; i <= j; i += step)
		   PER_SETBIT(base,PER_MIN,i);
	    }
	}

#if 0
for (i = 0; i < PER_BYTE_SIZE; i++)
    fprint(stderr,"%02hx ",base[i]);
fprint(stderr,"\n");
#endif

	/* Non-occurring wdays/mdays etc are not checked here... */
	return (7);					/* Return parsed OK! */
}/*period_parsetime()*/


/* ------------------------------------------------------------------------- */
void period_read (void)
{
	char	buffer[PATHLEN];
	FILE   *fp;
	PERIOD	per;
	int	i;
	word	year;
	byte	month, day, hour, min;

	sprint(buffer,"%sXENIA%u.PER",homepath,task);
	if ((fp = sfopen(buffer,"rb",DENY_ALL)) != NULL) {
	   if (getc(fp) == PER_FORMAT) {
	      while (fread(&per,sizeof (PERIOD),1,fp) == 1) {
		    for (i = 0; i < num_periods; i++) {
			if (!stricmp(per.tag,periods[i]->tag) &&
			    !memcmp(per.defbegin,periods[i]->defbegin,PER_BYTE_SIZE) &&
			    !memcmp(per.defend,periods[i]->defend,PER_BYTE_SIZE) &&
			    (per.length == periods[i]->length) &&
			    ((per.flags & PER_LOCAL) == (periods[i]->flags & PER_LOCAL))) {
			   periods[i]->flags = per.flags;
			   periods[i]->begin = per.begin;
			   periods[i]->end   = per.end;
			   break;
			}
		    }
	      }
	   }
	   fclose(fp);
	}

if (loglevel == 0) {
	for (i = 0; i < num_periods; i++) {
	    sprint(buffer,"Period %-8s %-8s: ", periods[i]->tag,
		   (periods[i]->flags & PER_ACTIVE) ? "Active" : "Inactive");
	    if (periods[i]->flags & PER_NOMORE)
	       message(0,"*%sNo more activity in the future",buffer);
	    else if (!periods[i]->begin)
	       message(0,"*%sNewly defined (uncalculated)",buffer);
	    else if (periods[i]->flags & PER_ACTIVE) {
	       unixtoperiod((periods[i]->flags & PER_LOCAL) ?
			    periods[i]->end : (periods[i]->end - gmtdiff),
			    &year,&month,&day,&hour,&min);
	       message(0,"*%sEnding     %s %02u %s %04u %02u:%02u",
			 buffer,
			 wkday[dayofweek(year,month,day)],
			 day, mon[month], year, hour, min);
	    }
	    else {
	       unixtoperiod((periods[i]->flags & PER_LOCAL) ?
			    periods[i]->begin : (periods[i]->begin - gmtdiff),
			    &year,&month,&day,&hour,&min);
	       message(0,"*%sNext begin %s %02u %s %04u %02u:%02u",
			 buffer,
			 wkday[dayofweek(year,month,day)],
			 day, mon[month], year, hour, min);
	    }
	}
}/*if(loglevel==0)*/

}/*period_read()*/


/* ------------------------------------------------------------------------- */
void period_write (void)
{
	char	buffer[PATHLEN];
	FILE   *fp;
	int	i;

	sprint(buffer,"%sXENIA%u.PER",homepath,task);
	if ((fp = sfopen(buffer,"wb",DENY_ALL)) == NULL)
	   message(6,"-Error creating file %s",buffer);
	else {
	   putc(PER_FORMAT,fp);
	   for (i = 0; i < num_periods; i++)
	       fwrite(periods[i],sizeof (PERIOD),1,fp);
	   fclose(fp);
	}
}/*period_write()*/


/* ------------------------------------------------------------------------- */
long period_calculate (void)  /* Do period calcs, return time of next change */
{
	int i;
	long curtime, curlocal, nextchange = SEC2030;
	char *changetag = NULL;
	boolean changewhat = false;
	word year;
	byte month, day, hour, min;

	statusmsg("Calculating period activity");
	curlocal = time(NULL);

	for (i = 0; i < num_periods; i++) {
	    /*print("Working on period %-8s\r",periods[i]->tag);*/
	    if (periods[i]->flags & PER_NOMORE)    /* Never active in future */
	       continue;

	    curtime = (periods[i]->flags & PER_LOCAL) ?
		      curlocal : (curlocal + gmtdiff);

	    if (periods[i]->flags & PER_ACTIVE) {   /* Now flagged as active */
	       if (curtime >= periods[i]->end) {	/* Period now ended  */
		  periods[i]->flags &= ~PER_ACTIVE;	/* Flag not active   */
		  periods[i]->flags |= PER_ENDED;	/* Flag per ended    */
		  message(3,"+Period %-8s end",periods[i]->tag);
	       }
	       else {				      /* Period still active */
		  if (periods[i]->flags & PER_LOCAL) {
		     if (periods[i]->end < nextchange) {
			changetag = periods[i]->tag;
			nextchange = periods[i]->end;
			changewhat = false;
		     }
		  }
		  else if ((periods[i]->end - gmtdiff) < nextchange) {
		     changetag = periods[i]->tag;
		     nextchange = periods[i]->end - gmtdiff;
		     changewhat = false;
		  }
		  continue;
	       }
	    }

	    while (curtime >= periods[i]->end) {	/* Current past end? */
		  periods[i]->begin = period_next(periods[i]->defbegin,
						  periods[i]->end);
		  if (!periods[i]->begin) {		 /* Found new begin? */
		     periods[i]->flags |= PER_NOMORE;	 /* No, never more   */
		     message(3,"+Period %-8s no more activity in the future",periods[i]->tag);
		     break;
		  }

		  if (periods[i]->length)		    /* Length 1 day? */
		     periods[i]->end = periods[i]->begin + periods[i]->length;
		  else					      /* No, endspec */
		     periods[i]->end = period_next(periods[i]->defend,periods[i]->begin);
		  if (!periods[i]->end || periods[i]->end > SEC2030)
		     periods[i]->end = SEC2030;

		  if (curtime < periods[i]->end) {    /* If for logging only */
		     unixtoperiod((periods[i]->flags & PER_LOCAL) ?
				  periods[i]->begin : (periods[i]->begin - gmtdiff),
				  &year,&month,&day,&hour,&min);
		     message(1,"+Period %-8s next active   %s %02u %s %04u %02u:%02u",
			       periods[i]->tag, wkday[dayofweek(year,month,day)],
			       day, mon[month], year, hour, min);
		  }
	    }

	    if (periods[i]->flags & PER_NOMORE)    /* Never active in future */
	       continue;

	    if (curtime >= periods[i]->begin) {        /* Period started now */
	       periods[i]->flags |= (PER_ACTIVE | PER_BEGUN);
	       unixtoperiod((periods[i]->flags & PER_LOCAL) ?
			    periods[i]->end : (periods[i]->end - gmtdiff),
			    &year,&month,&day,&hour,&min);
	       message(1,"+Period %-8s begin, ending %s %02u %s %04u %02u:%02u",
			 periods[i]->tag, wkday[dayofweek(year,month,day)],
			 day, mon[month], year, hour, min);
	       if (periods[i]->flags & PER_LOCAL) {
		  if (periods[i]->end < nextchange) {
		     changetag = periods[i]->tag;
		     nextchange = periods[i]->end;
		     changewhat = false;
		  }
	       }
	       else if ((periods[i]->end - gmtdiff) < nextchange) {
		  changetag = periods[i]->tag;
		  nextchange = periods[i]->end - gmtdiff;
		  changewhat = false;
	       }
	    }
	    else {
	       if (periods[i]->flags & PER_LOCAL) {
		  if (periods[i]->begin < nextchange) {
		     changetag = periods[i]->tag;
		     nextchange = periods[i]->begin;
		     changewhat = true;
		  }
	       }
	       else if (periods[i]->begin - gmtdiff < nextchange) {
		  changetag = periods[i]->tag;
		  nextchange = periods[i]->begin - gmtdiff;
		  changewhat = true;
	       }
	    }
	}

	period_write();

	if (changetag) {
	   unixtoperiod(nextchange,&year,&month,&day,&hour,&min);
	   message(0,"*Next period change: %-8s %s %s %02u %s %04u %02u:%02u",
		     changetag, changewhat ? "Begin" : "Ending",
		     wkday[dayofweek(year,month,day)],
		     day, mon[month], year, hour, min);
	}
	else
	   message(0,"*Next period change: none");

	if (!num_periods)
	   memset(&active_mask,0xff,sizeof (PERIOD_MASK));
	else {
	   memset(&active_mask,0,sizeof (PERIOD_MASK));
	   for (i = 0; i < num_periods; i++) {
	       if (periods[i]->flags & PER_ACTIVE)  /* Now flagged as active */
		  PER_MASK_SETBIT(&active_mask,i);
	   }
	}

	statusmsg(NULL);

	return (nextchange);
}/*period_calculate()*/


/* ------------------------------------------------------------------------- */
boolean period_active (PERIOD_MASK *pmp)
{
	int i;

	for (i = 0; i < PER_MASK_DWORDS; i++) {
	    if (pmp->mask[i] & active_mask.mask[i])
	       return (true);
	}

	return (false);
}/*period_active()*/


/* ------------------------------------------------------------------------- */
#if 0
int period_to_nobbs (void)
{
	int i;
	PERIOD_MASK permask;
	EXTAPP *ep;

	for (ep = extapp; ep; ep = ep->next) {
	    if (ep->flags & APP_BBS) {
	       for (i = 0; i < PER_MASK_DWORDS; i++)
		   permask |= ep->per_mask[i];
	    }
	}

}/*period_to_nobbs()*/
#endif


/* end of period.c */
