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


/* emsi.c EMSI implementation
 * This implementation originally written by Steven Green (2:252/25) for the
 * Atari ST version of BinkleyTerm.
 * Thoroughly messed up, modified, rewritten, etc. by Arjen Lentz for Xenia
 *
 * The original EMSI specifications were designed by Chris Irwin and
 * Joaquim H. Homrighausen.
 * This module is based on the information given in:
 *	EMSC-0001; Electronic Mail Standards Document #001; May 3, 1991
 *	(and later republished as FSC-0056)
 * Revision history:
 *	17Jul91 : SWG : Started
 *	27Oct91 : AGL : Finished ;-)
 *	14Oct92 : AGL : added SLK flag according to EMSC decision
 *	31Dec92 : AGL : added DZA flag according to EMSI spec (DirectZap)
 *	11Jan93 : AGL : Added TRX# field
 *	....... : AGL : various
 *	04Aug95 : AGL : MD5 password exchange using RFC-1725 shared secret
 */

#include "zmodem.h"
#include "md5.h"


#define DEBUG  0
#define IDEBUG 0


/* define the packet strings */
#define EMSI_IDLEN 14
char *emsi_pkt[] = {
	"**EMSI_INQ",
	"**EMSI_REQ",
	"**EMSI_CLI",
	"**EMSI_HBT",
	"**EMSI_DAT",
	"**EMSI_ACK",
	"**EMSI_NAK",
	"**EMSI_MD5",
	"**EMSI_M5R",
	"**EMSI_IRQ",
	"**EMSI_IIR",
	"**EMSI_ICI",
	"**EMSI_ISI",
	"**EMSI_CHT",
	"**EMSI_TCH",
	NULL
};

static long ourtrx;
static char md5_recv[81],
	    md5_send[81];


/* ------------------------------------------------------------------------- */
/* Check incoming character for completion of EMSI packet name
   Return:	  -1 : no completion
		0..5 : EMSI_type
   buf must be at least EMSI_IDLEN+1 bytes and the 1st element initialised
   to 0 before first call.

   This could be simplified by making it modal, i.e.:
	if (c == '*')
	   c = modem_in
	   if (c == '*')
	      read 8 bytes
	      compare to emsi_packet sequences
	      read 4 byte CRC

   However this may cause the calling routine to miss characters particularly
   if any external mailer strings have '**' in them, etc...
*/

int check_emsi (char c, char *buf)
{
	char  *s;
	int    l;
	char **id;
	int    type;

	/* Add character to end of buffer */
	for (s = buf; *s; s++);
	*s++ = c & 0x7f;
	*s = '\0';

	/* Compare to Packet ID's */
	/* Enough for full ID name + crc16 or len16 */
	if (strlen(buf) == EMSI_IDLEN) {
	   for (type = 0, id = emsi_pkt; *id; type++, id++) {
	       if (!strncmp(*id,buf,10)) {
		  word val;

		  buf[0] = '\0';      /* Clear buffer to restart next time */

		  if (sscanf(&buf[10],"%04hx",&val) != 1) /* read hex-value */
		     return (-1);

		  switch (type) {
			 case EMSI_DAT:
			 case EMSI_MD5:
			 case EMSI_ICI:
			 case EMSI_ISI:
			      break;

			 default:
			      { register word crc = 0;
				register int  i;

				for (i = 0; i < 8; i++)
				   crc = crc_update(crc,buf[2 + i]);
				crc = crc_finish(crc);
				if (crc != val)
				   return (-1);
			      }
			      break;
		  }

		  return (type);
	       }
	   }

	   /* should never get here... */
	   buf[0] = '\0';
	   return (-1);
	}

	/* We get here if we have less than 14 characters or it doesn't match
	   any of the packet types
	   Shift buffer along to 1st possible match
	   This probably doesn't need to be this complex, but I want to make
	   it handle all cases, e.g.
	   ***EMSI_REQ	   : must ignore the 1st * instead of resetting at the 3rd
	   **EMS**EMSI_REQ : Must not reset at the 2nd group of '*'.
	   In fact we could probably assume that all sequences start at the
	   start of a line (except for the double EMSI_INQ sent during initialisation
	*/

	l = strlen(buf);
	if (l <= 10) {	 /* > 10 && in <crc16> and must have already matched */
	   s = buf;
	   while (l) {
		 id = emsi_pkt;
		 while (*id) {
		       if (!strncmp(*id,s,l)) {
			  if (s != buf)
			     strcpy(buf,s);
			  return (-1);
		       }

		       id++;
		 }

		 l--;
		 s++;
	   }

	   buf[0] = '\0';	/* No matches */
	}

	return (-1);
}


/* ------------------------------------------------------------------------- */
static char hex[] = "0123456789ABCDEF";

static void put_hex (word val)
{
	com_bufbyte(hex[(val >> 12) & 0xf]);
	com_bufbyte(hex[(val >>  8) & 0xf]);
	com_bufbyte(hex[(val >>  4) & 0xf]);
	com_bufbyte(hex[ val	    & 0xf]);
}


/* ------------------------------------------------------------------------- */
static int byte_to_hex (int c)
{
	c = toupper(c);
	c -= '0';
	if (c > 9)
	   c -= ('A' - '9' - 1);
	return (c);
}


/* ------------------------------------------------------------------------- */
void put_emsi (int type, boolean cr)
{
	char *s = emsi_pkt[type];
	word  crc;

	com_putblock((byte *) s,(word) strlen(s));

	if (type != EMSI_DAT) {
	   crc = 0;
	   s += 2;	/* Skip over "**" */
	   while (*s)
		 crc = crc_update(crc,*s++);

	   if (type == EMSI_MD5) {
	      char buf[5];
	      word len;

	      len = (word) strlen(md5_recv);
	      buf[0] = hex[(len >> 12) & 0x0f];
	      buf[1] = hex[(len >>  8) & 0x0f];
	      buf[2] = hex[(len >>  4) & 0x0f];
	      buf[3] = hex[ len        & 0x0f];
	      buf[4] = '\0';
	      com_putblock((byte *) buf,4);
	      for (s = buf; *s; crc = crc_update(crc,*s++));

	      com_putblock((byte *) md5_recv,len);
	      for (s = md5_recv; *s; crc = crc_update(crc,*s++));
	   }

	   crc = crc_finish(crc);

	   put_hex(crc);
	   if (cr)
	      com_bufbyte('\r');
	   /*com_bufbyte(XON);*/
	   com_bufflush();
	}
}


/* ------------------------------------------------------------------------- */
/* Receive EMSI_DAT/ICI/ISI packet
   I'm going to do this just as one function instead of a state machine
   Extract a field from the data
   Similar to strtok() but understands escape sequences \ab \\ }} and ]]
   The first time it is called you should pass the buffer address
   future calls may pass NULL to continmue with next field
   fields are {...} and [...]
   double }} and ]] are converted to single characters
   Note the actual data buffer is altered by having nulls put in
   Returns NULL if any error
*/

static char *last_field = NULL;

static char *get_field (char *dat)
{
	char  sep;
	char *s, *s1;

	if (dat == NULL)
	   dat = last_field;

	if (dat == NULL)
	   return (NULL);

	sep = *dat++;
	if (sep == '{')
	   sep = '}';
	else if (sep == '[')
	   sep = ']';
	else
	   return (NULL);

	s = s1 = dat;
	while (*s) {
	      byte c = *s++;

	      if (c == sep) {
		 if (*s == sep) 	/* Double end brace */
		    s++;
		 else { 		/* Single end brace marks end */
		    *s1 = '\0';
		    last_field = s;
		    return (dat);
		 }
	      }
	      else if (c == '\\' && sep == '}') {	       /* hex escape */
		 int n;

		 c = *s++;
		 n = byte_to_hex(c);
		 if (!(n & ~0x0f)) {   /* If 1st digit !hex, pass it through */
		    c = n;
		    n = byte_to_hex(*s);
		    if (!(n & ~0x0f)) { 	/* If 2nd digit is not hex?? */
		       s++;
		       c = (c << 4) | n;
		    }
		 }
	      }

	      *s1++ = c;
	}

	return (NULL);
}


