/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Based off The-Box Mailer (C) Jac Kersing
 * Run BBS, Editor and Command
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


#if 0
void BBSexit (void)
{
	char command[80];
	FILE *fp;

	/* Wait a half second */
	tdelay(5);

	/* Now if there is no carrier bag it */
	if (!carrier()) {
	   message(1,"-Impatient user, he hung up on me!!");
	   return;
	}

	/* if BBS flag is "exit" - no FTB_dos */
	if (!strnicmp(BBSopt,"exit",4)) {
	   errl_exit((int) (cur_speed / 100), true,
		     "Exit to BBS");
	}
	else if (!strnicmp(BBSopt, "batch", 5)) {
	   message(0,"+Creating BBSBATCH.BAT");
	   fp = sfopen("BBSBATCH.BAT","wt",DENY_ALL);
	   if (fp == NULL) {
	      message(6,"!Could not create BBSBATCH.BAT");
	      return;
	   }

	   if (FTB_dos) fprint(fp,FTB_dos);
	   else 	fprint(fp,"SPAWNBBS %lu %d %d %s\n",
				  cur_speed,com_handle,time_to_next(),errorfree);

	   fclose(fp);
	   errl_exit((int) (cur_speed / 100), true,
		     "Exit to %s", FTB_dos ? "OS Service" : "BBS");
	}
	else if (!strnicmp(BBSopt, "spawn", 5)) {
	   message(1,"+Spawning %s", FTB_dos ? "OS Service" : "BBS");
	   if (!FTB_dos)
	      sprint(command, "SPAWNBBS %lu %d %d %s",
			      cur_speed, com_handle, time_to_next(), errorfree);

	   if (log != NULL) fclose (log);
	   sys_reset();
	   (void) win_savestate();
	   shell(FTB_dos ? FTB_dos : command);
	   (void) win_restorestate();
	   sys_init();

	   /* Re-enable ourselves */
	   if (log != NULL) {
	      if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
		 message(6,"!Can't re-open logfile!");
	   }

	   message(1,"+Back from %s", FTB_dos ? "OS Service" : "BBS");
	   if (FTB_dos) {
	      free(FTB_dos);
	      FTB_dos = NULL;
	   }
	}
}
#endif


void extapp_handler (EXTAPP *ep)
{
	char buffer[128];
	FILE *fp;

	com_dump();
	if (!FTB_dos && (ep->flags & APP_BBS)) extra.human++;
	if (callerid[0])
	   last_set(&last_bbs, callerid);
	show_stats();
	if (ep->flags & (APP_BBS | APP_MAIL)) {
	   sprint(buffer,"\rStarting application %s                    \r\n",
			 FTB_dos ? "OS Service" : ep->description);
	   script_outstr(buffer,NULL,NULL,0);
	   script_outstr("Please wait . . .\r\n",NULL,NULL,0);
	   com_flush();
	}

	if (!carrier()) {
	   message(1,"-%s",msg_carrier);
	   return;
	}

	sprint(buffer,"%sXENIA.CLI",homepath);
	if (fexist(buffer)) {
#if defined(__OS2__)
	   sprint(buffer,"%sXENID%u.CMD",homepath,task);
#else
	   sprint(buffer,"%sXENID%u.BAT",homepath,task);
#endif

	   if ((fp = sfopen(buffer,"wt",DENY_ALL)) != NULL) {
	      fprint(fp,"SET XENCLI_USERNAME=%s\n",
			callerid[0] && clip.username && clip.username[0] ? clip.username : "");
	      fclose(fp);
	   }
	}

	if (ep->flags & APP_EXIT) {
	   word erl = (word) (((cur_speed / 100) != 384) ? (cur_speed / 100) : 38);

	   errl_exit(erl, true,
		     "Exit to application %s",
		     FTB_dos ? "OS Service" : ep->description);
	}
	else if (ep->flags & APP_BATCH) {
#if defined(__OS2__)
	   sprint(buffer,"%sXENAP%u.CMD",homepath,task);
#else
	   sprint(buffer,"%sXENAP%u.BAT",homepath,task);
#endif
	   if ((fp = sfopen(buffer,"wt",DENY_ALL)) == NULL) {
	      message(6,"!Could not create '%s'",buffer);
	      return;
	   }
	   if (FTB_dos)
	      fprint(fp,FTB_dos);
	   else
	      fprint(fp,"%s %u %d %s %d %lu %lu %d %s %s\n",
			ep->batchname, task,
			mdm.port, mdm.comdev, com_handle,
			mdm.lock ? mdm.speed : cur_speed, cur_speed,
			time_to_next(), errorfree[0] ? errorfree : "/NONE",
			callerid[0] ? callerid : "N/A");
	   fclose(fp);

	   errl_exit(apperrorlevel, true,
		     "Batch exit to application %s",
		     FTB_dos ? "OS Service" : ep->description);
	}
	else {	/* (ep->flags & APP_SPAWN) */
	   if (!FTB_dos)
	      sprint(buffer,"%s %u %d %s %d %lu %lu %d %s %s",
			    ep->batchname, task,
			    mdm.port, mdm.comdev, com_handle,
			    mdm.lock ? mdm.speed : cur_speed, cur_speed,
			    time_to_next(), errorfree[0] ? errorfree : "/NONE",
			    callerid[0] ? callerid : "N/A");

	   message(3,"+Calling application %s",
		     FTB_dos ? "OS Service" : ep->description);

	   alloc_msg("pre extapp");

	   if (log != NULL) fclose (log);

	   if (mdm.shareport)
	      com_suspend = true;
	   sys_reset();
	   (void) win_savestate();
	   shell(FTB_dos ? FTB_dos : buffer);
	   (void) win_restorestate();
	   sys_init();

	   if (log != NULL) {
	      if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
		 message(6,"!Can't re-open logfile!");
	   }

	   read_bbsinfo();
	   show_stats();

	   message(1,"+Returned from application");
	   if (FTB_dos) {
	      free(FTB_dos);
	      FTB_dos = NULL;
	   }

	   alloc_msg("post extapp");
	}
}/*extapp_handler()*/


