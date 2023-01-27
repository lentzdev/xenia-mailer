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


/* Routines for modem handling */
#include "xenia.h"


static char  mdmsent[128] = ""; 	       /* string sent to modem	     */
static char *mdmsptr = NULL;		       /* ptr inside mdmsent	     */
static char  mdmresp[128];		       /* string received from modem */


void mdm_putc (char c)		    /* intelligent character output to modem */
{
    static boolean quote = false;

    if (!quote) {
       if (isspace(c) && c != '\r')
	  return;

       switch (c) {
	      case '\\': quote = true;
			 return;
	      case '-':  return;		/* ignore '-'		*/
	      case '_':  c = ' ';		/* translate '_' to ' ' */
			 break;
	      case '|': 			/* translate '|' to cr	*/
	      case '!':  c = '\r';		/* translate '!' to cr	*/
			 break;
#if 0
	      case '.':  c = ',';		/* translate '.' to ',' */
			 break;
#endif
	      case '~': 			/* if wait		*/
	      case '`':  com_flush();		/* then delay sec	*/
			 tdelay((c == '~') ? 10 : 1);
			 return;
	      case '^':
	      case 'v':  com_flush();
			 dtr_out(c == '^');
			 return;
       }
    }
    else
       quote = false;

    com_putbyte(c);			/* else send char	*/

    if (mdmsptr) {
       char ch = c & 0x7f;

       if (ch >= ' ' && ch <= '~')
	  *mdmsptr++ = toupper(ch);
    }

    if (!mdm.slowmodem) return;

    com_flush();
    tdelay((c == '\r') ? 5 : 1);	/* and give modem some time */
}


void mdm_puts (char *str)		/* string to modem */
{
    boolean quote = false;

    if (mdm.port == -1) return;

    mdmsptr = mdmsent;
    while (*str) {
	  if (*str == '\"')
	     quote = quote ? false : true;
	  else {
	     if (quote) mdm_putc('\\');
	     mdm_putc(*str++);
	  }
    }
    *mdmsptr = '\0';
    mdmsptr = NULL;
}


void mdm_reset()		/* Reset the modem to commandmode */
{
    if (mdm.port == -1) return;

    mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
    memset(&clip,0,sizeof (CLIP));
    arqlink = isdnlink = iplink = false;
    com_dump();
    if (carrier())
       mdm_puts(mdm.hangup);
    else {
       if (strchr(mdm.hangup,'+')) mdm_putc('+');
       mdm_putc('\r');
       dtr_out(0);
       tdelay(4);
       dtr_out(1);
       tdelay(2);
       mdm_putc('\r');
       mdm_putc('\r');
       tdelay(2);
    }
    com_purge();
}


/* INFO and MNP filter logic

   We get here right after the CONNECT message. It could happen
   so quickly that we don't even have DCD set. (On a 33MHz 386
   and a T2500, that happens!)

   So: this routine waits up to 1 second for a carrier.

   It then eats anything that looks like an MNP string, with 
   a total time allowed of 10 seconds (for streaming garbage)
   and maximum inter-character delay of 3 seconds.
*/

static void mnp_filter (void)
{
	long t, t1;
	int c;
	boolean logged = false;
	static char *fallbackchars = " \r\n";
	static char *mnpchars	   = "\026\021\b\377\376\375\374";

					     /* at most a one second delay   */
	for (t = timerset(10); !carrier() && !timeup(t); win_idle());

	if (arqlink || isdnlink || iplink) {
	   t1 = timerset((word) 30);		/* 3 second drop dead timer  */
	   t  = timerset((word) 10);		/* at most a 1 sec delay     */
	}
	else {
	   t1 = timerset((word) 100);		/* 10 second drop dead timer */
	   t  = timerset((word)  30);		/* at most a 3 sec delay     */
	}

	while (carrier() && !timeup(t) && !timeup(t1)) {
	      if (keyabort()) { 			/* Manual abort?     */
		 message(1,"-Aborted by operator");
		 hangup();				/* Yes, drop carrier */
		 break;
	      }

	      if (com_scan()) {
		 c = com_getbyte() & 0x7f;

		 if (c) {
		    if (strchr(fallbackchars,c) || strchr(fallbackchars,c | 0x80))
		       break;				/* MNP Fallback char */

       /* If we get an MNP or v.42 character, eat it and wait for clear line */
		    if (strchr(mnpchars,c) || strchr(mnpchars,c | 0x80)) {
		       if (!logged) {
			  message(0,"*TE/modem protocol negotiation filtered");
			  logged = true;
		       }
		       t = timerset(30);
		    }
		 }
	      }
	      else
		 win_idle();
	}
}


