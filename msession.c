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


/* Mail session routines */
#include "zmodem.h"
#include "mail.h"


#define YOOHOO	0xf1			/* not known yet..     */
#define ENQ	0x05			/* ask for more info   */
#define NAK	0x15			/* Not AcKnoledge      */
#define TSYNC	0xae			/* signal mailer       */
#define EOT	0x04			/* End Of Transmission */
#define ETB	0x17			/* End of Filerequests */


boolean do_dialback (char *number)
{
	long t;
	int try;
	dword result;

	com_dump();
	script_outstr("\rDialback selected - please hang up and wait for call\r\n",NULL,NULL,0);
	script_outstr("Xenia will try for 2 minutes, then quit if unsuccessful\r\n",NULL,NULL,0);
	message(6,"+Dialback requested to %s",number);
	for (t = time(NULL) + 10; carrier() && time(NULL) < t; win_idle());
	com_dump();
	hangup();
	init_modem();

	for (try = 1, t = time(NULL) + 120; time(NULL) < t; try++) {
	    statusmsg("Dialback to %s (try %d)",number,try);
	    mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
	    memset(&clip,0,sizeof (CLIP));
	    arqlink = isdnlink = iplink = false;
	    setspeed(mdm.speed,false);
	    com_purge();
	    ptt_enable();
	    ptt_reset();
	    result = dail(number,number,NULL);
	    if (result > 256) {
	       ptt_disable();
	       message(6,"+Dialback succesful");
	       if (cost != NULL) {
		  word ptt_money = (word) (ptt_units * ptt_cost);

		  fprint(cost,"$D $aM $Y $t  Connected to %s (dialback)", number);
#if PC
		  if (ptt_cost)
		     fprint(cost,", COST: %s%u.%02u (%u unit%s)",
				 ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
				 ptt_units, ptt_units == 1 ? "" : "s");
#endif
		  fprint(cost,"\n");
		  fflush(cost);
	       }
	       return (true);
	    }
	    mdm_reset();
	    ptt_disable();
#if PC
	    if (ptt_cost && ptt_units) {
	       word ptt_money = (word) (ptt_units * ptt_cost);

	       message(6,"$Charged no connect! COST: %s%u.%02u (%u unit%s)",
			 ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
			 ptt_units, ptt_units == 1 ? "" : "s");
	       if (cost != NULL) {
		  fprint(cost,"$D $aM $Y $t  Charged no connect %s (dialback), COST: %s%u.%02u (%u unit%s)\n",
			      number, ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
			      ptt_units, ptt_units == 1 ? "" : "s");
		  fflush(cost);
	       }

	       return (false);
	    }
#endif

	    if (result & MDMRSP_BADCALL) {
	       message(6,"$Charged no connect!");
	       return (false);
	    }

	    if (!result) {
	       message(6,"-Dialback aborted");
	       return (false);
	    }
	}

	message(6,"-Dialback timeout");
	return (false);
}/*do_dialback()*/