/* ------------------------------------------------------------------------- */
/* Process the incoming EMSI_DAT packet */
static boolean process_dat (char *dat)
{
	char *last_tok, *s, *s1, *s2, *s3;
	int i, j;
	boolean mismatch;
	char buf[128];
	char *city = "";
	long theirtrx = 0L;

	s = get_field(dat);		/* Fingerprint */
	if (!s || stricmp(s,"EMSI"))
	   return (false);

	/* Addresses */
	s = get_field(NULL);
	if (!s)
	   return (false);

	num_rem_adr = 0;
	buf[0] = '\0';
	for (s = strtok(s," "); s; s = strtok(NULL," ")) {
	    if (getaddress(s,&rem_adr[num_rem_adr].zone,
			     &rem_adr[num_rem_adr].net,
			     &rem_adr[num_rem_adr].node,
			     &rem_adr[num_rem_adr].point)) {

	       remap(&rem_adr[num_rem_adr].zone, &rem_adr[num_rem_adr].net,
		     &rem_adr[num_rem_adr].node, &rem_adr[num_rem_adr].point,
		     &rem_adr[num_rem_adr].pointnet);

	       if (!num_rem_adr) {
		  message(2,"=Address: %s",s);

		  sprint(buf,"%d:%d/%d%s%s",
			     rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
			     pointnr(rem_adr[0].point),zonedomabbrev(rem_adr[0].zone));
		  last_set(caller ? &last_out : &last_in, buf);
		  buf[0] = '\0';
		  show_stats();

		  win_xyprint(mailer_win,2,1,"Connected to %d:%d/%d%s",
			      rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
			      pointnr(rem_adr[0].point));
		  win_clreol(mailer_win);
	       }
	       else {
		  if (buf[0]) {
		     if ((strlen(buf) + 1 + strlen(s)) > 51) {
			message(2,"=Aka: %s", buf);
			buf[0] = '\0';
		     }
		     else
			strcat(buf," ");
		  }
		  strcat(buf,s);
	       }

	       if (num_rem_adr < 49)
		  num_rem_adr++;
	    }
	}

	if (buf[0])
	   message(2,"=Aka: %s", buf);

	/* Password */
	s = get_field(NULL);
	if (!s) return (false);
	*skip_to_blank(skip_blanks(s)) = '\0'; /* D'Bridge inserts a space!? */
	if (strlen(s) > 39) s[39] = '\0';
	strcpy(remote.password,s);

	/* Link codes */
	remote.capabilities = WZ_FREQ;	      /* defaults */
	remote.options	    = 0;
	remote.sysop[0]     = '\0';
	remote.name[0]	    = '\0';

	s = get_field(NULL);
	if (!s) return (false);
	message(0,"=Link codes: %s",s);
	for (s = strtok(s,","); s; s = strtok(NULL,",")) {
	    if (!stricmp(s,"PUP"))	      /* caller: pickup primary only */
	       remote.options |= EOPT_PRIMARY;
	    else if (!stricmp(s,"NPU"))       /* caller: no pickup wanted    */
	       remote.options |= EOPT_HOLDALL;
	    else if (!stricmp(s,"HXT"))       /* called: don't send echo/arc */
	       remote.options |= EOPT_HOLDECHO;
	    else if (!stricmp(s,"HRQ"))       /* called: don't send requests */
	       remote.capabilities &= ~WZ_FREQ;
	    else if (!stricmp(s,"RH1")) {     /* Only REQ in 1st Hydra batch */
	       if (ourbbs.options & EOPT_REQFIRST)
		  remote.options |= EOPT_REQFIRST;
	    }
	}

	/* Compatibility */
	s = get_field(NULL);
	if (!s) return (false);
	message(0,"=Compatibility: %s",s);
	for (s = strtok(s,","); s; s = strtok(NULL,",")) {
	    if (!stricmp(s,"NCP")) {
	       message(6,"!EMSI: No compatible protocol");
	       return (false);
	    }
	    else if (!stricmp(s,"HYD"))
	       remote.capabilities |= DOES_HYDRA;
	    else if (!stricmp(s,"DZA"))
	       remote.capabilities |= EMSI_DZA;
	    else if (!stricmp(s,"ZAP"))
	       remote.capabilities |= ZED_ZAPPER;
	    else if (!stricmp(s,"ZMO"))
	       remote.capabilities |= ZED_ZIPPER;
	    else if (!stricmp(s,"SLK"))
	       remote.capabilities |= EMSI_SLK;
	    else if (!stricmp(s,"NRQ"))
	       remote.capabilities &= ~WZ_FREQ;
	}

	/* Mailer code */
	s  = get_field(NULL);		/* Code    */
	s1 = get_field(NULL);		/* Name    */
	s2 = get_field(NULL);		/* Version */
	s3 = get_field(NULL);		/* Serial# */
	if (!s || !s1 || !s2 || !s3)
	   return (false);
	message(2,"=Mailer: %s %s%s%s (%s)",s1,s2,s3[0] ? "/" : "",s3,s);
	win_xyprint(mailer_win,10,4,"%s %s%s%s (%s)",s1,s2,s3[0] ? "/" : "",s3,s);

	if (!stricmp(s1,"FrontDoor")) {
	   char *p;
	   dword fdsn = 0UL;

	   for (p = s3; *p && isalnum(*p); p++);
	   if (*p ||
	       (!strnicmp(s3,"BR",2) &&
		(strlen(s3) != 8 || sscanf(s3+2,"%06lx",&fdsn) != 1 || fdsn < 1UL || fdsn > 0x26DUL))) {
	      message(6,"!Remote using FrontDoor with illegal key!");
	      if (!getenv("NOBADAPPLE"))
		 remote.options |= EOPT_BADAPPLE;
	   }
	}

	/* Extra fields */
	for (s = get_field(NULL); s; s = get_field(NULL)) {
	    s1 = get_field(NULL);			/* Identifier */
	    if (!s1) {
	       message(3,"-Bad Extra EMSI field(s)");
	       break;
	    }
	    if (!*s || !*s1) {			  /* Empty fields? hacks ;-) */
	       remote.options |= EOPT_BADAPPLE;
	       break;
	    }

	    last_tok = last_field;

	    if (!stricmp(s,"IDENT")) {		  /* 1st field is identifier */
							/* Parse IDENT */

	       s = get_field(s1);			/* System name */
	       if (s && *s) {
		  message(2,"=Name: %s",s);
		  win_xyputs(mailer_win,10,2,s);
		  if (strlen(s) > 59) s[59]= '\0';
		  strcpy(remote.name,s);
	       }
	       s = get_field(NULL);			/* Location */
	       if (s && *s) {
		  message(2,"=City: %s", s);
		  if (strlen(s) > 127) s[127]= '\0';
		  city = s;
	       }
	       s = get_field(NULL);			/* Sysop name */
	       if (s && *s) {
		  message(2,"=Sysop: %s",s);
		  win_xyputs(mailer_win,10,3,s);
		  if (*city) win_print(mailer_win,", %s",city);
		  if (strlen(s) > 59) s[59]= '\0';
		  strcpy(remote.sysop,s);
	       }
	       s = get_field(NULL);			/* Phone */
	       if (s && *s)
		  message(2,"=Phone: %s",s);
	       s = get_field(NULL);
	       if (s && *s)
		  message(2,"=Speed: %s",s);
	       s = get_field(NULL);
	       if (s && *s)
		  message(2,"=Flags: %s",s);

	       last_field = last_tok;
	    }
	    else if (!stricmp(s,"EMD5")) {		/* MD5 unique string */
	       s = get_field(s1);
	       if (s && *s == '<') {			/* "<somestring>"    */
		  if (strlen(s) > 80) s[80] = '\0';
		  strcpy(md5_send,s);
	       }

	       last_field = last_tok;
	    }
	    else if (!stricmp(s,"TRX#")) {	       /* Transaction number */
							/* Parse TRX# */
	       s = get_field(s1);			/* Trx number */
	       if (s && *s) {
		  long trxdiff;
		  struct tm *t;
		  extern char *mon[];	  /* declared in misc.c */
		  extern char *wkday[];   /* declared in misc.c */

		  sscanf(s,"%lx",&theirtrx);

		  trxdiff = theirtrx - ourtrx;
		  t = localtime((time_t *) &theirtrx);
		  message(2,"=Remote time: %s %d %s %d %02d:%02d:%02d (diff %c%02ld:%02ld:%02ld)",
			    wkday[t->tm_wday],
			    t->tm_mday, mon[t->tm_mon], t->tm_year + 1900,
			    t->tm_hour, t->tm_min, t->tm_sec,
			    trxdiff > 0 ? '+' : (trxdiff < 0 ? '-' : '='),
			    labs(trxdiff / 3600),
			    labs((trxdiff / 60) % 60), labs(trxdiff % 60));
	       }

	       last_field = last_tok;
	    }
	    else if (!stricmp(s,"MOH#")) {	       /* Mail bytes waiting */
							/* Parse MOH# */
	       s = get_field(s1);			/* MOH figure */
	       if (s && *s) {
		  dword moh;

		  sscanf(s,"%lx",&moh);

		  message(2,"=Mail for us: %lu KB", (moh + 1023UL) / 1024UL);
	       }

	       last_field = last_tok;
	    }
	    else if (!stricmp(s,"XENREG")) {
	       s = get_field(s1);
	       if (s && *s)
		  message(2,"=Remote licensee: %s", s);

	       last_field = last_tok;
	    }
	    else if (!stricmp(s,"XENCHK1")) {
	       /* nothing, just swallow the thing */
	    }
	    else if (!stricmp(s,"FDREV")) {
	       /* nothing, just swallow the thing */
	    }
	    else if (loglevel==0)
	       message(2,"=Extra field %s: '%s'",s,s1);
	}

	if ((remote.options & EOPT_PRIMARY) && num_rem_adr > 1) {
	   message(2,"=Remote only wants mail for primary address");
	   num_rem_adr = 1;
	}

	if (!caller && md5_send[0])
	   message(2,"+Shared secret password exchange");

	if (!caller)
	   ourbbs.password[0] = '\0';
	mismatch = false;
	for (i = 0; i < num_rem_adr; ) {
	    s = s1 = get_passw(rem_adr[i].zone,rem_adr[i].net,rem_adr[i].node,rem_adr[i].point);

	    if (md5_send[0]) {	/* did the other side support shared secret? */
	       MD5 md5;

	       md5_init(&md5);
	       md5_update(&md5,(byte *) md5_recv,(word) strlen(md5_recv));
	       strcpy(buf,s1);
	       strupr(buf);
	       md5_update(&md5,(byte *) buf,(word) strlen(buf));
	       sprint(buf,"<%s>",md5_result(&md5));
	       s = buf;
	    }

	    if (!s1[0] || !stricmp(s,remote.password)) {
	       if (s1[0] && !ourbbs.password[0] && i) {
		  /* make first passworded address (if exists) first of list */
		  j = rem_adr[0].zone;	   rem_adr[0].zone     = rem_adr[i].zone;     rem_adr[i].zone	  = j;
		  j = rem_adr[0].net;	   rem_adr[0].net      = rem_adr[i].net;      rem_adr[i].net	  = j;
		  j = rem_adr[0].node;	   rem_adr[0].node     = rem_adr[i].node;     rem_adr[i].node	  = j;
		  j = rem_adr[0].point;    rem_adr[0].point    = rem_adr[i].point;    rem_adr[i].point	  = j;
		  j = rem_adr[0].pointnet; rem_adr[0].pointnet = rem_adr[i].pointnet; rem_adr[i].pointnet = j;
	       }
	       i++;
	    }
	    else {
	       mismatch = true;
	       if (md5_send[0])
		  message(6,"!Password mismatch for %d:%d/%d%s%s",
			    rem_adr[i].zone, rem_adr[i].net, rem_adr[i].node,
			    pointnr(rem_adr[i].point), zonedomabbrev(rem_adr[i].zone));
	       else
		  message(6,"!Password mismatch '%s' <> '%s' for %d:%d/%d%s%s",
			    s1, remote.password,
			    rem_adr[i].zone, rem_adr[i].net, rem_adr[i].node,
			    pointnr(rem_adr[i].point), zonedomabbrev(rem_adr[i].zone));

	       num_rem_adr--;
	       for (j = i; j < num_rem_adr; j++) {
		   rem_adr[j].zone     = rem_adr[j + 1].zone;
		   rem_adr[j].net      = rem_adr[j + 1].net;
		   rem_adr[j].node     = rem_adr[j + 1].node;
		   rem_adr[j].point    = rem_adr[j + 1].point;
		   rem_adr[j].pointnet = rem_adr[j + 1].pointnet;
	       }
	    }

	    if (!caller && s1[0] && !ourbbs.password[0])
	       strcpy(ourbbs.password,s1);
	}

	if (num_rem_adr) {
	   sprint(buf,"%d:%d/%d%s%s",
		      rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		      pointnr(rem_adr[0].point),zonedomabbrev(rem_adr[0].zone));
	   last_set(caller ? &last_out : &last_in, buf);
	   if (!caller && callerid[0])
	      write_clip(remote.sysop,city,buf,remote.name);
	   show_stats();

	   if (!caller) {
	      set_alias(rem_adr[0].zone,rem_adr[0].net,rem_adr[0].node,rem_adr[0].point);
	      statusmsg("Incoming call from %s",buf);
	   }
	   win_xyprint(mailer_win,2,1,"Connected to %d:%d/%d%s",
		       rem_adr[0].zone, rem_adr[0].net, rem_adr[0].node,
		       pointnr(rem_adr[0].point));
	   win_print(mailer_win," as %d:%d/%d%s",
		     ourbbs.zone, ourbbs.net, ourbbs.node,
		     pointnr(ourbbs.point));
	   win_clreol(mailer_win);
	}

	if (ourbbs.password[0]) {
	   if (/*caller ||*/ remote.password[0]) {
	      message(2,"+Password protected session");
	      inbound = safepath;
	   }
	   else {
	      message(2,"-Remote presented no password");
	      mismatch = true;
	      if (caller && (ourbbsflags & NO_PWFORCE))
		 inbound = safepath;
	   }
	}
	else if (remote.password[0]) {
	   if (md5_send[0]) {	/* did the other side support shared secret? */
	      MD5 md5;

	      md5_init(&md5);		       /* Run through empty password */
	      md5_update(&md5,(byte *) md5_recv,(word) strlen(md5_recv));
	      sprint(buf,"<%s>",md5_result(&md5));
	      if (stricmp(buf,remote.password))     /* Different from empty? */
		 message(2,"=Remote proposes password");
	   }
	   else
	      message(2,"=Remote proposes password '%s'",remote.password);
	}

	if ((!caller || !(ourbbsflags & NO_PWFORCE)) &&
	    (mismatch || !num_rem_adr)) {
	   if (mismatch) message(6,"!Password security violation");
	   else 	 message(6,"!Remote presented no valid addresses");

	   return (false);
	}

	if (timesync && theirtrx && num_rem_adr) {
	   TIMESYNC *dp;
	   long trxdiff, absdiff;
	   boolean found = false;

	   for (dp = timesync; dp; dp = dp->next) {
	       for (i = 0; i < num_rem_adr; i++) {
		   if (dp->zone == rem_adr[i].zone && dp->net	== rem_adr[i].net &&
		       dp->node == rem_adr[i].node && dp->point == rem_adr[i].point) {
		      found = true;
		      break;
		   }
	       }
	       if (found) break;
	   }

	   if (found) {
	      theirtrx -= dp->timediff;
	      trxdiff = theirtrx - ourtrx;
	      absdiff = labs(trxdiff);
	      if ((absdiff > 5L && (!dp->maxdiff || absdiff <= dp->maxdiff)) ||
		  (dp->dstchange &&
		   (!dp->maxdiff ||
		    (absdiff - 3600L) <= dp->maxdiff ||
		    (absdiff + 3600L) <= dp->maxdiff))) {
		 FILE *fp;
		 char buffer[128];

		 sprint(buffer,"%sXENIA%u.SNC",homepath,task);
		 if ((fp = fopen(buffer,"wt")) != NULL) {
		    fprint(fp,"%08lx %c%ld %d %s %d:%d/%d%s%s\n",
			      ourtrx,
			      trxdiff >= 0 ? '+' : '-',
			      absdiff,
			      (absdiff > dp->maxdiff),
			      flagdir,
			      dp->zone, dp->net, dp->node,
			      pointnr(dp->point), zonedomabbrev(dp->zone));
		    fclose(fp);

		    message(2,"@TimeSync %c%ld:%02ld min. (%d:%d/%d%s%s)",
			      trxdiff >= 0 ? '+' : '-',
			      absdiff / 60L, absdiff % 60L,
			      dp->zone, dp->net, dp->node,
			      pointnr(dp->point), zonedomabbrev(dp->zone));
		 }
	      }
	      else {
		 message(2,"@TimeMark %c%ld:%02ld min. (%d:%d/%d%s%s)",
			   trxdiff > 0 ? '+' : (trxdiff < 0 ? '-' : '='),
			   absdiff / 60L, absdiff % 60L,
			   dp->zone, dp->net, dp->node,
			   pointnr(dp->point), zonedomabbrev(dp->zone));
	      }
	   }
	}

	return (true);
}


