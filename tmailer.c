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


/* Mailer routines */
#include "xenia.h"
#include "mail.h"


int polled_last = -1;


/* ------------------------------------------------------------------------- */
get_numbers(int poll)
{
    struct _nodeinfo *n;
    char number[85], *p;
    int i;

    if (poll)
       print("\r\nEnter nodenumber(s) (max 20), empty line to end.");
    else {
       print("\r\nEnter number(s) (max 20), empty line to end.");
       print("\r\n(Use @ to start phonenumber)");
    }

    for (i = 0; i < 20; i++) {
again:	print("\r\nNumber: ");
	get_str(number,80);
	if (!number[0]) break;

	p = strchr(number,'*');
	if (p != NULL) {
	   *p = '\0';
	   dial[i].speed = atol(p + 1);
       }
       else
	  dial[i].speed = 0;

       if (!poll && *number == '@') {
	  dial[i].zone = -1;
	  strcpy(dial[i].phone,number + 1);
       }
       else {
	  n = strtoinfo(number);
	  if (n == NULL) {
	     message(3,"!Invalid address");
	     goto again;
	  }
	  if ((n->nodeflags & DOWN) || (!poll && (n->nodeflags & (VIAHOST | VIABOSS)))) {
	     message(3,"!Can't call %d:%d/%d%s",n->zone,n->net,n->node,pointnr(n->point));
	     goto again;
	  }
	  if (!n->phone[0]) {
	     message(3,"!Address %d:%d/%d%s not listed",n->zone,n->net,n->node,pointnr(n->point));
	     goto again;
	  }
	  message(-1,":System: %s",n->name);
	  dial[i].zone	= n->zone;
	  dial[i].net	= n->net;
	  dial[i].node	= n->node;
	  dial[i].point = n->point;
	}
    }

    print("\r\n\n");
    dial[i].zone = -2;
    if (!poll && i)
       polled_last = -1;

    return (0);
}


/* ------------------------------------------------------------------------- */
void mailout (int poll)
{
     long ctime;
     int temp, i, call_one;
     int left;

     temp = pickup;

     if (poll > -1) {
	get_numbers(1);
	polled_last = poll;
     }
     pickup = polled_last;

     left = 0;
     for (i = 0; i < 20; i++) {
	 if	 (dial[i].zone == -2) break;
	 else if (dial[i].zone != -3) left++;
     }

     call_one = 1;
     while (call_one) {
	   call_one = 0;

	   for (i = 0; i < 20; i++) {
	       win_xyprint(schedule_win,8,5,"%d  ",left);

	       if (dial[i].zone == -2)
		  break;
	       else if (dial[i].zone != -3) {
		  if (dial[i].zone != remote.zone || dial[i].net   != remote.net ||
		      dial[i].node != remote.node || dial[i].point != remote.point) {
		     remote.zone  = dial[i].zone;
		     remote.net   = dial[i].net;
		     remote.node  = dial[i].node;
		     remote.point = dial[i].point;
		     nodetoinfo(&remote);
		     if (dial[i].speed) remote.speed = dial[i].speed;
		  }

		  if ((remote.nodeflags & DOWN) || !remote.phone[0]) {
		     if (remote.nodeflags & DOWN)
			message(3,"!Can't call %d:%d/%d%s",remote.zone,remote.net,remote.node,pointnr(remote.point));
		     else
			message(3,"!Address %d:%d/%d%s not listed",remote.zone,remote.net,remote.node,pointnr(remote.point));
		     dial[i].zone=-3;
		     left--;
		  }
		  else {
		     flag_session(true);
		     if (!flag_set(remote.zone,remote.net,remote.node,remote.point) ||
			 ((remote.nodeflags & VIAHOST) && !flag_set(remote.zone,remote.net,0,0)) ||
			 ((remote.nodeflags & VIABOSS) && !flag_set(remote.zone,remote.net,remote.node,0))) {
			flag_session(false);
			call_one++;
		     }
		     else {
			ctime = sendmail();
			flag_session(false);

			if (ctime == -28800L)
			   goto pend;

			if (ctime != 0) {
			   dial[i].zone = -3;
			   left--;
			}
			else
			   call_one++;
		     }
		  }
	       }
	   }

	   if (call_one) {	/* wait <avgwait> secs */
	      pickup = temp;
	      win_xyprint(schedule_win,8,5,"%d  ",left);
	      return;
	   }
    }

    win_xyputs(schedule_win,8,5,"0  ");
    polled_last = -1;

pend: pickup = temp;
}


