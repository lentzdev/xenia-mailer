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
#define YOOHOO	0xf1					  /* not known yet.. */
#define TSYNC	0xae					  /* signal mailer   */


SEA_sendreq ()
{
   char fname[PATHLEN];
   char gname[128];
   char reqf[128];
   char rtime[20];
   char *p, *name, *pw;
   int j, done, done1, nfiles;
   FILE *fp;
   long t1;

   t1 = timerset (1000);

   if (flag_set(remote.zone,remote.net,remote.node,remote.point)) {
      sprint(fname, "%s%04.4x%04.4x%s.REQ", holdarea(remote.zone),
	     (word) remote.net, (word) remote.node,
	     holdpoint(remote.point));

      if (fexist(fname)) goto fnd;
   }

   if (remote.pointnet && flag_set(remote.zone,remote.pointnet,remote.point,0)) {
      sprint(fname,"%s%04.4x%04.4x.REQ", holdarea(remote.zone),
	     (word) remote.pointnet, (word) remote.node);
      if (fexist(fname)) goto fnd;
   }

   if (!fexist(fname))		   /* If we have file requests, then do them */
      message(1,"+No outbound requests");
   else {
fnd:
      message(1,"+Outbound requests");
      if ((fp = sfopen (fname,"rt",DENY_ALL)) == NULL) { /* Open our .REQ file */
	 com_putbyte(ETB);
	 com_flush();
	 return (1);
      }

      com_purge();
      com_dump();

      *rtime= '\0';

      /* As long as we do not have EOF, read the request */
      while ((fgets (reqf, 79, fp) != NULL) && carrier()) {	   /* format */
	    p = reqf+strlen(reqf)-1;   /* First get rid of the trailing junk */
	    while ((p>=reqf)&&(isspace(*p)))
		  *p-- = '\0';

	    p = reqf;			/* Now get rid of the beginning junk */
	    while ((*p) && (isspace (*p)))
		  p++;
	    name = p;			    /* This is where the name starts */

	    if (*name == ';')		  /* if first char ; ignore the line */
	       continue;

	    if ((p=strchr(name,'+'))!=NULL) {
		strncpy(rtime, p+1, 19);
		*p='\0';
	    }

	    p = name;			   /* Now get to where the name ends */
	    while ((*p) && (!isspace (*p)))
		  p++;
	    if (*p) {
	       *p = '\0';
	       ++p;
	       while ((*p) && (*p != '!'))
		     p++;
	       if (*p == '!') {
		  *p = ' ';
	       }
	       pw = p;
	    }
	    else {
	       pw = p;
	    }

	    if (req_out(name, rtime, pw))
	       continue;

	    /* Dutchie's trick with .services -- but *WE* wait 2 mins, not 5 */
	    t1 = timerset((word) ((*name=='.') ? 1200 : 100));
	    done = 0;				      /* Wait for ACK or ENQ */
	    while ((!timeup(t1)) && carrier() && !done) {
		  j = com_getbyte();
		  if (j >= 0) {
		     if (j == ACK) {  /* If ACK, receive files using SEAlink */
			nfiles = 0;
			done1 = 0;
			do {
			   *gname=0;
			   if (modem) {    /* Modem7 filename wanted? */
			      p=rcvfname();
			      if (!p || !*p) goto dne;
			      if (!telink) {	/* use telink name, if poss. */
				 unparse(gname,p);  /* !telink use this name */
				 message(1,"+7RECV %s",gname);
				 if (fexist(gname)) {
				    unique_name(gname);
				    message(1,"+7RECV: Dup file renamed: %s",gname);
				 }
			      }
			   }

			   p = rcvfile(inbound, telink ? NULL : gname);
			   if (p == NULL || !*p)
dne:			      done1 = 1;    /* that's it. end or fatal error */
			   else
			      ++nfiles; 		/* increment counter */

			   *rtime= '\0';
			} while (carrier() && !done1);

			message(2,"+Received %d files", nfiles);
			done = 1;
			t1 = timerset (100);

			/* wait for request start */
			while ((com_getbyte() != ENQ) &&
			       (!timeup (t1)) && carrier()) ;
		     }
		     else if (j == ENQ)  /* If ENQ, report no files received */
			req_out (name, rtime, pw);
		 }/*if*/
	    }/*while*/
      }/*while*/

      fclose(fp);
      unlink(fname);
      message(1,"+End of outbound requests");
   }/*else*/

   com_putbyte(ETB);				/* Finish the file requests off */
   com_flush();
   return (0);
}/*SEA_sendreq()*/