/* ------------------------------------------------------------------------- */
/* Read the actual packet and its crc
   emsibuf excludes the **, i.e. it is "EMSI_DAT<len16>"
*/
static int get_emsi_dat (char *emsibuf)
{
	byte *dat, *ptr;
	int   length, count;
	word  crc, check;
	long  t1, t2;
	int   c;

	/* Get the packet length */

	if (sscanf(&emsibuf[8],"%04x",&length) != 1)	   /* read hex-value */
	   return (EMSI_BADHEX);
#if DEBUG
	/*message(0,">EMSI_DAT<len16>=%04x (%d)",(word) length,length);*/
	{ FILE *fp;
	  char buf[PATHLEN];

	  sprint(buf,"%sEMSI_DAT.%u",homepath,task);
	  if ((fp = sfopen(buf,"ab",DENY_WRITE)) != NULL) {
	     fprint(fp,"\r\nReceive EMSI_DAT<len16>=%04x (%d):\r\n",(word) length,length);
	     fclose(fp);
	  }
	}
#endif
	if (length > MAX_DATLEN)		      /* Assume >8K is silly */
	   return (EMSI_BADLEN);

	dat = (byte *) malloc(length + 16);
	if (!dat) {
	   message(6,"!Can't allocate buffer for EMSI_DAT pkt");
	   return (EMSI_ABORT);
	}

	crc = 0;
	for (count = 0; count < 8 + 4; count++) 	  /* EMSI_DAT<len16> */
	    crc = crc_update(crc,emsibuf[count]);

	/* Read in all the data - within a reasonable amount of time... */
	t1 = timerset((word) (((length / (cur_speed / 10)) + 10) * 10));

	count = length;
	ptr = dat;
	while (--count >= 0) {
	      if (!com_scan()) {
		 t2 = timerset(80);
		 do {
		    if (keyabort()) {
		       free(dat);
		       return (EMSI_SYSABORT);
		    }
		    if (!carrier()) {
		       free(dat);
		       return (EMSI_CARRIER);
		    }
		    if (timeup(t2) || timeup(t1)) {
		       free(dat);
		       return (EMSI_TIMEOUT);
		    }
		    win_timeslice();
		 } while (!com_scan());
	      }

	      c = com_getbyte() & 0x7f;
	      crc = crc_update(crc,(word) c);
	      *ptr++ = c;
	}

	*ptr = '\0';
	crc = crc_finish(crc);

	/* Get the checksum */

	count = 4;
	check = 0;
	while (--count >= 0) {
	      if (!com_scan()) {
		 t2 = timerset(80);
		 do {
		    if (keyabort()) {
		       free(dat);
		       return (EMSI_SYSABORT);
		    }
		    if (!carrier()) {
		       free(dat);
		       return (EMSI_CARRIER);
		    }
		    if (timeup(t2) || timeup(t1)) {
		       free(dat);
		       return (EMSI_TIMEOUT);
		    }
		    win_timeslice();
		 } while (!com_scan());
	      }

	      c = byte_to_hex(com_getbyte() & 0x7f);
	      if (c & ~0x0f) {
		 free(dat);
		 return (EMSI_BADHEX);
	      }
	      check = (word) ((check << 4) | c);
	}

#if DEBUG
	/* Dump the data into a file for perusal */
	{ FILE *fp;
	  char buf[PATHLEN];

	  sprint(buf,"%sEMSI_DAT.%u",homepath,task);
	  if ((fp = sfopen(buf,"ab",DENY_WRITE)) != NULL) {
	     fwrite(dat,length,1,fp);
	     fclose(fp);
	  }
	}
#endif

	if (check != crc) {
	   free(dat);
	   return (EMSI_BADCRC);
	}

	/* Acknowledge it */

	put_emsi(EMSI_ACK,true);
	put_emsi(EMSI_ACK,true);

	/* Process it */

	if (!process_dat((char *) dat)) {
	   free(dat);
	   return (EMSI_ABORT);
	}

	free(dat);
	return (EMSI_SUCCESS);
}


