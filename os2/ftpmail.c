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


/* FTP MAIL client: OS/2 + WATCOM only --------------------- AGL 16 jul 1995 */
#define INCL_BASE
#define INCL_DOSMODULEMGR
#include <os2.h>
#include <ftpapi.h>
#include "zmodem.h"


static char *bsyname = "ihub.bsy";


/* ------------------------------------------------------------------------- */
static HMODULE hmod_ftpapidll = 0UL;

static void _System (*dllftplogoff) (void);
static int  _System (*dllftpget   ) (char *, char *, char *, char *, char *, char *, char *, int);
static int  _System (*dllftpput   ) (char *, char *, char *, char *, char *, char *, int);
static int  _System (*dllftpcd	  ) (char *, char *, char *, char *, char *);
static int  _System (*dllftpdelete) (char *, char *, char *, char *, char *);
static int  _System (*dllftprename) (char *, char *, char *, char *, char *, char *);
static int  _System (*dllftpls	  ) (char *, char *, char *, char *, char *, char *);
static int  _System (*dllftpdir   ) (char *, char *, char *, char *, char *, char *);
static int  _System (*dllftpping  ) (char *, int, unsigned long *);
static int  _System (*dllftp_errno) (void);


/* ------------------------------------------------------------------------- */
static char *ftperrstr (int err)
{
	switch (err) {
	       case FTPSERVICE:    return ("Unknown service");
	       case FTPHOST:	   return ("Unknown host");
	       case FTPSOCKET:	   return ("Unable to obtain socket");
	       case FTPCONNECT:    return ("Unable to connect to server");
	       case FTPLOGIN:	   return ("Login failed");
	       case FTPABORT:	   return ("Transfer aborted");
	       case FTPLOCALFILE:  return ("Problem opening the local file");
	       case FTPDATACONN:   return ("Problem initializing data connection");
	       case FTPCOMMAND:    return ("Command failed");
	       case FTPPROXYTHIRD: return ("Proxy server does not support third party");
	       case FTPNOPRIMARY:  return ("No primary connection for proxy server");

	       case PINGREPLY:	   return ("Host does not reply");
	       case PINGSOCKET:    return ("Unable to obtain socket");
	       case PINGPROTO:	   return ("Unknown protocol ICMP");
	       case PINGSEND:	   return ("Send failed");
	       case PINGRECV:	   return ("Recv failed");
	       case PINGHOST:	   return ("Unknown host");

	       default: 	   return ("Unknown error");
	}
}/*ftperrstr()*/


static int ftpmail_send (FTPMAIL *mp, char *fname, char *alias, int doafter)
{
	struct stat f;
	char *p;
	int fd;

	if (getstat(fname,&f)) {      /* file exist? */
	   message(6,"!File %s not found", fname);
	   return (XFER_OK);
	}

	if (alias)
	   message(2,"*FTP PUT: %s->%s (%ldb)", fname, alias, f.st_size);
	else
	   message(2,"*FTP PUT: %s (%ldb)", fname, f.st_size);

	if (!alias) {
	   for (p = alias = fname; *p; p++) {
	       if (*p=='\\' || *p==':' || *p=='/')
		  alias = p + 1;
	   }
	}

	strlwr(alias);

	if (dllftpput(mp->host,mp->userid,mp->passwd,NULL,fname,alias,T_BINARY) < 0) {
	   message(3,"!FTP PUT(%s->%s): %s", fname, alias, ftperrstr(dllftp_errno()));
	   return (XFER_ABORT);
	}

	switch (doafter) {
	       case DELETE_AFTER:
		    if (unlink(fname))
		       message(6,"!Could not delete %s",fname);
		    else
		       message(1,"+FTP PUT deleted %s",fname);
		    break;
	       case TRUNC_AFTER:
		    if ((fd = dos_sopen(fname,O_WRONLY|O_CREAT|O_TRUNC,DENY_ALL)) < 0) {
		       message(6,"!Error truncating %s",fname);
		       break;
		    }
		    dos_close(fd);
		    message(1,"+FTP PUT truncated %s",fname);
		    break;
	       default:
		    message(1,"+FTP PUT %s",fname);
		    break;
	}

	return (XFER_OK);
}/*ftpmail_send()*/