int SEA_recvreq()
{
	int done;
	char req[64];
	long t1;

	if ((caller && (ourbbsflags & NO_REQ_ON)) ||
	    (remote.options & EOPT_BADAPPLE)) {
	   /* Refuse file requests if we're calling and no reqs allowed on us */
	   com_putbyte(CAN);
	   com_flush();
	   message(1,"=Refusing inbound requests");
	   return (TRUE);
	}

	t1 = timerset (200);

	/* Try the bark stuff */
	done = 0;
	message(1,"+Waiting for inbound requests");

	while (carrier() && !done && !timeup(t1)) {
	      com_putbyte(ENQ); 		   /* Send out the start signal */
	      com_flush();

	      switch (com_getc(20)) {		    /* Wait for the response */
		     case ACK:
			  if (get_req_str(req)) {
			     com_putbyte(ACK);
			     com_flush();
			     process_bark_req(req);
			  }
			  t1 = timerset (200);
			  break;

		     case ETB:
		     case ENQ:
			  done = 1;
			  break;

		     case 'C':
		     case NAK:
			  com_putbyte(EOT);
			  com_flush();
			  com_purge();
			  break;
	      }
	}

	req_close();
	message(1,"+End of inbound requests");
	return (TRUE);
}/*SEA_recvreq()*/


req_out(char *name, char *rtime, char *pw)
{
   char *p;
   word crc;

   /* send the request */
   p = name;
   if (!*p)
      return (1);

   message(2,"+Requesting '%s'%s%s%s", name, *rtime ? " (Update)" : "",
	   (*pw)?" with password:":"", pw);
   com_bufbyte(ACK);
   crc = 0;
   while (*p) {
	 com_bufbyte(*p);
	 crc = crc_update(crc,*p);
	 ++p;
   }

   /* This is for straight requests being the standard */
   com_bufbyte(' ');
   crc = crc_update(crc,' ');

   if (*rtime) p = rtime;
   else        p ="0";
   
   while (*p) {
	 com_bufbyte(*p);
	 crc = crc_update(crc,*p);
	 p++;
   }

   com_bufbyte(' ');
   crc = crc_update(crc,' ');

   p = pw;
   while (*p) {
	 com_bufbyte(*p);
	 crc = crc_update(crc,*p);
	 p++;
   }

   com_bufbyte(ETX);
   crc = crc_finish(crc);
   com_bufbyte(crc&0xff);
   com_bufbyte(crc>>8);
   com_flush();

   return (0);
}/*req_out()*/

int get_req_str(char *req)
{
	word crc;
	int i;
	char *p;

	p= req;

	crc = 0;
	while (carrier()) {
	      i = com_getc(2);

	      /* timeout? */
	      if (i < 0) {
		 message(1,"-%s",msg_timeout);
		 return (0);
	      }

	      /* end of block */
	      i &= 0xff;
	      if (i == ETX) {
		 *p = '\0';
		 i = com_getc(10) & 0xff;
		 crc = crc_update(crc,(word) (com_getc(10) & 0xff));
		 crc = crc_update(crc,(word) i);
		 if (crc) {
		    message(2,"-%s",msg_datacrc);
		    return (0);
		 }
		 return (1);
	      }
	      else {
		 crc = crc_update (crc,(word) i);
		 if ((p - req) < 60) *p++ = i;
	      }
	}

	return (0);
}/*get_req_str()*/


/* end of cdog.c */
