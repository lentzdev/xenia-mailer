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


#include "zmodem.h"

	    /* Session flow ------------------------------------------------ */
	    /* YooHoo/EMSI     YooHoo/EMSI     EMSI/Hydra      EMSI/HydraRH1 */
	    /* ==============  ==============  ==============  ============= */
	    /* <- All receive  -> REQ	       -> REQ	       -> REQ	     */
	    /* -> REQ	       -> PKT	       -> PKT	       -> End batch  */
	    /* -> RSP	       -> Files        -> Files        -> RSP	     */
	    /* -> PKT	       <- All receive  -> End batch    -> PKT	     */
	    /* -> Files        -> RSP	       -> RSP	       -> Files      */
	    /* <- RSP			       -> End batch    -> End batch  */
	    /* ------------------------------------------------------------- */

boolean did_skip;


/* SEND WaZOO (send another WaZOO-capable mailer its mail)
   returns TRUE (1) for good xfer, FALSE (0) for bad */
int send_WaZOO (int fsent)
{
	char fname[128];
	char s[128];
	int  c;
	int  break_now, found_pkt;
	int  dopoint;
	int  n;
	int  res;

	did_skip = false;
	made_request = NULL;

	/*-------------------------------------------------------------------*/
	/* Send our File requests to other system			     */
	/*-------------------------------------------------------------------*/
	break_now = 0;
	for (n = 0; !break_now && n < num_rem_adr; n++) {
	    if (!flag_set(rem_adr[n].zone,rem_adr[n].net,rem_adr[n].node,rem_adr[n].point))
	       continue;

	    sprint(fname,"%s%04.4x%04.4x%s.REQ", holdarea(rem_adr[n].zone),
		   (word) rem_adr[n].net, (word) rem_adr[n].node,
		   holdpoint(rem_adr[n].point));
	    if (!fexist(fname) && rem_adr[n].pointnet) {
		if (!flag_set(rem_adr[n].zone,rem_adr[n].pointnet,rem_adr[n].point,0))
		   continue;
		sprint(fname,"%s%04.4x%04.4x.REQ", holdarea(rem_adr[n].zone),
		       (word) rem_adr[n].pointnet, (word) rem_adr[n].point);
	    }

	    if (fexist(fname)) {	     /* Found a request to send! */
	       break_now = 1;	   /* Flag to break out of the two loops */
	       if (!(remote.capabilities & WZ_FREQ) ||
		   (e_ptrs[cur_event]->behavior & MAT_NOOUTREQ) ||
		   (remote.options & EOPT_HOLDALL)) {
		  /*did_skip = true;*/
		  message(2,"=Not allowed to make requests");
	       }
	       else {
		  message(1,"+Transmitting request(s)");
		  /* make a valid name for the other side... */
		  sprint(s,"%04.4x%04.4x.REQ",
			 (word) rem_adr[0].net,(word) rem_adr[0].node);
		  res = (*wzsendfunc)(fname,s,(int) NOTHING_AFTER,fsent++,DO_WAZOO);
		  if (res == XFER_ABORT) return (FALSE);
		  if (res == XFER_OK)	 made_request = ctl_string(fname);
		  /*if (res == XFER_SKIP)  did_skip = true;*/
	       }
	    }
	}

	if (wzsendfunc != send_hydra || (remote.options & EOPT_REQFIRST)) {
	   if (wzsendfunc == send_hydra &&
	       send_hydra(NULL,NULL,(int) NOTHING_AFTER,END_BATCH,DO_WAZOO) != XFER_OK)
	      return (FALSE);
	   fsent = process_req_files(fsent);
	   if (fsent < 0) return (FALSE);
	}

	/*-------------------------------------------------------------------*/
	/* Send all waiting ?UT files (mail bundles)			     */
	/*-------------------------------------------------------------------*/
	found_pkt = 0;
	break_now = 0;
	for (n = 0; !break_now && n < num_rem_adr; n++) {
	    if (!flag_set(rem_adr[n].zone,rem_adr[n].net,rem_adr[n].node,rem_adr[n].point))
	       continue;

	    for (dopoint = 0; !break_now && dopoint < 2; dopoint++) {
		if (dopoint) {
		   if (!rem_adr[n].pointnet ||
		       !flag_set(rem_adr[n].zone,rem_adr[n].pointnet,rem_adr[n].point,0))
		      continue;
		}

		for (c = 0; !break_now && c < N_FLAGS; c++) {
		    if (caller &&
			(ext_flag[c] == 'H' || ext_flag[c] == 'W' ||
			 (ext_flag[c] == 'Q' && (ourbbsflags & NO_REQ_ON))))
		       continue;

		    if (ext_flag[c] == 'N') continue;

		    if (dopoint)
		       sprint(fname,"%s%04.4x%04.4x.%cUT", holdarea(rem_adr[n].zone),
			      (word) rem_adr[n].pointnet, (word) rem_adr[n].point,
			      (short) ext_flag[c]);
		    else
		       sprint(fname,"%s%04.4x%04.4x%s.%cUT", holdarea(rem_adr[n].zone),
			      (word) rem_adr[n].net, (word) rem_adr[n].node,
			      holdpoint(rem_adr[n].point), (short) ext_flag[c]);

		    if (fexist(fname)) {
		       if ((remote.options & EOPT_HOLDALL) ||
			   (!caller && (remote.options & EOPT_BADAPPLE))) {
			  did_skip = true;
			  break_now = 1;
			  message(2,"=Not allowed to send any mail");
			  continue;
		       }

		       invent_pktname(s); /* Build a dummy PKT file name */

		       /* Tell xmit to handle this as a SEND then DELETE */
		       if (!found_pkt) {
			  found_pkt = 1;
			  message(1,"+Sending mailpacket(s)");
		       }

		       res = (*wzsendfunc)(fname,s,(int) DELETE_AFTER,fsent++,DO_WAZOO);
		       if (res == XFER_ABORT) return (FALSE);
		       if (res == XFER_SKIP)  did_skip = true;
		    }	 
		}/*for*/
	    }/*for*/
	}

	fsent = SendFiles(fsent);
	if (fsent < 0) return (FALSE);

	if (wzsendfunc == send_hydra && !(remote.options & EOPT_REQFIRST)) {
	   if (send_hydra(NULL,NULL,(int) NOTHING_AFTER,END_BATCH,DO_WAZOO) != XFER_OK)
	      return (FALSE);
	   fsent = process_req_files(fsent);
	   if (fsent < 0) return (FALSE);
	}

	if (!fsent)
	   message(2,"=Nothing to send to %d:%d/%d%s%s",
		     rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		     pointnr(rem_adr[0].point), zonedomabbrev(rem_adr[0].zone));

	if ((*wzsendfunc)(NULL,NULL,(int) NOTHING_AFTER,fsent ? END_BATCH : NOTHING_TO_DO,DO_WAZOO) == XFER_ABORT)
	   return (FALSE);

	return (TRUE);
}/*WaZOO()*/