dword mdm_result (char *str)		/* Interpret result from modem */
{
    dword     result;			/* resultcode		 */
    char     *tmp;			/* multiple purpose	 */
    dword     speed;			/* speed (if connect)	 */
    MDMRSP   *mrp;			/* Modem response ptr	 */

    if (!str || !str[0])
       return (MDMRSP_IGNORE);

    speed = 0;
    arqlink = isdnlink = iplink = false;
    for (mrp = mdm.rsplist; mrp; mrp = mrp->next) {
	if (!strncmp(str,mrp->string,strlen(mrp->string))) {
	   result = mrp->code;
	   if (result >= 256) {
	      strcpy(mdmconnect,str);
	      strcat(mdmconnect,errorfree2);
	      if ((tmp = strchr(mdmconnect,'/')) != NULL)
		 strcpy(errorfree,tmp);
	      if (errorfree[0]) {
		 if (strstr(errorfree,"ARQ") || strstr(errorfree,"MNP") ||
		     strstr(errorfree,"V42") || strstr(errorfree,"V.42") ||
		     strstr(errorfree,"LAPM") ||
		     strstr(errorfree,"X75") ||strstr(errorfree,"V120"))
		    arqlink = true;
		 if (strstr(errorfree,"X75")  || strstr(errorfree,"T90") ||
		     strstr(errorfree,"V110") || strstr(errorfree,"V120") ||
		     strstr(errorfree,"X.75")  || strstr(errorfree,"T.90") ||
		     strstr(errorfree,"V.110") || strstr(errorfree,"V.120") ||
		     strstr(errorfree,"ISDN"))
		    isdnlink = true;
		 if (strstr(errorfree,"TEL") || strstr(errorfree,"VMP"))
		    iplink = true;
	      }
	      if (result == 64000UL)
		 isdnlink = true;
	      setspeed(result,false);
	      mnp_filter();
	      /*result==speed*/

	      if (cur_speed == 64000UL)
		 line_speed += cur_speed / 6;
	      else if (errorfree[0]) {
		 if (strstr(errorfree,"X75") || strstr(errorfree,"T90") ||
		     strstr(errorfree,"X.75") || strstr(errorfree,"T.90") ||
		     strstr(errorfree,"V120"))
		    line_speed += cur_speed / 6;
		 else if (strstr(errorfree,"MNP") || strstr(errorfree,"V42") ||
			  strstr(errorfree,"V.42") || strstr(errorfree,"LAPM"))
		    line_speed += cur_speed / 7;
	      }
	   }
	   else if ((result & MDMRSP_CONNECT) && result != MDMRSP_DATA) {
	      strcpy(mdmconnect,str);		   /* CONNECT,SET TO or +FCO */
	      strcat(mdmconnect,errorfree2);
	      if (result == MDMRSP_FAX) {
		 tmp = str + 1; 	 /* skip the + in case of +FCO/+FCON */
		 while (*tmp && isalpha(*tmp)) tmp++;
		 speed = (mdm.speed < 19200) ? 9600 : 19200;
	      }
	      else {
		 if (strlen(str) < 8) speed = 300; /* Only connect */
		 tmp = &str[result == MDMRSP_SET ? 6 : 7];
		 while (*tmp && isalpha(*tmp)) tmp++;
	      }
	      tmp = skip_blanks(tmp);

	      if (!speed) {
		 if (!strncmp(tmp,"12/75",5) || !strncmp(tmp,"12/75",5)) {
		    speed=1200; tmp+=5;
		 }
		 else if (!strncmp(tmp,"128K",4)) {
		    speed=128000UL; tmp+=4;
		 }
		 else {
		    speed = atol(tmp);
		    while (isdigit(*tmp)) tmp++;

		    if (speed < 65536UL) {
		       switch (speed) {
			      case 103:
				   speed =  300; break;
			      case 212: case 75: case 12: case 1275: case 7512:
				   speed = 1200; break;
		       }
		    }
		 }
	      }

	      if ((tmp = strchr(mdmconnect,'/')) != NULL)
		 strcpy(errorfree,tmp);
	      if (errorfree[0]) {
		 if (strstr(errorfree,"ARQ") || strstr(errorfree,"MNP") ||
		     strstr(errorfree,"V42") || strstr(errorfree,"V.42") ||
		     strstr(errorfree,"LAPM") ||
		     strstr(errorfree,"X75") ||strstr(errorfree,"V120"))
		    arqlink = true;
		 if (strstr(errorfree,"X75")  || strstr(errorfree,"T90") ||
		     strstr(errorfree,"V110") || strstr(errorfree,"V120") ||
		     strstr(errorfree,"X.75")  || strstr(errorfree,"T.90") ||
		     strstr(errorfree,"V.110") || strstr(errorfree,"V.120") ||
		     strstr(errorfree,"ISDN"))
		    isdnlink = true;
		 if (strstr(errorfree,"TEL") || strstr(errorfree,"VMP"))
		    iplink = true;
	      }
	      if (speed == 64000UL)
		 isdnlink = true;

	      if (result == MDMRSP_FAX)
		 setspeed(speed,(fax_options & FAXOPT_LOCK) ? false : true);
	      else {
		 setspeed(speed,false);
		 mnp_filter();
	      }

	      if (cur_speed == 64000UL)
		 line_speed += cur_speed / 6;
	      else if (errorfree[0]) {
		 if (strstr(errorfree,"X75") || strstr(errorfree,"T90") ||
		     strstr(errorfree,"X.75") || strstr(errorfree,"T.90") ||
		     strstr(errorfree,"V120"))
		    line_speed += cur_speed / 6;
		 else if (strstr(errorfree,"MNP") || strstr(errorfree,"V42") ||
			  strstr(errorfree,"V.42") || strstr(errorfree,"LAPM"))
		    line_speed += cur_speed / 7;
	      }

	      return (speed);
	   }
	   else if (result == MDMRSP_RING) {
	      char *p;

	      tmp = skip_blanks(&str[strlen(mrp->string)]);
	      if (*tmp == '/') tmp++;
	      if (!strncmp(tmp,"ID=",3))
		 tmp += 3;
	      if (isdigit(tmp[0])) {
		 strcpy(callerid,tmp);
		 if ((p = strchr(callerid,'/')) != NULL) *p = '\0';
		 if (strlen(callerid) < 2) callerid[0] = '\0';
	      }
	      else if (tmp[0] == ';') { 	   /* ELSA MicroLINK ISDN/TL */
		 tmp++; 			   /* "RING;theirmsn;ourmsn" */
		 if ((p = strchr(tmp,';')) != NULL) {
		    *p = '\0';
		    if (strlen(tmp) && isdigit(tmp[0]))
		       strcpy(callerid,tmp);
		 }
	      }
	   }
	   else {
	      if (mdm.causelist) {
		 MDMCAUSE *mcp;
		 word cause, hcause;

		 if ((tmp = strrchr(str,'/')) != NULL &&
		     !strncmp(&tmp[1],"CAUSE=",6) &&
		     sscanf(&tmp[7],"%hx",&cause) == 1) {
		    for (mcp = mdm.causelist; mcp; mcp = mcp->next) {
			if (cause == mcp->cause) {
			   strcpy(&tmp[1],mcp->description);
			   break;
			}
		    }
		 }
		 else if ((tmp = strchr(str,';')) != NULL &&
			  strlen(tmp) == 8 && isdigit(tmp[1]) &&
			  tmp[4] == ';' && isdigit(tmp[5]) &&
			  sscanf(&tmp[1],"%hu;%hu",&hcause,&cause) == 2) {
		    cause |= (word) (hcause << 8);
		    for (mcp = mdm.causelist; mcp; mcp = mcp->next) {
			if (cause == mcp->cause) {
			   strcpy(&tmp[1],mcp->description);
			   break;
			}
		    }
		 }
	      }
	   }

	   return (result);
	}
    }

    if (!strncmp(str,"COMPRESSION:",12) || !strncmp(str,"PROTOCOL:",9)) {
       if ((tmp = strtok(strchr(str,':') + 1," ")) != NULL) {
	  strcat(errorfree2,"/");
	  strcat(errorfree2,tmp);
       }
       *str = '\0';
    }
    else if (!strncmp(str,"CALLER NUMBER:",14)) {
       tmp = skip_blanks(&str[14]);
       if (strncmp(tmp,"OUT_OF_AREA",11))
	  strcpy(callerid,tmp);
       *str = '\0';
    }
    else if (!strncmp(str,"CALLER NAME:",12)) {
       tmp = skip_blanks(&str[12]);
       if (strncmp(tmp,"PRIVACY",7)) {
	  if (callerid[0]) strcat(callerid," ");
	  strcat(callerid,tmp);
       }
       *str = '\0';
    }
    else if (!strncmp(str,"NMBR = ",7)) {	/* Supra 288		     */
       tmp = skip_blanks(&str[7]);
       if (*tmp)
	  strcpy(callerid,tmp);
       *str = '\0';
    }
    else if (!strncmp(str,"FM:",3)) {		/* ZyXEL Elite 2864I	     */
       tmp = skip_blanks(&str[3]);		/* FM: <number> TO: <number> */
       if (isdigit(*tmp)) {
	  strcpy(callerid,tmp);
	  if ((tmp = strstr(callerid," TO:")) != NULL)
	     *tmp = '\0';
       }
       /* don't do *str='\0' so this line WILL be printed for the log */
    }
    else if (!strncmp(str,"TIME:",5))
       *str = '\0';
    else if (!callerid[0]) {
       boolean definite = false;
       char *p;

       tmp = str;
       if (!strncmp(tmp,"ID=",3)) {			/* cFos/COMCAPI      */
	  tmp += 3;
	  definite = true;
       }
       tmp = skip_blanks(tmp);
       if (isdigit(*tmp)) {
	  if (!definite)
	     for (p = tmp; *p && isalnum(*p); p++);
	  if (definite || !*p) {
	     strcpy(callerid,tmp);
	     if ((tmp = strchr(callerid,'/')) != NULL) *tmp = '\0';
	     if (strlen(callerid) > 1) *str = '\0';
	     else		       *callerid = '\0';
	  }
       }
    }

    return (MDMRSP_IGNORE);			/* no known string */
}


