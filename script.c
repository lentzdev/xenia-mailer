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


#include "zmodem.h"


/* ------------------------------------------------------------------------- */
boolean isloginchr (char c)
{
	return ((isalnum(c) || c == '-') ? true : false);
}/*isloginchr()*/


/* ------------------------------------------------------------------------- */
static int escape_instr (char *s)
{
	char *p, *q;

	for (p = q = s; *p; p++) {
	    if (*p != '\\') {
	       *q++ = *p;
	       continue;
	    }

	    switch (*++p) {
		   case 'b':  *q++ = '\b';
			      break;
		   case 'n':  *q++ = '\n';
			      break;
		   case 'N':  *q++ = '\0';
			      break;
		   case 'r':  *q++ = '\r';
			      break;
		   case 's':  *q++ = ' ';
			      break;
		   case 't':  *q++ = '\t';
			      break;
		   case '0':  case '1': case '2': case '3':
		   case '4':  case '5': case '6': case '7':
			      { byte i;

				i = *p - '0';
				if (p[1] >= '0' && p[1] <= '7') {
				   i = (i << 3) + (*++p - '0');

				   if (p[1] >= '0' && p[1] <= '7')
				      i = (i << 3) + (*++p - '0');
				}
				*q++ = i;
			      }
			      break;
		   case 'x':  { byte i;

				i = 0;
				if (isxdigit(p[1])) {
				   i <<= 4;
				   if (isdigit(p[1])) i += (*++p - '0');
				   else i += ((toupper(*++p) - 'A') + 10);

				   if (isxdigit(p[1])) {
				      i <<= 4;
				      if (isdigit(p[1])) i += (*++p - '0');
				      else i += ((toupper(*++p) - 'A') + 10);
				   }
				}
				*q++ = i;
			      }
			      break;
		   case '\0': p--;
			      /* fallthrough to default */
		   default:   *q++ = *p;
			      break;
	    }
	}

	*q = '\0';

	return ((int) (q - s));
}/*escape_instr()*/


/* ------------------------------------------------------------------------- */
void script_outstr (char *s, char *loginname, char *password, WIN_IDX win)
{
    boolean addcr = loginname ? true : false;	      /* only when scripting */

    if (!strcmp(s,"\"\""))
       return;

    while (*s) {
	  if (!strncmp(s,"EOT",3)) {
	     com_bufbyte('\004');
	     s += 3;
	     continue;
	  }
	  if (!strncmp(s,"BREAK",5)) {
	     com_break();
	     s += 5;
	     continue;
	  }
#if 0
	  /* FX UUCICO
	     "P_NONE"	Set port to 8N1
	     "P_EVEN"	Set port to 7E1 - only remains through login sequence
	     "P_ODD"	Set port to 7O1 - only remains through login sequence
	  */
#endif
	  if (*s != '\\') {
	     if (win) win_putc(win,*s);
	     com_bufbyte(*s++);
	     continue;
	  }

	  switch (*++s) {
		 case 'b':  if (win) win_puts(win,"\b \b");
			    com_bufbyte('\b');
			    break;
		 case 'c':  addcr = false;
			    break;
		 case 'd':  tdelay(20);
			    break;
#if 0
		 /* Probably any uucico (Taylor UUCP, FX UUCICO)
		    I don't think we'll need this. FX UUCICO says it's useful
		    with slow modems, but as we don't use scripts for dialling.
		 */
		 case 'e':  check_echo = false;
			    break;
		 case 'E':  if (loginname)	      /* only when scripting */
			       check_echo = true;
			    break;
#endif
		 case 'K':  com_break();
			    break;
		 case 'L':  if (loginname) {
			       if (win) win_puts(win,loginname);
			       com_putblock((byte *) loginname, (word) strlen(loginname));
			    }
			    break;
#if 0
		 /* FX UUCICO: default is carrier checking disabled.
		    Taylor UUCP doesn't have this; probably never needed.
		    If WE implement these options, it must be default ENABLED
		    because we don't use stripts for dialling....
		 */
		 case 'm':  /* enable carrier checking	*/
		 case 'M':  /* disable carrier checking */
#endif
		 case 'n':  if (win) win_putc(win,'\n');
			    com_bufbyte('\n');
			    break;
		 case 'N':  com_bufbyte('\0');
			    break;
		 case 'p':  tdelay(5);
			    break;
		 case 'P':  if (password) {
			       /* don't print password locally of course */
			       com_putblock((byte *) password, (word) strlen(password));
			    }
			    break;
		 case 'r':  if (win) win_putc(win,'\n');
			    com_bufbyte('\r');
			    break;
		 case 's':  if (win) win_putc(win,' ');
			    com_bufbyte(' ');
			    break;
#if 0
		 /* FX UUCICO: As to why anyone would need this, not a clue!
		    And eh, Taylor UUCP doesn't have this one either....
		 */
		 case 'S':  /* send the remote host uucp name */
#endif
		 case 't':  if (win) win_putc(win,'\t');
			    com_bufbyte('\t');
			    break;
#if 0
		 /* Used in any unix dialer, but we don't need this because
		    we don't use scripts for dialling....
		 */
		 case 'T':  /* phone# string to dial */
#endif
		 case '0':  case '1': case '2': case '3':
		 case '4':  case '5': case '6': case '7':
			    { byte i;

			      i = *s - '0';
			      if (s[1] >= '0' && s[1] <= '7') {
				 i = (i << 3) + (*++s - '0');

				 if (s[1] >= '0' && s[1] <= '7')
				    i = (i << 3) + (*++s - '0');
			      }
			      if (win) win_putc(win,i);
			      com_bufbyte(i);
			    }
			    break;
		 case 'x':  { byte i;

			      i = 0;
			      if (isxdigit(s[1])) {
				 i <<= 4;
				 if (isdigit(s[1])) i += (*++s - '0');
				 else i += ((toupper(*++s) - 'A') + 10);

				 if (isxdigit(s[1])) {
				    i <<= 4;
				    if (isdigit(s[1])) i += (*++s - '0');
				    else i += ((toupper(*++s) - 'A') + 10);
				 }
			      }
			      if (win) win_putc(win,i);
			      com_bufbyte(i);
			    }
			    break;
		 case '\0': s--;
			    /* fallthrough to default */
		 default:   if (win) win_putc(win,*s);
			    com_bufbyte(*s);
			    break;
	  }

	  s++;
    }

    if (addcr) {
       if (win) win_putc(win,'\n');
       com_bufbyte('\r');
    }

    com_flush();
}/*script_outstr()*/


