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


#define XENIA_MAIN	/* define as main module */
#include "xenia.h"


#define END_BATCH (-1)

char progname[30];
long tonline;


static void Speed(void) /* Ask the user for the new speedrate */
{			/* and install then */
	char  buffer[11];
	dword spd;

	print("\r\n Select speed: ");
	if (*get_str(buffer,10)) {
	   spd = atol(buffer);
	   print(" New speed: %lu bps\r\n\r\n",spd);
	   mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
	   memset(&clip,0,sizeof (CLIP));
	   arqlink = isdnlink = iplink = false;
	   setspeed(spd,false);
	}
}


static void down_load (void)
{
	int  ch;
	char name[100];

	print("\r\nReceive protocol:\r\n");
#ifndef NOHYDRA
	print("[X]modem, [M]odem7, [T]elink, [S]EAlink, [Z]modem or [H]ydra\r\n");
#else
	print("[X]modem, [M]odem7, [T]elink, [S]EAlink or [Z]modem\r\n");
#endif

	telink = xmodem = modem = 0;
 
	do ch = toupper(win_keygetc());
	while (ch!=Enter && ch!='X' && ch!='M' && ch!='T' && ch!='S' && ch!='Z' && ch!='H');
    
	switch (ch) {
	       case 'X': print(" Filename (max 79 characters):\r\n");
			 get_str(name,79);
			 xmodem = 1;
			 if (!name[0])
			    print("- No file specified!\r\n");
			 else {
			    filewinopen();
			    wzgetfunc = NULL;
			    rcvfile(download,name);
			    alarm();
			    win_close(file_win);
			 }
			 break;
	       case 'T': telink = 1;
	       case 'M': modem = xmodem = 1;
	       case 'S': filewinopen();
			 wzgetfunc = NULL;
			 batchdown();
			 alarm();
			 win_close(file_win);
			 break;
	       case 'Z': filewinopen();
			 get_Zmodem();
			 alarm();
			 win_close(file_win);
			 break;
#ifndef NOHYDRA
	       case 'H': filewinopen();
			 hydra_init(hydra_options);
			 hydra(NULL,NULL);
			 hydra_deinit();
			 alarm();
			 win_close(file_win);
			 break;
#endif
	       default:  break;
	}
	print("\r\n\r\n");
}/*down_load()*/



static void upload (void)
{
	int  ch;
	char name[100];

	print("\r\nTransmit protocol:\r\n");
#ifndef NOHYDRA
	print("[X]modem, [M]odem7, [T]elink, [S]EAlink, [Z]modem or [H]ydra\r\n");
#else
	print("[X]modem, [M]odem7, [T]elink, [S]EAlink or [Z]modem\r\n");
#endif

	mail = telink = xmodem = modem = 0;

	do ch = toupper(win_keygetc());
	while (ch!=Enter && ch!='X' && ch!='M' && ch!='T' && ch!='S' && ch!='Z' && ch!='H');

	if (ch != Enter) {
	  print(" Filename%s (max 79 characters):\r\n", ch=='X' ? "" : "(s)");
	  get_str(name,79);
	}	     

	switch (ch) {
	       case 'X': xmodem=1;
			 if (!*name)
			    print("- No file specified!\r\n");
			 else {
			    filewinopen();
			    xmtfile(name,NULL);
			    alarm();
			    win_close(file_win);
			 }
			 break;
	       case 'T': telink=1;
	       case 'M': modem=xmodem=1;
	       case 'S': filewinopen();
			 batchup(name,'S');
			 end_batch();
			 alarm();
			 win_close(file_win);
			 break;
	       case 'Z': filewinopen();
			 if (!batchup(name,'Z'))
			    send_Zmodem(NULL,NULL,0,END_BATCH,0);
			 alarm();
			 win_close(file_win);
			 break;
#ifndef NOHYDRA
	       case 'H': filewinopen();
			 hydra_init(hydra_options);
			 if (!batchup(name,'H'))
			    hydra(NULL,NULL);
			 hydra_deinit();
			 alarm();
			 win_close(file_win);
			 break;
#endif
	       default:  break;
	}
	print("\r\n\r\n");
}/*upload()*/