char *mdm_gets (int tenths, boolean show) /* get str from modem with timeout */
{
    char *mdm = mdmresp;		/* pointer in response buffer */
    int ch;				/* received character	      */
    int abort = 0;			/* aborted??		      */
    long t;				/* timerstuff		      */

    *mdm = '\0';
    t = timerset((word) tenths);

    do {
       ch = com_getbyte();		/* get next char	*/
       if (ch == EOF) { 		/* no char, to checking */
	  win_idle();
	  continue;
       }
       ch &= 0x7f;
       if (ch == '\r' || ch == '\n') {
	  if (mdm != mdmresp)
	     break;			/* end of this line */
       }
       else if (ch >= ' ' && ch <= '~')
	  *mdm++ = (tenths < 30 && !show) ? ch : toupper(ch);  /* add to str */

       t = timerset(8);
    } while (!(abort = keyabort()) && !timeup(t) && mdm < (mdmresp + 120));

    *mdm = '\0';			    /* terminate string */

    if (show && *mdmresp) {
       win_setconemu(modem_win,CON_RAW|CON_SCROLL|CON_WRAP);
       if (strstr(mdmsent,mdmresp))
	  win_setattrib(modem_win,xenwin_colours.modem.hotnormal);
       win_print(modem_win,"\n%s",mdmresp);
       win_setattrib(modem_win,xenwin_colours.modem.normal);
       win_setconemu(modem_win,CON_RAW|CON_SCROLL);
    }

    if (abort) {
       message(1,"-Aborted by operator");
       mdm_reset();			    /* abort (possible) dial-command */
       return (NULL);
    }

    return (mdmresp);
}