int recv_session (void) 		/* start receive (mail) session */
{
    long t, t1; 			/* timerstuff */
    char buffer[128];			/* buffer for string to send */
    int ch, ch2;			/* character received */
    int ok;				/* flag for YOOHOO transmission */
    int i;				/* index in buffer */
    char *p;
    int login_tries = 0;		/* login tries (password) max 3 */
    char *logintext;
    long dialbacktime = 0L;
    char emsibuf[EMSI_IDLEN + 1];
    FILE *fp;
    EXTAPP   *ep;
    DIALBACK *dp;

    emsi_md5make();

    for (ep = extapp; ep; ep = ep->next) {
	if ((ep->flags & APP_MODEM) &&
	    period_active(&ep->per_mask) &&
	    strstr(mdmconnect,ep->string)) {
	   statusmsg("Incoming special connect");
	   extra.calls++;
	   show_stats();
	   got_arcmail = got_mail = got_bundle = got_fax = caller = 0;
	   extapp_handler(ep);
	   return (0);
	}
    }

    /* After extapp list, so modem extapps may override this one.... */
    if (strstr(mdmconnect,"+FCO")) {
       statusmsg("Incoming FAX call");
       message(2,"+FAX receive session");
       extra.calls++;
       show_stats();
       got_arcmail = got_mail = got_bundle = got_fax = caller = 0;
       fax_receive();					/* Sets got_fax flag */
       return (0);
    }

    if (hotstart) {
       mail_only = 1;
       statusmsg("Hotstart receive mailsession!");
       extra.calls++;
       show_stats();
    }
    else {
       extra.calls++;
       show_stats();
diddialback:
       statusmsg("%s data call", dialbacktime ? "Dialback" : "Incoming");

       ch = cmdlineanswer ? ' ' : EOF;
       t = timerset(20);
       while (ch == EOF && !timeup(t) && carrier()) {
	     if (com_scan()) {			   /* character received? */
		switch (ch = com_getbyte()) {	   /* then get it..	  */
		       case '\n':    /* LF */
		       case '\n'+128:
		       case '\r':    /* CR */
		       case '\r'+128:
		       case ' ':     /* space.. */
		       case ' '+128:
		       case 0x1b:    /* ESC */
		       case 0x1b+128:
		       case TSYNC:   /* TSYNC: can't do FTS-0001 in 7-bit */
		       case YOOHOO:  /* YOOHOO: can't do YooHOO in 7-bit  */
		       case ENQ:     /* ENQuiry: ditto */
		       case '*':     /* Start of an EMSI pkt */
		       case '*'+128:
			    break;

		       default:
			    ch = EOF;
			    break;
		}
	     }
	     else
		win_idle();
       }
    }

    if (!carrier()) {
       message(1,"-%s",msg_carrier);
       return (0);
    }

    set_alias(0,0,0,0);

    if (!mail_only) {
       for (ep = extapp; ep; ep = ep->next) {
	   if ((ep->flags & APP_BBS) && period_active(&ep->per_mask))
	      break;
       }
       mail_only = (ep == NULL);
    }
    logintext = mail_only ? noapptext : apptext;

    num_rem_adr = 0;
    memset (&remote,0,sizeof (struct _nodeinfo));
    inbound = filepath;
    got_arcmail = got_mail = got_bundle = got_fax = caller = 0;
    FTB_allowed = 0;
    if (FTB_dos) free(FTB_dos);
    FTB_dos = NULL;
    ourbbs.nrmail = ourbbs.kbmail = ourbbs.nrfiles = ourbbs.kbfiles = 0;
    sealink = 0;

    switch (hotstart) {
	   case 0x01: goto hot_tsync;	/* TSYNC   */
	   case 0x02: goto hot_yoohoo;	/* YOOHOO  */
	   case 0x04: goto hot_emsi;	/* EMSI    */
	   default:   break;		/* nothing */
    }

#if 0
    if (xenpoint_ver)
       sprint(buffer,"\r\r+ Xenia Point %s/%s; Address %d:%d/%d%s%s - ",
		     xenpoint_ver, xenpoint_reg,
		     ourbbs.zone, ourbbs.net, ourbbs.node,
		     pointnr(ourbbs.point), zonedomabbrev(ourbbs.zone));
    else
#endif
       sprint(buffer,"\r\r+ %s %s %s (%s); %s %d:%d/%d%s%s - Task %u",
		     PRGNAME, VERSION, XENIA_OS,
		     "OpenSource",
		     ourbbs.point ? "Point" : "Node",
		     ourbbs.zone, ourbbs.net, ourbbs.node,
		     pointnr(ourbbs.point), zonedomabbrev(ourbbs.zone),
		     task);
    strcat(buffer,"\r\n");
    script_outstr(buffer,NULL,NULL,0);	   /* tell the other side who we are */
    /*if (!(ourbbsflags & NO_EMSI))*/ {
       put_emsi(EMSI_MD5,true);
       put_emsi(EMSI_REQ,true);
    }
    script_outstr("\r+ Copyright (C) 1987-2001 Arjen G. Lentz\r\n",NULL,NULL,0);

    script_outstr("\r\n",NULL,NULL,0);

    sprint(buffer,"%sXENIA%u.BAN",homepath,task);
    if ((fp = sfopen(buffer,"rb",DENY_NONE)) == NULL) {
       sprint(buffer,"%sXENIA.BAN",homepath);
       fp = sfopen(buffer,"rb",DENY_NONE);
    }
    if (fp != NULL) {
       while (fgets(buffer,127,fp)) {
	     if ((p = strchr(buffer,'\032')) != NULL)
		*p = '\0';
	     script_outstr(buffer,NULL,NULL,0);
       }
       fclose(fp);
       script_outstr("\r\n",NULL,NULL,0);
    }
    else if (banner) {
       script_outstr(banner,NULL,NULL,0); /* also display more info */
       script_outstr("\r\n",NULL,NULL,0);
    }
    script_outstr(logintext,NULL,NULL,0);

again:
    if (++login_tries > 3)
       return (0);
    if (loginprompt)
       script_outstr(loginprompt,NULL,NULL,0);

    emsibuf[0] = '\0';
    i = 0;

    t = timerset(600);		     /* max. 1 min. for begin */

    switch (ch) {
	   case TSYNC:
	   case 0x1b:
	   case 0x1b + 128:
	   case YOOHOO:
	   case ENQ:
	   case '*':
	   case '*' + 128:
		break;

	   default:
		ch = EOF;
		break;
    }

    while (!timeup(t) && carrier()) {
	  if (ch != EOF || com_scan()) {
	     if (ch == EOF) ch = com_getbyte();

	     ch2 = ch & 0x7f;
	     if (ch2 == '\b' || ch2 == 127) {
		if (i > 0 && isloginchr(buffer[i-1])) {
		   buffer[--i] = '\0';
		   if (loginprompt)
		      script_outstr("\b \b",NULL,NULL,0);
		}
	     }
	     else {
		buffer[i++] = toupper(ch2);
		buffer[i]   = '\0';
		if (loginprompt && isloginchr(ch2))
		   com_putbyte(ch2);
	     }

	     if (!hotstart && !dialbacktime) {
		for (dp = dialback; dp; dp = dp->next) {
		    if ((dp->flags & (APP_MAIL | APP_BBS)) &&
			period_active(&dp->per_mask) &&
			strstr(buffer,dp->string)) {
		       if (loginprompt)
			  script_outstr("\r\n",NULL,NULL,0);
		       if (dp->password && dp->password[0] && !password_login(dp->password)) {
			  ch = EOF;
			  goto again;
		       }
		       if (!do_dialback(dp->number))
			  return (0);
		       dialbacktime = time(NULL);
		       goto diddialback;
		    }
		}
	     }

	     for (ep = extapp; ep; ep = ep->next) {
		 if ((ep->flags & (APP_MAIL | APP_BBS)) &&
		     period_active(&ep->per_mask) &&
		     strstr(buffer,ep->string)) {
		    if (loginprompt)
		       script_outstr("\r\n",NULL,NULL,0);
		    if (ep->password && ep->password[0] && !password_login(ep->password)) {
		       ch = EOF;
		       goto again;
		    }
		    extapp_handler(ep);
		    if (dialbacktime) {
		       long t = time(NULL) - dialbacktime;

		       message(6,"$Dialback duration %ld:%02ld min.", t / 60L, t % 60L);
		       if (cost != NULL) {
			  fprint(cost,"$D $aM $Y $t  Dialback duration %ld:%02ld min.\n", t / 60L, t % 60L);
			  fflush(cost);
		       }

		       hangup();
		       mdm_aftercall();
		    }
		    return (0);
		 }
	     }

	     if (i > 80) {
		strcpy(buffer,&buffer[30]);
		i -= 30;
	     }

	     /*if (!(ourbbsflags & NO_EMSI))*/ {
		switch (check_emsi(ch,emsibuf)) {
		       case EMSI_M5R:
			    put_emsi(EMSI_MD5,true);
			    break;

		       case EMSI_INQ:
hot_emsi:		    got_msession = 1;
			    flag_session(true);
			    com_dump();
			    mailerwinopen();
			    show_mail();
			    if (recv_emsi(true)) {
			       extra.mailer++;
			       show_stats();
			       ch = WaZOO(0,1);
			       win_close(file_win);
			       win_close(mailer_win);
			       flag_session(false);
			       return (ch);
			    }
			    win_close(file_win);
			    win_close(mailer_win);
			    flag_session(false);
			    return (0);

#if 0
		       case EMSI_CLI:
			    if (carrier() && !mail_only)
			       return (startbbs(0));
			    break;
#endif

		       default:
			    break;
		}
	     }

	     switch (ch) {
		    case '\r':
		    case '\r'+128:
		    case '\n':
		    case '\n'+128:
		    case ' ':
		    case ' '+128:
			 if (loginprompt)
			    script_outstr("\r\n",NULL,NULL,0);
			 /*if (!(ourbbsflags & NO_EMSI))*/ {
			    put_emsi(EMSI_MD5,true);
			    put_emsi(EMSI_REQ,true);
			 }
			 script_outstr("\r",NULL,NULL,0);
			 script_outstr(logintext,NULL,NULL,0);
			 if (loginprompt)
			    script_outstr(loginprompt,NULL,NULL,0);
			 break;

		    case YOOHOO: /* Intelligent mailer! */
found_yoohoo:		 /* first check if we wanna be smart too! */
			 if (ourbbsflags & NO_SMART_MAIL) break;
hot_yoohoo:		 got_msession = 1;
			 flag_session(true);
			 com_dump();
			 mailerwinopen();
			 show_mail();
			 if ((ok = get_YOOHOO(1)) > 0) {
			    extra.mailer++;
			    show_stats();
			    ch = WaZOO(0,0);
			    win_close(file_win);
			    win_close(mailer_win);
			    flag_session(false);
			    return (ch);
			 }
			 else if (!ok) {
			    win_close(file_win);
			    win_close(mailer_win);
			    flag_session(false);
			    return (0);
			 }
			 win_close(file_win);
			 win_close(mailer_win);
			 flag_session(false);
			 break;

		    case TSYNC:  /* looks like a mailer! */
hot_tsync:		 /* test for other char in 2 secs. (YOOHOO) */
			 com_dump();
			 com_purge();
			 t1=timerset(20);
			 while (carrier() && !timeup(t1)) {
			       if (com_scan()) {
				  ch = com_getbyte();
				  if (ch == YOOHOO && !(ourbbsflags & NO_SMART_MAIL))
				     goto found_yoohoo;
			       }
			 }
			 got_msession = 1;
			 flag_session(true);
			 extra.mailer++;
			 show_stats();
			 mailerwinopen();
			 show_mail();
			 if (carrier()) {
			    ch = FTSC_receiver(0);
			    win_close(file_win);
			    win_close(mailer_win);
			    flag_session(false);
			    return (ch);
			 }
			 else {
			    win_close(file_win);
			    win_close(mailer_win);
			    flag_session(false);
			    return (0);
			 }

		    default:
			 break;
	     } /* end-switch */

	     ch = EOF;
	  }
	  else
	     win_idle();
    }

    return (0);
}