/* ------------------------------------------------------------------------- */
void kill_poll (int zone, int net, int node, int point)
{
	char fname[128];
	char *p;
	struct stat f;

	sprint(fname,"%s%04.4x%04.4x%s.?LO",
	       holdarea(zone), (word) net, (word) node, holdpoint(point));
	for (p = ffirst(fname); p; p = fnext()) {
	    if (!getstat(p,&f) && !f.st_size) {
	       sprint(fname,"%s%04.4x%04.4x%s.%cLO",
		      holdarea(zone), (word) net, (word) node, holdpoint(point),
		      p[strlen(p) - 3]);
	       message(3,"KillPoll %s",fname);
	       /*unlink(fname);*/
	    }
	}
	fnclose();
}/*kill_poll()*/


/* ------------------------------------------------------------------------- */
long sendmail (void)			      /* call out for mail */
{
    char pstring[80];
    dword speed;			      /* speed for call */
    int succes; 			      /* call succesfull? */
    boolean didphone, hits, forcephone;
    long curgmt;
    long btime; 			      /* begin time of connection */
    word ptt_money;
    PHONE *pp;
    int zone, net, node, point;
    MAILSCRIPT *msp;
#if OS2
    FTPMAIL *mp;
    long ftpmail_client (FTPMAIL *mp);

    for (mp = ftpmail; mp; mp = mp->next) {
	if (remote.zone == mp->zone && remote.net   == mp->net &&
	    remote.node == mp->node && remote.point == mp->point) {
	   return (ftpmail_client(mp));
	}
    }
#endif

    curgmt = time(NULL) + gmtdiff;
    remote.capabilities = 0;		     /* don't know what he can (yet) */

    if (!(curgmt % 3LU) && ((strlen(VERSION) + 1) != sizeof (VERSION))) {
       message(0,"!Forget it, I don't work for hackers!");
       return (-1);
    }

    zone = remote.zone;
    net  = remote.net;
    if (remote.nodeflags & VIAHOST)
       node = point = 0;
    else {
       node = remote.node;
       if (remote.nodeflags & VIABOSS)
	  point = 0;
       else
	  point = remote.point;
    }

    if (remote.nodeflags & (VIAHOST | VIABOSS)) {
       statusmsg("Mailing %d:%d/%d for %d:%d/%d%s (%s)",
		 zone,net,node,
		 remote.zone,remote.net,remote.node,pointnr(remote.point),
		 remote.name);
       message(2,"+Mailing %d:%d/%d for %s (%d:%d/%d%s)",
		 zone,net,node,
		 remote.name,
		 remote.zone,remote.net,remote.node,pointnr(remote.point));
    }
    else {
       statusmsg("Mailing %d:%d/%d%s (%s)",
		 zone,net,node,pointnr(point),
		 remote.name);
       message(2,"+Mailing %s (%d:%d/%d%s)",
		 remote.name,
		 zone,net,node,pointnr(point));
    }

    strcpy(ourbbs.password,get_passw(zone,net,node,point));
    set_alias(zone,net,node,point);
    sprint(pstring,"%d:%d/%d%s",zone,net,node,pointnr(point));

    hits = didphone = forcephone = false;
doforce:
    for (pp = phone; pp; pp = pp->next) {
	if (zone == pp->zone && net   == pp->net &&
	    node == pp->node && point == pp->point) {
	   char *p, *q;

	   hits = true;

	   if (strpbrk(pp->number,":/")) {
	      int tzone, tnet, tnode, tpoint;

	      getaddress(pp->number,&tzone,&tnet,&tnode,&tpoint);
	      nlidx_getphone(tzone,tnet,tnode,tpoint,&q,&p);
	      if (!q) continue;
	      strcpy(remote.phone,q);
	      if (pp->nodeflags) p = pp->nodeflags;
	   }
	   else {
	      strcpy(remote.phone,pp->number);
	      p = pp->nodeflags ? pp->nodeflags : nlidx_getphone(zone,net,node,point,NULL,NULL);
	   }

	   if (!forcephone) {
	      if (p && !nlflag_isonline(zone,p,curgmt))
		 continue;

	      if (okdial) {
		 OKDIAL *dp;

		 for (dp = okdial; dp; dp = dp->next) {
		     if (patimat(remote.phone,dp->pattern) ||
			 (p && patimat(p,dp->pattern)))
			break;
		 }
		 if (!dp || !dp->allowed)
		    continue;
	      }
	   }

	   didphone = true;

	   if (p && isdigit(*p)) speed = atol(p);
	   else 		 speed = remote.speed;
	   if (speed < mdm.speed) speed = mdm.speed;
	   mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
	   memset(&clip,0,sizeof (CLIP));
	   arqlink = isdnlink = iplink = false;
	   setspeed(speed,false);
	   ptt_enable();
	   ptt_reset();
	   speed = dail(pstring,remote.phone,p);
	   if (speed == 255) {
	      if (check_kbmail(true,remote.zone,remote.net,remote.node,remote.point) > 0UL) {
		 message(3,"*Can't FreePoll - calling again");
		 mdm_reset();
		 speed = dail(pstring,remote.phone,p);
	      }
	      else {
		 message(3,"*FreePoll - removing empty poll");
		 kill_poll(remote.zone,remote.net,remote.node,remote.point);
		 break;
	      }
	   }
	   if (speed > 256) break;
	   if (!speed || (ptt_cost && ptt_units) ||
	       (!(ourbbsflags & NO_COLLIDE) && (speed & (MDMRSP_NODIAL | MDMRSP_COLLIDE))))
	      break;
	   mdm_reset();
	   ptt_disable();
	}
    }

    if (hits && !didphone) {
       forcephone = true;
       goto doforce;
    }

    if (!hits) {
       if (remote.speed < mdm.speed) speed = remote.speed;
       else			     speed = mdm.speed;
       mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
       memset(&clip,0,sizeof (CLIP));
       arqlink = isdnlink = iplink = false;
       setspeed(speed,false);
       ptt_enable();
       ptt_reset();
       speed = dail(pstring,remote.phone,
		    nlidx_getphone(zone,net,node,point,NULL,NULL));
       if (speed == 255) {
	  if (check_kbmail(true,remote.zone,remote.net,remote.node,remote.point) > 0UL) {
	     message(3,"*Can't FreePoll - calling again");
	     mdm_reset();
	     speed = dail(pstring,remote.phone,
			  nlidx_getphone(zone,net,node,point,NULL,NULL));
	  }
	  else {
	     message(3,"*FreePoll - removing empty poll");
	     kill_poll(remote.zone,remote.net,remote.node,remote.point);
	  }
       }
    }

    if (speed > 256) {				    /* really got a line? */
       btime = time(NULL) - ((isdnlink || iplink) ? 1L : 10L);	/* set begin time */

       for (msp = mailscript; msp; msp = msp->next) {
	   if ((*pstring && patimat(pstring,msp->pattern)) ||
		patimat(remote.phone,msp->pattern)) {
	      succes = run_script(msp->scriptptr,
				  msp->loginname ? msp->loginname : NULL,
				  msp->password ? msp->password : remote.password,
				  log_win);
	      break;
	   }
       }

       if (!msp || succes) {
	  alloc_msg("pre xmt session");
	  succes = xmt_session();		       /* and transmit mail! */
	  alloc_msg("post xmt session");
       }
       hangup();				    /* drop line	  */
       rm_requests();
       ptt_disable();

       btime = time(NULL) - btime;
#if PC
       if (ptt_cost) {
	  ptt_money = (word) (ptt_units * ptt_cost);
	  message(6,"$%s %ld:%02ld min., COST: %s%u.%02u (%u unit%s)",
		  succes ? "Connection for" : "Mail failed",
		  btime / 60L, btime % 60L,
		  ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
		  ptt_units, ptt_units == 1 ? "" : "s");
       }
       else
#endif
	  message(6,"+%s %ld:%02ld min.",
		  succes ? "Connection for" : "Mail failed",
		  btime / 60L, btime % 60L);
       if (cost != NULL) {
	  fprint(cost,"$D $aM $Y $t  Mailed %s (%d:%d/%d%s%s) %s %ld:%02ld min.",
		 remote.phone,
		 zone, net, node, pointnr(point), zonedomabbrev(zone),
		 succes ? "OK" : "FAIL", btime / 60L, btime % 60L);
#if PC
	  if (ptt_cost)
	     fprint(cost,", COST: %s%u.%02u (%u unit%s)",
		    ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
		    ptt_units, ptt_units == 1 ? "" : "s");
#endif
	  fprint(cost,"\n");
	  fflush(cost);
       }

       extra.called++;
       show_stats();
       mdm_aftercall();
       init_modem();

       return (succes ? btime : -(btime));	/* and return */
    }

    if (hits &&
	(speed == 255UL || (ptt_cost && ptt_units) ||
	 (ourbbsflags & NO_COLLIDE) || !(speed & (MDMRSP_NODIAL | MDMRSP_COLLIDE))))
       mdm_reset();
    statusmsg(NULL);
    ptt_disable();
    ptt_money = (word) (ptt_units * ptt_cost);

#if PC
    if (ptt_cost && ptt_units) {
       message(6,"$Charged no connect! COST: %s%u.%02u (%u unit%s)",
	       ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
	       ptt_units, ptt_units == 1 ? "" : "s");
       if (cost != NULL) {
	  fprint(cost,"$D $aM $Y $t  Charged no connect %s (%d:%d/%d%s%s), COST: %s%u.%02u (%u unit%s)\n",
		 remote.phone,
		 zone, net, node, pointnr(point), zonedomabbrev(zone),
		 ptt_valuta, ptt_money / 100, ptt_money % 100,
		 ptt_units, ptt_units == 1 ? "" : "s");
	  fflush(cost);
       }

       init_modem();
       return (-1L);				/* Save us from the PTT! */
    }
#endif

    if (speed != 255UL && (speed & MDMRSP_BADCALL)) {
       message(6,"$Charged no connect!");
       init_modem();
       return (-1L);				/* Save us from the PTT! */
    }

    if (speed != 255UL &&
	!(ourbbsflags & NO_COLLIDE) && (speed & (MDMRSP_NODIAL | MDMRSP_COLLIDE))) {
       if (speed == MDMRSP_NODIAL && time(NULL) > waitforline &&
	   !(e_ptrs[cur_event]->behavior & MAT_OUTONLY)) {
	  message(3,"*Call collision - answering immediately");
	  flag_session(false);
	  handle_inbound_mail(1);
       }
       else {
	  message(3,"*Possible call collision");
	  if (speed != MDMRSP_COLLIDE)
	     init_modem();
       }

       return (-28800L);
    }
    else
       init_modem();

    if (speed == 255UL) return (1L);

    return (speed ? 0L : -28800L);		/* no succes, no time */
}


/* ------------------------------------------------------------------------- */
int do_mail (int zone, int net, int node, int point)
{
     long ctime;			/* connection time */
    
     remote.zone  = zone;
     remote.net   = net;
     remote.node  = node;
     remote.point = point;

     if (nodetoinfo(&remote)) {
	if (!remote.phone[0] || (remote.nodeflags & DOWN)) {
	   message(3,"!Can't call %d:%d/%d%s",zone,net,node,pointnr(point));
	   return (2);			/* bad call code */
	}

	if (((remote.nodeflags & VIAHOST) && !flag_set(remote.zone,remote.net,0,0)) ||
	    ((remote.nodeflags & VIABOSS) && !flag_set(remote.zone,remote.net,remote.node,0)))
	   return (3);

	ctime = sendmail();
	if (ctime == -28800L) return (3);
	if (ctime <  0)       return (2);
	if (ctime == 0)       return (0);
	if (ctime >  0)       return (1);
     }

     message(3,"!Can't mail %d:%d/%d%s (not listed)",zone,net,node,pointnr(point));
     return (2);			/* bad call code */
}


/* end of tmailer.c */