static WIN_IDX	   term_win;
static char	  *user_password;
static TERMSCRIPT *user_termscript;


static int mdail(int contin)
{
     char pstring[80], number[85];
     struct _nodeinfo *n;
     dword speed;
     int i, call_one=1, didphone;
     PHONE *pp;
     IEMSI_PWD	*ip;
     TERMSCRIPT *tp;

     if (!contin) get_numbers(0);

     n = &remote;
     win_settop(modem_win);
     while(call_one)
     {
	  call_one=0;
	  for (i=0;i<20;i++)
	  {
	       if (dial[i].zone==-2) break;
	       if (dial[i].zone==-3) continue;
	       if (dial[i].zone!=-1)
	       {
		    if (dial[i].zone!=n->zone || dial[i].net!=n->net ||
			dial[i].node!=n->node)
		    {
			n->zone= dial[i].zone;
			n->net=  dial[i].net;
			n->node= dial[i].node;
			nodetoinfo(n);
		    }
		    if (n->speed < mdm.speed) speed=n->speed;
		    else speed=mdm.speed;
		    if (dial[i].speed) speed=dial[i].speed;
		    message(2,"+Calling %s", n->name);
		    strcpy(number,n->phone);
	       }
	       else
	       {
		    n->zone = -1;
		    strcpy(number,dial[i].phone);
		    speed= dial[i].speed;
	       }

	       didphone = 0;
	       if (n->zone >= 0) {
		  sprint(pstring,"%d:%d/%d%s",n->zone,n->net,n->node,pointnr(n->point));

		  for (pp = phone; pp; pp = pp->next) {
		      if (n->zone == pp->zone && n->net   == pp->net &&
			  n->node == pp->node && n->point == pp->point) {
			 char *p, *q;

			 didphone = 1;

			 if (strpbrk(pp->number,":/")) {
			    int tzone, tnet, tnode, tpoint;

			    getaddress(pp->number,&tzone,&tnet,&tnode,&tpoint);
			    nlidx_getphone(tzone,tnet,tnode,tpoint,&q,&p);
			    if (!q) continue;
			    strcpy(number,q);
			    if (pp->nodeflags) p = pp->nodeflags;
			 }
			 else {
			    strcpy(number,pp->number);
			    p = pp->nodeflags ? pp->nodeflags : nlidx_getphone(pp->zone,pp->net,pp->node,pp->point,NULL,NULL);
			 }

			 mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
			 memset(&clip,0,sizeof (CLIP));
			 arqlink = isdnlink = iplink = false;
			 if (speed) setspeed(speed,false);
			 com_purge();
			 ptt_reset();
			 speed = dail(pstring,number,p);
			 if (speed > 256) break;
			 mdm_reset();
			 setspeed(mdm.speed,false);
			 if (!speed || (ptt_cost && ptt_units))
			    break;
		      }
		  }
	       }
	       else
		  *pstring = '\0';

	       if (!didphone) {
		  mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
		  memset(&clip,0,sizeof (CLIP));
		  arqlink = isdnlink = iplink = false;
		  if (speed) setspeed(speed,false);
		  com_purge();
		  ptt_reset();
		  if (n->zone >= 0)
		     speed = dail(pstring,number,nlidx_getphone(n->zone,n->net,n->node,n->point,NULL,NULL));
		  else
		     speed = dail(number,number,NULL);
	       }

	       if (speed > 256) {
		  if (cost != NULL) {
		     fprint(cost,"$D $aM $Y $t  Connected to %s",number);
		     if (dial[i].zone != -1)
			fprint(cost," (%d:%d/%d%s%s)",n->zone,n->net,n->node,
				    pointnr(n->point),zonedomabbrev(n->zone));
		     fprint(cost," for ",number);
		     fflush(cost);
		  }
		  tonline = time(NULL);
		  print("\r\n");
		  (void) win_bell();
		  (void) win_bell();
		  dial[i].zone = -3;
		  user_password   = NULL;
		  user_termscript = NULL;
		  for (ip = iemsi_pwd; ip; ip = ip->next) {
		      if ((*pstring && patimat(pstring,ip->pattern)) ||
			  patimat(number,ip->pattern)) {
			 user_password = ip->password;
			 break;
		      }
		  }
		  for (tp = termscript; tp; tp = tp->next) {
		      if ((*pstring && patimat(pstring,tp->pattern)) ||
			  patimat(number,tp->pattern)) {
			 user_termscript = tp;
			 run_script(user_termscript->scriptptr,
				    user_termscript->loginname ? user_termscript->loginname : iemsi_profile.name,
				    user_termscript->password ? user_termscript->password : user_password,
				    term_win);
			 break;
		      }
		  }
		  alarm();
		  /*com_putbyte('\r');*/	/* could screw up unix login */
		  /*com_putbyte(' ');*/ 	/* ... too ...		     */

		  return (1);
	       }
	       else {
		  call_one++;

#if PC
		  if (ptt_cost && ptt_units) {
		     word ptt_money = (word) (ptt_units * ptt_cost);

		     message(6,"$Charged no connect! COST: %s%u.%02u (%u unit%s)",
			     ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
			     ptt_units, ptt_units == 1 ? "" : "s");
		     if (cost != NULL) {
			fprint(cost,"$D $aM $Y $t  Charged no connect %s",number);
			if (dial[i].zone != -1)
			   fprint(cost," (%d:%d/%d%s%s)",n->zone,n->net,n->node,
				       pointnr(n->point), zonedomabbrev(n->zone));
			fprint(cost,", COST: %s%u.%02u (%u unit%s)\n",
			       ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
			       ptt_units, ptt_units == 1 ? "" : "s");
			fflush(cost);
		     }
		     ptt_reset();
		     dial[i].zone = -3;
		  }
#endif

		  if (speed & MDMRSP_BADCALL) {
		     message(6,"$Charged no connect!");
		     dial[i].zone = -3;
		  }
	       }

	       if (!speed) {
		  user_password   = NULL;
		  user_termscript = NULL;
		  return (1);
	       }
	  }
     }

     user_password   = NULL;
     user_termscript = NULL;

     return (0);
}

