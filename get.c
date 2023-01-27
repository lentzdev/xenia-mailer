/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Based off The-Box Mailer (C) Jac Kersing
 * get.c Asks for files for request/send
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


/* next routine gets names */
getnames (FILE *fp, int pwd)
{
    char name[80];
    char password[40];
    
    print("\nEnter filename(s). Empty line to end");
    for (;;) {
	print("\nFilename: ");
	get_str(name,80);
	if (!*name) break;
	*password = '\0';
	if (pwd) {
	   print("Password: ");
	   get_str(password,30);
	}
	else if (!fexist(name)) {
	   print("File '%s' not found",name);
	   continue;
	}
	fprint(fp, "%s%s%s\n", name, *password ? " !" : "", password);
	if (ferror(fp)) return (1);
    }

    print("All OK? ('N' will delete everything, also previously entered) [Y/n]\n");
    do *name = toupper(win_keygetc());
    while (*name != Enter && *name != 'Y' && *name != 'N');

    return (*name == 'N');
}


void makelist (int fl)
{
    char buffer[256], c;
    int test;
    struct _nodeinfo *n;
    FILE *fp;

    /* First get the address */
    print("\nAddress for %s: ", fl ? "files" : "requests");
    get_str(buffer,80);
    if (!buffer[0] || (n = strtoinfo(buffer)) == NULL) {
       print("No (valid) address; aborted.\n\n");
       return;
    }

    flag_session(true);
    if (!flag_set(n->zone,n->net,n->node,n->point)) {
       flag_session(false);
       return;
    }

    if (!n->phone[0] || (n->nodeflags & (VIAHOST | VIABOSS))) {
       print("Can't call %d:%d/%d%s (only pickup)\n",
	     n->zone,n->net,n->node,pointnr(n->point));
       (void) win_bell();
    }
    else if (n->name[0])
       print("System: %s\n",n->name);

    print("Select: [D]irect, [I]mmediate, [H]old, [A]long with mail or [Q]uit?\n");
    do c = toupper(win_keygetc());
    while (c != 'D' && c != 'I' && c != 'H' && c != 'A' && c != 'Q') ;
    if (c == 'Q') {
       print("aborted.\n\n");
       flag_session(false);
       return;
    }

    if (fl)
       sprint(buffer, "%s%04.4x%04.4x%s.%cLO", holdarea(n->zone),
		      (word) n->net, (word) n->node,
		      holdpoint(n->point), (short) c);
    else
       sprint(buffer, "%s%04.4x%04.4x%s.REQ", holdarea(n->zone),
		      (word) n->net, (word) n->node,
		      holdpoint(n->point));

    createhold(n->zone,n->net,n->node,n->point);
    fp = sfopen(buffer,"at",DENY_WRITE);
    if (fp == NULL) {
       message(6,"!Error opening %s",buffer);
       flag_session(false);
       return;
    }
    test = getnames(fp,!fl);
    fclose(fp);

    if (test)
       unlink(buffer);
    else {
       if (!fl && (c == 'D' || c == 'I')) {
	  sprint(buffer, "%s%04.4x%04.4x%s.%cLO", holdarea(n->zone),
			 (word) n->net, (word) n->node,
			 holdpoint(n->point), (short) c);

	  fp = sfopen(buffer,"at",DENY_WRITE);
	  if (fp == NULL) {
	     message(6,"!Error opening %s",buffer);
	     flag_session(false);
	     return;
	  }

	  fclose(fp);
       }

       sprint(buffer,"%sXMRESCAN.FLG",flagdir);
       if (fexist(buffer)) unlink(buffer);
#if 0
       if ((fd = dos_sopen(buffer,O_WRONLY|O_CREAT|O_TRUNC,DENY_NONE)) >= 0) dos_close(fd);
       /*rescan = 0L;*/
#endif
    }

    print("\nDone.\n\n");
    flag_session(false);
}


/* end of get.c */