void read_bbsinfo (void)
{
	char buffer[128];
	int  fd;
	int  i;

	switch (bbstype) {
	   case BBSTYPE_MAXIMUS:
		{ struct _max_usr {
			 char name[36],
			      city[36];
			 /* ... many more fields ... */
		  } max_usr;
		  static long oldstamp = 0L;
		  struct stat f;

		  if (task) sprint(buffer,"%sLASTUS%02x.BBS",bbspath,(word) task);
		  else	    sprint(buffer,"%sLASTUSER.BBS",bbspath);

		  if ((fd = dos_sopen(buffer,O_RDONLY,DENY_NONE)) >= 0) {
		     if (dos_read(fd,&max_usr,sizeof (struct _max_usr)) == sizeof (struct _max_usr)) {
			last_set(&last_bbs, max_usr.name);

			fstat(fd,&f);
			if (callerid[0] && max_usr.name[0] && f.st_mtime != oldstamp)
			   write_clip(max_usr.name,max_usr.city,"","");
			oldstamp = f.st_mtime;
		     }
		     dos_close(fd);
		  }
		}
		break;

	   case BBSTYPE_RA:
		{ struct _ra_lcal {
			 byte line;
			 char name[36],
			      handle[36],
			      city[26];
			 word speed;
			 long times;
			 char logon[6],
			      logoff[6];
			 byte attrib;
		  } ra_lcal;
		  static int oldrec = -1;
		  int newrec, countrec;
		  char city[26];

		  sprint(buffer,"%sLASTCALL.BBS",bbspath);
		  if ((fd = dos_sopen(buffer,O_RDONLY,DENY_NONE)) >= 0) {
		     buffer[0] = city[0] = '\0';

		     newrec = countrec = 0;
		     while (dos_read(fd,&ra_lcal,sizeof (struct _ra_lcal)) == sizeof (struct _ra_lcal)) {
			   if (ra_lcal.line == task) {
			      i = ra_lcal.name[0];
			      strncpy(buffer,&ra_lcal.name[1],i);
			      buffer[i] = '\0';

			      i = ra_lcal.city[0];
			      strncpy(city,&ra_lcal.city[1],i);
			      city[i] = '\0';

			      newrec = countrec;
			   }
			   countrec++;
		     }
		     dos_close(fd);

		     if (buffer[0]) {
			last_set(&last_bbs, buffer);

			if (callerid[0] && newrec != oldrec)
			   write_clip(buffer,city,"","");
		     }
		     oldrec = newrec;
		  }
		}
		break;

	   default:
		break;
	}
}/*read_bbsinfo()*/


