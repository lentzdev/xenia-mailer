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


/* Window routines */
#include "xenia.h"


void logwinopen()
{
	byte left, top, right, bottom;

	log_win = win_open(1,14,win_maxhor,win_maxver - 1, CUR_NORMAL,
			   CON_COOKED|CON_WRAP|CON_SCROLL|CON_UNBLANK,
			   xenwin_colours.log.normal, KEY_RAW);
	win_setattrib(log_win,xenwin_colours.log.border);
	win_line(log_win,1,1,win_maxhor,1,LINE_SINGLE);
	win_setattrib(log_win,xenwin_colours.log.title);
	win_xyputs(log_win,2,1,"Logfile");
	win_setattrib(log_win,xenwin_colours.log.normal);
	win_getrange(log_win,&left,&top,&right,&bottom);
	win_setrange(log_win,left,top + 1,right,bottom);
}/*logwinopen()*/


void mailerwinopen()
{
	char buf[80];

	sprint(buf,"Mail session (started $D $aM $t)");
	mailer_win = win_boxopen(19,4,65,12, CUR_NONE, CON_RAW,
				 xenwin_colours.mailer.normal, KEY_RAW,
				 LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,
				 xenwin_colours.mailer.border,
				 buf,xenwin_colours.mailer.title);
	win_setattrib(mailer_win,xenwin_colours.mailer.border);
	win_line(mailer_win, 1, 6,47, 6,LINE_DOUBLE);	    /* top structure */
	win_line(mailer_win,24, 6,24, 9,LINE_DOUBLE);
	win_setattrib(mailer_win,xenwin_colours.mailer.hotnormal);
	win_xyputs(mailer_win, 2,5,"Received");
	win_xyputs(mailer_win,25,5,"Transmitted");
	win_setattrib(mailer_win,xenwin_colours.mailer.template);
	win_xyputs(mailer_win, 2,2,"System:");
	win_xyputs(mailer_win, 2,3,"Sysop :");
	win_xyputs(mailer_win, 2,4,"Mailer:");
	win_xyputs(mailer_win, 2,6,"Mail :");
	win_xyputs(mailer_win,25,6,"Mail :");
	win_xyputs(mailer_win, 2,7,"Files:");
	win_xyputs(mailer_win,25,7,"Files:");
	win_setattrib(mailer_win,xenwin_colours.mailer.normal);
	win_xyputs(mailer_win,10,2,"---");
	win_xyputs(mailer_win,10,3,"---");
	win_xyputs(mailer_win,10,4,"---");

	filewinopen();
	filewincls("---",0);
}


void show_mail()
{
	win_xyprint(mailer_win, 8,6,"%5u (%uK)",(word) ourbbs.nrmail,(word) ourbbs.kbmail);
	win_xyprint(mailer_win,31,6,"%5u (%uK)",(word) remote.nrmail,(word) remote.kbmail);
	win_xyprint(mailer_win, 8,7,"%5u (%uK)",(word) ourbbs.nrfiles,(word) ourbbs.kbfiles);
	win_xyprint(mailer_win,31,7,"%5u (%uK)",(word) remote.nrfiles,(word) remote.kbfiles);
}


void filewinopen()
{
	file_win = win_boxopen(19,13,65,21, CUR_NONE, CON_RAW,
			       xenwin_colours.file.normal, KEY_RAW,
			       LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,
			       xenwin_colours.file.border,
			       "File transfer",xenwin_colours.file.title);
	win_setattrib(file_win,xenwin_colours.file.border);
	win_line(file_win, 1,5,47,5,LINE_DOUBLE);
	win_line(file_win,24,5,24,9,LINE_DOUBLE);
	win_setattrib(file_win,xenwin_colours.file.normal);
}


