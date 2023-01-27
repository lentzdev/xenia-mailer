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


/* Menu routines */
#include "xenia.h"
#define MAX_APPKEYS 19


int cdecl xenia_info(WIN_MENU *mp, word keycode)
{
	WIN_IDX win;

	mp = mp;

	win = win_create(9, 5, 71, 16, CUR_NONE,
			 CON_RAW, xenwin_colours.menu.hotnormal, KEY_RAW);
	if (!win)
	   return (0);

	win_line(win,1,1,1,wp->win_verlen,LINE_DOUBLE);
	win_line(win,1,1,wp->win_horlen,1,LINE_DOUBLE);
	win_line(win,wp->win_horlen,1,
			    wp->win_horlen,wp->win_verlen,LINE_DOUBLE);
	win_line(win,1,wp->win_verlen,
			    wp->win_horlen,wp->win_verlen,LINE_DOUBLE);
	win_xyputs(win,wp->win_horlen - 20,wp->win_verlen,
			      "Press Space or Enter");

	win_setrange(win,2,2,wp->win_horlen - 1,wp->win_verlen - 1);

	win_cyprint(win, 1,"%s - Version %s %s", PRGNAME, VERSION, XENIA_OS);
	win_cyprint(win, 2,"Copyright (C) 1987-2001 Arjen G. Lentz");
	win_cyprint(win, 3,"Original The-Box Mailer by J.A.Kersing & A.G.Lentz");
	win_cyprint(win, 4,"SEAlink design by System Enhancement Associates");
	win_cyprint(win, 5,"Hydra design by A.G.Lentz & J.H.Homrighausen");

	win_cyprint(win, 7,"Xenia comes with ABSOLUTELY NO WARRANTY.");
	win_cyprint(win, 8,"This is free software, and you are welcome");
	win_cyprint(win, 9,"to redistribute it under certain conditions;");
	win_cyprint(win,10,"See COPYING file for full details.");

	win_settop(win);
	win_keypurge();
	do keycode = win_keygetc();
	while (keycode != MKEY_LEFTBUTTON && keycode != Esc &&
	       keycode != MKEY_RIGHTBUTTON && keycode != Enter &&
	       keycode != ' ');
	win_close(win);
	return (0);
}/*xenia_info()*/


int cdecl mode_term(WIN_MENU *mp, word keycode)
{
	if (mdm.port == -1) return (1);

	statusmsg("Terminal mode");
	message(2,"*Entering terminal");
#if JAC
	phone_out(0);
#endif
	mdm_terminit();
	if (Term(0)) mode_exit(mp,keycode);

	if (carrier()) hangup();
	init_modem();
	message(2,"*Back from terminal");
	return (1);
}/*mode_term()*/


int cdecl mode_jump(WIN_MENU *mp, word keycode)
{
	mp = mp;
	keycode = keycode;
	dos_shell(0);
	return (1);
}/*mode_jump()*/


int cdecl mode_exit(WIN_MENU *mp, word keycode)
{
	mp = mp;
	keycode = keycode;
	errl_exit(0,false,NULL);
	return (1);
}/*mode_exit()*/


int cdecl do_command (WIN_MENU *mp, word keycode)
{
	extern WIN_MENU mode_menu;
	extern WIN_MENU outbound_menu;
	extern WIN_MENU session_menu;

	if (mp == &mode_menu) {
	   switch (keycode) {
		  case 'M': win_keypush(Alt_M); break;
		  case 'U': win_keypush(Alt_U); break;
		  case 'Q': win_keypush(Alt_Q); break;
		  case 'R': win_keypush(Alt_R); break;
		  case 'B': win_keypush(Alt_B); break;
	   }
	}
	else if (mp == &outbound_menu) {
	   switch (keycode) {
		  case 'P': win_keypush(Alt_P); break;
		  case 'C': win_keypush(Alt_I); break;
		  case 'O': win_keypush(Alt_O); break;
		  case 'S': win_keypush(Alt_S); break;
		  case 'G': win_keypush(Alt_G); break;
	   }
	}
	else if (mp == &session_menu) {
	   switch (keycode) {
		  case 'C': win_keypush(Alt_C); break;
		  case 'A': win_keypush(Alt_A); break;
		  case 'W': win_keypush(Alt_W); break;
	   }
	}

	return (0);
}/*do_command()*/