/* ------------------------------------------------------------------------- */
/* add character to buffer with escaping */
static char *add_char (char c, char *s)
{
	while (*s)
	      s++;	/* Get to end of string */
	if (c == '\\' || c == '}' || c == ']') {
	   *s++ = c;
	   *s++ = c;
	}
	else if(isprint(c))
	   *s++ = c;
	else {
	   *s++ = '\\';
	   *s++ = hex[(c >> 4) & 0x0f];
	   *s++ = hex[ c       & 0x0f];
	}

	*s = '\0';
	return (s);
}

/* ------------------------------------------------------------------------- */
/* add's character without any escaping */
static char *add_field (char c, char *s)
{
	while (*s)
	      s++;
	*s++ = c;

	*s = '\0';
	return (s);
}

/* ------------------------------------------------------------------------- */
/* Add string to buffer with escaping */
static char *add_str (char *str, char *s)
{
	while (*str)
	      s = add_char(*str++,s);

	return (s);
}


/* ------------------------------------------------------------------------- */
boolean send_emsi (void)
{
	int   i;
	int   length;
	word  crc;
	int   tries;
	long  t1, t2;
	char  emsibuf[EMSI_IDLEN + 1];
	char  tmpbuf[40];
	char *buffer;
	word *crc16tab;
	char *p;
	OUTBLIST *op;
	dword moh;

	if (caller)
	   ourtrx = time(NULL);

	moh = 0UL;
	for (op = outblist; op; op = op->next) {
	    if (caller) {
	       if (op->zone == remote.zone && op->net	== remote.net &&
		   op->node == remote.node && op->point == remote.point) {
		  moh += op->outbytes;
		  break;
	       }
	    }
	    else {
	       for (i = 0; i < num_rem_adr; i++) {
		   if (op->zone == rem_adr[i].zone && op->net	== rem_adr[i].net &&
		       op->node == rem_adr[i].node && op->point == rem_adr[i].point) {
		      moh += op->outbytes + op->waitbytes;
		      break;
		   }
	       }
	    }
	}

	/* Create the Data */

	buffer	 = (char *) malloc(MAX_DATLEN);
	crc16tab = (word *) malloc(CRC_TABSIZE * ((int) sizeof (word)));
	if (!buffer) {					   /* Out of memory! */
	   message(6,"!Can't allocate buffer for EMSI_DAT pkt");
	   return (false);
	}

	p = buffer;
	strcpy(buffer,"**EMSI_DAT0000{EMSI}");	      /* DAT+len+Fingerprint */

	crc16init(crc16tab,CRC16POLY);		      /* for XENCHK1 */

	/* System addresses... current alias goes first */

	sprint(tmpbuf,"%d:%d/%d%s%s", ourbbs.zone, ourbbs.net, ourbbs.node,
				      pointnr(ourbbs.point), zonedomname(ourbbs.zone));
	p = add_field('{',p);
	p = add_str(tmpbuf,p);
	for (i = 0; i < emsi_adr; i++) {
	    if (aka[i].zone != ourbbs.zone || aka[i].net != ourbbs.net ||
		aka[i].node != ourbbs.node || aka[i].point != ourbbs.point) {
	       p = add_char(' ',p);
	       sprint(tmpbuf,"%d:%d/%d%s%s", aka[i].zone, aka[i].net, aka[i].node,
					     pointnr(aka[i].point), zonedomname(aka[i].zone));
	       p = add_str(tmpbuf,p);
	    }
	}
	p = add_field('}',p);

	/* Password */

	p = add_field('{',p);
	if (md5_send[0]) {	/* did the other side support shared secret? */
	   MD5 md5;

	   if (caller)
	      message(2,"+Shared secret password exchange");

	   md5_init(&md5);
	   md5_update(&md5,(byte *) md5_send,(word) strlen(md5_send));
	   strcpy(tmpbuf,ourbbs.password);
	   strupr(tmpbuf);
	   md5_update(&md5,(byte *) tmpbuf,(word) strlen(tmpbuf));
	   p = add_char('<',p);
	   p = add_str(md5_result(&md5),p);
	   p = add_char('>',p);
	}
	else
	   p = add_str(ourbbs.password,p);
	p = add_field('}',p);

	/* Link codes */

	p = add_field('{',p);
	p = add_str("8N1",p);
	if (caller)
	   p = add_str(",PUA",p);			 /* Pick up all mail */
	if ((ourbbs.options & EOPT_REQFIRST) &&
	    (caller || (remote.options & EOPT_REQFIRST)))
	   p = add_str(",RH1",p);	      /* Only REQ in 1st Hydra batch */
	p = add_field('}',p);

	/* Compatibility codes */

	p = add_field('{',p);

	if (caller) {		 /* Send everything we can do */
	   if (ourbbs.capabilities & DOES_HYDRA)
	      p = add_str("HYD,",p);
	   if (ourbbs.capabilities & EMSI_DZA)
	      p = add_str("DZA,",p);
	   if (ourbbs.capabilities & ZED_ZAPPER)
	      p = add_str("ZAP,",p);
	   p = add_str("ZMO,",p);
	   if (ourbbs.capabilities & EMSI_SLK)
	      p = add_str("SLK,",p);
	   if ((ourbbsflags & NO_REQ_ON) || (remote.options & EOPT_BADAPPLE))
	      p = add_str("NRQ,",p);
	}
	else {			 /* Pick best protocol */
	   if ((remote.capabilities & DOES_HYDRA) && (ourbbs.capabilities & DOES_HYDRA))
	      p = add_str("HYD,",p);
	   else if ((remote.capabilities & EMSI_DZA) && (ourbbs.capabilities & EMSI_DZA)) {
	      p = add_str("DZA,",p);
	      remote.capabilities &= ~(ZED_ZAPPER | ZED_ZIPPER | EMSI_SLK);
	   }
	   else if ((remote.capabilities & ZED_ZAPPER) && (ourbbs.capabilities & ZED_ZAPPER)) {
	      p = add_str("ZAP,",p);
	      remote.capabilities &= ~(EMSI_DZA | ZED_ZIPPER | EMSI_SLK);
	   }
	   else if ((remote.capabilities & ZED_ZIPPER) &&
		    ((ourbbs.capabilities & ZED_ZIPPER) ||
		     !(remote.capabilities & EMSI_SLK) || !(ourbbs.capabilities & EMSI_SLK))) {
	      p = add_str("ZMO,",p);
	      remote.capabilities &= ~(EMSI_DZA | ZED_ZAPPER | EMSI_SLK);
	   }
	   else if ((remote.capabilities & EMSI_SLK) && (ourbbs.capabilities & EMSI_SLK)) {
	      p = add_str("SLK,",p);
	      remote.capabilities &= ~(EMSI_DZA | ZED_ZIPPER | ZED_ZAPPER);
	   }
	   else
	      p = add_str("NCP,",p);
	}
#if OS2 || WINNT
	p = add_str("ARC,XMA",p);
#else
	p = add_str("ARC,XMA,FNC",p);
#endif
	p = add_field('}',p);

	/* mailer_product_code */

	p = add_field('{',p);
	sprint(tmpbuf,"%02x",(word) isXENIA);
	p = add_str(tmpbuf,p);
	p = add_field('}',p);

	/* mailer_name */

	p = add_field('{',p);
	sprint(tmpbuf,"Xenia%s %s", XENIA_SFX, /*xenpoint_ver ? "Point" :*/ "Mailer");
	p = add_str(tmpbuf,p);
	p = add_field('}',p);

	/* mailer_version */

	p = add_field('{',p);
#if 0
	if (xenpoint_ver)
	   strcpy(tmpbuf,xenpoint_ver);
	else
#endif
	   strncpy(tmpbuf,VERSION,sizeof(VERSION));
	p = add_str(tmpbuf,p);
	p = add_field('}',p);

	/* mailer_serial */

	p = add_field('{',p);
	strcpy(tmpbuf,"OpenSource");
	p = add_str(tmpbuf,p);
	p = add_field('}',p);

	/* ----------------------------------------------------------------- */

	p = add_field('{',p);
	p = add_str("IDENT",p); 		 /* Do the IDENT extra field */
	p = add_field('}',p);

	p = add_field('{',p);

	p = add_field('[',p);				      /* System name */
	p = add_str(emsi_ident.name,p);
	p = add_field(']',p);

	p = add_field('[',p);					     /* City */
	p = add_str(emsi_ident.city,p);
	p = add_field(']',p);

	p = add_field('[',p);				    /* Operator Name */
	p = add_str(emsi_ident.sysop,p);
	p = add_field(']',p);

	p = add_field('[',p);				     /* Phone number */
	p = add_str(emsi_ident.phone,p);
	p = add_field(']',p);

	p = add_field('[',p);					    /* Speed */
	p = add_str(emsi_ident.speed,p);
	p = add_field(']',p);

	p = add_field('[',p);					    /* Flags */
	p = add_str(emsi_ident.flags,p);
	p = add_field(']',p);

	p = add_field('}',p);

	/* ----------------------------------------------------------------- */

	if (md5_send[0]) {
	   p = add_field('{',p);
	   p = add_str("EMD5",p);		  /* Do the EMD5 extra field */
	   p = add_field('}',p);

	   p = add_field('{',p);

	   p = add_field('[',p);		    /* Our MD5 unique string */
	   p = add_str(md5_recv,p);
	   p = add_field(']',p);

	   p = add_field('}',p);
	}

	/* ----------------------------------------------------------------- */

	p = add_field('{',p);
	p = add_str("TRX#",p);			  /* Do the TRX# extra field */
	p = add_field('}',p);

	p = add_field('{',p);

	p = add_field('[',p);			       /* Transaction number */
	sprint(tmpbuf,"%08lx",ourtrx);
	strupr(tmpbuf);
	p = add_str(tmpbuf,p);
	p = add_field(']',p);

	p = add_field('}',p);

	/* ----------------------------------------------------------------- */

	if (moh) {
	   message(2,"=Mail for them: %lu KB", (moh + 1023UL) / 1024UL);

	   p = add_field('{',p);
	   p = add_str("MOH#",p);		  /* Do the MOH# extra field */
	   p = add_field('}',p);

	   p = add_field('{',p);

	   p = add_field('[',p);		       /* Transaction number */
	   sprint(tmpbuf,"%lx",moh);
	   strupr(tmpbuf);
	   p = add_str(tmpbuf,p);
	   p = add_field(']',p);

	   p = add_field('}',p);
	}

	/* ----------------------------------------------------------------- */

	/* Length of packet [not **EMSI_DATxxxx] */
	length = strlen(buffer) - 14;
	buffer[10] = hex[(length >> 12) & 0x0f];
	buffer[11] = hex[(length >>  8) & 0x0f];
	buffer[12] = hex[(length >>  4) & 0x0f];
	buffer[13] = hex[ length	& 0x0f];
	length += 14;

	/* Calculate CRC */
	for (crc = 0, p = &buffer[2]; *p; crc = crc_update(crc,*p++));
	crc = crc_finish(crc);

#if DEBUG
	{ FILE *fp;
	  char buf[PATHLEN];

	  sprint(buf,"%sEMSI_DAT.%u",homepath,task);
	  if ((fp = sfopen(buf,"ab",DENY_WRITE)) != NULL) {
	     fprint(fp,"\r\nTransmit EMSI_DAT:\r\n");
	     fwrite(buffer,length,1,fp);
	     fclose(fp);
	  }
	}
#endif

	/* ----------------------------------------------------------------- */
	filewincls("EMSI Transmit",0);
	message(1,"+EMSI Transmit");

	emsibuf[0] = '\0';

	tries = 0;						   /* step 1 */
	t1 = timerset(600);

	do {							   /* step 2 */
	   if (++tries > 6) {		    /* Terminate, and report failure */
	      p = msg_manyerrors;
	      goto abort;
	   }

							/* Transmit EMSI_DAT */
	   com_putblock((byte *) buffer,(word) strlen(buffer));
	   com_purge();
	   put_hex(crc);
	   com_bufbyte('\r');
	   /*com_bufbyte(XON);*/
	   com_bufflush();

	   t2 = timerset(200);					   /* step 3 */

	   do { 						   /* step 4 */
	      if (keyabort()) { 			      /* Sysop abort */
		 p = msg_oabort;
		 goto abort;
	      }
	      if (!carrier()) { 			     /* Carrier lost */
		 p = "Carrier lost (password failure?)";
		 goto abort;
	      }
	      if (timeup(t2)) { 	   /* If timer2 expired goto step 2 */
		 filemsg("Retry %d: %s",tries,msg_timeout);
		 message(0,"-EMSI SEND Retry %d: %s",tries,msg_timeout);
		 break;
	      }

	      if (com_scan()) {
		 byte c   = (byte) com_getbyte();
		 int  pkt = check_emsi(c,emsibuf);

		 if (pkt == EMSI_ACK) { 			   /* step 5 */
		    free(buffer);
		    free(crc16tab);
		    filemsg(msg_success);
		    message(1,"+EMSI%s SEND: %s",md5_send[0] ? "/MD5" : "",msg_success);
		    return (caller ? recv_emsi(false) : true);
		 }					  /* Any else (NAK)? */
		 else if (pkt >= 0 && pkt != EMSI_REQ && pkt != EMSI_MD5) {
		    if (!caller && pkt == EMSI_DAT)
		       put_emsi(EMSI_ACK,true);
		    filemsg("Retry %d: %s",tries,&emsi_pkt[pkt][7]); /* goto step 2 */
		    message(0,"-EMSI SEND Retry %d: %s",tries,&emsi_pkt[pkt][7]);
		    break;
		 }
						 /* EMSI_REQ, stay in step 4 */
	      }
	      else
		 win_timeslice();
	   } while (!timeup(t1)); /* Wait for EMSI sequence until t1 expires */
	} while (!timeup(t1));

	p = msg_fatimeout;

abort:	free(buffer);
	free(crc16tab);
	filemsg(p);
	message(2,"-EMSI%s SEND: %s",md5_send[0] ? "/MD5" : "",p);
	return (false);
}