/* ------------------------------------------------------------------------- */
boolean password_login (char *password)
{
	int i, c;
	long t;
	boolean fail;

	if (!password)
	   return (true);

	script_outstr("Password: ",NULL,NULL,0);
	t = timerset(600);
	fail = false;
	i = 0;
	while (!timeup(t) && carrier()) {
	      if (com_scan()) {
		 c = com_getbyte() & 0x7f;
		 if (c == '\r') {
		    if (!strcmp(password,"*") || (password[i] == '\0' && !fail)) {
		       script_outstr("\r\n",NULL,NULL,0);
		       return (true);
		    }
		    break; /*false*/
		 }
		 if (c != '\b' && c < ' ' || c > '~')
		    continue;
		 com_putbyte('.');
		 if (c == password[i]) i++;
		 else		       fail = true;
	      }
	      else
		 win_idle();
	}

	script_outstr("\r\nLogin incorrect\r",NULL,NULL,0);
	com_purge();
	return (false);
}/*password_login()*/


/* ------------------------------------------------------------------------- */
static char *script_instr (char *expect, int timeout, WIN_IDX win)
{
	long  t, last_t;
	char *buf;
	int   i, len;

	if (!(len = escape_instr(expect)))
	   return (NULL);
	for (i = 0; i < len; i++)
	    *(expect+i) &= 0x7f;
	if ((buf = (char *) malloc(len)) == NULL)
	   return ("Expect allocation error!");

	i = 0;
	last_t = time(NULL);

	while (1) {
	      if (!carrier()) { free(buf); return (msg_carrier); }
	      if (last_t != (t = time(NULL))) {
		 if (--timeout <= 0) { free(buf); return (msg_timeout); }
		 win_xyprint(script_win,15,7,"%d", timeout);
		 win_clreol(script_win);
		 last_t = t;
	      }
	      if (keyabort()) { free(buf); return (msg_oabort);  }
	      if (!com_scan()) {
		 win_idle();
		 continue;
	      }

	      buf[i] = com_getbyte() & 0x7f;
	      if (win) win_putc(win,buf[i]);
	      if (++i == len) {
		 if (!memcmp(buf,expect,i)) break;
		 memmove(buf,buf + 1,--i);
	      }
	}

	free(buf);
	return (NULL);
}/*script_instr()*/