#define NUM_HOTKEYS 16
struct _hotkey_item mailerhotkeys_items[NUM_HOTKEYS + MAX_APPKEYS] = {
	/*{ Alt_Z,  OPT_SHUTALL, NULL, NULL },*/

	{ Alt_U,  OPT_SHUTALL, NULL,	  NULL },	/* Unattended toggle */
	{ Alt_Q,  OPT_SHUTALL, NULL,	  NULL },	/* Quit events	     */
	{ Alt_R,  OPT_SHUTALL, NULL,	  NULL },	/* Restart events    */
	{ Alt_T,  OPT_SHUTALL, mode_term, NULL },	/* Terminal	     */
	{ Alt_B,  OPT_SHUTALL, NULL,	  NULL },	/* Blank screen      */
	{ Alt_J,  OPT_SHUTALL, mode_jump, NULL },	/* Jump to OS shell  */
	{ Alt_X,  OPT_SHUTALL, mode_exit, NULL },	/* Exit Xenia Mailer */

	{ Alt_P,  OPT_SHUTALL, NULL,	  NULL },	/* Poll 	     */
	{ Alt_I,  OPT_SHUTALL, NULL,	  NULL },	/* Immediate poll    */
	{ Alt_O,  OPT_SHUTALL, NULL,	  NULL },	/* Remove polls      */
	{ Alt_S,  OPT_SHUTALL, NULL,	  NULL },	/* Send files	     */
	{ Alt_G,  OPT_SHUTALL, NULL,	  NULL },	/* Get files	     */

	{ Alt_C,  OPT_SHUTALL, NULL,	  NULL },	/* Call immediate    */
	{ Ctrl_C, OPT_SHUTALL, NULL,	  NULL },	/* Call immediate    */
	{ Alt_A,  OPT_SHUTALL, NULL,	  NULL },	/* Answer call	     */
	{ Alt_W,  OPT_SHUTALL, NULL,	  NULL },	/* Wait for call     */

	/* max 19 user programmable application hotkeys */
};
WIN_HOTKEYS mailer_hotkeys = { NUM_HOTKEYS, mailerhotkeys_items };


static char *appcmds[MAX_APPKEYS];
struct _menu_item applications_items[2 + MAX_APPKEYS] = {
	{ " Information on Xenia ", 'I',     OPT_NONE,	  xenia_info,  NULL },
	{ NULL, 		     No_Key, OPT_NONE,	  NULL,        NULL }
	/* null line only activated if one or more appkeys are activated */
	/* max 19 user programmable application hotkeys 		 */
};
WIN_MENU applications_menu = { MENU_PULLDOWN | MENU_SHUTALL,
			       LINE_SINGLE, 2, 2, NULL, 0, NULL,
			       0, 0, 0, 0, 0, 0, 0,
			       NULL, 1, 0, applications_items };


struct _menu_item mode_items[] = {
	{ " Unattended on/off  Alt-U ", 'U',	OPT_NONE,    do_command, NULL },
	{ NULL, 			No_Key, OPT_NONE,    NULL,	 NULL },
	{ " Quit events        Alt-Q ", 'Q',	OPT_NONE,    do_command, NULL },
	{ " Restart events     Alt-R ", 'R',	OPT_NONE,    do_command, NULL },
	{ NULL, 			No_Key, OPT_NONE,    NULL,	 NULL },
	{ " Terminal           Alt-T ", 'T',	OPT_SHUTALL, mode_term,  NULL },
	{ " Blank screen       Alt-B ", 'B',	OPT_NONE,    do_command, NULL },
	{ " Jump to OS Shell   Alt-J ", 'J',	OPT_SHUTALL, mode_jump,  NULL },
	{ " Exit Xenia         Alt-X ", 'E',	OPT_SHUTALL, mode_exit,  NULL }
};
WIN_MENU mode_menu = { MENU_PULLDOWN | MENU_SHUTALL,
		       LINE_SINGLE, 16, 2, NULL, 0, NULL,
		       0, 0, 0, 0, 0, 0, 0,
		       NULL, 9, 0, mode_items };