/* ------------------------------------------------------------------------- */
boolean recv_emsi (boolean send_too)
{
	char  emsibuf[EMSI_IDLEN + 1];
	char *p;
	int   tries;
	long  t1, t2;

	if (send_too)
	   ourtrx = time(NULL);

	filewincls("EMSI Receive",0);
	message(1,"+EMSI Receive");

	emsibuf[0] = '\0';

	tries = 0;						   /* step 1 */
	t1 = timerset(200);
	t2 = timerset(600);

	do {							   /* step 2 */
	   if (++tries > 6) {		    /* Terminate, and report failure */
	      p = msg_manyerrors;
	      goto abort;
	   }

	   if (!caller) {
	      com_dump();	/* Binkley MD5 bullshit hack */
	      if (!com_scan())	/* Binkley MD5 bullshit hack */
		 put_emsi(EMSI_MD5,true);
	      put_emsi(EMSI_REQ,true);
	   }
	   if (tries > 1)
	      put_emsi(EMSI_NAK,true);
	   else
	      goto step4;

step3:	   t1 = timerset(200);					   /* step 3 */

step4:	   do { 						   /* step 4 */
	      if (keyabort()) { 			      /* Sysop abort */
		 p = msg_oabort;
		 goto abort;
	      }
	      if (!carrier()) { 			     /* Carrier lost */
		 p = caller ? "Carrier lost (password failure?)" : msg_carrier;
		 goto abort;
	      }
	      if (timeup(t1)) { 	  /* If timer1 expired goto step 2 */
		 filemsg("Retry %d: %s",tries,msg_timeout);
		 message(0,"-EMSI RECV Retry %d: %s",tries,msg_timeout);
		 break;
	      }

	      if (com_scan()) {
		 byte c   = (byte) com_getbyte();
		 int  pkt = check_emsi(c,emsibuf);

		 if (pkt == EMSI_HBT)				/* Heartbeat */
		    goto step3;
		 else if (pkt == EMSI_M5R)
		    put_emsi(EMSI_MD5,true);
		 else if (pkt == EMSI_DAT) {			/* Data pkt  */
		    switch (get_emsi_dat(&emsibuf[2])) {
			   case EMSI_SUCCESS:
				filemsg(msg_success);
				message(1,"+EMSI%s RECV: %s",md5_send[0] ? "/MD5" : "",msg_success);
				return (send_too ? send_emsi() : true);

			   case EMSI_ABORT:
				return (false);

			   case EMSI_BADHEX:
				p = "Bad hex sequence";
				break;

			   case EMSI_BADLEN:
				p = "Bad data length";
				break;

			   case EMSI_BADCRC:
				p = msg_datacrc;
				break;

			   case EMSI_TIMEOUT:
				p = msg_timeout;
				break;

			   case EMSI_CARRIER:
				p = msg_carrier;
				goto abort;

			   case EMSI_SYSABORT:
				p = msg_oabort;
				goto abort;
		    }
		    filemsg("Retry %d: %s",tries,p);
		    message(0,"-EMSI RECV Retry %d: %s",tries,p);
		    break;				      /* Goto step 2 */
		 }
	      }
	      else
		 win_timeslice();
	   } while (!timeup(t2)); /* Wait for EMSI sequence until t2 expires */
	} while (!timeup(t2));

	p = msg_fatimeout;

abort:	filemsg(p);
	message(2,"-EMSI%s RECV: %s",md5_send[0] ? "/MD5" : "",p);
	return (false);
}