void filewincls(char *protocol,int prottype)
/*int prottype; 	       /+ prottype: 0=EMSI/YooHOO/Terminal, 1=Modem7 */
{				  /* 2=SEAlink/Xmodem/etc, 3=Zmodem, 4=Hydra */
	win_setattrib(file_win,xenwin_colours.file.template);

#ifndef NOHYDRA
	if (prottype == 4) {
	   win_xyputs(file_win,1,1," Protocol :"); win_clreol(file_win);
	   win_xyputs(file_win,1,2," Last msg :"); win_clreol(file_win);
	   win_xyputs(file_win,1,4," Sending  :"); win_clreol(file_win);
	   win_xyputs(file_win,1,5," Filesize :"); win_clreol(file_win);
	   win_xyputs(file_win,1,6," Receiving:"); win_clreol(file_win);
	   win_xyputs(file_win,1,7," Filesize :"); win_clreol(file_win);
	   win_setattrib(file_win,xenwin_colours.file.border);
	   win_line(file_win,1,4,47,4,LINE_DOUBLE);	 /* hor division     */
	   win_line(file_win,1,5,1,5,LINE_DOUBLE);	 /* rem left cross   */
	   win_line(file_win,47,5,47,5,LINE_DOUBLE);	 /* rem right cross  */
	   win_line(file_win,24,9,24,9,LINE_DOUBLE);	 /* rem bottom cross */
	   win_setattrib(file_win,xenwin_colours.file.hotnormal);
	   win_xyputs(file_win,13,2,"<none>");
	   win_setattrib(file_win,xenwin_colours.file.normal);
	   win_xyprint(file_win,13,1,"HYDRA (%s-directional mode)", protocol);
	}
	else
#endif
	{
	   win_xyputs(file_win,2,1,"Protocol  :"); win_clreol(file_win);

	   win_setpos(file_win,2,2);
	   if (prottype)
	      win_puts(file_win,"Filename  :");
	   win_clreol(file_win);

	   win_xyputs(file_win,2,3,"Last msg  :"); win_clreol(file_win);

	   if (prottype > 1) {
	      win_xyprint(file_win,2,5,"Filesize  : %8s","");
	      win_xyprint(file_win,2,6,"Transfered: %8s","");
	      win_xyprint(file_win,2,7,prottype == 3 ? "Packetsize: %8s" : "Last ACK  : %8s","");

	      win_xyputs(file_win,25,5,"Left:"); win_clreol(file_win);

	      win_xyputs(file_win,25,6,"CPS :"); win_clreol(file_win);

#if 0
	      win_xyputs(file_win,25,7,prottype == 3 ? "Mach:" : "Ctrl: ---");
	      win_clreol(file_win);
#endif
	   }

	   win_setattrib(file_win,xenwin_colours.file.hotnormal);
	   win_xyputs(file_win,14,3,"<none>");
	   win_setattrib(file_win,xenwin_colours.file.normal);
	   win_xyputs(file_win,14,1,protocol);
	   if (prottype > 1)
	      win_xyputs(file_win,31,6,"---");
	}
}/*filewincls()*/