dword mdm_command (char *str, int code, int time1, int time2)
						/* send command to modem     */
/*char *str;					/+ commandstring	     */
/*int code;					/+ code we're waiting for    */
/*int time1;					/+ max.time to wait each try */
/*int time2;					/+ total time to wait	     */
{
    long   t1, t2;				/* timerstuff	 */
    char   st[256];				/* part of str	 */
    char  *p;					/* pointer in st */
    char  *temp;				/* temp storage  */
    dword  result = code;			/* resultcode	 */
    int    wait;				/* wait for OK.. */
    int    rings = 0;

    while (*str) {				/* still commands left? */
	  for (p = st; *str && *str != '|' && *str != '!'; *p++ = *str++);

	  wait = 0;
	  if (*str == '|' || *str == '!') {
	     wait = (*str++ == '|');
	     *p++ = '\r';
	  }
	  *p = '\0';

	  t1 = timerset((word) time2);
	  while (!timeup(t1)) {
		if (strchr(st,'|'))
		   com_purge();
		mdm_puts(st);			/* send command to modem */
		t2 = timerset((word) time1);
		while (!timeup(t2)) {		/* still some time to go */
		      temp = mdm_gets(40,true); /* get input from modem  */
		      if (!temp)		/* aborted by operator	 */
			 goto abrt;
		      if (!wait)
			 goto ok;

		      result = mdm_result(temp);
		      if (result == MDMRSP_ERROR)
			 break;
		      if (*str) {
			 if (result == MDMRSP_OK)
			    goto ok;
		      }
		      else {
			 if ((code & MDMRSP_RING) && result == MDMRSP_RING) {
			    if (mdm.maxrings && ++rings >= mdm.maxrings)
			       goto ok;
			 }
			 else if ((result & code) ||
				  ((code & MDMRSP_CONNECT) && result >= 256))
			    goto ok;
		      }
		}				/* modem didn't work alright */

		message(2,"-TE/modem does not respond correctly");
		mdm_reset();
	  }					/* even retry's didn't help  */

	  message(6,"!TE/modem error, cannot force correct operation");
abrt:	  message(0,"-Aborting TE/modem command sequence");
	  return (0);
ok:	  ;
	  /*if (*str) tdelay(1);*/
    }

    return (result);
}