/* ------------------------------------------------------------------------- */
/* Read the actual packet and its crc
   emsibuf excludes the **, i.e. it is "EMSI_ISI<len16>"
*/
static int get_iemsi_isi (char *emsibuf, dword *crc32tab)
{
	byte *dat, *ptr;
	int   length, count;
	dword crc, check;
	long  t1, t2;
	char *s;
	int   c;

	/* Get the packet length */

	if (sscanf(&emsibuf[8],"%04x",&length) != 1)	   /* read hex-value */
	   return (EMSI_BADHEX);
	if (length > 2048)			      /* Assume >2K is silly */
	   return (EMSI_BADLEN);

	dat = (byte *) malloc(length + 16);
	if (!dat) {
	   message(6,"!Can't allocate buffer for EMSI_ISI pkt");
	   return (EMSI_ABORT);
	}

	crc = crc32block(crc32tab,CRC32INIT,(byte *) emsibuf,12);

	/* Read in all the data - within a reasonable amount of time... */
	t1 = timerset((word) (((length / (cur_speed / 10)) + 10) * 10));

	count = length;
	ptr = dat;
	while (--count >= 0) {
	      if (!com_scan()) {
		 t2 = timerset(80);
		 do {
		    if (keyabort()) {
		       free(dat);
		       return (EMSI_SYSABORT);
		    }
		    if (!carrier()) {
		       free(dat);
		       return (EMSI_CARRIER);
		    }
		    if (timeup(t2) || timeup(t1)) {
		       free(dat);
		       return (EMSI_TIMEOUT);
		    }
		    win_timeslice();
		 } while (!com_scan());
	      }

	      c = com_getbyte() & 0x7f;
	      *ptr++ = c;
	}

	*ptr = '\0';
	crc = crc32block(crc32tab,crc,(byte *) dat,(word) length);

	/* Get the checksum */

	count = 8;
	check = 0;
	while (--count >= 0) {
	      if (!com_scan()) {
		 t2 = timerset(80);
		 do {
		    if (keyabort()) {
		       free(dat);
		       return (EMSI_SYSABORT);
		    }
		    if (!carrier()) {
		       free(dat);
		       return (EMSI_CARRIER);
		    }
		    if (timeup(t2) || timeup(t1)) {
		       free(dat);
		       return (EMSI_TIMEOUT);
		    }
		    win_timeslice();
		 } while (!com_scan());
	      }

	      c = byte_to_hex(com_getbyte() & 0x7f);
	      if (c & ~0x0f) {
		 free(dat);
		 return (EMSI_BADHEX);
	      }
	      check = (check << 4) | c;
	}

#if IDEBUG
	/* Dump the data into a file for perusal */
	{
		FILE *fp;

		if ((fp = sfopen("IEMSI.ISI","ab",DENY_WRITE)) != NULL) {
		   fwrite(emsibuf,12,1,fp);
		   fwrite(dat,length,1,fp);
		   fprint(fp,"%08lx\n",check);
		   fclose(fp);
		}
	}
#endif

	if (check != crc) {
	   free(dat);
	   return (EMSI_BADCRC);
	}

	/* Acknowledge it */

	put_emsi(EMSI_ACK,true);
	put_emsi(EMSI_ACK,true);

	/* Process it */

	s = get_field((char *) dat);
	if (s && *s) message(2,"=ID: %s",s);

	s = get_field(NULL);
	if (s && *s) message(2,"=Name: %s",s);

	s = get_field(NULL);
	if (s && *s) message(2,"=Location: %s",s);

	s = get_field(NULL);
	if (s && *s) message(2,"=Operator: %s",s);

	s = get_field(NULL);
	/* local time in long hex since 1970 */

	s = get_field(NULL);
	if (s && *s) message(2,"=Notice: %s",s);

	s = get_field(NULL);
	/* Wait byte */

	s = get_field(NULL);
	if (s && *s) message(2,"=Capabilities: %s",s);

	free(dat);
	return (EMSI_SUCCESS);
}