void show_help(int mail)
{
	WIN_IDX win;
	long t;

	win = win_create(1, mail ? 2 : 1, win_maxhor, win_maxver - 1, CUR_NONE,
			 CON_RAW, xenwin_colours.help.normal, KEY_RAW);
	if (!win)
	   return;

	win_line(win,1,1,1,wp->win_verlen,LINE_DOUBLE);
	win_line(win,1,1,wp->win_horlen,1,LINE_DOUBLE);
	win_line(win,wp->win_horlen,1,
			  wp->win_horlen,wp->win_verlen,LINE_DOUBLE);
	win_line(win,1,wp->win_verlen,
			  wp->win_horlen,wp->win_verlen,LINE_DOUBLE);

	win_xyputs(win,2,1,"Key help");

	win_setrange(win,2,2,wp->win_horlen - 1,wp->win_verlen - 1);

	win_print(win,"\n %s has the following functions in %s mode:\n\n",
			   progname, mail ? "MAILER" : "TERMINAL");

	if (mail) {
#if 0
	   win_puts(win," Alt-W  : Toggle wait for mail (event 0)      Alt-J  : Jump to DOS\n");
	   win_puts(win," Alt-P  : Poll node(s)                        Alt-I  : Crash node(s)\n");
	   win_puts(win," Alt-O  : stOp polling                        Alt-R  : restart events\n");
	   win_puts(win," Alt-C  : Call now                            Alt-A  : Answer phone\n");
	   win_puts(win," Alt-X  : eXit mailer (errorlevel 0)          Alt-T  : Terminal\n");
	   win_puts(win," Alt-G  : Get files                           Alt-S  : Send files\n");
	   win_puts(win," Alt-M  : Quit unattended                     Alt-U  : Start unattended\n");
	   win_puts(win," Alt-Q  : Quit current event\n");
#endif
	}
	else {
	   win_puts(win," Alt P  : Set speed of comport                Alt B : Strip bit 7\n");
	   win_puts(win," Alt M  : Back to mailer                      Alt X : Exit program\n");
	   win_puts(win," Alt E  : Local echo                          Alt H : Hangup\n");
	   win_puts(win," Alt D  : Dial (make queue)                   Alt Q : Finish queue\n");
	   win_puts(win," Alt R  : Download a file                     Alt T : Upload a file\n");
	   win_puts(win," Alt C  : Clear screen                        Alt J : Jump to OS Shell\n");
	   win_puts(win," Alt A  : ANSI/VT-100, Avatar/0+ANSI, VT-52   Alt I : Send login name\n");
	   win_puts(win," Alt L  : Logfile                             Alt O : Send password\n");
	   win_puts(win," Alt =  : Avatar/Doorway key mode             Alt F : Split screen mode\n");
	   win_puts(win," F1-F12 : User defined macros                 Alt S : Execute a script\n");
	}

	win_settop(win);
	win_keypurge();
	t = time(NULL) + 30L;
	while (!win_keyscan() && !com_scan() && time(NULL) < t)
	      win_idle();
	win_close(win);
}


void show_stats()			 /* print information in statswindow */
{
	win_setattrib(schedule_win,xenwin_colours.stats.normal);

	if (last_in)  win_xyputs(status_win,11,3,last_in);
	if (last_bbs) win_xyputs(status_win,11,4,last_bbs);
	if (last_out) win_xyputs(status_win,11,5,last_out);

	win_xyprint(schedule_win, 6, 7,"%5u",(word) extra.calls);
	win_xyprint(schedule_win, 6, 8,"%5u",(word) extra.human);
	win_xyprint(schedule_win, 6, 9,"%5u",(word) extra.mailer);
	win_xyprint(schedule_win, 6,10,"%5u",(word) extra.called);

	win_xyprint(schedule_win,17, 7,"%5u",(word) extra.nrmail);
	win_xyprint(schedule_win,17, 8,"%5u",(word) extra.kbmail);
	win_xyprint(schedule_win,17, 9,"%5u",(word) extra.nrfiles);
	win_xyprint(schedule_win,17,10,"%5u",(word) extra.kbfiles);

	win_setattrib(schedule_win,xenwin_colours.schedule.normal);
}


void event0()
{
	char	flagbuf[20];
	char   *p;
	int i = e_ptrs[0]->behavior;

	win_xyputs(schedule_win,8,2,"0");
	win_clreol(schedule_win);
	p = flagbuf;
	if (i & MAT_CM	     ) *p++ = 'C';
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
	if (flagbuf[0])
	   win_print(schedule_win,"/%s",flagbuf);
	win_xyputs(schedule_win,8,3,"---  ");
	win_xyprint(schedule_win,8,4,"%u",(word) e_ptrs[0]->wait_time);
	win_clreol(schedule_win);
}