int xmt_session (void)			    /* start of outbound mail.. */
{
    long t, t1; 			    /* timerstuff */
    int ch;				    /* multiple purpose... */
    char buffer[128];			    /* buffer for received text */
    char c;				    /* character */
    char got=0; 			    /* flag.. */
    int count=0;			    /* guess, would it be a counter? */
    char got_emsi = 0;			    /* emsi flag */
    char emsibuf[EMSI_IDLEN + 1];

    emsi_md5make();
    emsibuf[0] = '\0';

    caller = 1;
    num_rem_adr = 0;
    remote.options = 0;
    inbound = ourbbs.password[0] ? safepath : filepath;
    got_mail = got_arcmail = got_bundle = got_fax = 0;
    remote.kbmail = remote.nrmail = remote.kbfiles = remote.nrfiles = 0;
    ourbbs.nrmail = ourbbs.kbmail = ourbbs.nrfiles = ourbbs.kbfiles = 0;
    sealink = 0;

    t = timerset(300);			    /* 30 sec. max */

    message(-1,"+Rousing remote");
    while (!timeup(t) && carrier()) {
	  com_putbyte('\r'); com_putbyte(' ');
	  com_flush();
	  t1 = timerset(10);
	  while (!timeup(t1)) {
		if (com_scan()) {
		   c = com_getbyte();
		   if (c == '\r')  {	    /* got return?	       */
		      if (got) goto athome; /* got string? Ok	       */
		   }
		   else if (c > 31) {	    /* valid char received?    */
		      buffer[count++] = c;  /* store it 	       */
		      if (count > 80)	   /* too many chars?	      */
			 count = 80;
		      t1 = timerset(10);
		      got = 1;		    /* signal we got something */

		      /*if (!(ourbbsflags & NO_EMSI))*/ {
			 switch (check_emsi(c,emsibuf)) {
				case EMSI_MD5:
				     emsi_md5get(&emsibuf[2]);
				     got_emsi = 1;
				     goto athome;

				case EMSI_REQ:
				     if (ourbbsflags & NO_EMSI)
					break;
				     got_emsi = 1;
				     goto athome;

				default:
				     break;
			 }
		      }
		   }
		}
		else
		   win_idle();
	  }
    }

    if (!carrier()) message(2,"-%s",msg_carrier);
    else	    message(2,"-Still sleeping");
    return (0); 			    /* no one at home..     */

athome: 				    /* geee, they woke up.. */
    com_putbyte(0x0b);			    /* stop output for Opus */
    if (c == '\r' && count) {
       buffer[count] = '\0';		    /* terminate string     */
       message(3,"+Called: %s",buffer);     /* and display	    */
    }

    mailerwinopen();
    show_mail();
    win_xyprint(mailer_win,2,1,"Connected to %d:%d/%d%s",
		remote.zone, remote.net, remote.node, pointnr(remote.point));
    win_print(mailer_win," as %d:%d/%d%s",
	       ourbbs.zone, ourbbs.net, ourbbs.node, pointnr(ourbbs.point));

    if (!got_emsi) {
       t1 = timerset(200);
       while (!timeup(t1)) {		    /* wait for .5 sec timeout */
	     if (!carrier()) {
		message(2,"-%s",msg_carrier);
		win_close(file_win);
		win_close(mailer_win);
		return (0);
	     }

	     ch = com_getc(5);
	     if (ch == EOF) break;
	     /*if (!(ourbbsflags & NO_EMSI))*/ {
		switch (check_emsi(ch,emsibuf)) {
		       case EMSI_MD5:
			    emsi_md5get(&emsibuf[2]);
			    got_emsi = 1;
			    break;

		       case EMSI_REQ:
			    if (ourbbsflags & NO_EMSI)
			       break;
			    got_emsi = 1;
			    break;

		       default:
			    break;
		}
		if (got_emsi) break;
	     }
       }

       if (!got_emsi && ch != EOF) {
	  message(1,"-Receiving only garbage");
	  win_close(file_win);
	  win_close(mailer_win);
	  return (0);
       }
    }

    message(1,"+They're awake");

    if (got_emsi) goto do_emsi;

    t = timerset(300);			    /* now get them to mail */

    while (carrier() && (!timeup(t))) {
	  if (!(ourbbsflags & NO_EMSI)) {
	     put_emsi(EMSI_INQ,false);	    /* send EMSI_INQ twice */
	     put_emsi(EMSI_INQ,false);
	  }
	  if (!(ourbbsflags&NO_SMART_MAIL))
	     com_putbyte(YOOHOO);
	  com_putbyte(TSYNC);		    /* send TSYNC - not 2x for Fido  */
	  com_putbyte('\r');

	  t1 = timerset(10);		    /* and wait 2 sec. before resend */
	  while (!timeup(t1)) {
		if (com_scan()) {	    /* received something? */
		   ch = com_getbyte();	    /* get it */

		   /*if (!(ourbbsflags & NO_EMSI))*/ {
		      switch (check_emsi(ch,emsibuf)) {
			     case EMSI_MD5:
				  emsi_md5get(&emsibuf[2]);
				  /* don't wait for an EMSI_REQ, start fast */
				  goto do_emsi;

			     case EMSI_REQ:
				  if (ourbbsflags & NO_EMSI)
				     break;

do_emsi:			  put_emsi(EMSI_INQ,true);
				  if (send_emsi()) {
				     ch = WaZOO(1,1);
				     win_close(file_win);
				     win_close(mailer_win);
				     return (ch);
				  }
				  win_close(file_win);
				  win_close(mailer_win);
				  return (0);
			     
			     default:
				  break;
		      }
		   }

		   switch (ch) {
			  case ENQ:
			       if (ourbbsflags & NO_SMART_MAIL)
				  break;

			       if (send_YOOHOO()) {
				  ch = WaZOO(1,0);
				  win_close(file_win);
				  win_close(mailer_win);
				  return (ch);
			       }
			       else {
				  win_close(file_win);
				  win_close(mailer_win);
				  return (0);
			       }

			  case 0x00:
			  case 0x01:
			  case NAK:		       /* old fashioned mail */
			  case 'C':
			       if (c==NAK || c=='C') {
				  ch = FTSC_sender(0);
				  win_close(file_win);
				  win_close(mailer_win);
				  return (ch);
			       }
			       break;

			  case 0xfe:
			  case 0xff:
			       if (c == (0x100 - ch)) {
				  ch = FTSC_sender(0);
				  win_close(file_win);
				  win_close(mailer_win);
				  return (ch);
			       }
		   }

		   if (ch != -1)
		      c = ch;
		}
		else
		   win_idle();
	  }/*end of TSYNC while*/
    }/*end of wait for reaction!*/

    message(2,"-They threw me out");
    win_close(file_win);
    win_close(mailer_win);
    return (0);
}