/* ------------------------------------------------------------------------- */
void client_iemsi (char *user_password, byte emu)
{
	int   length;
	dword crc32, *crc32tab;
	int   tries;
	long  t1, t2;
	char  emsibuf[EMSI_IDLEN + 1];
	char  tmpbuf[40];
	char *buffer;
	char *p;
	boolean sent_ok = false;

	message(2,"*Attempting IEMSI login");

	buffer = (char *) malloc(MAX_DATLEN);
	crc32tab = (dword *) malloc(CRC_TABSIZE * ((int) sizeof (dword)));
	if (!buffer || !crc32tab) {			   /* Out of memory! */
	   message(6,"!Can't allocate buffers for EMSI_ICI pkt");
	   if (buffer)	 free(buffer);
	   if (crc32tab) free(crc32tab);
	   return;
	}
	crc32init(crc32tab,CRC32POLY);

	p = buffer;
	strcpy(buffer,"**EMSI_ICI0000");			  /* DAT+len */

	p = add_field('{',p);
	if (iemsi_profile.name) p = add_str(iemsi_profile.name,p);
	p = add_field('}',p);

	p = add_field('{',p);
	if (iemsi_profile.alias) p = add_str(iemsi_profile.alias,p);
	p = add_field('}',p);

	p = add_field('{',p);
	if (iemsi_profile.location) p = add_str(iemsi_profile.location,p);
	p = add_field('}',p);

	p = add_field('{',p);
	if (iemsi_profile.data) p = add_str(iemsi_profile.data,p);
	p = add_field('}',p);

	p = add_field('{',p);
	if (iemsi_profile.voice) p = add_str(iemsi_profile.voice,p);
	p = add_field('}',p);

	p = add_field('{',p);
	if (user_password) p = add_str(user_password,p);
	p = add_field('}',p);

	p = add_field('{',p);
	if (iemsi_profile.birthdate) {
	   sprint(tmpbuf,"%08lx",iemsi_profile.birthdate);
	   strupr(tmpbuf);
	   p = add_str(tmpbuf,p);
	}
	p = add_field('}',p);

	p = add_field('{',p);		/* emu, rows(lines), columns, NULs */
	sprint(tmpbuf,"%s,%u,%u,0",
	       emu == CON_AVATAR ? "AVT0" : (emu == CON_ANSI ? "ANSI" : "VT52"),
	       (word) win_maxver - 1, (word) win_maxhor);
	p = add_str(tmpbuf,p);
	p = add_field('}',p);

	p = add_field('{',p);
	p = add_str("HYD,ZAP,ZMO,SLK",p);	/* Add DZA !! */
	p = add_field('}',p);

	p = add_field('{',p);
	p = add_str("CHT,TAB,ASCII8",p);	/* Add MNU !! */
	p = add_field('}',p);

	p = add_field('{',p);
	p = add_str("HOT,CLR,MORE,FSED",p);	/* NEWS,MAIL,FILE */
	p = add_field('}',p);

	p = add_field('{',p);
#if 0
	if (xenpoint_ver)
	   sprint(tmpbuf,"Xenia Point,%s %s,%s",
			 xenpoint_ver, XENIA_OS, xenpoint_reg);
	else
#endif
	   sprint(tmpbuf,"%s,%s %s,%s",
			 PRGNAME, VERSION, XENIA_OS, "OpenSource");
	p = add_str(tmpbuf,p);
	p = add_field('}',p);

	p = add_field('{',p);
	/* XlatTabl */
	p = add_field('}',p);

	/* Length of packet [not **EMSI_ICIxxxx] */
	length = strlen(buffer) - 14;
	buffer[10] = hex[(length >> 12) & 0x0f];
	buffer[11] = hex[(length >>  8) & 0x0f];
	buffer[12] = hex[(length >>  4) & 0x0f];
	buffer[13] = hex[ length	& 0x0f];
	length += 14;

	/* Calculate CRC */
	p = &buffer[2];
	crc32 = crc32block(crc32tab,CRC32INIT,(byte *) p,(word) strlen(p));

#if IDEBUG
	{
		FILE *fp;
		if ((fp = sfopen("IEMSI.ICI","ab",DENY_WRITE)) != NULL) {
		   fwrite(buffer,length,1,fp);
		   fprint(fp,"%08lx\n",crc32);
		   fclose(fp);
		}
	}
#endif

	/* ----------------------------------------------------------------- */
	tries = 0;						   /* step 1 */
	t1 = timerset(600);

	do {							   /* step 2 */
	   if (++tries > 3) {		    /* Terminate, and report failure */
	      p = msg_manyerrors;
	      goto abort;
	   }

	   if (!sent_ok) {				/* Transmit EMSI_ICI */
	      message(1,"+IEMSI Client Information Transmit");
	      emsibuf[0] = '\0';

	      com_putblock((byte *) buffer,(word) strlen(buffer));
	      com_purge();
	      put_hex((word) ((crc32 >> 16) & 0xffff));
	      put_hex((word) (crc32 & 0xffff));
	      com_bufbyte('\r');
	      com_bufflush();
	   }

	   t2 = timerset(200);					   /* step 5 */

	   do { 						   /* step 4 */
	      if (keyabort()) { 			      /* Sysop abort */
		 p = msg_oabort;
		 goto abort;
	      }
	      if (!carrier()) { 			     /* Carrier lost */
		 p = msg_carrier;
		 goto abort;
	      }
	      if (timeup(t2)) { 	   /* If timer2 expired goto step 2 */
		 message(0,"-IEMSI SEND Retry %d: %s",tries,msg_timeout);
		 sent_ok = false;
		 break;
	      }

	      if (com_scan()) {
		 byte c   = (byte) com_getbyte();
		 int  pkt = check_emsi(c,emsibuf);

		 if (pkt == EMSI_ISI) { 			   /* step 6 */
		    sent_ok = true;

		    message(1,"+IEMSI Server Information Receive");

		    switch (get_iemsi_isi(&emsibuf[2],crc32tab)) {
			   case EMSI_SUCCESS:
				free(buffer);
				free(crc32tab);
				message(2,"+IEMSI login succesful");
				return;

			   case EMSI_ABORT:
				p = NULL;
				goto abort;

			   case EMSI_BADHEX:
				p = "Bad hex sequence";
				break;

			   case EMSI_BADLEN:
				p = "Bad data length";
				break;

			   case EMSI_BADCRC:
				p = msg_datacrc;
				break;

			   case EMSI_TIMEOUT:
				p = msg_timeout;
				break;

			   case EMSI_CARRIER:
				p = msg_carrier;
				goto abort;

			   case EMSI_SYSABORT:
				p = msg_oabort;
				goto abort;
		    }
		    message(0,"-IEMSI RECV Retry %d: %s",tries,p);
		    com_purge();
		    put_emsi(EMSI_NAK,true);
		    break;				      /* Goto step 2 */
		 }
		 else if (pkt == EMSI_IIR) {
		    p = "Negotiation aborted";
		    goto abort;
		 }
		 else if (!sent_ok && pkt >= 0) {	  /* Any else (NAK)? */
		    message(0,"-IEMSI SEND Retry %d: %s",tries,&emsi_pkt[pkt][7]);
		    break;				      /* goto step 2 */
		 }
	      }
	      else
		 win_timeslice();
	   } while (!timeup(t1));     /* Wait for IEMSI seq until t1 expires */
	} while (!timeup(t1));

	p = msg_fatimeout;

abort:	com_dump();
	if (carrier())
	   put_emsi(EMSI_IIR,true);
	com_purge();
	free(buffer);
	free(crc32tab);
	if (p)
	   message(2,"-IEMSI login failed (%s)",p);
}