void write_clip (char *username, char *city, char *nodenumber, char *systemname)
{
	char  buffer[256];
	char  line[256], ignore[128];
	char *p;
	FILE *fp;
	int   i;

	while ((p = strchr(callerid  ,',')) != NULL) *p = ';';
	while ((p = strchr(username  ,',')) != NULL) *p = ';';
	while ((p = strchr(city      ,',')) != NULL) *p = ';';
	while ((p = strchr(nodenumber,',')) != NULL) *p = ';';
	while ((p = strchr(systemname,',')) != NULL) *p = ';';
	sprint(line,"%s,%s,%s,%s,%s\r\n",callerid,username,city,nodenumber,systemname);
	sprint(ignore,"%s,ignore",callerid);

	sprint(buffer,"%sXENIA.CLI",homepath);
	if (!fexist(buffer))
	   return;

	for (i = 0; i < 10; i++) {
	    if ((fp = sfopen(buffer,"r+b",DENY_WRITE)) != NULL)
	       break;
	}
	if (i == 10) return;

	while (fgets(buffer,250,fp)) {
	      if (strlen(buffer) < 10 || buffer[0] == ';')
		 continue;
	      if (!strnicmp(buffer,line,strlen(line)) ||
		  !strnicmp(buffer,ignore,strlen(ignore)))
		 goto fini;
	}

	{ int fd = fileno(fp); char zc;
	  dos_seek(fd,-1L,SEEK_END);
	  dos_read(fd,&zc,1);
	  fseek(fp, (zc == '\032') ? -1L : 0L, SEEK_END);
	}
	fputs(line,fp);

fini:	fclose(fp);
}/*write_clip()*/


void check_clip (void)
{
	static char buffer[256];
	char *field[5];
	FILE *fp;
	char *p, *q;
	int   i;

	message(3,"=ID caller #: %s", callerid);
	memset(&clip,0,sizeof (CLIP));

	sprint(buffer,"%sXENIA.CLI",homepath);
	if ((fp = sfopen(buffer,"rt",DENY_NONE)) == NULL)
	   return;
	while (fgets(buffer,250,fp)) {
	      if (strlen(buffer) < 10 || buffer[0] == ';')
		 continue;
	      for (i = 0; i < 5; field[i++] = "");
	      q = buffer;
	      for (i = 0; i < 5 && q; i++) {
		  p = q;
		  if ((q = strpbrk(p,",\n")) != NULL) {
		     *q++ = '\0';
		  }
		  field[i] = p;
	      }
	      if (!stricmp(field[0],callerid)) {
		 if (!stricmp(field[1],"ignore")) break;
		 while ((p = strchr(field[1],';')) != NULL) *p = ',';
		 while ((p = strchr(field[2],';')) != NULL) *p = ',';
		 while ((p = strchr(field[3],';')) != NULL) *p = ',';
		 while ((p = strchr(field[4],';')) != NULL) *p = ',';
		 if (*field[1]) message(3,"=ID username: %s", field[1]);
		 if (*field[2]) message(3,"=ID location: %s", field[2]);
		 if (*field[3]) message(3,"=ID address : %s", field[3]);
		 if (*field[4]) message(3,"=ID system  : %s", field[4]);
		 clip.username = field[1];
		 clip.location = field[2];
		 clip.address  = field[3];
		 clip.system   = field[4];
		 break;
	      }
	}
	fclose(fp);
}/*check_clip()*/


void dos_shell(int no_modem)
{
    alloc_msg("pre dos shell");

    message(3,"*Jumping to OS Shell");
    if (!no_modem) mdm_busy();

    if (log != NULL) fclose (log);
    if (no_modem)
       com_suspend = true;
    sys_reset();
    (void) win_savestate();
    fprint(stderr,"Type EXIT to return to %s\n",PRGNAME);
    shell(NULL);
    (void) win_restorestate();
    sys_init();
    if (log != NULL) {
       if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL)
	  message(6,"!Can't re-open logfile!");
    }

    message(3,"*Returned from OS Shell");
    statusmsg(NULL);
    if (!no_modem) {
       init_modem();
       waitforline = time(NULL) + 60;
    }

    alloc_msg("post dos shell");
}


/* end of bbs.c */