FTSC_sender(int smart)				/* Send mail */
/*int smart;					  /+ old fashioned way? */
{
    long t;					/* timerstuff */
    int ch;					/* character from line */

    if (!smart) message(1,"+Start of outbound mail");

    if (!FTSC_sendmail(smart))
    {
	message(3,"-Mail went wrong");
	return 0;
    }
    message(-1,"+Waiting for receiver");

    t=timerset(200);			     /* wait 20 secs for receiver */
    while (!timeup(t) && carrier())
    {
	if (com_scan())
	{
	    ch=com_getbyte();
	    switch(ch)
	    {
	      case TSYNC:   com_purge();    /* receiver got mail for us! */
			    if (!FTSC_recvmail(smart)) goto error_out;
			    t=timerset(100);
			    break;

	      case 0x16:    if (!smart) {
			       com_purge(); /* file-request */
			       SEA_recvreq();
			       t=timerset(100);
			    }
			    break;

	      case ENQ:     if (!smart) {
			       com_purge(); /* file-request */
			       SEA_sendreq();
			    }
			    break;

	      case 'C':
	      case NAK:     com_getc(1);    /* purge blocknumbers */
			    com_getc(1);
			    com_putbyte(EOT);	/* send EOT */
			    com_flush();
			    t=timerset(100);
			    break;

	      case 0xbe:    /* We asked for a BBS session! */
			    com_putbyte('\r');	/* first ACK it */
			    com_putbyte('\r');
			    com_flush();
			    alarm();
			    Term(1);
			    t=timerset(10);
			    break;

	      default:
			    com_putbyte(EOT);	/* send EOT */
			    com_flush();
	    }
	}
	else
	    win_idle();
    }

    if (!carrier())
    {
	message(2,"=They dropped the line");
	tdelay(10);
	com_purge();
	return 1;
    }

    if (timeup(t))
    {
	message(2,"=They fell asleep");
	return 1;
    }

    tdelay(10); 	    /* wait a second */
    if (!smart) message(2,"=Mail done");

#if 0
    if (xenpoint_ver) {
       int  fd;
       char buffer[128];

       sprint(buffer,"%s\\%02.2z%03.3z%03.3z.OK", hold,
	      (word) rem_adr[0].zone, (word) rem_adr[0].net,
	      (word) rem_adr[0].node);
       if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT,DENY_NONE)) >= 0)
	  dos_close(fd);
    }
#endif

    return (1);

error_out:
    tdelay(10);
    message(2,"-Error during session.");

    return (0);
}

FTSC_receiver(int smart)		/* receive mail */
/*int smart;				  /+smart mailer? */
{
    char name[256];			/* buffer for filenames */
    long t, t1; 			/* timerstuff */
    int temp;				/* temp. storage */
    char *found;			/* found file (ffirst) */
    char ch;				/* character received from modem */

    if (!smart) message(3,"+Start receiving mail");

    com_purge();

    temp = pickup;			/* save status of pickup */
    pickup = 1;

    if (!FTSC_recvmail(smart)) {
       pickup = temp;
       if (!smart) message(2,"-Mailpacket went wrong");
       return (0);
    }

    pickup = temp;

    if (flag_set(remote.zone,remote.net,remote.node,remote.point)) {
       sprint(name,"%s%04.4x%04.4x%s.?UT", holdarea(remote.zone),
		   (word) remote.net, (word) remote.node,
		   holdpoint(remote.point));
       found = ffirst(name);
       fnclose();

       if (!found) {
	  sprint(name,"%s%04.4x%04.4x%s.?LO", holdarea(remote.zone),
		      (word) remote.net, (word) remote.node,
		      holdpoint(remote.point));
	  found = ffirst(name); 	/* outbound files? */
	  fnclose();
       }
    }

    if (!found && remote.pointnet &&
	flag_set(remote.zone,remote.pointnet,remote.point,0)) {

       sprint(name,"%s%04.4x%04.4x.?UT", holdarea(remote.zone),
		   (word) remote.pointnet, (word) remote.point);
       found = ffirst(name);
       fnclose();

       if (!found) {
	  sprint(name,"%s%04.4x%04.4x.?LO", holdarea(remote.zone),
		  (word) remote.pointnet, (word) remote.point);
	  found = ffirst(name);
	  fnclose();
       }

       if (!found && !(e_ptrs[cur_event]->behavior & MAT_NOOUTREQ)) {
	  sprint(name,"%s%04.4x%04.4x.REQ", holdarea(remote.zone),
		      (word) remote.pointnet, (word) remote.point);
	  found = ffirst(name);
	  fnclose();
       }
    }

    if (!found) {
       sprint(name,"%sXENREQ%02x.*",inbound,task);
       found = ffirst(name);
       fnclose();
    }

    if (!found) {
       message(2,"+Nothing to send to %d:%d/%d%s%s",
		 remote.zone, remote.net, remote.node,
		 pointnr(remote.point), zonedomabbrev(remote.zone));
    }
    else {
       message(2,"+Mail for %d:%d/%d%s%s",
		 remote.zone, remote.net, remote.node,
		 pointnr(remote.point), zonedomabbrev(remote.zone));
       t = timerset(300);		/* 30 secs max */
       temp = 0;

       while (!timeup(t) && carrier() && !temp) {
	     com_putbyte(TSYNC);	/* TSYNC out */
	     com_flush();
	     t1 = timerset(20); 	/* 2 secs max. */

	     while (!timeup(t1) && carrier() && !temp) {
		   ch = com_getbyte();
		   if (ch == NAK || ch == 'C') {
		      temp = 1;
		      if (!FTSC_sendmail(smart)) {
			 message(2,"-End of Mail");
			 return (0);
		      }
		   }
	     }
	}
    }

    if (!smart)
       do_request();

    if (!smart) message(2,"=Mail done");

#if 0
    if (xenpoint_ver) {
       int  fd;
       char buffer[128];

       sprint(buffer,"%s\\%02.2z%03.3z%03.3z.OK", hold,
	      (word) rem_adr[0].zone, (word) rem_adr[0].net,
	      (word) rem_adr[0].node);
       if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT,DENY_NONE)) >= 0)
	  dos_close(fd);
    }
#endif

    return (1);
}


void do_request()
{
    char name[256];			/* buffer for filenames */
    long t, t1; 			/* timerstuff */
    int temp;				/* temp. storage */
    char ch;				/* character received from modem */

    /* Now see if we want to request anything */

    name[0] = '\0';

    if (!(e_ptrs[cur_event]->behavior & MAT_NOOUTREQ)) {
       if (flag_set(remote.zone,remote.net,remote.node,remote.point)) {
	  sprint(name, "%s%04.4x%04.4x%s.REQ", holdarea(remote.zone),
		 (word) remote.net, (word) remote.node,
		 holdpoint(remote.point));

	  if (fexist(name)) goto fnd;
       }

       if (remote.pointnet && flag_set(remote.zone,remote.pointnet,remote.point,0)) {
	  sprint(name,"%s%04.4x%04.4x.REQ", holdarea(remote.zone),
		      (word) remote.pointnet, (word) remote.node);
	  if (fexist(name)) goto fnd;
       }
    }

    if (name[0])
    {
	/* we do so send SYN and wait */
fnd:
	t = timerset(300);		/* set 30 second timeout */
	temp = 0;
	while (!timeup(t) && carrier() && !temp) /*till all done or no carrier*/
	{
	    com_putbyte(SYN);
	    com_flush();

	    t1 = timerset (30);
	    while (carrier() && (!timeup(t1)) && !temp)
	    {
		ch = com_getbyte();

		switch (ch)
		{
		  case ENQ:  SEA_sendreq();
		  case CAN:  temp = 1;
			     break;
		  case 'C':
		  case NAK:  com_putbyte(EOT);
			     com_flush();
			     break;
		}
	    }
	}
    }

    /* Finally, can he request anything from us */
    SEA_recvreq ();
}