struct _menu_item outbound_items[] = {
	{ " Poll node(s)  Alt-P ", 'P',    OPT_NONE, do_command, NULL },
	{ " Crash nodes   Alt-I ", 'C',    OPT_NONE, do_command, NULL },
	{ " Stop polling  Alt-O ", 'O',    OPT_NONE, do_command, NULL },
	{ NULL, 		   No_Key, OPT_NONE, NULL,	 NULL },
	{ " Send files    Alt-S ", 'S',    OPT_NONE, do_command, NULL },
	{ " Get files     Alt-G ", 'G',    OPT_NONE, do_command, NULL }
};
WIN_MENU outbound_menu = { MENU_PULLDOWN | MENU_SHUTALL,
			   LINE_SINGLE, 22, 2, NULL, 0, NULL,
			   0, 0, 0, 0, 0, 0, 0,
			   NULL, 6, 0, outbound_items };


struct _menu_item session_items[] = {
	{ " Call out now        C/Alt-C ", 'C', OPT_NONE, do_command, NULL },
	{ " Answer phone now      Alt-A ", 'A', OPT_NONE, do_command, NULL },
	{ " Wait for call on/off  Alt-W ", 'W', OPT_NONE, do_command, NULL }
};
WIN_MENU session_menu = { MENU_PULLDOWN | MENU_SHUTALL,
			  LINE_SINGLE, 32, 2, NULL, 0, NULL,
			  0, 0, 0, 0, 0, 0, 0,
			  NULL, 3, 0, session_items };


struct _menu_item mailerbar_items[] = {
	{ " ",			No_Key, OPT_NONE,      NULL, NULL,		},
	{ " Applications ",	'A',	OPT_IMMEDIATE, NULL, &applications_menu },
	{ " Mode ",		'M',	OPT_IMMEDIATE, NULL, &mode_menu 	},
	{ " Outbound ", 	'O',	OPT_IMMEDIATE, NULL, &outbound_menu	},
	{ " Session ",		'S',	OPT_IMMEDIATE, NULL, &session_menu	}
};
WIN_MENU mailerbar_menu = { MENU_BAR | MENU_DELAYIMMEDIATE | MENU_NOOPEN,
			    LINE_NONE, 1, 1, NULL, 0, NULL,
			    0, 0, 0, 0, 0, 0, 0,
			    NULL, 5, 1, mailerbar_items };


int cdecl handle_appkey(WIN_MENU *mp, word keycode)
{
	byte i, errorlevel = 0, repack = 0;
	char *cmdline = NULL;
	char buffer[256];

	mp++;

	for (i = 0; 2 + i < applications_menu.num_items; i++) {
	    if (keycode == applications_items[2 + i].key ||
		(NUM_HOTKEYS + i < mailer_hotkeys.num_items &&
		keycode == mailerhotkeys_items[NUM_HOTKEYS + i].key))
	       break;
	}
	if (2 + i == applications_menu.num_items)
	   return (1);	/* can't really fail, but just in case */

	if (*appcmds[i] == '*')
	   errorlevel = atoi(appcmds[i] + 1);
	else {
	   cmdline = appcmds[i];
	   if (*appcmds[i] == '+') {
	      repack = 1;
	      cmdline++;
	   }
	}

	if (cmdline) {
	   message(3,"*Starting application %d:%s",
		   (int) (i + 1), applications_items[2 + i].text);
	   mdm_busy();
	   if (log != NULL) fclose (log);
	   sys_reset();
	   (void) win_savestate();
	   execute(cmdline);
	   (void) win_restorestate();
	   sys_init();
	   if (log != NULL) {
	      if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
		 message(6,"!Can't re-open logfile!");
	   }
	   message(3,"*Returned from application");
	   if (repack) {
	      if (cur_event > 0 &&
		  e_ptrs[cur_event]->tag != 'X' &&
		  e_ptrs[cur_event]->tag != 'Z') {
		 if (mailpack) {
		    message(3,"*Re-packing mail for event %d (tag %c)",
			      cur_event + 1, (char) e_ptrs[i]->tag);
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
		    message(3,"*Returned from mailpack");
		 }
		 else
		    errl_exit (4,false,"Re-packing mail for event %d",cur_event);
	      }
	   }
	   message(0,"+Enabling TE/modem");
	   init_modem();
	   waitforline = time(NULL) + 60;
	}
	else {
	   errl_exit(errorlevel,false,"Exit requested from keyboard");
	}

	return (1);
}/*handle_appkey()*/