/* SendFiles is used by both WaZOO and FTSC mail sender */
SendFiles (int fsent)
{
	FILE *fp;
	char  fname[PATHLEN + 1];
	char  s[PATHLEN + 1];
	struct stat f;
	char *name, *fnalias;
	int   c;
	int   i;
	long  current, last_start;
	int   found_file, found_arc, skipped_any;
	int   dopoint;
	int   n;
	int   res;

	found_file = found_arc = 0;

	/*-------------------------------------------------------------------*/
	/* Send files listed in ?F files (attached files)		     */
	/*-------------------------------------------------------------------*/
	for (n = 0; n < num_rem_adr; n++) {
	    if (!flag_set(rem_adr[n].zone,rem_adr[n].net,rem_adr[n].node,rem_adr[n].point))
	       continue;

	    for (dopoint = 0; dopoint < 2; dopoint++) {
		if (dopoint) {
		   if (!rem_adr[n].pointnet ||
		       !flag_set(rem_adr[n].zone,rem_adr[n].pointnet,rem_adr[n].point,0))
		      continue;
		}

		for (c = 0; c < N_FLAGS; c++) {
		    if (caller &&
			(ext_flag[c] == 'H' || ext_flag[c] == 'W' ||
			 (ext_flag[c] == 'Q' && (ourbbsflags & NO_REQ_ON))))
		       continue;

		    if (ext_flag[c] == 'N') continue;

		    if (dopoint)
		       sprint(fname,"%s%04.4x%04.4x.%cLO", holdarea(rem_adr[n].zone),
			      (word) rem_adr[n].pointnet, (word) rem_adr[n].point,
			      (short) ext_flag[c]);
		    else
		       sprint(fname,"%s%04.4x%04.4x%s.%cLO", holdarea(rem_adr[n].zone),
			      (word) rem_adr[n].net, (word) rem_adr[n].node,
			      holdpoint(rem_adr[n].point), (short) ext_flag[c]);

		    if (!fexist(fname)) continue;

		    if ((fp = sfopen(fname,"r+b",DENY_ALL)) != NULL && !ferror(fp)) {
		       skipped_any = 0;
		       current	= 0L;
		       while (!feof(fp)) {
			     s[0] = '\0';
			     last_start = current;
			     com_disk(true);
			     fgets(s,PATHLEN,fp);
			     com_disk(false);
			     if (ferror(fp)) {
				message(6,"!Error reading %s",fname);
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

			     if (!fexist(name)) {	  /* file exist? */
				message(6,"!File %s not found",name);
				continue;
			     }

			     if ((remote.options & EOPT_HOLDALL) ||
				 (!caller && (remote.options & EOPT_BADAPPLE))) {
				did_skip = true;
				skipped_any = 1;
				if (!found_file) {
				   found_file = 1;
				   message(2,"=Not allowed to send any mail/files");
				}
			     }
			     else if ((remote.options & EOPT_HOLDECHO) &&
				      is_arcmail(name,(int) strlen(name) - 1)) {
				did_skip = true;
				skipped_any = 1;
				if (!found_arc) {
				   message(2,"=Not allowed to send any (echo)archives");
				   found_arc = 1;
				}
			     }
			     else {
				getstat(name,&f);
				if (!f.st_size && is_arcmail(name,(int) strlen(name) - 1))
				   res = XFER_OK;
				else {
				   if (wzsendfunc == send_SEA)
				      com_purge();
				   res = (*wzsendfunc)(name,fnalias,(int) i,fsent++,DO_WAZOO);
				   if (res == XFER_ABORT) {
				      fclose(fp);
				      return (-1);
				   }
				}

				if (res == XFER_SKIP && c != 'Q') {
				   did_skip = true;
				   skipped_any = 1;
				}
				else {	/* File was sent, flag file name */
				   fseek(fp,last_start,SEEK_SET);
				   fprint(fp,"~");	      /* flag it */
				   fflush(fp);
				   rewind(fp);	  /* clear any eof flags */
				   fseek(fp,current,SEEK_SET);
				}
			     }
		       }/*while*/

		       fclose(fp);
		       if (!skipped_any)
			  unlink(fname);
		    }/*sfopen*/
		}/*for*/
	    }/*for*/
	}

	return (fsent);
}/*SendFiles()*/


/* end of wzsend.c */