FTSC_sendmail(int smart)
{
    FILE *fd;				    /* file(stuf) */
    char name[80], aname[20];		    /* filename */
    struct dostime_t t; 		    /* time */
    struct dosdate_t d; 		    /* and date */
    int i;				    /* simple counter */
    struct _pkthdr *phdr;		    /* space for packetheader */
    int made=0; 			    /* we made a dummy? */

    if (!smart && !num_rem_adr) {
       rem_adr[0].zone	   = remote.zone;
       rem_adr[0].net	   = remote.net;
       rem_adr[0].node	   = remote.node;
       rem_adr[0].point    = remote.point;
       rem_adr[0].pointnet = remote.pointnet;
       num_rem_adr = 1;
    }

    if ((phdr = (struct _pkthdr *) malloc(sizeof (struct _pkthdr))) == NULL) {
       message(6,"!Mem error");
       return (0);
    }
    memset(phdr,0,sizeof (struct _pkthdr));

    for (i = 0; i < N_FLAGS; i++) {
	if (caller &&
	    (ext_flag[i] == 'H' || ext_flag[i] == 'W' ||
	     (ext_flag[i] == 'Q' && (ourbbsflags & NO_REQ_ON))))
	   continue;

	if (ext_flag[i] == 'N') continue;

	if (!flag_set(remote.zone,remote.net,remote.node,remote.point))
	   continue;
	sprint(name,"%s%04.4x%04.4x%s.%cUT",
		    holdarea(remote.zone), (word) remote.net,
		    (word) remote.node, holdpoint(remote.point),
		    (short) ext_flag[i]);
	if (fexist(name)) break;
    }

    if (caller && (remote.nodeflags & VIAHOST))
       message(2,"+Passing bundle to %d:%d/0 for %d:%d/%d%s",
		 remote.zone,remote.net,
		 remote.zone,remote.net,remote.node,pointnr(remote.point));
    else if (caller && (remote.nodeflags & VIABOSS))
       message(2,"+Passing bundle to %d:%d/%d for %d:%d/%d%s",
		 remote.zone,remote.net,remote.node,
		 remote.zone,remote.net,remote.node,pointnr(remote.point));
    else {
       message(2,"+Passing bundle to %d:%d/%d%s%s",
		 rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		 pointnr(rem_adr[0].point), zonedomabbrev(rem_adr[0].zone));
       strcpy(ourbbs.password,get_passw(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point));
       if (!caller && !smart) {
	  set_alias(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point);
	  statusmsg("Incoming call from %d:%d/%d%s%s",
		    rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		    pointnr(rem_adr[0].point), zonedomabbrev(rem_adr[0].zone));
       }
    }

    if (!smart && caller && ourbbs.password[0])
       message(2,"+Password protected session");

    if (i == N_FLAGS) { 			       /* no packet exists ? */
       made++;

       sprint(name,"%s\\DUMMY%u.OUT",hold,task); /* no, so make dummy packet */

       if ((fd = sfopen(name,"wb",DENY_WRITE)) == NULL) {
	  message(6,"!Could not open dummy packet!");
	  free(phdr);
	  return (0);
       }

       phdr->ph_ozone1 = phdr->ph_ozone2 = (short) inteli(ourbbs.zone);
       if (!ourbbs.point || ourbbs.usepnt == 2) {
	  phdr->ph_onet    = (short) inteli(ourbbs.net);
	  phdr->ph_auxnet  = inteli(0);
#if 0	  /* Enable this for FSC-0048 instead of FSC-0039 */
	  if (ourbbs.point) {
	     phdr->ph_onet   = inteli(-1);
	     phdr->ph_auxnet = inteli(ourbbs.net);
	  }
#endif
	  phdr->ph_onode   = (short) inteli(ourbbs.node);
	  phdr->ph_opoint  = (short) inteli(ourbbs.point);
	  phdr->ph_cwd	   = inteli(1);      /* CapWord: can handle type 2+ */
	  phdr->ph_cwdcopy = inteli(256);    /* CapWordCopy = byteswapped   */
       }
       else if (ourbbs.point) {
	  if (remote.zone == ourbbs.zone && remote.net == ourbbs.net &&
	      (remote.node == ourbbs.node || ourbbs.usepnt == 1)) {
	     phdr->ph_onet  = (short) inteli(ourbbs.pointnet);
	     phdr->ph_onode = (short) inteli(ourbbs.point);
	  }
	  else {
	     phdr->ph_onet  = (short) inteli(ourbbs.net);
	     phdr->ph_onode = inteli(-1);
	  }
       }

       _dos_getdate(&d);
       _dos_gettime(&t);
       phdr->ph_yr     = (short) inteli(d.year);
       phdr->ph_mo     = (short) (inteli(d.month) - 1);
       phdr->ph_dy     = inteli(d.day);
       phdr->ph_hr     = inteli(t.hour);
       phdr->ph_mn     = inteli(t.minute);
       phdr->ph_sc     = inteli(t.second);
       phdr->ph_rate   = (short) inteli(cur_speed < 65535UL ? (word) cur_speed : 9600);
       phdr->ph_ver    = inteli(2);
       phdr->ph_prodh  = 0;
       phdr->ph_prodl  = isXENIA;
       phdr->ph_revh   = XEN_MAJ;
       phdr->ph_revl   = XEN_MIN;
       phdr->ph_dzone1 = phdr->ph_dzone2 = (short) inteli(rem_adr[0].zone);
       phdr->ph_dnet   = (short) inteli(rem_adr[0].net);
       phdr->ph_dnode  = (short) inteli((remote.nodeflags & VIAHOST) ? 0 : rem_adr[0].node);
       phdr->ph_dpoint = (short) inteli((remote.nodeflags & (VIAHOST | VIABOSS)) ? 0 : rem_adr[0].point);
       strncpy(phdr->ph_pwd,ourbbs.password,8);
       /* *(long *) phdr->ph_spec = intell(xenkey->serial); */

       if (fwrite((char *)phdr, sizeof(struct _pkthdr), 1, fd) != 1) {
	  message(6,"!Error while writing dummy packet!");
	  fclose(fd);
	  free(phdr);
	  return (0);
       }
       fwrite("\0\0", 2, 1, fd);
       fclose(fd);
    }

    free(phdr);

    xmodem=modem=telink=0;		    /* select SEAlink mode */

    com_purge();			    /* discard all input */

    mail=1;

    invent_pktname(aname);		    /* make packetname */
    if ((i=xmtfile(name,aname))==0)
    {
	message(2,"-Fatal error during Mailtransfer");
	if (made) unlink(name); 	    /* if dummy, then delete it */
	mail=0;
	return 0;
    }
    remote.nrmail++;
    remote.kbmail+= kbtransfered;
    show_mail();

    mail=0;

    if (i<0)
    {
	message(2,"=They refuse our mail");
	if (made) unlink(name); 	    /* delete dummy (not real!) */
	return 1;
    }

    unlink(name);	    /* send OK, delete it */

    /* attached files and requests are up now.... */

    if (!sealink)				/* select correct protocol */
    {
	telink=xmodem=1;
	modem=!smart;
    }

    message(1,"+Attached files");

    wzgetfunc  = batchdown;
    wzsendfunc = send_SEA;

    if (smart) {
       if (!send_WaZOO(1)) return 0;
    }
    else {
       if (SendFiles(0) < 0) return 0;

       if (!caller) {
	  if (process_req_files(0) < 0) return (0);
       }
    }

    end_batch();			    /* end of the files */

    return 1;				    /* OK */
}