/* ------------------------------------------------------------------------- */
boolean run_script (SCRIPT *sp, char *loginname, char *password, WIN_IDX win)
{
	char *s, *send, *expect;
	char *p, *q;
	boolean success;
	int timeout;
	int i;

	if ((s = strdup(sp->string)) == NULL) {
	   message(6,"!Script '%s' allocation error!", sp->tag);
	   return (false);
	}

	if (!win || loglevel == 0)
	   message(3,"*Executing script '%s'", sp->tag);

	script_win = win_boxopen(19,13,65,21, CUR_NONE, CON_RAW,
				 xenwin_colours.script.normal, KEY_RAW,
				 LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,
				 xenwin_colours.script.border,
				 "Script",xenwin_colours.script.title);
#if 0
		  +Script======================================+
		  | Script name: bitbike		       |
		  | Login name : agl			       |
		  | Password   : *****			       |
		  +============================================+
		  | Status     : Timeout - doing subexpect     |
		  | Expecting  : 'ogin:'		       |
		  | Timer left : 10			       |
		  +============================================+
#endif
	win_setattrib(script_win,xenwin_colours.script.border);
	win_line(script_win,1,5,47,5,LINE_DOUBLE);
	win_setattrib(script_win,xenwin_colours.script.template);
	win_xyputs(script_win,2,1, "Script name:");
	win_xyputs(script_win,2,2, "Login name :");
	win_xyputs(script_win,2,3, "Password   :");
	win_xyputs(script_win,2,5, "Status     :");
	win_xyputs(script_win,2,6, "Expecting  :");
	win_xyputs(script_win,2,7, "Timer left :");
	win_setattrib(script_win,xenwin_colours.script.normal);
	win_xyputs(script_win,15,1, sp->tag);
	win_xyputs(script_win,15,2, loginname ? loginname : "<none>");
	i = password ? strlen(password) : 6;
	win_xyprint(script_win,15,3, "%*.*s",
	     i, i, password ? "*****************************" : "<none>");
	win_xyputs(script_win,15,5, "Starting");

	success = true;

	for (send = strtok(s," "); send && success; send = strtok(NULL," ")) {
	    if (keyabort()) {
	       message(3,"-Script: %s",msg_oabort);
	       success = false;
	       break;
	    }

	    /* first a send string */
	    script_outstr(send,loginname,password,0);

	    /* then an expect string (with possible subexpect thingies) */
	    if ((expect = strtok(NULL," ")) == NULL)
	       break;

	    win_xyputs(script_win,15,5,"Send ok");
	    win_clreol(script_win);

	    while (1) {
		  if ((send = strchr(expect,'-')) != NULL)
		     *send++ = '\0';

		  /* default timeout on expect strings: 10 secs */
		  timeout = 10;
		  if ((p = strrchr(expect,'\\')) != NULL && p[1] == 'W') {
		     i = (int) strtol(p + 2,&q,10);
		     if (q != p + 2 && *q == '\0') {
			timeout = i;
			*p = '\0';
		     }
		  }

		  if (loglevel == 0)
		     message(0,"+Expecting '%s' (timeout=%d)", expect, timeout);
		  win_xyprint(script_win,15,6,"'%s'", expect);
		  win_clreol(script_win);
		  win_xyprint(script_win,15,7,"%d", timeout);
		  win_clreol(script_win);
		  p = script_instr(expect,timeout,win);
		  if (!p) {
		     win_xyputs(script_win,15,5,"Expect ok");
		     win_clreol(script_win);
		     break;
		  }

		  if (p != msg_timeout || !send) {
		     message(3,"-Script: %s",p);
		     success = false;
		     break;
		  }

		  if (loglevel == 0)
		     message(0,"-Timeout on expect - doing subexpect");
		  win_xyputs(script_win,15,5,"Timeout - doing subexpect");
		  win_clreol(script_win);
		  if ((expect = strchr(send,'-')) != NULL)
		     *expect++ = '\0';
		  script_outstr(send,loginname,password,0);
	    }
	}

	free(s);
	win_close(script_win);

	if (success && (!win || loglevel == 0))
	   message(3,"+Script: %s", msg_success);

	return (success);
}/*run_script()*/


/* end of script.c */