dword dail (char *pstring, char *number, char *nodeflags)
{
    dword result;		/* resultcode */
    char str[300];		/* string with command */
    char *p;
    DIALXLAT	 *dp;
    PREFIXSUFFIX *pp;

    /* pstring = z:n/n, or number for dialback and @terminal */
    for (pp = prefixsuffix; pp && !patimat(pstring,pp->pattern) &&
				  !patimat(number,pp->pattern) &&
				  (!nodeflags || !patimat(nodeflags,pp->pattern));
				  pp = pp->next);

    strcpy(str,pp->prefix ? pp->prefix : "");
    for (dp = dialxlat; dp; dp = dp->next) {
	if (!dp->number || !strnicmp(dp->number,number,strlen(dp->number))) {
	   if (!patimat(pstring,dp->pattern) &&
	       !patimat(number,dp->pattern) &&
	       (!nodeflags || !patimat(nodeflags,dp->pattern)))
	      continue;
	   if (dp->prefix) strcat(str,dp->prefix);
	   strcat(str,&number[dp->number ? strlen(dp->number) : 0]);
	   if (dp->suffix) strcat(str,dp->suffix);
	   break;
	}
    }
    if (!dp) strcat(str,number);
    if (nodeflags && nodeflags[0])
       message(0,"+Dialling %s (%s)", &str[strlen(pp->prefix)], nodeflags);
    else
       message(0,"+Dialling %s", &str[strlen(pp->prefix)]);
    if (pp->suffix) strcat(str,pp->suffix);
    if (xlatdash && (p = strchr(str,'-')) != NULL)
       *p = xlatdash[0];

    if (com_scan())
       return (MDMRSP_COLLIDE);
    mdm_reset();
    if (com_scan())
       return (MDMRSP_COLLIDE);
    /* '\r' added by mdm_command!! */
    result = mdm_command(str, MDMRSP_CONNECT | MDMRSP_NOCONN | MDMRSP_BADCALL | MDMRSP_NODIAL | MDMRSP_RING,
			      mdm.timeout + 150, mdm.timeout);
    if (result == MDMRSP_DATA) {
       if (mdm.data)
	  result = mdm_command(mdm.data,MDMRSP_CONNECT | MDMRSP_NOCONN | MDMRSP_BADCALL, 150, 300);
       else
	  result = mdm_result(mdm_gets(40,true));
    }

    if (!result)
       mdm_reset();
    else if (result < 256) {
       if (result == MDMRSP_RING) {
	  message(3,"-No answer within %u rings",mdm.maxrings);
	  mdm_reset();
       }
       else {
	  if ((p = strchr(mdmresp,'/')) != NULL) {
	     *p = '\0';
	     strlwr(mdmresp);
	     *p = '/';
	  }
	  else
	     strlwr(mdmresp);

	  message(3,"-No connect (%s)",mdmresp[0] ? mdmresp : "timeout");

	  if (mdm.freepattern && patimat(mdmresp,mdm.freepattern)) {
	     FREEDIAL *fdp;

	     for (fdp = mdm.freedial; fdp; fdp = fdp->next) {
		 if (patimat(pstring,fdp->address))
		    return (255UL);
	     }
	  }
       }
    }
    else
       message(3,"+Connected at %lu bps %s",result,errorfree);

    return (result);
}