Term(int noexit)
{
	int half_dupl = 0;	/* start in full-duplex 	    */
	int extkeys = 0;	/* start with extended key mode off */
	register int c, t;
	register int strip = 0xff;
	long sleeptimer = 0L;
	char cap_log[80];
	FILE *cap = NULL, *tmp;
	int use_cap = 0;
	boolean have_dcd;
	WIN_IDX statline_win, savlog_win, remchat_win = 0, savterm_win = 0;
#ifndef NOHYDRA
	static char *hauto = "\030cA\\f5\\a3\030a";
	register char *hseek = hauto;
#endif
	static char *zauto = "**\030B0";
	register char *zseek = zauto;
	char *emustr;
	char emsibuf[EMSI_IDLEN + 1];
	int seen_iemsireq;
	boolean done_iemsi;

	savlog_win = log_win;
	have_dcd = false;
	tonline = 0L;
	dtr_out(1);
	ptt_enable();
	ptt_reset();
	remote.capabilities = 0; /* be sure protocols know we're NOT mailing */
	mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
	memset(&clip,0,sizeof (CLIP));
	arqlink = isdnlink = iplink = false;
	user_password	= NULL;
	user_termscript = NULL;

	/*mcur_disable();*/
#if ATARIST
	log_win = term_win = win_open(1,1,win_maxhor,win_maxver - 1, CUR_NORMAL,
				      CON_VT52 | CON_WRAP | CON_SCROLL,
				      xenwin_colours.term.normal, KEY_VT52);
#else
	log_win = term_win = win_open(1,1,win_maxhor,win_maxver - 1, CUR_NORMAL,
				      CON_AVATAR | CON_WRAP | CON_SCROLL,
				      xenwin_colours.term.normal, KEY_ANSI);
#endif
	emustr = "AVT0";
	statline_win = win_open(1,win_maxver,win_maxhor,win_maxver, CUR_NONE, CON_RAW,
				xenwin_colours.statline.normal, KEY_RAW);
	win_settop(term_win);

#if 0
	if (xenpoint_ver && !un_attended && dial[0].zone >= 0) {
	   win_keypurge();
	   win_keypush(Alt_Q);
	}
#endif

again:	win_xyprint(statline_win,1,1,
		    " %-10s � %s � Echo: %-3s � %s: %-6lu� Log: %-12s � Strip8: %-3s ",
		    extkeys ? "Alt-= Ext!" : "Alt-Z Help",
		    emustr,
		    half_dupl ? "On":"Off",
		    have_dcd ? "<DCD>" : "Speed",
		    cur_speed,
		    use_cap ? capture : "Off",
		    strip & 0x80 ? "Off":"On");
	if (!carrier())
	   sleeptimer = time(NULL) + (blank_secs ? blank_secs : 300);

	do {
	     if (win_keyscan()) {     /* key pressed? */

		c = win_keygetc();    /* get it */
		if (extkeys && c >= 0x100 && c != 0x183) {
		   com_putbyte('\0');
		   c &= 0x0ff;
		}
		switch (c) {	      /* function key? */
		  case 0x183: extkeys = !extkeys;	/* Alt-= */
			      if (extkeys)
				 win_setkeyemu(term_win,KEY_RAW);
			      else
				 win_setkeyemu(term_win,(win_getconemu(term_win) & CON_EMUMASK) == CON_VT52 ? KEY_VT52 : KEY_ANSI);
			      goto again;
		  case Alt_Z: show_help(0); break;
      case F1: case F2: case F3: case F4: case F5:
      case F6: case F7: case F8: case F9: case F10:
      case F11: case F12:
		  if (c > F10) c = (c - F11) + 11;
		  else	       c = (c - F1) + 1;
		  if (funkeys[c] != NULL)
		     script_outstr(funkeys[c], NULL, NULL,
				   (half_dupl || remchat_win) ? term_win : 0);
		  break;
		  case Alt_P: Speed(); goto again;
		  case Alt_M: break;
		  case Alt_E: half_dupl= !half_dupl; goto again;
		  case Alt_B: strip ^= 0x80; goto again;
		  case Alt_A: switch (win_getconemu(term_win) & CON_EMUMASK) {
				     case CON_ANSI:
					  win_setconemu(term_win,CON_AVATAR | CON_WRAP | CON_SCROLL);
					  win_setkeyemu(term_win,KEY_ANSI);
					  emustr = "AVT0";
					  break;
				     case CON_AVATAR:
					  win_setconemu(term_win,CON_VT52 | CON_WRAP | CON_SCROLL);
					  win_setkeyemu(term_win,KEY_VT52);
					  emustr = "VT52";
					  break;
				     case CON_VT52:
				     default:
					  win_setconemu(term_win,CON_ANSI | CON_WRAP | CON_SCROLL);
					  win_setkeyemu(term_win,KEY_ANSI);
					  emustr = "ANSI";
					  break;
			      }
			      if (remchat_win) {
				 win_setconemu(remchat_win,win_getconemu(term_win));
				 win_setconemu(savterm_win,win_getconemu(term_win));
				 win_setkeyemu(savterm_win,win_getkeyemu(term_win));
			      }
			      goto again;
		  case Alt_H: hangup(); break;
		  case Alt_Q: mdail(1);
			      if (remchat_win)
				 win_settop(remchat_win);
			      win_settop(term_win);
			      emsibuf[0] = '\0';
			      seen_iemsireq = 0;
			      done_iemsi = false;
			      goto again;
		  case Alt_S: if (!script)
				 break;
			      { char loginname[21], password[21];
				SCRIPT *sp;
				int i, n, len;
				WIN_SELECT list_select;
				struct _select_item *select_items;

				for (n = len = 0, sp = script; sp; n++, sp = sp->next)
				    if (strlen(sp->tag) > len) len = strlen(sp->tag);
				select_items = (struct _select_item *) calloc(n,sizeof (struct _select_item));
				for (i = 0, sp = script; sp; i++, sp = sp->next)
				    select_items[i].text = sp->tag;

				list_select.select_flags     = SELECT_SCROLLBAR;
				list_select.select_border    = LINE_DOUBLE;
				list_select.select_left      = 10;
				list_select.select_top	     = 5;
				list_select.select_width     = 15;
				list_select.select_depth     = 10;
				list_select.select_title     = "Select script";
				list_select.select_win	     = 0;
				list_select.atr_normal	     = xenwin_colours.select.normal;
				list_select.atr_select	     = xenwin_colours.select.select;
				list_select.atr_border	     = xenwin_colours.select.border;
				list_select.atr_title	     = xenwin_colours.select.title;
				list_select.atr_scrollbar    = xenwin_colours.select.scrollbar;
				list_select.atr_scrollselect = xenwin_colours.select.scrollselect;
				list_select.num_items	     = (word) n;
				list_select.opt_last	     = 0;
				list_select.opt_topline      = 0;
				list_select.opt_scrollbar    = 0;
				list_select.item	     = select_items;
				list_select.init	     = NULL;
				list_select.action	     = NULL;

				switch (win_select(&list_select)) {
				       case ' ':
				       case Enter:
				       case MKEY_LEFTBUTTON:
				       case MKEY_CURBUTTON:
					    print(" Login name: "); get_str(loginname,20);
					    print(" Password  : "); get_str(password,20);
					    n = list_select.opt_last;
					    for (i = 0, sp = script; i < n; i++, sp = sp->next);
					    run_script(sp,loginname,password,term_win);
					    break;

				       default:
					    break;
				}

				free(select_items);
			      }
#if 0
			      { char scripttag[21], loginname[21], password[21];
				print("\r\n Select script: ");
				if (*get_str(scripttag,20)) {
				   SCRIPT *sp;

				   print(" Login name: "); get_str(loginname,20);
				   print(" Password  : "); get_str(password,20);
				   for (sp = script; sp; sp = sp->next)
				       if (!stricmp(scripttag,sp->tag)) break;
				   if (sp) run_script(sp,loginname,password,term_win);
				   else    print(" No script with tag '%s'\r\n");
				}
			      }
#endif
			      break;
		  case Alt_R: down_load();
			      break;
		  case Alt_U:
		  case Alt_T: upload();
			      break;
		  case Alt_W:
		  case Alt_C: win_cls(term_win);
			      break;
		  case Alt_D: mdail(0);
			      if (remchat_win)
				 win_settop(remchat_win);
			      win_settop(term_win);
			      emsibuf[0] = '\0';
			      seen_iemsireq = 0;
			      done_iemsi = false;
			      goto again;
		  case Alt_J: dos_shell(1);
			      goto again;
		  case Alt_I: if (user_termscript && user_termscript->loginname) {
				 com_putblock((byte *) user_termscript->loginname,
					      (word) strlen(user_termscript->loginname));
				 com_putbyte('\r');
			      }
			      break;
		  case Alt_O: if (user_termscript && user_termscript->password) {
				 com_putblock((byte *) user_termscript->password,
					      (word) strlen(user_termscript->password));
				 com_putbyte('\r');
			      }
			      else if (user_password) {
				 com_putblock((byte *) user_password,
					      (word) strlen(user_password));
				 com_putbyte('\r');
			      }
			      break;
		  case Alt_X: if (noexit)
				 message(6,"*Returning to mailer");
			      else {
				 print("\r\n* Exit program [Y/N]? ");
				 do c = toupper(win_keygetc());
				 while (c != 'N' && c != 'Y');
				 print("%c\r\n\r\n",(short) c);
				 if (c != 'Y') break;
			      }

			      if (remchat_win) {
				 win_close(remchat_win);
				 win_close(term_win);
				 log_win = term_win = savterm_win;
				 print("\r\n\r\n");
			      }
			      if (cap!=NULL) {
				    tmp=log;
				    log=cap;
				    fprint(cap,"\n");
				    message(300,"*End");
				    log=tmp;
				    fclose(cap);
			      }
			      log_win = savlog_win;
			      win_close(term_win);
			      win_close(statline_win);
			      win_settop(log_win);
			      /*mcur_enable();*/
			      ptt_disable();
			      return (1);
		  case Alt_L: /* capturelog */
    print("\r\nType OFF to switch the terminal log off or the name of the file to use\r\n");
    print("<return> for '%s'\r\n",capture);
			      get_str(cap_log,12);
			      if (strnicmp(cap_log,"OFF",3))
			      {
				 if (cap!=NULL)
				 {
				    tmp=log;
				    log=cap;
				    fprint(cap,"\n");
				    message(300,"*End");
				    log=tmp;
				    fclose(cap);
				    cap=NULL;
				 }
				 if (*cap_log)
				 {
					if (*capture) free(capture);
					capture=ctl_string(cap_log);
				 }
				 sprint(cap_log,"%s%s",download,capture);
				 if ((cap=sfopen(cap_log,"a+b",DENY_WRITE))==NULL)
				 {
				    message(6,"-Unable to open %s",cap_log);
				    use_cap=0;
				    break;
				 }
				 tmp=log;
				 log=cap;
				 message(300,"*Start");
				 log=tmp;
				 use_cap=1;
			      }
			      else
			      {
				 if (cap!=NULL)
				 {
				    tmp=log;
				    log=cap;
				    fprint(cap,"\n");
				    message(300,"*End");
				    log=tmp;
				    fclose(cap);
				    cap=NULL;
				 }
				 use_cap=0;
			      }
			      print("\r\n\r\n");
			      goto again;

		  case Alt_F: goto iemsichat;

		  case Esc:   if (remchat_win) {
				 win_close(remchat_win);
				 win_close(term_win);
				 log_win = term_win = savterm_win;
				 print("\r\n\r\n");
				 remchat_win = 0;
				 break;
			      }
			      /* fallthrough to default */

		  default   : if (c >= 0x100)
				 break;
			      com_putbyte(c);		/* transmit char */
			      if (half_dupl || remchat_win) {
				 if (c == BS || c == Ctrl_BS) {
				    win_puts(term_win,"\b \b");
				    if (!remchat_win && cap && ftell(cap) > 0L)
				       fseek(cap,-1l,1);
				 }
				 else if (c == Enter) {
				    win_putc(term_win,'\n');
				    if (!remchat_win && cap)
				       fprint(cap,"\n");
				 }
				 else {
				    win_putc(term_win,c);
				    if (!remchat_win && cap)
				       fprint(cap,"%c",(short) c);
				 }
			      }
		}

		if (!carrier())
		   sleeptimer = time(NULL) + (blank_secs ? blank_secs : 300);
	     }

	     if (com_scan()) {		/* received char? */
		t = com_getbyte();	/* get it, and to display! */
		if (half_dupl && (t&strip)=='\r')
		   win_putc(remchat_win ? remchat_win : term_win,'\n');
		else
		   win_putc(remchat_win ? remchat_win : term_win,t & strip);
		if (use_cap && cap!=NULL) fprint(cap,"%c",(short) (t & strip));

		if (user_password) {
		   switch (check_emsi(t,emsibuf)) {
			  case EMSI_REQ:
			       if (seen_iemsireq++ < 3)
				  put_emsi(EMSI_CLI,true);
			       break;

			  case EMSI_IRQ:
			       if (done_iemsi) break;
			       client_iemsi(user_password,
					    (win_getconemu(term_win) & CON_EMUMASK));
			       done_iemsi = true;
			       win_putc(term_win,'\n');
			       (void) win_bell();
			       break;

			  case EMSI_CHT:
			       if (!done_iemsi) break;
			       put_emsi(EMSI_ACK,true);

iemsichat:		       if (!remchat_win) {
				  byte left, top, right, bottom;

				  remchat_win = win_create(1,1,win_maxhor,win_maxver / 2,
							   CUR_NORMAL,
							   win_getconemu(term_win),
							   xenwin_colours.statline.normal,
							   win_getkeyemu(term_win));
				  win_fill(remchat_win,177);
				  win_print(remchat_win," Remote ");
				  win_setrange(remchat_win,1,2,win_maxhor,win_maxver / 2);
				  win_setattrib(remchat_win,xenwin_colours.term.normal);
				  win_cls(remchat_win);
				  win_settop(remchat_win);

				  savterm_win = term_win;
				  term_win = win_create(1,(win_maxver / 2) + 1,win_maxhor,win_maxver - 1,
							CUR_NORMAL,
							win_getconemu(term_win),
							xenwin_colours.statline.normal,
							win_getkeyemu(term_win));
				  log_win = term_win;
				  win_setattrib(term_win,xenwin_colours.statline.normal);
				  win_fill(term_win,177);
				  win_print(term_win," Local ");
				  win_setpos(term_win,43,1);
				  win_print(term_win," Press [ESC] to exit splitscreen mode ");
				  win_getrange(term_win,&left,&top,&right,&bottom);
				  win_setrange(term_win,left,top + 1,right,bottom);
				  win_setattrib(term_win,xenwin_colours.term.normal);
				  win_cls(term_win);
				  win_settop(term_win);
			       }

			       break;

			  case EMSI_TCH:
			       if (remchat_win) {
				  win_close(remchat_win);
				  win_close(term_win);
				  log_win = term_win = savterm_win;
				  print("\r\n\r\n");
				  remchat_win = 0;
			       }

			       if (done_iemsi)
				  put_emsi(EMSI_ACK,true);
			       break;
		   }
		}

#ifndef NOHYDRA
		if (t == *hseek) {
		   if (!*++hseek) {
		      win_keypush('H');
		      upload();
		      hseek = hauto;
		   }
		}
		else
		   hseek = hauto;
#endif

		if (t == *zseek) {
		   if (!*++zseek) {
		      t = com_getc(3);
		      if (t == '1') {
			 win_keypush('Z');
			 upload();
		      }
		      else if (t == '0') {
			 filewinopen();
			 get_Zmodem();
			 win_close(file_win);
			 print("\r\n\r\n");
		      }
		      zseek = zauto;
		   }
		}
		else
		   zseek = zauto;
	     }
	     else
		win_idle();

	     if (carrier()) {
		sleeptimer = 0L;
		if (!have_dcd) {
		   have_dcd = true;
		   goto again;
		}
	     }
	     else {
		if (tonline != 0L || ptt_units) {
		   long ontime = tonline ? (time(NULL) - tonline) : 0L;
		   word ptt_money;

		   if (remchat_win) {
		      win_close(remchat_win);
		      win_close(term_win);
		      log_win = term_win = savterm_win;
		      print("\r\n\r\n");
		      remchat_win = 0;
		   }

#if PC
		   if (ptt_cost) {
		      ptt_money = (word) (ptt_units * ptt_cost);
		      message(6,"$Connection for %ld:%02ld min., COST: %s%u.%02u (%u unit%s)",
			      ontime / 60L, ontime % 60L,
			      ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
			      ptt_units, ptt_units == 1 ? "" : "s");
		   }
		   else
#endif
		      message(6,"$Connection for %ld:%02ld min.",
			      ontime / 60L, ontime % 60L);

		   if (cost != NULL) {
		      fprint(cost,"%ld:%02ld min.",
			     ontime / 60L, ontime % 60L);
#if PC
		      if (ptt_cost)
			 fprint(cost,", COST: %s%u.%02u (%u unit%s)",
				ptt_valuta, (word) (ptt_money / 100), (word) (ptt_money % 100),
				ptt_units, ptt_units == 1 ? "" : "s");
#endif
		      fprint(cost,"\n");
		      fflush(cost);
		   }
		   tonline = 0L;
		   ptt_reset();

		   mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
		   memset(&clip,0,sizeof (CLIP));
		   arqlink = isdnlink = iplink = false;
		   emsibuf[0] = '\0';
		}

		if (!sleeptimer)
		   sleeptimer = time(NULL) + (blank_secs ? blank_secs : 300);
		else if (time(NULL) > sleeptimer) {
		   message(6,"+Terminal idle timeout");
		   break;
		}

		if (have_dcd) {
		   have_dcd = false;
		   goto again;
		}
	     }
	}
	while (c != Alt_M);	/* Alt_M -> back to main */

	if (remchat_win) {
	   win_close(remchat_win);
	   win_close(term_win);
	   log_win = term_win = savterm_win;
	   print("\r\n\r\n");
	}

	if (cap!=NULL)
	{
	    tmp=log;
	    log=cap;
	    fprint(cap,"\n");
	    message(300,"*End");
	    log=tmp;
	    fclose(cap);
	}
	log_win = savlog_win;
	win_close(term_win);
	win_close(statline_win);
	win_settop(log_win);
	/*mcur_enable();*/
	ptt_disable();
	return (0);
}