/* ------------------------------------------------------------------------- */
long ftpmail_client (FTPMAIL *mp)	       /* only called from tmailer.c */
{
	char buffer[PATHLEN], s[PATHLEN], locname[PATHLEN];
	char obj[9];
	unsigned long addr;
	struct stat f;
	char *name, *fnalias;
	int   found_pkt, fsent;
	long  current, last_start;
	long btime;
	int i, c, res = -1;
	FILE *fp;

	if (!DosLoadModule((PSZ) obj, sizeof (obj), (PSZ) "FTPAPI", &hmod_ftpapidll)) {
	   if (DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPLOGOFF",(PFN *) &dllftplogoff) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPGET"   ,(PFN *) &dllftpget	) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPPUT"   ,(PFN *) &dllftpput	) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPCD"	  ,(PFN *) &dllftpcd	) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPDELETE",(PFN *) &dllftpdelete) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPRENAME",(PFN *) &dllftprename) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPLS"	  ,(PFN *) &dllftpls	) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPDIR"   ,(PFN *) &dllftpdir	) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTPPING"  ,(PFN *) &dllftpping	) ||
	       DosQueryProcAddr(hmod_ftpapidll,0,(PSZ) "FTP_ERRNO",(PFN *) &dllftp_errno)) {
	      message(6,"-Can't load/query FTPAPI.DLL");
	      DosFreeModule(hmod_ftpapidll);
	      hmod_ftpapidll = 0UL;
	      return (res);
	   }
	}

	mdm_busy();

	statusmsg("FTPmail %d:%d/%d%s%s (%s)",
		  mp->zone, mp->net, mp->node,
		  pointnr(mp->point), zonedomabbrev(mp->zone),
		  mp->host);
	message(2,"+FTPmail %s (%d:%d/%d%s%s)",
		  mp->host,
		  mp->zone, mp->net, mp->node,
		  pointnr(mp->point), zonedomabbrev(mp->zone));

	btime = time(NULL);

	sprint(buffer,"%sTASK.%02x",flagdir,task);
	if (dllftpput(mp->host,mp->userid,mp->passwd,NULL,buffer,bsyname,T_ASCII) < 0) {
	   message(3,"!FTP PUT(%s): %s", bsyname, ftperrstr(dllftp_errno()));
	   goto err;
	}

	if ((i = dllftpping(mp->host,56,&addr)) < 0)
	   message(3,"-FTP PING: %s", ftperrstr(dllftp_errno()));
	else
	   message(3,"*FTP PING: %d millisecond%s", i, i == 1 ? "" : "s");

	if (dllftpcd(mp->host,mp->userid,mp->passwd,NULL,"in") < 0) {
	   message(3,"!FTP CD in: %s", ftperrstr(dllftp_errno()));
	   goto fini;
	}

	fsent = found_pkt = 0;
	for (c = 0; c < N_FLAGS; c++) {
	    sprint(buffer,"%s%04.4x%04.4x%s.%cUT", holdarea(mp->zone),
			  (word) mp->net, (word) mp->node,
			  holdpoint(mp->point), (short) ext_flag[c]);

	    if (!getstat(buffer,&f)) {
	       invent_pktname(s); /* Build a dummy PKT file name */

	       /* Tell xmit to handle this as a SEND then DELETE */
	       if (!found_pkt) {
		  found_pkt = 1;
		  message(1,"+Sending mailpacket(s)");
	       }

	       if (ftpmail_send(mp,buffer,s,DELETE_AFTER) == XFER_ABORT)
		  goto fini;
	       fsent++;
	    }
	}

	for (c = 0; c < N_FLAGS; c++) {
	    sprint(buffer,"%s%04.4x%04.4x%s.%cLO", holdarea(mp->zone),
			  (word) mp->net, (word) mp->node,
			  holdpoint(mp->point), (short) ext_flag[c]);

	    if (ext_flag[c] == 'N') continue;

	    if ((fp = sfopen(buffer,"r+b",DENY_ALL)) != NULL && !ferror(fp)) {
	       current = 0L;
	       while (!feof(fp)) {
		     s[0] = '\0';
		     last_start = current;
		     fgets(s,PATHLEN,fp);
		     if (ferror(fp)) {
			message(6,"!Error reading %s",buffer);
			s[0] = '\0';
		     }

		     name = strtok(s," \t\r\n\032");

		     current = ftell(fp);

		     if (!name) continue;

		     if (name[0] == '#') {
			name++;
			i = TRUNC_AFTER;
		     }
		     else if (name[0] == '^') {
			name++;
			i = DELETE_AFTER;
		     }
		     else if (name[0] == ';' || name[0] == '~')
			continue;
		     else
			i = NOTHING_AFTER;

		     if (!name[0]) continue;

		     fnalias = strtok(NULL," \t\r\n\032");

		     if (getstat(name,&f)) {	  /* file exist? */
			message(6,"!File %s not found",name);
			continue;
		     }

		     if (f.st_size || !is_arcmail(name,(int) strlen(name) - 1)) {
			if (ftpmail_send(mp,name,fnalias,i) == XFER_ABORT) {
			   fclose(fp);
			   goto fini;
			}
		     }

		     fsent++;

		     /* File was sent, flag file name */
		     fseek(fp,last_start,SEEK_SET);
		     fprint(fp,"~");		/* flag it */
		     fflush(fp);
		     rewind(fp);    /* clear any eof flags */
		     fseek(fp,current,SEEK_SET);
	       }/*while*/

	       fclose(fp);
	       unlink(buffer);
	    }/*sfopen*/
	}/*for*/

	if (!fsent)
	   message(2,"=Nothing to send to %d:%d/%d%s%s",
		     mp->zone, mp->net, mp->node,
		     pointnr(mp->point), zonedomabbrev(mp->zone));

	if (dllftpcd(mp->host,mp->userid,mp->passwd,NULL,"../out") < 0) {
	   message(3,"!FTP CD ../out: %s", ftperrstr(dllftp_errno()));
	   goto fini;
	}

	sprint(buffer,"%sFTPMAIL.%02x",flagdir,task);
	if (dllftpls(mp->host,mp->userid,mp->passwd,NULL,buffer,".") < 0) {
	   message(3,"!FTP LS .: %s", ftperrstr(dllftp_errno()));
	   goto fini;
	}

	if ((fp = fopen(buffer,"rt")) != NULL) {
	   while (fgets(s,127,fp)) {
		 *skip_to_blank(skip_blanks(s)) = '\0';

		 sprint(locname,"%sXENBAD%02x.000",safepath,task);
		 unique_name(locname);

		 message(2,"*FTP GET: %s",s);
		 if (dllftpget(mp->host,mp->userid,mp->passwd,NULL,locname,s,"w",T_BINARY) < 0) {
		    message(3,"!FTP GET(%s): %s", s, ftperrstr(dllftp_errno()));
		    if (fexist(locname)) unlink(locname);
		    goto fini;
		 }

		 getstat(locname,&f);
		 message(1,"+FTP GET %s (%ldb)", s, f.st_size);

		 strcpy(buffer,safepath);
		 name = &buffer[strlen(buffer)];
		 strcpy(name,s);
		 unique_name(buffer);
		 if (rename(locname,buffer))
		    message(6,"!Could not rename '%s' to '%s'",locname,buffer);
		 else if (strcmp(s,name))
		    message(2,"+FTP GET: Dup file renamed: %s", name);

		 if (dllftpdelete(mp->host,mp->userid,mp->passwd,NULL,s) < 0)
		    message(3,"-FTP DELETE(%s): %s", s, ftperrstr(dllftp_errno()));
	   }
	   fclose(fp);

	   sprint(buffer,"%sFTPMAIL.%02x",flagdir,task);
	   unlink(buffer);
	}

	res = 1;

fini:	if (dllftpcd(mp->host,mp->userid,mp->passwd,NULL,"..") < 0)
	   message(3,"!FTP CD ..: %s", ftperrstr(dllftp_errno()));
	else if (dllftpdelete(mp->host,mp->userid,mp->passwd,NULL,bsyname) < 0)
	   message(3,"-FTP DELETE(%s): %s", bsyname, ftperrstr(dllftp_errno()));

err:	dllftplogoff();

	btime = time(NULL) - btime;
	message(6,"+FTPmail session %s %ld:%02ld min.",
		res < 0 ? "failed" : "completed",
		btime / 60L, btime % 60L);

	if (hmod_ftpapidll) {
	   DosFreeModule(hmod_ftpapidll);
	   hmod_ftpapidll = 0UL;
	}

	init_modem();
	waitforline = time(NULL) + 60;
	return (res);

	/* return values:      */
	/* <0 = failed session */
	/* =0 = try no connect */
	/* >0 = okido  session */
}/*ftpmail_client()*/


/* end of ftpmail.c */