int add_appkey(word hotkey,word menukey,char *cmd,char *text)
{
	char work[80];
	char *p;

	if (applications_menu.num_items == MAX_APPKEYS) return (0);

	if (applications_menu.num_items == 1)
	   applications_menu.num_items++;	       /* activate NULL line */

	sprint(work," %s ",text);
	p = (char *) malloc(strlen(work) + 1);
	if (p == NULL) return (0);
	strcpy(p,work);
	applications_items[applications_menu.num_items].text	 = p;
	applications_items[applications_menu.num_items].key	 = menukey;
	applications_items[applications_menu.num_items].flags	 = OPT_SHUTALL;
	applications_items[applications_menu.num_items].function = handle_appkey;
	applications_items[applications_menu.num_items].menu	 = NULL;
	appcmds[applications_menu.num_items - 2] = ctl_string(cmd);
	applications_menu.num_items++;

	if (hotkey) {
	   mailerhotkeys_items[mailer_hotkeys.num_items].key	  = hotkey;
	   mailerhotkeys_items[mailer_hotkeys.num_items].flags	  = OPT_SHUTALL;
	   mailerhotkeys_items[mailer_hotkeys.num_items].function = handle_appkey;
	   mailerhotkeys_items[mailer_hotkeys.num_items].menu	  = NULL;
	   mailer_hotkeys.num_items++;
	}

	return (1);
}/*add_appkey()*/


void init_mailermenu()
{
	/* move TB default colours to menus */
	mode_menu.atr_normal		=
	applications_menu.atr_normal	=
	outbound_menu.atr_normal	=
	session_menu.atr_normal 	=
	mailerbar_menu.atr_normal	= xenwin_colours.menu.normal;

	mode_menu.atr_hotnormal 	=
	applications_menu.atr_hotnormal =
	outbound_menu.atr_hotnormal	=
	session_menu.atr_hotnormal	= xenwin_colours.menu.hotnormal;
	mailerbar_menu.atr_hotnormal	= xenwin_colours.menu.normal;

	mode_menu.atr_select		=
	applications_menu.atr_select	=
	outbound_menu.atr_select	=
	session_menu.atr_select 	=
	mailerbar_menu.atr_select	= xenwin_colours.menu.select;

	mode_menu.atr_hotselect 	=
	applications_menu.atr_hotselect =
	outbound_menu.atr_hotselect	=
	session_menu.atr_hotselect	=
	mailerbar_menu.atr_hotselect	= xenwin_colours.menu.select;

	mode_menu.atr_inactive		=
	applications_menu.atr_inactive	=
	outbound_menu.atr_inactive	=
	session_menu.atr_inactive	= xenwin_colours.menu.inactive;
	mailerbar_menu.atr_inactive	= xenwin_colours.menu.hotnormal;

	mode_menu.atr_border		=
	applications_menu.atr_border	=
	outbound_menu.atr_border	=
	session_menu.atr_border 	=
	mailerbar_menu.atr_border	= xenwin_colours.menu.border;

	mode_menu.atr_title		=
	applications_menu.atr_title	=
	outbound_menu.atr_title 	=
	session_menu.atr_title		=
	mailerbar_menu.atr_title	= xenwin_colours.menu.title;
}/*init_mailermenu()*/


word call_mailermenu()
{
	byte i;
	word keycode;

	keycode = win_keyscan();
	if (keycode < 0x100)
	   keycode = (word) toupper(keycode);
	for (i = 0; i < mailer_hotkeys.num_items; i++) {
	    if (keycode == mailer_hotkeys.item[i].key)
	       goto call_menu;
	}
	win_keygetc();
	/*
	if (keycode == MKEY_RIGHTBUTTON || keycode == Esc)
	   goto call_menu;
	*/
	if (keycode == Alt_Z) {
	   win_keypush(Enter);
	   goto call_menu;
	}
	win_settop(log_win);
	return (0);

call_menu:
	mailerbar_menu.menu_win = win_open(1,1,win_maxhor,1, CUR_NONE, CON_RAW,
					   xenwin_colours.menu.normal, KEY_RAW);
	/*mailerbar_menu.opt_last = 1;*/
	keycode = win_menu(&mailerbar_menu,&mailer_hotkeys);
	win_settop(log_win);
	return (keycode);
}/*call_mailermenu()*/


/* end of tmenu.c */