FTSC_recvmail(int smart)
{
    char name[80];			    /* filename */
    char badname[80];			    /* for renaming bad packet */
    char *p;				    /* workhorse */
    int  i;				    /* yep, workhorse also */
    char adrbuf[128];

    message(2,"+Receiving mail");

    if (!pickup) {
       message(2,"-Pickup turned off - refusing mail");
       com_putbyte(CAN);	   /* CANcel */
       com_putbyte(CAN);
       com_flush();
       return (1);
    }

    message(-1,"+Inbound mail");

    wzgetfunc = batchdown;
    invent_pktname(name);			/* make packetname */
    com_purge();

    modem=telink=xmodem=0;			/* go to sealink mode */
    mail=1;

    if ((p=rcvfile(inbound,name)) == NULL || !*p)
    {
	message(2,"-Mail went wrong");
	mail=0;
	return 0;
    }

    extra.nrmail++;
    extra.kbmail += (word) kbtransfered;
    show_stats();
    ourbbs.nrmail++;
    ourbbs.kbmail += kbtransfered;
    show_mail();

    got_mail=got_bundle=1;
    mail=0;

    if (!smart) {
       int pointnet;

       remap(&found_zone,&found_net,&found_node,&found_point,&pointnet);
       if (!num_rem_adr) {
	  rem_adr[0].zone     = found_zone;
	  rem_adr[0].net      = found_net;
	  rem_adr[0].node     = found_node;
	  rem_adr[0].point    = found_point;
	  rem_adr[0].pointnet = pointnet;
	  num_rem_adr = 1;
       }
       if (!remote.net) {
	  remote.zone	  = rem_adr[0].zone;
	  remote.net	  = rem_adr[0].net;
	  remote.node	  = rem_adr[0].node;
	  remote.point	  = rem_adr[0].point;
	  remote.pointnet = rem_adr[0].pointnet;
       }

       flag_set(remote.zone,remote.net,remote.node,remote.point);
       if (caller) {
	  if (found_zone != remote.zone || found_net != remote.net ||
	      ((remote.nodeflags & VIAHOST) && found_node != 0) ||
	      ((remote.nodeflags & VIABOSS) && found_point != 0) ||
	      found_node != remote.node || found_point != remote.point)
	     message(2,"=Got %d:%d/%d%s when calling %d:%d/%d%s",
		       found_zone,found_net,found_node,pointnr(found_point),
		       remote.zone,remote.net,remote.node,pointnr(remote.point));

	  sprint(adrbuf,"%d:%d/%d%s%s",
			remote.zone, remote.net, remote.node,
			pointnr(remote.point),zonedomabbrev(remote.zone));
	  last_set(&last_out, adrbuf);
	  show_stats();
       }
       else {
	  sprint(adrbuf,"%d:%d/%d%s%s",
			remote.zone, remote.net, remote.node,
			pointnr(remote.point),zonedomabbrev(remote.zone));
	  last_set(&last_in, adrbuf);
	  show_stats();
	  if (callerid[0])
	     write_clip("","",adrbuf,"");
	  message(2,"=Address: %s", adrbuf);

	  strcpy(ourbbs.password,get_passw(remote.zone,remote.net,remote.node,remote.point));
	  set_alias(remote.zone,remote.net,remote.node,remote.point);
	  win_xyprint(mailer_win,2,1,"Connected to %d:%d/%d%s",
		      remote.zone,remote.net,remote.node,pointnr(remote.point));
	  win_print(mailer_win," as %d:%d/%d%s",
		    ourbbs.zone,ourbbs.net,ourbbs.node,pointnr(ourbbs.point));
	  win_clreol(mailer_win);

	  nodetoinfo(&remote);		/* Try to find system & sysop name.. */
       }

       if (remote.name[0]) {
	  win_xyputs(mailer_win,10,2,remote.name);
	  message(2,"=Name: %s",remote.name);
       }
       if (remote.sysop[0]) {
	  win_xyputs(mailer_win,10,3,remote.sysop);
	  message(2,"=Sysop: %s",remote.sysop);
       }

       if (ourbbs.password[0]) {
	  if (strnicmp(ourbbs.password,found_pwd,6)) {
	     message(6,"!Password mismatch '%s' <> '%s'",
		       ourbbs.password,found_pwd);
#if 0 /* apparently (after all these years!) FTS-1 has no pwd.prot sessions? */
	     if (!caller) {
		sprint(badname,"%s%s",inbound,name);
		strcpy(name,badname);
		i = (int) strlen(badname) - 3;
		strcpy(&badname[i],"bad");
		unique_name(badname);
		if (rename(name,badname))
		   message(6,"!Error renaming %s to %s",name,badname);
		got_mail = got_bundle = 0;

		return (0);
	     }
#endif
	  }
	  else if (caller || found_pwd[0]) {
	     message(2,"+Password protected session");
	     inbound = safepath;
	  }
       }
       else if (found_pwd[0])
	  message(2,"=Remote proposes password '%s'",found_pwd);
    }

    message(2,"+Attached files");

    if (!sealink)				/* other side used sealink? */
    {
	telink=1;
	modem=!smart;
    }

    if (!batchdown())				/* get the files */
    {
	message(2,"-error in files");
	return 0;
    }

    message(2,"=Files done");
    return 1;
}