/* ------------------------------------------------------------------------- */
void emsi_md5make (void)
{
	struct dosdate_t d;
	struct dostime_t t;

	_dos_getdate(&d);
	_dos_gettime(&t);

	sprint(md5_recv,"<%02x.%04d%02d%02d%02d%02d%02d%02d-Xenia>",
			task,
			d.year, d.month, d.day,
			t.hour, t.minute, t.second, t.hsecond);
	md5_send[0] = '\0';
}


/* ------------------------------------------------------------------------- */
/* Read an MD5 packet and its crc
   emsibuf excludes the **, i.e. it is "EMSI_MD5<len16>"
*/
int emsi_md5get (char *emsibuf)
{
	byte  buffer[81];
	byte *ptr;
	int   length, count;
	word  crc, check;
	long  t1, t2;
	int   c;

	if (sscanf(&emsibuf[8],"%04x",&length) != 1)	   /* read hex-value */
	   return (EMSI_BADHEX);
#if DEBUG
	/*message(0,">EMSI_MD5<len16>=%04x (%d)",(word) length,length);*/
	{ FILE *fp;
	  char buf[PATHLEN];

	  sprint(buf,"%sEMSI_DAT.%u",homepath,task);
	  if ((fp = sfopen(buf,"ab",DENY_WRITE)) != NULL) {
	     fprint(fp,"\r\nReceive EMSI_MD5<len16>=%04x (%d):\r\n",(word) length,length);
	     fclose(fp);
	  }
	}
#endif
	if (length > 80)			  /* Maximum length 80 chars */
	   return (EMSI_BADLEN);

	crc = 0;
	for (count = 0; count < 8 + 4; count++) 	  /* EMSI_MD5<len16> */
	    crc = crc_update(crc,emsibuf[count]);

	/* Read in all the data - within a reasonable amount of time... */
	t1 = timerset((word) (((length / (cur_speed / 10)) + 10) * 10));

	count = length;
	ptr = buffer;
	while (--count >= 0) {
	      if (!com_scan()) {
		 t2 = timerset(80);
		 do {
		    if (!carrier())
		       return (EMSI_CARRIER);
		    if (timeup(t2) || timeup(t1))
		       return (EMSI_TIMEOUT);
		    win_timeslice();
		 } while (!com_scan());
	      }

	      c = com_getbyte() & 0x7f;
	      crc = crc_update(crc,(word) c);
	      *ptr++ = c;
	}

	*ptr = '\0';
	crc = crc_finish(crc);

	/* Get the checksum */

	count = 4;
	check = 0;
	while (--count >= 0) {
	      if (!com_scan()) {
		 t2 = timerset(80);
		 do {
		    if (!carrier())
		       return (EMSI_CARRIER);
		    if (timeup(t2) || timeup(t1))
		       return (EMSI_TIMEOUT);
		    win_timeslice();
		 } while (!com_scan());
	      }

	      c = byte_to_hex(com_getbyte() & 0x7f);
	      if (c & ~0x0f)
		 return (EMSI_BADHEX);
	      check = (word) ((check << 4) | c);
	}

#if DEBUG
	/* Dump the data into a file for perusal */
	{ FILE *fp;
	  char buf[PATHLEN];

	  sprint(buf,"%sEMSI_DAT.%u",homepath,task);
	  if ((fp = sfopen(buf,"ab",DENY_WRITE)) != NULL) {
	     fwrite(buf,length,1,fp);
	     fclose(fp);
	  }
	}
#endif

	if (check != crc)
	   return (EMSI_BADCRC);


	strcpy(md5_send,(char *) buffer);

	return (EMSI_SUCCESS);
}


/* ------------------------------------------------------------------------- */


/* end of emsi.c */