/* ------------------------------------------------------------------------- */
#if TURBOST
char **environ;
#endif


/* ------------------------------------------------------------------------- */
int main(int argc, char *argv[], char *env[])
{
	char *p;

#if TURBOST
	environ = env;
#else
	env = env;     /* To get rid of 'never used warning'; optimizes away */
#endif

	setvbuf(stderr,NULL,_IONBF,256);

	sprint(progname,"%s %s %s", PRGNAME, VERSION, XENIA_OS);

	fprint(stderr, "%s; Version %s %s, Created on %s at %s\n",
		       PRGNAME, VERSION, XENIA_OS, __DATE__, __TIME__);
	fprint(stderr, "Copyright (C) 1987-2001 Arjen G. Lentz\n");
	fprint(stderr, "Xenia comes with ABSOLUTELY NO WARRANTY.  This is free software,\n");
	fprint(stderr, "and you are welcome to redistribute it under certain conditions;\n");
	fprint(stderr, "See COPYING file for full details.\n\n");

#if PC
	if (_osmajor < 3) {
	   fprint(stderr,"Sorry, Xenia requires DOS 3.0 or higher!\n");
	   exit (1);
	}
#endif

#ifdef __OVERLAY__
	_OvrInitEms(0,0,0);
#endif

	dos_sharecheck();

#if PC || OS2 || WINNT
	putenv("TZ=GMT0");
	tzset();
#endif

	/* Try to find the path for Xenia */
	if ((p = getenv("MAILER")) != NULL) {
	   if (!*(homepath = ctl_dir(p)))
	      return (1);
	}

	if (init_conf(argc,argv))
	   exit (1);

	if (!win_init(50,CUR_NORMAL,CON_COOKED|CON_WRAP|CON_SCROLL|CON_UNBLANK,
		      xenwin_colours.desktop.normal, KEY_RAW)) {
	   fprint(stderr,"Can't initialize window system!\n");
	   exit (1);
	}

	mcur_enable();
	mcur_hide();

	init_mailermenu();
	topwinopen();
	statuswinopen();
	modemwinopen();
	logwinopen();
	helpwinopen();

	win_xyprint(top_win,2,1, progname);

	blank_time = time(NULL);
	show_stats();

	win_settop(log_win);
	sys_init();

	flag_session(false);

	nlidx_init();

	if (hotstart) {
	   un_attended = 1;
	   setspeed(0,false);			/* Just print, don't change! */
	   fprint(log,"\n");
	   message(6,"*%s : HotStart (%lu%s)",progname,cur_speed,errorfree);
	   recv_session();
	   hangup();
	   rm_requests();
	   statusmsg(NULL);
	   /*find_event();*/
	   /* This doesn't work now because we can't call find_event() !! */
	   if (got_arcmail || got_bundle || got_mail) /* We got inbound mail */
	      receive_exit();
	   errl_exit(0,false,NULL);
	}

	if (cmdlineterm) {
	   setspeed(mdm.speed,false);
#if JAC
	   phone_out(0);
	   if (!carrier())	/* yes, this is done on purpose! */
#endif
	   {
	      if (!strchr(mdm.hangup,'+')) mdm_puts(mdm.hangup);
	      if (init_modem()) errl_exit(1,false,NULL);
	      if (carrier()) hangup();
	      mdm_terminit();
	   }
	   Term(0);
	   errl_exit(0,false,NULL);
	}

	/* Main loop */

	if (cmdlineanswer) {
	   win_keypurge();
	   win_keypush(Alt_A);
	}
	else {
	  if (!strchr(mdm.hangup,'+')) mdm_puts(mdm.hangup);
	  if (init_modem()) errl_exit(1,false,NULL);
	  if (carrier()) hangup();
	}
	unattended();	/* always uses errl_exit() to get out */

	return 1;
}/*main()*/


/* end of xenia.c */