WaZOO(int call, int emsi)
{
    int  stat = 0;		/* status of xfer */
    long t;			/* timer	  */
    int  ch;			/* character	  */
    int  i;
#if 0
    int  fd;
#endif
    char buffer[256];
    static char *BBSnow = "\36++to BBS++\36";
    char *p = BBSnow;
    EXTAPP *ep;
    extern boolean did_skip;

    got_arcmail = got_mail = got_bundle = got_fax = 0;

    caller = call;
    did_skip = false;

    if (caller) {
       for (i = 0; i < num_rem_adr; i++) {
	   if (rem_adr[i].zone == remote.zone && rem_adr[i].net   == remote.net &&
	       rem_adr[i].node == remote.node && rem_adr[i].point == remote.point)
	      break;
       }
       if (i >= num_rem_adr) {		/* Didn't see the address we dialed? */
	  rem_adr[num_rem_adr].zone	= remote.zone; /* Add dialed to list */
	  rem_adr[num_rem_adr].net	= remote.net;
	  rem_adr[num_rem_adr].node	= remote.node;
	  rem_adr[num_rem_adr].point	= remote.point;
	  rem_adr[num_rem_adr].pointnet = remote.pointnet;
	  num_rem_adr++;
       }
    }
    else {						   /* They called us */
       if (!num_rem_adr) {     /* Didn't we get any address from handshake!? */
	  message(6,"-Remote presented no valid address");
	  rem_adr[0].zone     = ourbbs.zone;
	  rem_adr[0].net      = -1;
	  rem_adr[0].node     = -1;
	  rem_adr[0].point    = 0;
	  rem_adr[0].pointnet = 0;
	  num_rem_adr = 1;
       }

       remote.zone     = rem_adr[0].zone;
       remote.net      = rem_adr[0].net;
       remote.node     = rem_adr[0].node;
       remote.point    = rem_adr[0].point;
       remote.pointnet = rem_adr[0].pointnet;
    }

    for (i = 0; i < num_rem_adr; i++) {
	flag_set(rem_adr[i].zone,rem_adr[i].net,rem_adr[i].node,rem_adr[i].point);
	if (rem_adr[i].pointnet)
	   flag_set(rem_adr[i].zone,rem_adr[i].pointnet,rem_adr[i].point,0);
    }

#ifndef NOHYDRA
    /* HYDRA --------------------------------------------------------------- */
    if ((remote.capabilities&DOES_HYDRA) && (ourbbs.capabilities&DOES_HYDRA)) {
       message(2,"+Hydra mailsession");
       wzsendfunc = send_hydra;
       hydra_init(hydra_options);
       if (!send_WaZOO(0)) {
	  hydra_deinit();
	  goto endwazoo;
       }
       hydra_deinit();
    }
    else
#endif

    /* ZMODEM (EMSI Direct, WaZoo or plain) and EMSI SEAlink --------------- */
    if (((remote.capabilities&EMSI_DZA) && (ourbbs.capabilities&EMSI_DZA)) ||
	((remote.capabilities&ZED_ZAPPER) && (ourbbs.capabilities&ZED_ZAPPER)) ||
	((remote.capabilities&ZED_ZIPPER) && ((emsi && caller) || (ourbbs.capabilities&ZED_ZIPPER))) ||
	((remote.capabilities&EMSI_SLK)   && (ourbbs.capabilities&EMSI_SLK)))
    {
       if (((remote.capabilities&EMSI_DZA) && (ourbbs.capabilities&EMSI_DZA)) ||
	   ((remote.capabilities&ZED_ZAPPER) && (ourbbs.capabilities&ZED_ZAPPER)) ||
	   ((remote.capabilities&ZED_ZIPPER) && ((emsi && caller) || (ourbbs.capabilities&ZED_ZIPPER)))) {
	  message(2,"+%s mailsession",
		  ((remote.capabilities&EMSI_DZA) && (ourbbs.capabilities&EMSI_DZA)) ?
		  "DirectZap" :
		  (((remote.capabilities&ZED_ZAPPER) && (ourbbs.capabilities&ZED_ZAPPER)) ?
		   "ZedZap" : "ZedZip"));
	  wzgetfunc = get_Zmodem;
	  wzsendfunc = send_Zmodem;
       }
       else {
	  message(2,"+SEAlink mailsession");
	  wzgetfunc = batchdown;
	  wzsendfunc = send_SEA;
	  mail = 0;	 /* address/password processing already done in EMSI */
	  sealink = 1;
	  xmodem = modem = telink = 0;
       }

       if (caller) {	/* Originator: send/receive/send */
	  got_request = 0;
	  if (!send_WaZOO(0) || !carrier()) goto endwazoo;
	  if (!(*wzgetfunc)()) goto endwazoo;

	  if (got_request) {
	     if (!carrier()) goto endwazoo;
	     stat = process_req_files(0);
	     (*wzsendfunc)(NULL,NULL,(int) NOTHING_AFTER,
			   (stat ? END_BATCH : NOTHING_TO_DO),DO_WAZOO);
	  }
       }
       else {		/* Called system: receive/send/receive */
	  if (!(*wzgetfunc)() || !carrier()) goto endwazoo;
	  if (!send_WaZOO(0)) goto endwazoo;
	  if (made_request) {
	     if (!carrier()) goto endwazoo;
	     (*wzgetfunc)();
	  }
       }
    }

    /* SEAlink ------------------------------------------------------------- */
    else {
       message(2,"+Smart-FTS mailsession");
       mail = 0;       /* address/password processing already done in YooHoo */

       if (caller) {
	  got_request = 0;
	  if (!FTSC_sender(1)) goto endwazoo;
	  if (got_request) {
	     if (!carrier()) goto endwazoo;
	     stat = process_req_files(0);
	     (*wzsendfunc)(NULL,NULL,(int) NOTHING_AFTER,
			   (stat ? END_BATCH : NOTHING_TO_DO),DO_WAZOO);
	  }
       }
       else {
	  if (!FTSC_receiver(1)) goto endwazoo;
	  if (made_request) {
	     if (!carrier()) goto endwazoo;
	     (*wzgetfunc)();
	  }
       }

#if 0
       wzgetfunc = batchdown;
       wzsendfunc = send_SEA;
       xmodem = modem = telink = 0;

       if (caller) {	/* Originator: send/receive/send */
	  got_request = 0;
	  if (!send_WaZOO(0) || !carrier()) goto endwazoo;
	  if (!(*wzgetfunc)()) goto endwazoo;

	  if (got_request) {
	     if (!carrier()) goto endwazoo;
	     stat = process_req_files(0);
	     (*wzsendfunc)(NULL,NULL,(int) NOTHING_AFTER,
			   (stat ? END_BATCH : NOTHING_TO_DO),DO_WAZOO);
	  }
       }
       else {		/* Called system: receive/send/receive */
	  if (!(*wzgetfunc)() || !carrier()) goto endwazoo;
	  if (!send_WaZOO(0)) goto endwazoo;
	  if (made_request) {
	     if (!carrier()) goto endwazoo;
	     (*wzgetfunc)();
	  }
       }
#endif
    }

    /*-----------------------------------------------------------------------*/
    if (caller && made_request) {
       t = timerset(100);
       while (!timeup(t) && carrier()) {
	     if (com_scan()) {
		ch = com_getbyte();
		if (ch != *p) {
		   com_putbyte(EOT);
		   com_flush();
		   com_purge();
		   p = BBSnow;
		}
		else {
		   p++;
		   com_putbyte(0x1e);
		   com_flush();
		   if (!*p) {
#ifndef NOHYDRA
		      if (wzsendfunc == send_hydra) {
			 win_xyputs(file_win,13,1,"Terminal service");
			 win_clreol(file_win);
			 hydramsg("Press any key for terminal");
			 win_setpos(file_win,13,4); win_clreol(file_win);
			 win_setpos(file_win,13,5); win_clreol(file_win);
			 win_setpos(file_win,13,6); win_clreol(file_win);
			 win_setpos(file_win,13,7); win_clreol(file_win);
		      }
		      else
#endif
		      {
			 filewincls("Terminal service",0);
			 filemsg("Press any key for terminal");
		      }
		      alarm();
		      Term(1);
		      win_settop(mailer_win);
		   }
		}
	     }
       }
    }
    else if (!caller && (FTB_dos || FTB_allowed)) {
       t = timerset(100);
       for (p = BBSnow; *p && carrier() && !timeup(t); ) {
	   com_putbyte(*p++);
	   com_flush();
	   if (com_getc(10) != 0x1e) p = BBSnow;
	   com_purge();
       }

       if (timeup(t) || !carrier())
	  message(2,"-Special request ignored");
       else {
#ifndef NOHYDRA
	  if (wzsendfunc == send_hydra)
	     hydramsg(FTB_dos ? "OS Service" : "BBS Service",0);
	  else
#endif
	     filewincls(FTB_dos ? "OS Service" : "BBS Service",0);

	  for (ep = extapp; ep && !(ep->flags & (APP_BBS | APP_MAIL)); ep = ep->next);
	  extapp_handler(ep);

	  win_settop(mailer_win);
       }
    }

    if (did_skip) goto endwazoo;

    message(2,"+End of mail session");

    if (made_request) {
       unlink(made_request);
       free(made_request);
       made_request = NULL;
    }

#if 0
    if (xenpoint_ver) {
       sprint(buffer,"%s\\%02.2z%03.3z%03.3z.OK",
		     hold,(word) rem_adr[0].zone,(word) rem_adr[0].net, (word) rem_adr[0].node);
       if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT,DENY_NONE)) >= 0) dos_close(fd);
    }