void statuswinopen()
{
	schedule_win = win_boxopen(1,2,58,13, CUR_NONE, CON_RAW,
				   xenwin_colours.schedule.template, KEY_NONE,
				   LINE_SINGLE,LINE_SINGLE,LINE_SINGLE,LINE_SINGLE,
				   xenwin_colours.schedule.border,
				   "Schedule",xenwin_colours.schedule.title);

	win_setattrib(schedule_win,xenwin_colours.schedule.border);
	win_line(schedule_win,1,7,58,7,LINE_SINGLE);
	win_line(schedule_win,12,7,12,12,LINE_SINGLE);
	win_line(schedule_win,23,1,23,12,LINE_SINGLE);

	win_setattrib(schedule_win,xenwin_colours.stats.title);
	win_xyputs(schedule_win, 1, 6,"Calls");
	win_xyputs(schedule_win,12, 6,"Moved");

	win_setrange(schedule_win,1,1,58,12);

	win_setattrib(schedule_win,xenwin_colours.status.title);
	win_xyputs(schedule_win,24, 1,"Status");

	win_setattrib(schedule_win,xenwin_colours.outbound.title);
	win_xyputs(schedule_win,24,7,"Outbound");
	win_xyputs(schedule_win,33,7,"\031\030");
	win_xyputs(schedule_win,38,7,"size");
	win_xyputs(schedule_win,43,7,"age");
	win_xyputs(schedule_win,47,7,"types");
	win_xyputs(schedule_win,57,7,"s");

	win_setrange(schedule_win,2,2,22,11);

	win_setattrib(schedule_win,xenwin_colours.schedule.template);
	win_xyputs(schedule_win,1,1,"Task :");
	win_xyputs(schedule_win,1,2,"Event:");
	win_xyputs(schedule_win,1,3,"Until:");
	win_xyputs(schedule_win,1,4,"Delay:");
	win_xyputs(schedule_win,1,5,"Polls:");

	win_setattrib(schedule_win,xenwin_colours.stats.template);
	win_xyputs(schedule_win, 1, 7,"In  :");
	win_xyputs(schedule_win, 1, 8,"BBS :");
	win_xyputs(schedule_win, 1, 9,"Mail:");
	win_xyputs(schedule_win, 1,10,"Out :");
	win_xyputs(schedule_win,12, 7,"Mail:");
	win_xyputs(schedule_win,12, 8,"KB  :");
	win_xyputs(schedule_win,12, 9,"File:");
	win_xyputs(schedule_win,12,10,"KB  :");

	win_setattrib(schedule_win,xenwin_colours.schedule.normal);

	win_xyprint(schedule_win,8,1,"%u",task);
	win_xyputs(schedule_win,8,5,"0");	/* 0 Polls */

	status_win = win_open(24,3,57,7, CUR_NONE, CON_RAW,
			      xenwin_colours.status.template, KEY_NONE);

	win_xyputs(status_win,1,3,"Last in :");
	win_xyputs(status_win,1,4,"Last BBS:");
	win_xyputs(status_win,1,5,"Last out:");

	win_setattrib(status_win,xenwin_colours.status.normal);
	win_xyputs(status_win,11,3,"---");
	win_xyputs(status_win,11,4,"---");
	win_xyputs(status_win,11,5,"---");

	outbound_win = win_open(24,9,57,12, CUR_NONE, CON_RAW,
				xenwin_colours.outbound.normal, KEY_NONE);
}
 

void modemwinopen()
{
	modem_win = win_boxopen(59,2,win_maxhor,13, CUR_NONE, CON_RAW|CON_SCROLL,
				xenwin_colours.modem.normal, KEY_RAW,
				LINE_SINGLE,LINE_SINGLE,LINE_SINGLE,LINE_SINGLE,
				xenwin_colours.modem.border,
				"TE/modem",xenwin_colours.modem.title);
	win_setattrib(modem_win,xenwin_colours.modem.border);
	win_line(modem_win,1,3,windows[modem_win].win_horlen,3,LINE_SINGLE);
	win_setrange(modem_win,2,4,windows[modem_win].win_horlen - 1,11);
	win_setpos(modem_win,1,8);
	win_setattrib(modem_win,xenwin_colours.modem.normal);
}


void topwinopen()
{
	top_win = win_open(1,1,win_maxhor,1, CUR_NONE, CON_RAW,
			   xenwin_colours.top.normal, KEY_RAW);
	/*win_xyprint(top_win,win_maxhor - 24,1,"\263");*/
}


void helpwinopen()
{
	help_win = win_open(1,win_maxver,win_maxhor,win_maxver, CUR_NONE, CON_RAW,
			    xenwin_colours.help.normal, KEY_RAW);

	win_print(help_win," Alt-Z Menu/help     C/Alt-C Call immediately     Alt-T Terminal     Alt-X Exit");
}


/* end of twindow.c */