dword got_caller(int forced)
{
    long   t;				/* timer stuff */
    char  *res = "";			/* string got from modem */
    dword  result;			/* resultcode */
    int    abort;			/* aborted ?? */
    word   seenrings = 0;
    char  *p;
    ANSWER *ap = NULL;

    callerid[0] = '\0';
    memset(&clip,0,sizeof (CLIP));

    dtr_out(1); 			/* make sure we've got DTR valid */
    if (forced) goto get_it_now;

again:
    res = mdm_gets(40,true);
    if (res == NULL)			/* check if it is the phone ringing */
       goto aabort;
    if (!res[0])
       return (0);

    if ((cur_event >= 0) && (e_ptrs[cur_event]->behavior & MAT_OUTONLY)) {
       message(strlen(res) >= 4 ? 3 : 0,"#%s",res);
       return (0);
    }

    result = mdm_result(res);
    if (result == MDMRSP_DATA) {
       if (mdm.data)
	  result = mdm_command(mdm.data,MDMRSP_CONNECT | MDMRSP_NOCONN | MDMRSP_BADCALL, 150, 300);
       else
	  goto again;
    }
    if (result == MDMRSP_IGNORE)	/* FAX or CONNECT FAX, so try again */
       goto again;

    if (result >= 256) goto gotnow;
    if (result != MDMRSP_RING) {	/* no, error condition */
       init_modem();
       goto aabort;
    }

    message(strlen(res) > 4 ? 3 : 0,"#%s",res);
    statusmsg("Detected %s",res);

    if (mdm.rings == 1)  /* Answer 1st ring; Danish ISDN hangs up after 2nd! */
       goto ans1ring;

    seenrings = 1;
    t=timerset(100);	/* must have another ring within 10 sec. */
    while (!timeup(t) && (abort=keyabort())==0 && res!=NULL) {
	  if ((res = mdm_gets(40,true)) == NULL) continue;
	  result = mdm_result(res);
	  if (result == MDMRSP_RING) {	   /* Ok got one, now modem answer   */
	     if (++seenrings < mdm.rings) {
		t = timerset(100);
		continue;
	     }
ans1ring:    for (ap = mdm.answer; ap->next && !patimat(res,ap->pattern) && !patimat(callerid,ap->pattern); ap = ap->next);
	     if (!ap->command) {
		message(strlen(res) > 4 ? 3 : 0,"-Ignoring call");
		if (callerid[0])
		   check_clip();
		statusmsg(NULL);
		return (0);
	     }
	     if (!stricmp(ap->command,"reject")) {
		message(3,"-Rejecting call");
		if (callerid[0])
		   check_clip();
		if (mdm.reject)
		   mdm_command(mdm.reject,MDMRSP_OK,30,600);
		statusmsg(NULL);
		return (0);
	     }

	     if (mdm.freeanswer && mdm.reject && callerid[0]) {
		FREEANSWER *fdp;
		char fname[128];
		int fd;
		int tzone, tnet, tnode, tpoint;
		struct stat f;
		boolean dofreepoll = false;

		for (fdp = mdm.freeanswer; fdp; fdp = fdp->next) {
		    if (patimat(callerid,fdp->callerid)) {
		       getaddress(fdp->address,&tzone,&tnet,&tnode,&tpoint);
		       if (check_kbmail(false,tzone,tnet,tnode,tpoint) > fdp->minbytes) {
			  message(3,"*No FreePoll - above %lu KB threshold", fdp->minbytes / 1024LU);
			  break;
		       }
		       sprint(fname, "%s%04.4x%04.4x%s.FPL", holdarea(tzone),
			      (word) tnet, (word) tnode, holdpoint(tpoint));
		       if (!getstat(fname,&f) &&	/* window 5 minutes! */
			   time(NULL) < (f.st_mtime + 300)) {
			  message(3,"*No FreePoll - address called again");
			  break;
		       }
		       createhold(tzone,tnet,tnode,tpoint);
		       if ((fd = dos_sopen(fname,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0)
			  dos_close(fd);
		       else {
			  message(3,"*No FreePoll - can't create semaphore");
			  break;
		       }

		       dofreepoll = true;
		    }
		}

		if (!fdp && dofreepoll) {
		   message(3,"*FreePoll try - reject call & set semaphore");
		   check_clip();
		   mdm_command(mdm.reject,MDMRSP_OK | MDMRSP_NOCONN | MDMRSP_BADCALL,30,600);
		   statusmsg(NULL);
		   return (0);
		}
	     }

	     tdelay(6); 		   /* wait a bit for modem to settle */
get_it_now:  if (ringapp) {
		char buffer[128];
		int i;

		message(3,"+Calling ring application");
		if (log != NULL) fclose(log);
		if (mdm.shareport)
		   com_suspend = true;
		sys_reset();
		(void) win_savestate();
		sprint(buffer,"%s %u %d %s %d %lu",
			      ringapp, task,
			      mdm.port, mdm.comdev, com_handle,
			      mdm.lock ? mdm.speed : cur_speed);
		i = execute(buffer);
		(void) win_restorestate();
		sys_init();
		message(1,"+Returned from ring application");
		if (log != NULL) {
		   if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
		   message(6,"!Can't re-open logfile!");
		}
		if (i) goto aabort;	/* erl <> 0  ringapp completed call */
		/* erl = 0  ringapp reports Xenia still has to answer call  */
	     }
	     statusmsg("Answering call");
	     if (!ap)
		for (ap = mdm.answer; ap->next && !patimat(res,ap->pattern) && !patimat(callerid,ap->pattern); ap = ap->next);
	     result = mdm_command(ap->command, MDMRSP_CONNECT | MDMRSP_NOCONN | MDMRSP_BADCALL,
				  mdm.timeout + 150, mdm.timeout + 150);
	     if (result == MDMRSP_DATA) {
		if (mdm.data)
		   result = mdm_command(mdm.data,MDMRSP_CONNECT | MDMRSP_NOCONN | MDMRSP_BADCALL, 150, 300);
		else
		   result = mdm_result(mdm_gets(40,true));
	     }

	     if (result < 256) {
		if ((p = strchr(mdmresp,'/')) != NULL) {
		   *p = '\0';
		   strlwr(mdmresp);
		   *p = '/';
		}
		else
		   strlwr(mdmresp);
		message(3,"-Caller not connected (%s)",mdmresp[0] ? mdmresp : "timeout");
	     }
	     else {
gotnow: 	if ((p = strstr(errorfree," FROM ")) != NULL) {
		   if (!callerid[0]) strcpy(callerid,&p[6]);
		   *p = '\0';
		}
		message(3,"+Incoming call at %lu bps %s",result,errorfree);
	     }

	     if (callerid[0])
		check_clip();

	     if (result < 256)
		init_modem();

	     return (result);
	  }
	  else if (result >= 256)
	     goto gotnow;
	  else if (strlen(mdmresp) > 1)
	     message(3,"#%s",mdmresp);
    }

    if (!abort && res != NULL)
       message(1,"-Less than %u rings detected", mdm.rings);
    else
       message(1,"-Aborted by operator");

    if (callerid[0])
       check_clip();

aabort:
    init_modem();
    statusmsg(NULL);

    return (0);
}


int init_modem()			/* initialise modem */
{
    dword result;			/* resultcode */

    if (mdm.port == -1) {
       init_timer = timerset(6000);	    /* Reset 10 minute re-init timer */
       return (0);
    }

    statusmsg("Initializing TE/modem");
    mdm_reset();
    setspeed(mdm.speed,false);
    result = mdm_command(mdm.setup,MDMRSP_OK,30,600);	   /* try max. 1 min */
    init_timer = timerset(6000);	    /* Reset 10 minute re-init timer */
    statusmsg(NULL);
    if (!result) {
       message(6,"!Fatal, TE/modem not initialised.");
       return (1);
    }

    if (loglevel==0)
       message(-1,"+TE/Modem initialized");

    tdelay(10);
    com_purge();

    return (0);
}


void reset_modem()
{
    dword result;			/* resultcode */

    if (mdm.port == -1) return;
    
    statusmsg("De-initializing TE/modem");
    mdm_reset();
    setspeed(mdm.speed,false);
    result = mdm_command(mdm.reset,MDMRSP_OK,30,600);	  /* try max. 1 min. */
    statusmsg(NULL);
    if (!result) message(6,"!TE/modem not de-initialised.");
    else if (loglevel==0) message(-1,"+TE/modem de-initialized");
}


void mdm_busy()
{
    if (mdm.port == -1) return;

    statusmsg("Busying out TE/modem");
    mdm_reset();
    setspeed(mdm.speed,false);
    mdm_command(mdm.busy,MDMRSP_OK,30,600);		  /* try max. 1 min. */
    statusmsg(NULL);
}


void mdm_terminit (void)
{
    if (mdm.terminit) {
       statusmsg("TE/modem terminal init");
       mdm_reset();
       setspeed(mdm.speed,false);
       mdm_command(mdm.terminit,MDMRSP_OK,30,600);
       statusmsg(NULL);
    }
}


void mdm_aftercall (void)
{
    if (mdm.aftercall) {
       char *res;

       statusmsg("TE/modem after call sequence");
       com_dump();
       com_purge();
       mdm_puts(mdm.aftercall);
       while (res = mdm_gets(15,false),
	      res && res[0] && strncmp(res,"RING",4)) {
	     if (!strstr(mdmsent,mdmresp))
		message(3,"#%s",res);
       }
       mdmsent[0] = '\0';
       statusmsg(NULL);
    }
}


/* end of modem.c */