#endif

    sprint(buffer,"%sXMRESCAN.FLG",flagdir);
    if (fexist(buffer)) unlink(buffer);
#if 0
    if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0) dos_close(fd);
    /*rescan = 0L;*/
#endif

    return (1);

endwazoo:
    message(2,"-Error during mail session");
    if (made_request) {
       free(made_request);
       made_request = NULL;
    }
    /*process_req_files(0);*/

    sprint(buffer,"%sXMRESCAN.FLG",flagdir);
    if (fexist(buffer)) unlink(buffer);
#if 0
    if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0) dos_close(fd);
    /*rescan = 0L;*/
#endif

    return (0);
}/*WaZOO()*/


/* ------------------------------------------------------------------------- */
typedef struct _flag_adr FLAG_ADR;
struct _flag_adr {
	int zone, net, node, point;
	boolean nofile;
};
#define MAX_FLAG_ADR 125

static char	*taskflag = NULL;
static FLAG_ADR *flag_adr = NULL;
static word	 num_flag_adr = 0;


void flag_session (boolean set)
{
	word i;
	char buffer[256], bsyname[20];

	sprint(buffer,"%sTASK.%02x",flagdir,task);
	taskflag = strdup(buffer);

	if (set) {
	   int fd;

	   if (flag_adr) flag_session(false);

	   flag_adr = (FLAG_ADR *) calloc(MAX_FLAG_ADR,sizeof (FLAG_ADR));

	   if ((fd = dos_sopen(taskflag,O_WRONLY|O_CREAT|O_TRUNC,DENY_WRITE)) >= 0)
	      dos_close(fd);
	}
	else {
	   struct stat f;

	   for (i = 0; i < num_flag_adr; i++) {
	       if (flag_adr[i].nofile)
		  continue;

	       if (flag_adr[i].point) {
		  sprint(buffer,"%s%04.4x%04.4x.PNT\\",
				holdarea(flag_adr[i].zone),
				(word) flag_adr[i].net,(word) flag_adr[i].node);
		  sprint(bsyname,"%08.8x.BSY",(word) flag_adr[i].point);
	       }
	       else {
		  strcpy(buffer,holdarea(flag_adr[i].zone));
		  sprint(bsyname,"%04.4x%04.4x.BSY",
				 (word) flag_adr[i].net, (word) flag_adr[i].node);
	       }

	       strcat(buffer,bsyname);
	       if (fexist(buffer))
		  unlink(buffer);
	   }

	   if (!getstat(taskflag,&f)) {
	      FILE *fp;

	      if (f.st_size > 0 && (fp = sfopen(taskflag,"rt",DENY_WRITE)) != NULL) {
		 while (fgets(buffer,255,fp)) {
		       *skip_to_blank(buffer) = '\0';
		       if (fexist(buffer)) unlink(buffer);
		 }
		 fclose(fp);
	      }

	      unlink(taskflag);
	   }

	   if (flag_adr) {
	      free(flag_adr);
	      flag_adr = NULL;
	   }
	}

	num_flag_adr = 0;
}/*flag_session()*/


boolean flag_set (int zone, int net, int node, int point)
{
	word i;
	int  fd;
	char buffer[128], bsytemp[128], tmpname[20], bsyname[20];
	boolean nofile;

	if (!flag_adr) return (true);

	for (i = 0; i < num_flag_adr; i++) {
	    if (zone == flag_adr[i].zone && net == flag_adr[i].net &&
		node == flag_adr[i].node && point == flag_adr[i].point)
	       return (true);
	}

	if (point) {
	   sprint(buffer,"%s%04.4x%04.4x.PNT\\",holdarea(zone),(word) net, (word) node);
	   sprint(bsyname,"%08.8x.BSY",(word) point);
	}
	else {
	   strcpy(buffer,holdarea(zone));
	   sprint(bsyname,"%04.4x%04.4x.BSY",(word) net,(word) node);
	}

	sprint(tmpname,"XENIABSY.%02x",task);

	strcpy(bsytemp,buffer);
	strcat(bsytemp,tmpname);

	if ((fd = dos_sopen(bsytemp,O_WRONLY|O_CREAT,DENY_WRITE)) < 0)
	   nofile = true;
	else {
	   nofile = false;

	   dos_close(fd);
	   strcat(buffer,bsyname);
	   if (rename(bsytemp,buffer)) {
	      unlink(bsytemp);
	      goto islock;
	   }
	}

	if (num_flag_adr == MAX_FLAG_ADR) {
	   if (!nofile)
	      unlink(buffer);
	}
	else {
	   FILE *fp;

	   flag_adr[num_flag_adr].zone	 = zone;
	   flag_adr[num_flag_adr].net	 = net;
	   flag_adr[num_flag_adr].node	 = node;
	   flag_adr[num_flag_adr].point  = point;
	   flag_adr[num_flag_adr].nofile = nofile;
	   num_flag_adr++;

	   if (!nofile && (fp = sfopen(taskflag,"at",DENY_WRITE)) != NULL) {
	      fprint(fp,"%s\n",buffer);
	      fclose(fp);
	   }
	}

	/*
	message(0,"@Address %d:%d/%d%s%s lock created",
		  zone,net,node,pointnr(point),zonedomabbrev(zone));
	*/
	return (true);

islock: message(1,"@Address %d:%d/%d%s%s locked by another task",
		  zone,net,node,pointnr(point),zonedomabbrev(zone));
	return (false);
}/*flag_set()*/


boolean flag_test (int zone, int net, int node, int point)
{
	word i;
	char buffer[128], bsyname[20];

	if (flag_adr) {
	   for (i = 0; i < num_flag_adr; i++) {
	       if (zone == flag_adr[i].zone && net == flag_adr[i].net &&
		   node == flag_adr[i].node && point == flag_adr[i].point)
		  return (true);
	   }
	}

	if (point) {
	   sprint(buffer,"%s%04.4x%04.4x.PNT\\",holdarea(zone),(word) net,(word) node);
	   sprint(bsyname,"%08.8x.BSY",(word) point);
	}
	else {
	   strcpy(buffer,holdarea(zone));
	   sprint(bsyname,"%04.4x%04.4x.BSY",(word) net,(word) node);
	}

	strcat(buffer,bsyname);
	if (fexist(buffer))
	   return (false);

	return (true);
}/*flag_test()*/


/* end of msession.c */
