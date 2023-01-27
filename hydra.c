/*=============================================================================

			      HydraCom Version 1.00

			 A sample implementation of the
		   HYDRA Bi-Directional File Transfer Protocol

			     HydraCom was written by
		   Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT
		  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED

		       The HYDRA protocol was designed by
		 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
	       Joaquim H. Homrighausen, Advanced Engineering sarl
		  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED


  Revision history:
  06 Sep 1991 - (AGL) First tryout
  .. ... .... - Internal development
  11 Jan 1993 - HydraCom version 1.00, Hydra revision 001 (01 Dec 1992)


  For complete details of the Hydra and HydraCom licensing restrictions,
  please refer to the license agreements which are published in their entirety
  in HYDRACOM.C and LICENSE.DOC, and also contained in the documentation file
  HYDRACOM.DOC

  Use of this file is subject to the restrictions contained in the Hydra and
  HydraCom licensing agreements. If you do not find the text of this agreement
  in any of the aforementioned files, or if you do not have these files, you
  should immediately contact LENTZ SOFTWARE-DEVELOPMENT and/or Advanced
  Engineering sarl at one of the addresses listed below. In no event should
  you proceed to use this file without having accepted the terms of the Hydra
  and HydraCom licensing agreements, or such other agreement as you are able
  to reach with LENTZ SOFTWARE-DEVELOMENT and Advanced Engineering sarl.


  Hydra protocol design and HydraCom driver:	     Hydra protocol design:
  Arjen G. Lentz				     Joaquim H. Homrighausen
  LENTZ SOFTWARE-DEVELOPMENT			     Advanced Engineering sarl
  Langegracht 7B				     8, am For
  3811 BT  Amersfoort				     L-5351 Oetrange
  The Netherlands				     Luxembourg
  FidoNet 2:283/512, AINEX-BBS +31-33-633916	     FidoNet 2:270/17
  arjen_lentz@f512.n283.z2.fidonet.org		     joho@ae.lu

  Please feel free to contact us at any time to share your comments about our
  software and/or licensing policies.

=============================================================================*/


#include "xenia.h"				  /* need 2types.h & hydra.h */


#define H_DEBUG 1


/* HYDRA Some stuff to aid readability of the source and prevent typos ----- */
#define h_crc16test(crc)   (((crc) == CRC16TEST) ? 1 : 0)	/*AGL:10mar93*/
#define h_crc32test(crc)   (((crc) == CRC32TEST) ? 1 : 0)	/*AGL:10mar93*/
#define h_uuenc(c)	   (((c) & 0x3f) + '!')
#define h_uudec(c)	   (((c) - '!') & 0x3f)
#define h_long1(buf)	   (*((long *) (buf)))
#define h_long2(buf)	   (*((long *) ((buf) + ((int) sizeof (long)))))
#define h_long3(buf)	   (*((long *) ((buf) + (2 * ((int) sizeof (long))))))
typedef long		       h_timer;
#define h_timer_get()	       (time(NULL))			/*AGL:15jul93*/
#define h_timer_set(t)	       (h_timer_get() + (t))		/*AGL:15jul93*/
#define h_timer_running(t)     (t != 0L)
#define h_timer_expired(t,now) ((now) > (t))			/*AGL:15jul93*/
#define h_timer_reset()        (0L)


/* HYDRA's memory ---------------------------------------------------------- */
static	boolean originator;			/* are we the orig side?     */
static	int	batchesdone;			/* No. HYDRA batches done    */
static	boolean hdxlink;			/* hdx link & not orig side  */
static	dword	options;			/* INIT options hydra_init() */
static	word	timeout;			/* general timeout in secs   */
static	char	abortstr[] = { 24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0 };
static	char   *hdxmsg	   = "Fallback to one-way xfer";
static	char   *pktprefix  = "";
static	char   *autostr    = "hydra\r";
static	word   *crc16tab;			/* CRC-16 table 	     */
static	dword  *crc32tab;			/* CRC-32 table 	     */

static	byte   *txbuf,	       *rxbuf;		/* packet buffers	     */
static	dword	txoptions,	rxoptions;	/* HYDRA options (INIT seq)  */
static	char	txpktprefix[H_PKTPREFIX + 1];	/* pkt prefix str they want  */
static	long	txwindow,	rxwindow;	/* window size (0=streaming) */
static	h_timer 		braindead;	/* braindead timer	     */
static	byte   *txbufin;			/* read data from disk here  */
static	byte	txlastc;			/* last byte put in txbuf    */
static	byte			rxdle;		/* count of received H_DLEs  */
static	byte			rxpktformat;	/* format of pkt receiving   */
static	byte		       *rxbufptr;	/* current position in rxbuf */
static	byte		       *rxbufmax;	/* highwatermark of rxbuf    */
static	char txfname[NAMELEN], rxfname[NAMELEN];/* fname of current files    */
static	char		       *rxpathname;	/* pointer to rx pathname    */
static	long	txftime,	rxftime;	/* file timestamp (UNIX)     */
static	long	txfsize,	rxfsize;	/* file length		     */
static	int	txfd,		rxfd;		/* file handles 	     */
static	word			rxpktlen;	/* length of last packet     */
static	word			rxblklen;	/* len of last good data blk */
static	byte	txstate,	rxstate;	/* xmit/recv states	     */
static	long	txpos,		rxpos;		/* current position in files */
static	word	txblklen;			/* length of last block sent */
static	word	txmaxblklen;			/* max block length allowed  */
static	long	txlastack;			/* last dataack received     */
static	long	txstart,	rxstart;	/* time we started this file */
static	long	txoffset,	rxoffset;	/* offset in file we begun   */
static	h_timer txtimer,	rxtimer;	/* retry timers 	     */
static	word	txretries,	rxretries;	/* retry counters	     */
static	long			rxlastsync;	/* filepos last sync retry   */
static	long	txsyncid,	rxsyncid;	/* id of last resync	     */
static	word	txgoodneeded;			/* to send before larger blk */
static	word	txgoodbytes;			/* no. sent at this blk size */

struct _h_flags {
	char  *str;
	dword  val;
};

static struct _h_flags h_flags[] = {
	{ "XON", HOPT_XONXOFF },
	{ "TLN", HOPT_TELENET },
	{ "CTL", HOPT_CTLCHRS },
	{ "HIC", HOPT_HIGHCTL },
	{ "HI8", HOPT_HIGHBIT },
	{ "BRK", HOPT_CANBRK  },
	{ "ASC", HOPT_CANASC  },
	{ "UUE", HOPT_CANUUE  },
	{ "C32", HOPT_CRC32   },
	{ "DEV", HOPT_DEVICE  },
	{ "FPT", HOPT_FPT     },
	{ NULL , 0x0L	      }
};


/*---------------------------------------------------------------------------*/
#define CHAT_TIMEOUT 60
#define CHATLEN 256
static word	chatfill;
static long	chattimer,
		lasttimer;
static WIN_IDX	chat_win;
static byte	chat_remote_x,
		chat_remote_y;
static char *chatstart = "\007\007 * Chat mode start\r\n";
static char *chatend   = "\007\007\r\n * Chat mode end\r\n";
static char *chattime  = "\007\007\r\n * Chat mode end - timeout\r\n";


/*---------------------------------------------------------------------------*/
static void rem_chat (byte *data, word len)
{
	byte x, y;

	len = len;

	if (!chat_win) {
	   if (len > 10) return;

	   chat_win = win_boxopen(21,10,60,20, CUR_NONE, CON_COOKED | CON_WRAP | CON_SCROLL,
				  xenwin_colours.file.normal, KEY_RAW,
				  LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,LINE_DOUBLE,
				  xenwin_colours.file.border,
				  "Remote HydraChat",xenwin_colours.file.title);
	   chat_remote_x = chat_remote_y = 1;
	}
	win_setattrib(chat_win,xenwin_colours.file.border);
	win_line(chat_win,1,6,40,6,LINE_SINGLE);
	win_xyputs(chat_win,1,5,"Local [Press Alt-C to start/end]");
	win_setattrib(chat_win,xenwin_colours.file.normal);

	win_getpos(chat_win,&x,&y);
	win_setrange(chat_win,2,2,39,5);
	win_setpos(chat_win,chat_remote_x,chat_remote_y);

	while (*data) {
	      switch (*data) {
		     case '\a':
			  break;

		     case '\n':
			  win_putc(chat_win,'\r');
			  /* fallthrough to default */

		     default:
			  win_putc(chat_win,(int) *data);
			  break;
	      }
	      data++;
	}

	win_getpos(chat_win,&chat_remote_x,&chat_remote_y);
	win_setrange(chat_win,2,7,39,10);
	win_setpos(chat_win,x,y);
}/*rem_chat()*/


/*---------------------------------------------------------------------------*/
static void hydra_msgdev (byte *data, word len)
{	/* text is already NUL terminated by calling func hydra_devrecv() */
	len = len;
	hydramsg("MSGDEV %s",data);
	message(3,"*HMSGDEV: %s",data);
}/*hydra_msgdev()*/


/*---------------------------------------------------------------------------*/
static	word	devtxstate;			/* dev xmit state	     */
static	h_timer devtxtimer;			/* dev xmit retry timer      */
static	word	devtxretries;			/* dev xmit retry counter    */
static	long	devtxid,	devrxid;	/* id of last devdata pkt    */
static	char	devtxdev[H_FLAGLEN + 1];	/* xmit device ident flag    */
static	byte   *devtxbuf;			/* ptr to usersupplied dbuf  */
static	word	devtxlen;			/* len of data in xmit buf   */

struct _h_dev {
	char  *dev;
	void (*func) (byte *data, word len);
};

static	struct _h_dev h_dev[] = {
	{ "MSG", hydra_msgdev },		/* internal protocol msg     */
	{ "CON", NULL	      },		/* text to console (chat)    */
	{ "PRN", NULL	      },		/* data to printer	     */
	{ "ERR", NULL	      },		/* text to error output      */
	{ NULL , NULL	      }
};


/*---------------------------------------------------------------------------*/
boolean hydra_devfree (void)
{
	if (devtxstate || !(txoptions & HOPT_DEVICE) || txstate >= HTX_END)
	   return (false);			/* busy or not allowed	     */
	else
	   return (true);			/* allowed to send a new pkt */
}/*hydra_devfree()*/


/*---------------------------------------------------------------------------*/
boolean hydra_devsend (char *dev, byte *data, word len)
{
	if (!dev || !data || !len || !hydra_devfree())
	   return (false);

	strncpy(devtxdev,dev,H_FLAGLEN);
	devtxdev[H_FLAGLEN] = '\0';
	strupr(devtxdev);
	devtxbuf = data;
	devtxlen = (word) ((len > H_MAXBLKLEN) ? H_MAXBLKLEN : len);

	devtxid++;
	devtxtimer   = h_timer_reset();
	devtxretries = 0;
	devtxstate   = HTD_DATA;

	/* special for chat, only prolong life if our side keeps typing! */
	if (chattimer > 0L && !strcmp(devtxdev,"CON") && txstate == HTX_REND)
	   braindead = h_timer_set(H_BRAINDEAD);

	return (true);
}/*hydra_devsend()*/


/*---------------------------------------------------------------------------*/
boolean hydra_devfunc (char *dev, void (*func) (byte *data, word len))
{
	register int i;

	for (i = 0; h_dev[i].dev; i++) {
	    if (!strnicmp(dev,h_dev[i].dev,H_FLAGLEN)) {
	       h_dev[i].func = func;
	       return (true);
	    }
	}

	return (false);
}/*hydra_devfunc()*/


/*---------------------------------------------------------------------------*/
static void hydra_devrecv (void)
{
	register char *p = (char *) rxbuf;
	register int   i;
	word len = rxpktlen;

	p += (int) sizeof (long);			/* skip the id long  */
	len -= (word) sizeof (long);
	for (i = 0; h_dev[i].dev; i++) {		/* walk through devs */
	    if (!strncmp(p,h_dev[i].dev,H_FLAGLEN)) {
	       if (h_dev[i].func) {
		  len -= (word) (strlen(p) + 1);	/* sub devstr len    */
		  p += (int) (strlen(p) + 1);		/* skip devtag	     */
		  p[len] = '\0';			/* NUL terminate     */
		  (*h_dev[i].func)((byte *) p,len);	/* call output func  */
	       }
	       break;
	    }
	}
}/*hydra_devrecv()*/


/*---------------------------------------------------------------------------*/
static void put_flags (char *buf, struct _h_flags flags[], long val)
{
	register char *p;
	register int   i;

	p = buf;
	for (i = 0; flags[i].val; i++) {
	    if (val & flags[i].val) {
	       if (p > buf) *p++ = ',';
	       strcpy(p,flags[i].str);
	       p += H_FLAGLEN;
	    }
	}
	*p = '\0';
}/*put_flags()*/


/*---------------------------------------------------------------------------*/
static dword get_flags (char *buf, struct _h_flags flags[])
{
	register dword	val;
	register char  *p;
	register int	i;

	val = 0x0L;
	for (p = strtok(buf,","); p; p = strtok(NULL,",")) {
	    for (i = 0; flags[i].val; i++) {
		if (!strcmp(p,flags[i].str)) {
		   val |= flags[i].val;
		   break;
		}
	    }
	}

	return (val);
}/*get_flags()*/


/*---------------------------------------------------------------------------*/
/* CRC-16/32 code now separate source */			/*AGL:10mar93*/


/*---------------------------------------------------------------------------*/
static byte *put_binbyte (register byte *p, register byte c)
{
	register byte n;

	n = c;
	if (txoptions & HOPT_HIGHCTL)
	   n &= 0x7f;

	if (n == H_DLE ||
	    ((txoptions & HOPT_XONXOFF) && (n == XON || n == XOFF)) ||
	    ((txoptions & HOPT_TELENET) && n == '\r' && txlastc == '@') ||
	    ((txoptions & HOPT_CTLCHRS) && (n < 32 || n == 127))) {
	   *p++ = H_DLE;
	   c ^= 0x40;
	}

	*p++ = c;
	txlastc = n;

	return (p);
}/*put_binbyte()*/


/*---------------------------------------------------------------------------*/
static void txpkt (register word len, int type)
{
	register byte *in, *out;
	register word  c, n;
	boolean crc32 = false;
	byte	format;
	static char hexdigit[] = "0123456789abcdef";

	txbufin[len++] = type;

	switch (type) {
	       case HPKT_START:
	       case HPKT_INIT:
	       case HPKT_INITACK:
	       case HPKT_END:
	       case HPKT_IDLE:
		    format = HCHR_HEXPKT;
		    break;

	       default:
		    /* COULD do smart format selection depending on data and options! */
		    if (txoptions & HOPT_HIGHBIT) {
		       if ((txoptions & HOPT_CTLCHRS) && (txoptions & HOPT_CANUUE))
			  format = HCHR_UUEPKT;
		       else if (txoptions & HOPT_CANASC)
			  format = HCHR_ASCPKT;
		       else
			  format = HCHR_HEXPKT;
		    }
		    else
		       format = HCHR_BINPKT;
		    break;
	}

	if (format != HCHR_HEXPKT && (txoptions & HOPT_CRC32))
	   crc32 = true;

#if H_DEBUG
if (hydra_debug) {
   char *s1, *s2, *s3, *s4;

   message(6," -> PKT (format='%c'  type='%c'  crc=%d  len=%d)",
	     (short) format, (short) type, crc32 ? 32 : 16, (int) (len - 1));

   switch (type) {
	  case HPKT_START:    message(6,"    <autostr>START");
			      break;
	  case HPKT_INIT:     s1 = ((char *) txbufin) + ((int) strlen((char *) txbufin)) + 1;
			      s2 = s1 + ((int) strlen(s1)) + 1;
			      s3 = s2 + ((int) strlen(s2)) + 1;
			      s4 = s3 + ((int) strlen(s3)) + 1;
			      message(6,"    INIT (appinfo='%s'  can='%s'  want='%s'  options='%s'  pktprefix='%s')",
				      (char *) txbufin, s1, s2, s3, s4);
			      break;
	  case HPKT_INITACK:  message(6,"    INITACK");
			      break;
	  case HPKT_FINFO:    message(6,"    FINFO (%s)",txbufin);
			      break;
	  case HPKT_FINFOACK: if (rxfd >= 0) {
				 if (rxpos > 0L) s1 = "RES";
				 else		 s1 = "BOF";
			      }
			      else if (rxpos == -1L) s1 = "HAVE";
			      else if (rxpos == -2L) s1 = "SKIP";
			      else		     s1 = "EOB";
			      message(6,"    FINFOACK (pos=%ld %s  rxstate=%d  rxfd=%d)",
				      rxpos,s1,(int) rxstate,rxfd);
			      break;
	  case HPKT_DATA:     message(6,"    DATA (ofs=%ld  len=%d)",
				      intell(h_long1(txbufin)), (int) (len - 5));
			      break;
	  case HPKT_DATAACK:  message(6,"    DATAACK (ofs=%ld)",
				      intell(h_long1(txbufin)));
			      break;
	  case HPKT_RPOS:     message(6,"    RPOS (pos=%ld%s  blklen=%ld  syncid=%ld)",
				      rxpos, rxpos < 0L ? " SKIP" : "",
				      intell(h_long2(txbufin)), rxsyncid);
			      break;
	  case HPKT_EOF:      message(6,"    EOF (ofs=%ld%s)",
				      txpos, txpos < 0L ? " SKIP" : "");
			      break;
	  case HPKT_EOFACK:   message(6,"    EOFACK");
			      break;
	  case HPKT_IDLE:     message(6,"    IDLE");
			      break;
	  case HPKT_END:      message(6,"    END");
			      break;
	  case HPKT_DEVDATA:  message(6,"    DEVDATA (id=%ld  dev='%s'  len=%u)",
				      devtxid, devtxdev, devtxlen);
			      break;
	  case HPKT_DEVDACK:  message(6,"    DEVDACK (id=%ld)",
				      intell(h_long1(rxbuf)));
			      break;
	  default:	      /* This couldn't possibly happen! ;-) */
			      break;
   }
}
#endif

	if (crc32) {
	   dword crc = CRC32POST(crc32block(crc32tab,CRC32INIT,txbufin,len)); /*AGL:10mar93*/

	   txbufin[len++] = crc;
	   txbufin[len++] = crc >> 8;
	   txbufin[len++] = crc >> 16;
	   txbufin[len++] = crc >> 24;
	}
	else {
	   word crc = (word) CRC16POST(crc16block(crc16tab,CRC16INIT,txbufin,len)); /*AGL:10mar93*/

	   txbufin[len++] = crc;
	   txbufin[len++] = crc >> 8;
	}

	in = txbufin;
	out = txbuf;
	txlastc = 0;
	*out++ = H_DLE;
	*out++ = format;

	switch (format) {
	       case HCHR_HEXPKT:
		    for (; len > 0; len--, in++) {
			if (*in & 0x80) {
			   *out++ = '\\';
			   *out++ = hexdigit[((*in) >> 4) & 0x0f];
			   *out++ = hexdigit[(*in) & 0x0f];
			}
			else if (*in < 32 || *in == 127) {
			   *out++ = H_DLE;
			   *out++ = (*in) ^ 0x40;
			}
			else if (*in == '\\') {
			   *out++ = '\\';
			   *out++ = '\\';
			}
			else
			   *out++ = *in;
		    }
		    break;

	       case HCHR_BINPKT:
		    for (; len > 0; len--)
			out = put_binbyte(out,*in++);
		    break;

	       case HCHR_ASCPKT:
		    for (n = c = 0; len > 0; len--) {
			c |= (word) ((*in++) << n);
			out = put_binbyte(out,c & 0x7f);
			c >>= 7;
			if (++n >= 7) {
			   out = put_binbyte(out,c & 0x7f);
			   n = c = 0;
			}
		    }
		    if (n > 0)
		       out = put_binbyte(out,c & 0x7f);
		    break;

	       case HCHR_UUEPKT:
		    for ( ; len >= 3; in += 3, len -= (word) 3) {
			*out++ = h_uuenc(in[0] >> 2);
			*out++ = h_uuenc(((in[0] << 4) & 0x30) | ((in[1] >> 4) & 0x0f));
			*out++ = h_uuenc(((in[1] << 2) & 0x3c) | ((in[2] >> 6) & 0x03));
			*out++ = h_uuenc(in[2] & 0x3f);
		    }
		    if (len > 0) {
		       *out++ = h_uuenc(in[0] >> 2);
		       *out++ = h_uuenc(((in[0] << 4) & 0x30) | ((in[1] >> 4) & 0x0f));
		       if (len == 2)
			  *out++ = h_uuenc((in[1] << 2) & 0x3c);
		    }
		    break;
	}

	*out++ = H_DLE;
	*out++ = HCHR_PKTEND;

	if (type != HPKT_DATA && format != HCHR_BINPKT) {
	   *out++ = '\r';
	   *out++ = '\n';
	}

	for (in = (byte *) txpktprefix; *in; in++) {
	    switch (*in) {
		   case 221: /* transmit break signal for one second */
			     com_break();
			     break;
		   case 222: { h_timer t = h_timer_set(2);
			       while (!h_timer_expired(t,h_timer_get()))
				     win_idle();
			     }
			     break;
		   case 223: com_bufbyte(0);
			     break;
		   default:  com_bufbyte(*in);
			     break;
	    }
	}

	com_putblock(txbuf,(word) (out - txbuf));
}/*txpkt()*/


/*---------------------------------------------------------------------------*/
static int rxpkt (void)
{
	static byte	chatbuf1[CHATLEN + 5],
			chatbuf2[CHATLEN + 5],
		       *curbuf = chatbuf1;
	static boolean	warned = false;
	register byte *p, *q;
	register int   c, n, i;
	h_timer tnow = h_timer_get();				/*AGL:16jul93*/

	if (chattimer > 0L) {
	   if (tnow > chattimer) {
	      chattimer = lasttimer = 0L;
	      hydra_devsend("CON",(byte *) chattime,(word) strlen(chattime));
	      win_puts(chat_win,&chattime[2]);
	   }
	   else if ((tnow + 10L) > chattimer && !warned) {
	      win_puts(chat_win,"\007\n * Warning: chat mode timeout in 10 seconds\n");
	      warned = true;
	   }
	}
	else if (chattimer != lasttimer) {
	   if (chattimer ==  0L) {
	      p = (byte *) " * Remote has chat facility (bell disabled)\r\n";
	      hydra_devsend("CON",p,(word) strlen((char *) p));
	      if (chat_win)
		 win_puts(chat_win," * Hydra session in progress, chat facility now available\n");
	   }
	   else if (chat_win) {
	      if (chattimer == -1L)
		 win_puts(chat_win," * Hydra session in init state, can't chat yet\n");
	      else if (chattimer == -2L)
		 win_puts(chat_win," * Remote has no chat facility available\n");
	      else if (chattimer == -3L) {
		 if (lasttimer > 0L) win_putc(chat_win,'\n');
		 win_puts(chat_win," * Hydra session in exit state, can't chat anymore\n");
	      }
	   }
	   lasttimer = chattimer;
	}

	if (keyabort())
	   return (H_SYSABORT);

	while (win_keyscan()) {
	      if (keyabort())
		 return (H_SYSABORT);

	      switch (c = win_keygetc()) {
		     case Up:
			  if (chat_win && windows[chat_win].win_top) {
			     win_hide(chat_win);
			     windows[chat_win].win_top--;
			     windows[chat_win].win_bottom--;
			     win_settop(chat_win);
			  }
			  break;

		     case Down:
			  if (chat_win && (windows[chat_win].win_bottom < (win_maxver - 1))) {
			     win_hide(chat_win);
			     windows[chat_win].win_top++;
			     windows[chat_win].win_bottom++;
			     win_settop(chat_win);
			  }
			  break;
			  
		     case Left:
			  if (chat_win && windows[chat_win].win_left) {
			     win_hide(chat_win);
			     windows[chat_win].win_left--;
			     windows[chat_win].win_right--;
			     win_settop(chat_win);
			  }
			  break;

		     case Right:
			  if (chat_win && (windows[chat_win].win_right < (win_maxhor - 1))) {
			     win_hide(chat_win);
			     windows[chat_win].win_left++;
			     windows[chat_win].win_right++;
			    win_settop(chat_win);
			  }
			  break;

		     case Alt_C:
			  if (chattimer == 0L) {
			     if (!chat_win) rem_chat((byte *) "\r",1);
			     hydra_devsend("CON",(byte *) chatstart,(word) strlen(chatstart));
			     win_puts(chat_win,&chatstart[2]);
			     chattimer = lasttimer = time(NULL) + CHAT_TIMEOUT;
			  }
			  else if (chattimer > 0L) {
			     chattimer = lasttimer = 0L;
			     hydra_devsend("CON",(byte *) chatend,(word) strlen(chatend));
			     win_puts(chat_win,&chatend[2]);
			     win_close(chat_win);
			     chat_win = 0;
			  }
			  else {
			     (void) win_bell();
			  }
			  break;

		     default:
			  if (c < ' ' || c == 127 || c == 255)	/*AGL:23apr93*/
			     break;

		     case '\r':
		     case '\a':
		     case '\b':
			  if (chattimer <= 0L)
			     break;

			  chattimer = time(NULL) + CHAT_TIMEOUT;
			  warned = false;

			  if (chatfill >= CHATLEN) {
			     (void) win_bell();
			  }
			  else {
			     switch (c) {
				    case '\r':
					 curbuf[chatfill++] = '\n';
					 win_putc(chat_win,'\n');
					 break;

				    case '\b':
					 if (chatfill > 0 && curbuf[chatfill - 1] != '\n')
					    chatfill--;
					 else {
					    curbuf[chatfill++] = '\b';
					    curbuf[chatfill++] = ' ';
					    curbuf[chatfill++] = '\b';
					 }
					 win_puts(chat_win,"\b \b");
					 break;

				    default:
					 curbuf[chatfill++] = (byte) c;
					 if (c != 7)
					    win_putc(chat_win,c);
					 break;
			     }
			  }
			  break;
	      }
	}

	if (chatfill > 0 && hydra_devsend("CON",curbuf,chatfill)) {
	   curbuf = (curbuf == chatbuf1) ? chatbuf2 : chatbuf1; /*AGL:03aug95*/
	   chatfill = 0;
	}

	if (!carrier())
	   return (H_CARRIER);

	if (h_timer_running(braindead) && h_timer_expired(braindead,tnow)) {
#if H_DEBUG
if (hydra_debug)
   message(6," <- BrainDead (timer=%08lx  time=%08lx)", braindead,tnow);
#endif
	   return (H_BRAINTIME);
	}
	if (h_timer_running(txtimer) && h_timer_expired(txtimer,tnow)) {
#if H_DEBUG
if (hydra_debug)
   message(6," <- TxTimer (timer=%08lx  time=%08lx)", txtimer,tnow);
#endif
	   return (H_TXTIME);
	}
	if (h_timer_running(devtxtimer) && h_timer_expired(devtxtimer,tnow)) {
#if H_DEBUG
if (hydra_debug)
   message(6," <- DevTxTimer (timer=%08lx  time=%08lx)", devtxtimer,tnow);
#endif
	   return (H_DEVTXTIME);
	}

	p = rxbufptr;

	while ((c = com_getbyte()) >= 0) {
	      if (rxoptions & HOPT_HIGHBIT)
		 c &= 0x7f;

	      n = c;
	      if (rxoptions & HOPT_HIGHCTL)
		 n &= 0x7f;
	      if (n != H_DLE &&
		  (((rxoptions & HOPT_XONXOFF) && (n == XON || n == XOFF)) ||
		   ((rxoptions & HOPT_CTLCHRS) && (n < 32 || n == 127))))
		 continue;

	      if (rxdle || c == H_DLE) {
		 switch (c) {
			case H_DLE:
			     if (++rxdle >= 5)
				return (H_CANCEL);
			     break;

			case HCHR_PKTEND:
			     if (p == NULL) break;		/*AGL:03aug95*/

			     rxbufptr = p;

			     switch (rxpktformat) {
				    case HCHR_BINPKT:
					 q = rxbufptr;
					 break;

				    case HCHR_HEXPKT:
					 for (p = q = rxbuf; p < rxbufptr; p++) {
					     if (*p == '\\' && *++p != '\\') {
						i = *p;
						n = *++p;
						if ((i -= '0') > 9) i -= ('a' - ':');
						if ((n -= '0') > 9) n -= ('a' - ':');
						if ((i & ~0x0f) || (n & ~0x0f)) {
						   c = H_NOPKT; 	/*AGL:17aug93*/
						   break;
						}
						*q++ = (i << 4) | n;
					     }
					     else
						*q++ = *p;
					 }
					 if (p > rxbufptr)
					    c = H_NOPKT;
					 break;

				    case HCHR_ASCPKT:
					 n = i = 0;
					 for (p = q = rxbuf; p < rxbufptr; p++) {
					     i |= ((*p & 0x7f) << n);
					     if ((n += 7) >= 8) {
						*q++ = (byte) (i & 0xff);
						i >>= 8;
						n -= 8;
					     }
					 }
					 break;

				    case HCHR_UUEPKT:
					 n = (int) (rxbufptr - rxbuf);
					 for (p = q = rxbuf; n >= 4; n -= 4, p += 4) {
					     if (p[0] <= ' ' || p[0] >= 'a' ||
						 p[1] <= ' ' || p[1] >= 'a' ||
						 p[2] <= ' ' || p[2] >= 'a' ||	/*AGL:17aug93*/
						 p[3] <= ' ' || p[3] >= 'a') {	/*AGL:17aug93*/
						c = H_NOPKT;
						break;
					     }
					     *q++ = (byte) ((h_uudec(p[0]) << 2) | (h_uudec(p[1]) >> 4));
					     *q++ = (byte) ((h_uudec(p[1]) << 4) | (h_uudec(p[2]) >> 2));
					     *q++ = (byte) ((h_uudec(p[2]) << 6) | h_uudec(p[3]));
					 }
					 if (n >= 2) {
					    if (p[0] <= ' ' || p[0] >= 'a' ||	  /*AGL:17aug93*/
						p[1] <= ' ' || p[1] >= 'a') {	  /*AGL:17aug93*/
					       c = H_NOPKT;
					       break;
					    }
					    *q++ = (byte) ((h_uudec(p[0]) << 2) | (h_uudec(p[1]) >> 4));
					    if (n == 3) {
					       if (p[2] <= ' ' || p[2] >= 'a') {  /*AGL:17aug93*/
						  c = H_NOPKT;
						  break;
					       }
					       *q++ = (byte) ((h_uudec(p[1]) << 4) | (h_uudec(p[2]) >> 2));
					    }
					 }
					 break;

				    default:   /* This'd mean internal fluke */
#if H_DEBUG
if (hydra_debug) {
   message(6," <- <PKTEND> (pktformat='%c' dec=%d hex=%02x) ??",
	     (short) rxpktformat, (int) rxpktformat, (word) rxpktformat);
}
#endif
					 c = H_NOPKT;
					 break;
			     }

			     rxbufptr = NULL;

			     if (c == H_NOPKT)
				break;

			     rxpktlen = (word) (q - rxbuf);
			     if (rxpktformat != HCHR_HEXPKT && (rxoptions & HOPT_CRC32)) {
				if (rxpktlen < 5) {
				   c = H_NOPKT;
				   break;
				}
				n = h_crc32test(crc32block(crc32tab,CRC32INIT,rxbuf,rxpktlen)); /*AGL:10mar93*/
				rxpktlen -= (word) sizeof (long);  /* remove CRC-32 */
			     }
			     else {
				if (rxpktlen < 3) {
				   c = H_NOPKT;
				   break;
				}
				n = h_crc16test(crc16block(crc16tab,CRC16INIT,rxbuf,rxpktlen)); /*AGL:10mar93*/
				rxpktlen -= (word) sizeof (word);  /* remove CRC-16 */
			     }

			     rxpktlen--;		     /* remove type  */

			     if (n) {
#if H_DEBUG
if (hydra_debug) {
   char *s1, *s2, *s3, *s4;

   message(6," <- PKT (format='%c'  type='%c'  len=%d)",
	   (short) rxpktformat, (short) rxbuf[rxpktlen], (int) rxpktlen);

   switch (rxbuf[rxpktlen]) {
	  case HPKT_START:    message(6,"    START");
			      break;
	  case HPKT_INIT:     s1 = ((char *) rxbuf) + ((int) strlen((char *) rxbuf)) + 1;
			      s2 = s1 + ((int) strlen(s1)) + 1;
			      s3 = s2 + ((int) strlen(s2)) + 1;
			      s4 = s3 + ((int) strlen(s3)) + 1;
			      message(6,"    INIT (appinfo='%s'  can='%s'  want='%s'  options='%s'  pktprefix='%s')",
				      (char *) rxbuf, s1, s2, s3, s4);
			      break;
	  case HPKT_INITACK:  message(6,"    INITACK");
			      break;
	  case HPKT_FINFO:    message(6,"    FINFO ('%s'  rxstate=%d)",rxbuf,(int) rxstate);
			      break;
	  case HPKT_FINFOACK: message(6,"    FINFOACK (pos=%ld  txstate=%d  txfd=%d)",
				      intell(h_long1(rxbuf)), (int) txstate, txfd);
			      break;
	  case HPKT_DATA:     message(6,"    DATA (rxstate=%d  pos=%ld  len=%u)",
				      (int) rxstate, intell(h_long1(rxbuf)),
				      (word) (rxpktlen - ((int) sizeof (long))));
			      break;
	  case HPKT_DATAACK:  message(6,"    DATAACK (rxstate=%d  pos=%ld)",
				      (int) rxstate, intell(h_long1(rxbuf)));
			      break;
	  case HPKT_RPOS:     message(6,"    RPOS (pos=%ld%s  blklen=%u->%ld  syncid=%ld%s  txstate=%d  txfd=%d)",
				      intell(h_long1(rxbuf)),
				      intell(h_long1(rxbuf)) < 0L ? " SKIP" : "",
				      txblklen, intell(h_long2(rxbuf)),
				      intell(h_long3(rxbuf)),
				      intell(h_long3(rxbuf)) == rxsyncid ? " DUP" : "",
				      (int) txstate, txfd);
			      break;
	  case HPKT_EOF:      message(6,"    EOF (rxstate=%d  pos=%ld%s)",
				      (int) rxstate, intell(h_long1(rxbuf)),
				      intell(h_long1(rxbuf)) < 0L ? " SKIP" : "");
			      break;
	  case HPKT_EOFACK:   message(6,"    EOFACK (txstate=%d)", (int) txstate);
			      break;
	  case HPKT_IDLE:     message(6,"    IDLE");
			      break;
	  case HPKT_END:      message(6,"    END");
			      break;
	  case HPKT_DEVDATA:  s1 = ((char *) rxbuf) + ((int) sizeof (long));
			      message(6,"    DEVDATA (id=%ld  dev=%s  len=%u)",
				      intell(h_long1(rxbuf)), s1,
				      (word) (rxpktlen - (((int) sizeof (long)) + (int) strlen(s1) + 1)));
			      break;
	  case HPKT_DEVDACK:  message(6,"    DEVDACK (devtxstate=%d  id=%ld)",
				      (int) devtxstate, intell(h_long1(rxbuf)));
			      break;
	  default:	      message(6,"    Unkown pkttype %d (txstate=%d  rxstate=%d)",
				      (int) rxbuf[rxpktlen], (int) txstate, (int) rxstate);
			      break;
   }
}
#endif
				return ((int) rxbuf[rxpktlen]);
			     }/*goodpkt*/

#if H_DEBUG
if (hydra_debug)
   message(6," Bad CRC (format='%c'  type='%c'  len=%d)",
	     (short) rxpktformat, (int) rxbuf[rxpktlen], (int) rxpktlen);
#endif
			     break;

			case HCHR_BINPKT: 
			case HCHR_HEXPKT: 
			case HCHR_ASCPKT: 
			case HCHR_UUEPKT:
#if H_DEBUG
if (hydra_debug)
   message(6," <- <PKTSTART> (pktformat='%c')",(short) c);
#endif
			     rxpktformat = c;
			     p = rxbufptr = rxbuf;
			     rxdle = 0;
			     break;

			default:
			     if (p) {
				if (p < rxbufmax)
				   *p++ = (byte) (c ^ 0x40);
				else {
#if H_DEBUG
if (hydra_debug)
   message(6," <- Pkt too long - discarded");
#endif
				   p = NULL;
				}
			     }
			     rxdle = 0;
			     break;
		 }
	      }
	      else if (p) {
		 if (p < rxbufmax)
		    *p++ = (byte) c;
		 else {
#if H_DEBUG
if (hydra_debug)
   message(6," <- Pkt too long - discarded");
#endif
		    p = NULL;
		 }
	      }
	}

	rxbufptr = p;

	win_idle();
	return (H_NOPKT);
}/*rxpkt()*/


/*---------------------------------------------------------------------------*/
static void hydra_status (boolean xmit)
{
	long pos    = xmit ? txpos    : rxpos,
	     fsize  = xmit ? txfsize  : rxfsize;
	char *p = "";

	win_setpos(file_win,13,xmit ? 5 : 7);
	if (pos >= fsize)
	   win_print(file_win,"%ld/%ld (EOF)",pos,fsize);
	else {
	   int left = (int) ((((fsize - pos) / 128L) * 1340L) / line_speed);

	   if (xmit) {
	      if      (txstate == HTX_DATAACK) p = "ACK ";
	      else if (txstate == HTX_XWAIT)   p = "WAIT ";
	   }
	   win_print(file_win,"%ld/%ld (%s%d:%02d left)",
		     pos, fsize, p, left / 60, left % 60);
	}

	if (!*p) {						/*AGL:17jun94*/
	   long elapsed,
		bytes,
		start = xmit ? txstart	: rxstart;		/*AGL:06jul94*/

	   if (start) { 					/*AGL:06jul94*/
	      if (xmit) {
		 elapsed = time(NULL) - start;
		 bytes = (pos - com_outfull()) - txoffset;
	      }
	      else {
		 elapsed = time(NULL) - start;
		 bytes = pos - rxoffset;
	      }
	      if (bytes >= 1024L && elapsed >= 60L)
		 win_print(file_win," %ld CPS", bytes / elapsed);
	   }
	}

	win_clreol(file_win);
}/*hydra_status()*/


/*---------------------------------------------------------------------------*/
static void hydra_cps (boolean xmit)
{
	long offset = xmit ? txoffset : rxoffset,
	     fsize  = xmit ? txfsize  : rxfsize,
	     start  = xmit ? txstart  : rxstart,
	     elapsed, bytes, cps /*, pct*/ ;

	if (!start) return;					/*AGL:06jul94*/
	elapsed = time(NULL) - start;
	bytes = fsize - offset;
	if (bytes < 1024L || elapsed < 60L)			/*AGL:03mar93*/
	   return;
	cps = bytes / elapsed;
#if 0
	pct = (cps * 1000L) / ((long) line_speed);
	message(2,"+%s-H CPS: %ld (%ld bytes), %d:%02d min.  Eff: %ld%%",
		xmit ? "Sent" : "Rcvd", cps, bytes,
		(int) (elapsed / 60L), (int) (elapsed % 60L), pct);
#else
	message(2,"+%s-H CPS: %ld (%ld bytes), %d:%02d min.",
		xmit ? "Sent" : "Rcvd", cps, bytes,
		(int) (elapsed / 60L), (int) (elapsed % 60L));
#endif
}/*hydra_cps()*/


/*---------------------------------------------------------------------------*/
void hydra_badxfer (void)
{
	if (rxfd >= 0) {
	   dos_close(rxfd);
	   rxfd = -1;
	   if (xfer_bad())
	      message(2,"+HRECV: Bad xfer recovery-info saved");
	   else
	      message(0,"-HRECV: Bad xfer - file deleted");
	}
}/*hydra_badxfer()*/


/*---------------------------------------------------------------------------*/
void hydra_init (dword want_options)
{
	alloc_msg("pre hydra init");

	txbuf	 = (byte *)  malloc(H_BUFLEN);
	rxbuf	 = (byte *)  malloc(H_BUFLEN);
	crc16tab = (word *)  malloc(CRC_TABSIZE * ((int) sizeof (word)));  /*AGL:10mar93*/
	crc32tab = (dword *) malloc(CRC_TABSIZE * ((int) sizeof (dword))); /*AGL:10mar93*/
	if (!txbuf || !rxbuf || !crc16tab || !crc32tab)
	   errl_exit(2,false,"Can't allocate Hydra buffers");
	txbufin  = txbuf + ((H_MAXBLKLEN + H_OVERHEAD + 5) * 2);
	rxbufmax = rxbuf + H_MAXPKTLEN;

	crc16init(crc16tab,CRC16POLY);				/*AGL:10mar93*/
	crc32init(crc32tab,CRC32POLY);				/*AGL:10mar93*/

	batchesdone = 0;

	originator = remote.capabilities ? (caller ? true : false) : true;

	if (originator)
	   hdxlink = false;
	else {
	   if ((hydra_minspeed && cur_speed < hydra_minspeed) ||
	       (hydra_maxspeed && cur_speed > hydra_maxspeed))
	      hdxlink = true;
	   else if (hydra_nofdx && mdmconnect[0] && strstr(mdmconnect,hydra_nofdx))
	      hdxlink = true;
	   else if (hydra_fdx && mdmconnect[0] && strstr(mdmconnect,hydra_fdx))
	      hdxlink = false;
	   else
	      hdxlink = hydra_fdx ? true : false;
	}

	options = (want_options & HCAN_OPTIONS) & ~HUNN_OPTIONS;

	timeout = (word) (40960L / cur_speed);
	if	(timeout < H_MINTIMER) timeout = H_MINTIMER;
	else if (timeout > H_MAXTIMER) timeout = H_MAXTIMER;

	if (cur_speed >= 2400UL)				/*AGL:09mar94*/
	   txmaxblklen = H_MAXBLKLEN;				/*AGL:09mar94*/
	else {							/*AGL:09mar94*/
	   txmaxblklen = (word) ((cur_speed / 300) * 128);	/*AGL:09mar94*/
	   if (txmaxblklen < 256) txmaxblklen = 256;		/*AGL:09mar94*/
	}							/*AGL:09mar94*/

	if (isdnlink || arqlink) {				/*AGL:14jan95*/
	   rxblklen = txblklen = txmaxblklen;
	   txgoodneeded = 0;
	}
	else {
	   rxblklen = txblklen = (word) ((cur_speed < 2400UL) ? 256 : 512);
	   txgoodneeded = txmaxblklen;				/*AGL:23feb93*/
	}
	txgoodbytes  = 0;

	txstate = HTX_DONE;

	hydra_devfunc("CON",rem_chat);
	chatfill  = 0;
	chattimer = -1L;
	lasttimer = 0L;
	chat_win  = 0;

	filewincls(hdxlink ? "Uni" : "Bi",4);	       /* Set up xfer window */

	alloc_msg("post hydra init");
}/*hydra_init()*/


/*---------------------------------------------------------------------------*/
void hydra_deinit (void)
{
	alloc_msg("pre hydra deinit");

	free(txbuf);
	free(rxbuf);
	free(crc16tab);
	free(crc32tab);

	if (chat_win) {
	   win_close(chat_win);
	   chat_win = 0;
	}

	alloc_msg("post hydra deinit");
}/*hydra_deinit()*/


/*---------------------------------------------------------------------------*/
int hydra (char *txpathname, char *txalias)
{
	int   res;
	int   pkttype;
	char *p, *q;
	int   i;
	struct stat f;

	/*-------------------------------------------------------------------*/
	if (txstate == HTX_DONE) {
	   txstate	  = HTX_START;
	   win_xyprint(file_win,13,4,"Init"); win_clreol(file_win);
	   txoptions	  = HTXI_OPTIONS;
	   txpktprefix[0] = '\0';

	   rxstate   = HRX_INIT;
	   win_xyprint(file_win,13,6,"Init"); win_clreol(file_win);
	   rxoptions = HRXI_OPTIONS;
	   rxfd      = -1;
	   rxdle     = 0;
	   rxbufptr  = NULL;
	   rxtimer   = h_timer_reset();

	   devtxid    = devrxid = 0L;
	   devtxtimer = h_timer_reset();
	   devtxstate = HTD_DONE;

	   braindead = h_timer_set(H_BRAINDEAD);
	}
	else
	   txstate = HTX_FINFO;

	txtimer   = h_timer_reset();
	txretries = 0;

	/*-------------------------------------------------------------------*/
	win_setpos(file_win,13,5); win_clreol(file_win);
	if (txpathname) {
	   com_disk(true);
	   getstat(txpathname,&f);
	   com_disk(false);
	   txfsize = f.st_size;
	   txftime = f.st_mtime;

	   if ((txfd = dos_sopen(txpathname,O_RDONLY,DENY_NONE)) < 0) {
	      message(3,"-HSEND: Unable to open %s",txpathname);
	      return (XFER_SKIP);
	   }

#ifdef __MSDOS__						/*AGL:10mar93*/
	   if (isatty(txfd)) {					/*AGL:10mar93*/
	      message(6,"-HSEND: %s is a device name!",txpathname); /*AGL:10mar93*/
	      dos_close(txfd);					/*AGL:10mar93*/
	      return (XFER_SKIP);				/*AGL:10mar93*/
	   }							/*AGL:10mar93*/
#endif								/*AGL:10mar93*/

	   strupr(txpathname);
	   for (p=txpathname, q=txfname; *p; p++) {
	       if (*q=*p, *p=='\\' || *p==':' || *p=='/')
		  q=txfname;
	       else q++;
	   }
	   *q = '\0';

	   if (txalias)
	      strupr(txalias);

	   txstart  = 0L;
	   txsyncid = 0L;
	}
	else {
	   txfd = -1;
	   strcpy(txfname,"");
	}

	/*-------------------------------------------------------------------*/
	do {
	   /*----------------------------------------------------------------*/
	   switch (devtxstate) {
		  /*---------------------------------------------------------*/
		  case HTD_DATA:
		       if (txstate > HTX_RINIT) {
			  h_long1(txbufin) = intell(devtxid);
			  p = ((char *) txbufin) + ((int) sizeof(long));
			  strcpy(p,devtxdev);
			  p += H_FLAGLEN + 1;
			  memcpy(p,devtxbuf,devtxlen);
			  txpkt((word) (sizeof (long) + H_FLAGLEN + 1 + devtxlen),HPKT_DEVDATA);
			  devtxtimer = h_timer_set((!rxstate && txstate == HTX_REND) ? timeout / 2 : timeout); /*AGL:10mar93*/
			  devtxstate = HTD_DACK;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  default:
		       break;

		  /*---------------------------------------------------------*/
	   }

	   /*----------------------------------------------------------------*/
	   switch (txstate) {
		  /*---------------------------------------------------------*/
		  case HTX_START:
		       com_putblock((byte *) autostr,(word) strlen(autostr));
		       txpkt(0,HPKT_START);
		       txtimer = h_timer_set(H_START);
		       txstate = HTX_SWAIT;
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_INIT:
		       p = (char *) txbufin;
#if 0
		       if (xenpoint_ver)
			  sprint(p,"%08lxXenia Point,%s %s,%s", H_REVSTAMP,
				   xenpoint_ver, XENIA_OS, xenpoint_reg);
		       else
#endif
			     sprint(p,"%08lx%s,%s %s,%s", H_REVSTAMP,
				    PRGNAME, VERSION, XENIA_OS, "OpenSource");
		       p += ((int) strlen(p)) + 1;/* our app info & HYDRA rev. */
#if 0
		       if (isdnlink && arqlink) 		/*AGL:08jan94*/
			  put_flags(p,h_flags,HCAN_OPTIONS & ~HOPT_CRC32); /*AGL:08jan94*/
		       else					/*AGL:08jan94*/
#endif
			  put_flags(p,h_flags,HCAN_OPTIONS); /* what we CAN  */
		       p += ((int) strlen(p)) + 1;
		       put_flags(p,h_flags,options);	     /* what we WANT */
		       p += ((int) strlen(p)) + 1;
		       sprint(p,"%08lx%08lx",		     /* TxRx windows */
				hydra_txwindow,hydra_rxwindow);
		       p += ((int) strlen(p)) + 1;
		       strcpy(p,pktprefix);	/* pkt prefix string we want */
		       p += ((int) strlen(p)) + 1;

		       txoptions = HTXI_OPTIONS;
		       txpkt((word) (((byte *) p) - txbufin), HPKT_INIT);
		       txoptions = rxoptions;
		       txtimer = h_timer_set(timeout / 2);
		       txstate = HTX_INITACK;
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_FINFO:
		       if (txfd >= 0) {
			  if (!txretries) {
			     win_xyputs(file_win,13,4,txfname);
			     if (txalias)
				win_xyprint(file_win,25,4,"  ->  %s",txalias);
			     win_clreol(file_win);
			     message(2,"+HSEND: %s%s%s (%ldb), %d min.",
				     txpathname, txalias ? " -> " : "", txalias ? txalias : "",
				     txfsize, (int) (txfsize * 10L / line_speed + 27L) / 54L);
			  }
			  sprint((char *) txbufin,"%08lx%08lx%08lx%08lx%08lx",
				 txftime, txfsize, 0L, 0L, 0L);
#if OS2 || WINNT
			  MakeShortName(txalias ? txalias : txfname,
					(char *) &txbufin[strlen((char *) txbufin)]);
			  i = strlen((char *) txbufin) + 1;
			  if (!txalias) {
			     strcpy((char *) (txbufin + i), txfname);
			     i += strlen(txfname) + 1;
			  }
#else
			  strcat((char *) txbufin, txalias ? txalias : txfname);
			  strlwr(txfname);
			  i = strlen((char *) txbufin) + 1;
#endif
		       }
		       else {
			  if (!txretries) {
			     win_xyprint(file_win,13,4,"End of batch");
			     win_clreol(file_win);
			     message(1,"+HSEND: End of batch");
			  }
			  strcpy((char *) txbufin,txfname);
			  i = strlen((char *) txbufin) + 1;
		       }
		       txpkt((word) i,HPKT_FINFO);
		       txtimer = h_timer_set(txretries ? timeout / 2 : timeout);
		       txstate = HTX_FINFOACK;
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_XDATA:
		       if (com_outfull() > txmaxblklen)
			  break;

		       if (txpos < 0L)
			  i = -1;				     /* Skip */
		       else {
			  h_long1(txbufin) = intell(txpos);
			  if ((i = dos_read(txfd,txbufin + ((int) sizeof (long)),txblklen)) < 0) {
			     message(6,"!HSEND: File read error");
			     dos_close(txfd);
			     txfd = -1;
			     txpos = -2L;			     /* Skip */
			  }
		       }

		       if (i > 0) {
			  txpos += i;
			  txpkt((word) (sizeof (long) + i), HPKT_DATA);

			  if (txblklen < txmaxblklen &&
			      (txgoodbytes += (word) i) >= txgoodneeded) {
			     txblklen <<= 1;
			     if (txblklen >= txmaxblklen) {
				txblklen = txmaxblklen;
				txgoodneeded = 0;
			     }
			     txgoodbytes = 0;
			  }

			  if (txwindow && (txpos >= (txlastack + txwindow))) {
			     txtimer = h_timer_set(txretries ? timeout / 2 : timeout);
			     txstate = HTX_DATAACK;
			  }

			  if (!txstart)
			     txstart = time(NULL);
			  hydra_status(true);
			  break;
		       }

		       /* fallthrough to HTX_EOF */

		  /*---------------------------------------------------------*/
		  case HTX_EOF:
		       h_long1(txbufin) = intell(txpos);
		       txpkt((int) sizeof (long),HPKT_EOF);
		       txtimer = h_timer_set(txretries ? timeout / 2 : timeout);
		       txstate = HTX_EOFACK;
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_END:
		       txpkt(0,HPKT_END);
		       txpkt(0,HPKT_END);
		       txtimer = h_timer_set(timeout / 2);
		       txstate = HTX_ENDACK;
		       break;

		  /*---------------------------------------------------------*/
		  default:
		       break;

		  /*---------------------------------------------------------*/
	   }

	   /*----------------------------------------------------------------*/
	   pkttype = rxpkt();

	   /*----------------------------------------------------------------*/
	   switch (pkttype) {
		  /*---------------------------------------------------------*/
		  case H_CARRIER:
		  case H_CANCEL:
		  case H_SYSABORT:
		  case H_BRAINTIME:
		       switch (pkttype) {
			      case H_CARRIER:	p = "Carrier lost";	     break;
			      case H_CANCEL:	p = "Aborted by other side"; break;
			      case H_SYSABORT:	p = "Aborted by operator";   break;
			      case H_BRAINTIME: p = "Other end died";	     break;
		       }
		       message(3,"-HYDRA: %s",p);
		       txstate = HTX_DONE;
		       res = XFER_ABORT;
		       break;

		  /*---------------------------------------------------------*/
		  case H_TXTIME:
		       if (txstate == HTX_XWAIT || txstate == HTX_REND) {
			  txpkt(0,HPKT_IDLE);
			  txtimer = h_timer_set(H_IDLE);
			  break;
		       }

		       if (++txretries > H_RETRIES) {
			  message(3,"-HSEND: Too many errors");
			  txstate = HTX_DONE;
			  res = XFER_ABORT;
			  break;
		       }

		       hydramsg("TX Timeout - Retry %u",txretries);
		       message(0,"-HSEND: Timeout - Retry %u",txretries);

		       txtimer = h_timer_reset();

		       switch (txstate) {
			      case HTX_SWAIT:	 txstate = HTX_START; break;
			      case HTX_INITACK:  txstate = HTX_INIT;  break;
			      case HTX_FINFOACK: txstate = HTX_FINFO; break;
			      case HTX_DATAACK:  txstate = HTX_XDATA; break;
			      case HTX_EOFACK:	 txstate = HTX_EOF;   break;
			      case HTX_ENDACK:	 txstate = HTX_END;   break;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case H_DEVTXTIME:
		       if (++devtxretries > H_RETRIES) {
			  message(3,"-HDEVTX: Too many errors");
			  txstate = HTX_DONE;
			  res = XFER_ABORT;
			  break;
		       }

		       hydramsg("DEV TX Timeout - Retry %u",devtxretries);
		       message(0,"-HDEVTX: Timeout - Retry %u",devtxretries);

		       devtxtimer = h_timer_reset();
		       devtxstate = HTD_DATA;
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_START:
		       if (txstate == HTX_START || txstate == HTX_SWAIT) {
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_INIT;
			  braindead = h_timer_set(H_BRAINDEAD);
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_INIT:
		       if (rxstate == HRX_INIT) {
			  p = (char *) rxbuf;
			  p += ((int) strlen(p)) + 1;
			  q = p + ((int) strlen(p)) + 1;
			  rxoptions  = options | HUNN_OPTIONS;
			  rxoptions |= get_flags(q,h_flags);
			  rxoptions &= get_flags(p,h_flags);
			  rxoptions &= HCAN_OPTIONS;
#if 0
			  if (isdnlink && arqlink)		/*AGL:08jan94*/
			     rxoptions &= ~HOPT_CRC32;		/*AGL:08jan94*/
#endif
			  if (rxoptions < (options & HNEC_OPTIONS)) {
			     message(6,"!HYDRA: Incompatible on this link");
			     txstate = HTX_DONE;
			     res = XFER_ABORT;
			     break;
			  }
			  p = q + ((int) strlen(q)) + 1;
			  rxwindow = txwindow = 0L;
			  sscanf(p,"%08lx%08lx", &rxwindow,&txwindow);
			  if (rxwindow < 0L) rxwindow = 0L;
			  if (hydra_rxwindow &&
			      (!rxwindow || hydra_rxwindow < rxwindow))
			     rxwindow = hydra_rxwindow;
			  if (txwindow < 0L) txwindow = 0L;
			  if (hydra_txwindow &&
			      (!txwindow || hydra_txwindow < txwindow))
			     txwindow = hydra_txwindow;
			  p += ((int) strlen(p)) + 1;
			  strncpy(txpktprefix,p,H_PKTPREFIX);
			  txpktprefix[H_PKTPREFIX] = '\0';

			  if (!batchesdone) {
			     long revstamp;

			     p = (char *) rxbuf;
			     sscanf(p,"%08lx",&revstamp);
			     message(0,"*HYDRA: Other's HydraRev=%s",
				     h_revdate(revstamp));
			     p += 8;
			     if ((q = strchr(p,',')) != NULL) *q = ' ';
			     if ((q = strchr(p,',')) != NULL) *q = '/';
			     message(0,"*HYDRA: Other's App.Info '%s'",p);
			     put_flags((char *) rxbuf,h_flags,rxoptions);
			     message(1,"*HYDRA: Using link options '%s'",rxbuf);
			     if (txwindow || rxwindow)
				message(0,"*HYDRA: Window tx=%ld rx=%ld",
					  txwindow,rxwindow);
			  }

			  txoptions = rxoptions;
			  rxstate = HRX_FINFO;

			  chattimer = (rxoptions & HOPT_DEVICE) ? 0L : -2L;
#if 0
			  if (rxoptions & HOPT_DEVICE) {
			     p = "Remote has no chat facility available\r\n";
			     hydra_devsend("CON",(byte *) p,(word) strlen(p));
			  }
#endif
		       }

		       txpkt(0,HPKT_INITACK);
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_INITACK:
		       if (txstate == HTX_INIT || txstate == HTX_INITACK) {
			  braindead = h_timer_set(H_BRAINDEAD);
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_RINIT;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_FINFO:
		       if (rxstate == HRX_FINFO) {
			  braindead = h_timer_set(H_BRAINDEAD);
			  if (!rxbuf[0]) {
			     win_xyprint(file_win,13,6,"End of batch");
			     win_clreol(file_win);
			     win_setpos(file_win,13,7);
			     win_clreol(file_win);
			     message(1,"*HRECV: End of batch");
			     rxpos = 0L;
			     rxstate = HRX_DONE;
			     batchesdone++;
			  }
			  else {
			     long diskfree;

			     rxfsize = rxftime = 0L;
			     rxfname[0] = '\0';
			     sscanf((char *) rxbuf,"%08lx%08lx%*08lx%*08lx%*08lx%s",
				    &rxftime, &rxfsize, rxfname);
			     /*strupr(rxfname);*/
#if OS2 || WINNT
			     i = strlen((char *) rxbuf) + 1;
			     if (rxpktlen > i && rxbuf[i] && !strpbrk((char *) &rxbuf[i],":/\\"))
				strcpy(rxfname,(char *) &rxbuf[i]);
#endif

			     win_xyputs(file_win,13,6,rxfname);
			     win_clreol(file_win);
			     win_setpos(file_win,13,7);
			     win_clreol(file_win);

			     rxpathname = xfer_init(rxfname,rxfsize,rxftime);

			     if (!rxpathname) {   /* Already have file */
				win_xyprint(file_win,13,7,"Already have file");
				message(2,"+HRECV: Already have %s",rxfname);
				rxpos = -1L;
			     }
			     else {
				if (fexist(rxpathname)) { /* Resuming? */
				   if ((rxfd = dos_sopen(rxpathname,O_WRONLY|O_CREAT,DENY_WRITE)) < 0) {
				      message(6,"!HRECV: Unable to re-open %s",rxpathname);
				      rxpos = -2L;
				   }
				}
				else if ((rxfd = dos_sopen(rxpathname,O_WRONLY|O_CREAT,DENY_WRITE)) < 0) {
				   message(6,"!HRECV: Unable to create %s",rxpathname);
				   rxpos = -2L;
				}

				if (rxfd >= 0) {
				   message(2,"+HRECV: %s (%ldb), %d min.",
					   rxfname, rxfsize,
					   (int) (rxfsize * 10L / line_speed + 27L) / 54L);
				   if (dos_seek(rxfd,0L,SEEK_END) < 0L) {
				      message(6,"!HRECV: File seek error");
				      hydra_badxfer();
				      rxpos = -2L;
				   }
				   else {
				      diskfree = freespace(rxpathname);
				      rxoffset = rxpos = dos_tell(rxfd);
				      if (rxpos < 0L) {
					 message(6,"!HRECV: File tell error");
					 hydra_badxfer();
					 rxpos = -2L;
				      }
				      else if ((rxfsize - rxoffset) + 10240L > diskfree) {
					 win_xyprint(file_win,13,7,"Not enough diskspace");
					 message(6,"!HRECV: %s not enough diskspace: %ld > %ld",
						 rxfname, (rxfsize - rxoffset) + 10240L, diskfree);
					 hydra_badxfer();
					 rxpos = -2L;
				      }
				      else {
					 rxstart = 0L;
					 rxtimer = h_timer_reset();
					 rxretries = 0;
					 rxlastsync = 0L;
					 rxsyncid = 0L;
					 hydra_status(false);
					 if (rxpos > 0L) {
					    win_xyprint(file_win,13,7,"%ld/%ld",rxpos,rxfsize);
					    message(2,"+HRECV: Resuming from offset %ld (%d min. to go)",
						    rxpos, (int) ((rxfsize - rxoffset) * 10L / line_speed + 27L) / 54L);
					 }
					 rxstate = HRX_DATA;
				      }
				   }
				}
			     }
			  }
		       }
		       else if (rxstate == HRX_DONE)
			  rxpos = (!rxbuf[0]) ? 0L : -2L;

		       h_long1(txbufin) = intell(rxpos);
		       txpkt((int) sizeof (long),HPKT_FINFOACK);
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_FINFOACK:
		       if (txstate == HTX_FINFOACK) {
			  braindead = h_timer_set(H_BRAINDEAD);
			  txretries = 0;
			  if (!txfname[0]) {
			     txtimer = h_timer_set(H_IDLE);
			     txstate = HTX_REND;
			  }
			  else if (txfd >= 0) { 		/*AGL:06jan94*/
			     long txstartpos;			/*AGL:06jan94*/

			     txtimer = h_timer_reset();
			     txstartpos = intell(h_long1(rxbuf)); /*AGL:06jan94*/
			     if (txstartpos < 0L) {		/*AGL:12mar94*/
				dos_close(txfd);
				if (txstartpos == -1L) {	/*AGL:12mar94*/
				   win_xyprint(file_win,13,5,"They already have file");
				   win_clreol(file_win);
				   message(2,"+HSEND: They already have %s",txfname);
				   return (XFER_OK);
				}
				else {	/* (txstartpos < -1L) file NOT sent */ /*AGL:12mar94*/
				   win_xyprint(file_win,13,5,"Skipping");
				   win_clreol(file_win);
				   message(2,"+HSEND: Skipping %s",txfname);
				   return (XFER_SKIP);
				}
			     }
			     else {				/*AGL:06jan94*/
				txstart = 0L;			/*AGL:06jan94*/
				txpos = txstartpos;		/*AGL:06jan94*/
				txoffset = txpos;
				txlastack = txpos;
				hydra_status(true);
				if (txpos > 0L) {
				   message(2,"+HSEND: Transmitting from offset %ld (%d min. to go)",
					   txpos, (int) ((txfsize - txoffset) * 10L / line_speed + 27L) / 54L);
				   if (dos_seek(txfd,txpos,SEEK_SET) < 0L) {
				      message(6,"!HSEND: File seek error");
				      dos_close(txfd);
				      txfd = -1;
				      txpos = -2L;
				      txstate = HTX_EOF;
				      break;
				   }
				}
				txstate = HTX_XDATA;
			     }
			  }
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_DATA:
		       if (rxstate == HRX_DATA) {
			  if (intell(h_long1(rxbuf)) != rxpos ||
			      intell(h_long1(rxbuf)) < 0L) {
			     if (intell(h_long1(rxbuf)) <= rxlastsync) {
				rxtimer = h_timer_reset();
				rxretries = 0;
			     }
			     rxlastsync = intell(h_long1(rxbuf));

			     if (!h_timer_running(rxtimer) ||
				 h_timer_expired(rxtimer,h_timer_get())) {
				 if (rxretries > 4) {
				    if (txstate < HTX_REND &&
					!originator && !hdxlink) {
				       hdxlink = true;
				       rxretries = 0;
				    }
				}
				if (++rxretries > H_RETRIES) {
				   message(3,"-HRECV: Too many errors");
				   txstate = HTX_DONE;
				   res = XFER_ABORT;
				   break;
				}
				if (rxretries == 1 || rxretries == 4)  /*AGL:14may93*/
				   rxsyncid++;

				rxblklen /= (word) 2;
				i = rxblklen;
				if	(i <=  64) i =	 64;
				else if (i <= 128) i =	128;
				else if (i <= 256) i =	256;
				else if (i <= 512) i =	512;
				else		   i = 1024;
				hydramsg("Bad pkt at %ld - Retry %u",
					 rxpos,rxretries);
				message(0,"-HRECV: Bad pkt at %ld - Retry %u (newblklen=%u)",
					rxpos,rxretries,i);
				h_long1(txbufin) = intell(rxpos);
				h_long2(txbufin) = intell((long) i);
				h_long3(txbufin) = intell(rxsyncid);
				txpkt(3 * ((int) sizeof(long)),HPKT_RPOS);
				rxtimer = h_timer_set(timeout);
			     }
			  }
			  else {
			     braindead = h_timer_set(H_BRAINDEAD);
			     rxpktlen -= (word) sizeof (long);
			     rxblklen = rxpktlen;
			     if (dos_write(rxfd,rxbuf + ((int) sizeof (long)),rxpktlen) < 0) {
				message(6,"!HRECV: File write error");
				hydra_badxfer();
				rxpos = -2L;
				rxretries = 1;
				rxsyncid++;
				h_long1(txbufin) = intell(rxpos);
				h_long2(txbufin) = intell(0L);
				h_long3(txbufin) = intell(rxsyncid);
				txpkt(3 * ((int) sizeof(long)),HPKT_RPOS);
				rxtimer = h_timer_set(timeout);
				break;
			     }
			     rxretries = 0;
			     rxtimer = h_timer_reset();
			     rxlastsync = rxpos;
			     rxpos += rxpktlen;
			     if (rxwindow) {
				h_long1(txbufin) = intell(rxpos);
				txpkt((int) sizeof(long),HPKT_DATAACK);
			     }
			     if (!rxstart)
				rxstart = time(NULL) -
					  ((((long) rxpktlen) * 10L) / line_speed);
			     hydra_status(false);
			  }/*badpkt*/
		       }/*rxstate==HRX_DATA*/
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_DATAACK:
		       if (txstate == HTX_XDATA || txstate == HTX_DATAACK || /*AGL:06jan94*/
			   txstate == HTX_XWAIT ||			     /*AGL:06jan94*/
			   txstate == HTX_EOF || txstate == HTX_EOFACK) {    /*AGL:06jan94*/
			  if (txwindow && intell(h_long1(rxbuf)) > txlastack) {
			     txlastack = intell(h_long1(rxbuf));
			     if (txstate == HTX_DATAACK &&
				 (txpos < (txlastack + txwindow))) {
				txstate = HTX_XDATA;
				txretries = 0;
				txtimer = h_timer_reset();
			     }
			  }
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_RPOS:
		       if (txstate == HTX_XDATA || txstate == HTX_DATAACK || /*AGL:06jan94*/
			   txstate == HTX_XWAIT ||			     /*AGL:06jan94*/
			   txstate == HTX_EOF || txstate == HTX_EOFACK) {    /*AGL:06jan94*/
			  if (intell(h_long3(rxbuf)) != txsyncid) {
			     txsyncid = intell(h_long3(rxbuf));
			     txretries = 1;
			  }					/*AGL:14may93*/
			  else {				/*AGL:14may93*/
			     if (++txretries > H_RETRIES) {
				message(3,"-HSEND: Too many errors");
				txstate = HTX_DONE;
				res = XFER_ABORT;
				break;
			     }
			     if (txretries != 4) break; 	/*AGL:14may93*/
			  }

			  txtimer = h_timer_reset();
			  txpos = intell(h_long1(rxbuf));
			  if (txpos < 0L) {
			     if (txfd >= 0) {
				win_xyprint(file_win,13,5,"Skipping");
				win_clreol(file_win);
				message(2,"+HSEND: Skipping %s",txfname);
				dos_close(txfd);
				txfd = -1;
				txstate = HTX_EOF;
			     }
			     txpos = -2L;
			     break;
			  }

			  if (txblklen > intell(h_long2(rxbuf)))
			     txblklen = (word) intell(h_long2(rxbuf));
			  else
			     txblklen >>= 1;
			  if	  (txblklen <=	64) txblklen =	 64;
			  else if (txblklen <= 128) txblklen =	128;
			  else if (txblklen <= 256) txblklen =	256;
			  else if (txblklen <= 512) txblklen =	512;
			  else			    txblklen = 1024;
			  txgoodbytes = 0;
			  txgoodneeded += (word) (txmaxblklen * 2);   /*AGL:23feb93*/
			  if (txgoodneeded > txmaxblklen * 8)	      /*AGL:23feb93*/
			     txgoodneeded = (word) (txmaxblklen * 8); /*AGL:23feb93*/

			  hydra_status(true);
			  hydramsg("Resending from %ld", txpos);
			  message(0,"+HSEND: Resending from offset %ld (newblklen=%u)",
				  txpos,txblklen);
			  if (dos_seek(txfd,txpos,SEEK_SET) < 0L) {
			     message(6,"!HSEND: File seek error");
			     dos_close(txfd);
			     txfd = -1;
			     txpos = -2L;
			     txstate = HTX_EOF;
			     break;
			  }

			  if (txstate != HTX_XWAIT)
			     txstate = HTX_XDATA;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_EOF:
		       if (rxstate == HRX_DATA) {
			  if (intell(h_long1(rxbuf)) < 0L) {
			     hydra_badxfer();
			     win_xyprint(file_win,13,7,"Skipping");
			     win_clreol(file_win);
			     message(2,"+HRECV: Skipping %s",rxfname);
			     rxstate = HRX_FINFO;
			     braindead = h_timer_set(H_BRAINDEAD);
			  }
			  else if (intell(h_long1(rxbuf)) != rxpos) {
			     if (intell(h_long1(rxbuf)) <= rxlastsync) {
				rxtimer = h_timer_reset();
				rxretries = 0;
			     }
			     rxlastsync = intell(h_long1(rxbuf));

			     if (!h_timer_running(rxtimer) ||
				 h_timer_expired(rxtimer,h_timer_get())) {
				if (++rxretries > H_RETRIES) {
				   message(3,"-HRECV: Too many errors");
				   txstate = HTX_DONE;
				   res = XFER_ABORT;
				   break;
				}
				if (rxretries == 1 || rxretries == 4)  /*AGL:14may93*/
				   rxsyncid++;

				rxblklen /= (word) 2;
				i = rxblklen;
				if	(i <=  64) i =	 64;
				else if (i <= 128) i =	128;
				else if (i <= 256) i =	256;
				else if (i <= 512) i =	512;
				else		   i = 1024;
				hydramsg("Bad EOF at %ld - Retry %u",
					  rxpos,rxretries);
				message(0,"-HRECV: Bad EOF at %ld - Retry %u (newblklen=%u)",
					rxpos,rxretries,i);
				h_long1(txbufin) = intell(rxpos);
				h_long2(txbufin) = intell((long) i);
				h_long3(txbufin) = intell(rxsyncid);
				txpkt(3 * ((int) sizeof(long)),HPKT_RPOS);
				rxtimer = h_timer_set(timeout);
			     }
			  }
			  else {
			     rxfsize = rxpos;
			     dos_close(rxfd);
			     rxfd = -1;
			     hydra_cps(false);

			     p = xfer_okay();
			     if (p) {
				win_xyprint(file_win,25,6,"  -> %s",p);
				message(2,"+HRECV: Dup file renamed: %s",p);
			     }

			     if (remote.capabilities) {
				int n;

				kbtransfered = (int) ((rxfsize + 1023L) / 1024L);
				n = ((int) strlen(rxfname)) - 1;
				if (n > 3 && !stricmp(&rxfname[n - 3],".PKT")) {
				   got_bundle = 1;
				   got_mail = 1;
				   extra.nrmail++;
				   extra.kbmail += (word) kbtransfered;
				   ourbbs.nrmail++;
				   ourbbs.kbmail += kbtransfered;
				   show_mail();
				   show_stats();
				   p = "Mailpacket";
				}
				else if (n > 3 && !stricmp(&rxfname[n - 3],".REQ")) {
				   got_request = 1;
				   p = "Request";
				}
				else if (is_arcmail(rxfname,n)) {
				   got_mail = 1;
				   got_arcmail = 1;
				   extra.nrmail++;
				   extra.kbmail += (word) kbtransfered;
				   ourbbs.nrmail++;
				   ourbbs.kbmail += kbtransfered;
				   show_mail();
				   show_stats();
				   p = "ARCmail";
				}
				else {
				   got_mail = 1;
				   extra.nrfiles++;
				   extra.kbfiles += (word) kbtransfered;
				   ourbbs.nrfiles++;
				   ourbbs.kbfiles += kbtransfered;
				   show_mail();
				   show_stats();
				   p = "Attached file";
				}
			     }
			     else p = "Download";

			     hydra_status(false);
			     win_print(file_win," (%s)",p);
			     message(2,"+Rcvd-H %s %s",p,rxfname);
			     rxstate = HRX_FINFO;
			     braindead = h_timer_set(H_BRAINDEAD);
			  }/*skip/badeof/eof*/
		       }/*rxstate==HRX_DATA*/

		       if (rxstate == HRX_FINFO)
			  txpkt(0,HPKT_EOFACK);
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_EOFACK:
		       if (txstate == HTX_EOF || txstate == HTX_EOFACK) {  /*AGL:06jan94*/
			  braindead = h_timer_set(H_BRAINDEAD);
			  if (txfd >= 0) {
			     txfsize = txpos;
			     dos_close(txfd);
			     hydra_cps(true);
			     return (XFER_OK);
			  }
			  else
			     return (XFER_SKIP);
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_IDLE:
		       if (txstate == HTX_XWAIT) {
			  hdxlink = false;
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_XDATA;
		       }
		       else if (txstate >= HTX_FINFO && txstate < HTX_REND)
			  braindead = h_timer_set(H_BRAINDEAD);
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_END:
		       /* special for chat, other side wants to quit */
		       if (chattimer > 0L && txstate == HTX_REND) {
			  chattimer = -3L;
			  break;
		       }

		       if (txstate == HTX_END || txstate == HTX_ENDACK) {
			  txpkt(0,HPKT_END);
			  txpkt(0,HPKT_END);
			  txpkt(0,HPKT_END);
			  message(1,"+HYDRA: Completed");
			  txstate = HTX_DONE;
			  res = XFER_OK;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_DEVDATA:
		       if (devrxid != intell(h_long1(rxbuf))) {
			  hydra_devrecv();
			  devrxid = intell(h_long1(rxbuf));
		       }
		       h_long1(txbufin) = h_long1(rxbuf);	/*AGL:10feb93*/
		       txpkt((int) sizeof (long),HPKT_DEVDACK);
		       break;

		  /*---------------------------------------------------------*/
		  case HPKT_DEVDACK:
		       if (devtxstate && (devtxid == intell(h_long1(rxbuf)))) {
			  devtxtimer = h_timer_reset();
			  devtxstate = HTD_DONE;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  default:  /* unknown packet types: IGNORE, no error! */
		       break;

		  /*---------------------------------------------------------*/
	   }/*switch(pkttype)*/

	   /*----------------------------------------------------------------*/
	   switch (txstate) {
		  /*---------------------------------------------------------*/
		  case HTX_START:
		  case HTX_SWAIT:
		       if (rxstate == HRX_FINFO) {
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_INIT;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_RINIT:
		       if (rxstate == HRX_FINFO) {
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_FINFO;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_XDATA:
		       if (rxstate && hdxlink) {
			  hydramsg(hdxmsg);
			  message(3,"*HYDRA: %s",hdxmsg);
			  hydra_devsend("MSG",(byte *) hdxmsg,(word) strlen(hdxmsg));

			  txtimer = h_timer_set(H_IDLE);
			  txstate = HTX_XWAIT;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_XWAIT:
		       if (!rxstate) {
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_XDATA;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  case HTX_REND:
		       /* special for chat, braindead will protect */
		       if (chattimer > 0L) break;
		       if (chattimer == 0L) chattimer = -3L;

		       if (!rxstate && !devtxstate) {
			  txtimer = h_timer_reset();
			  txretries = 0;
			  txstate = HTX_END;
		       }
		       break;

		  /*---------------------------------------------------------*/
		  default:  /* any other state - nothing to do */
		       break;

		  /*---------------------------------------------------------*/
	   }/*switch(txstate)*/
	} while (txstate);

	if (txfd >= 0)
	   dos_close(txfd);
	hydra_badxfer();

	if (res == XFER_ABORT) {
	   com_dump();
	   if (carrier()) {
	      h_timer t;

	      com_putblock((byte *) abortstr,(word) strlen(abortstr));
	      t = h_timer_set(2);
	      while (!h_timer_expired(t,h_timer_get()))
		    if (com_getbyte() == H_DLE && carrier()) t = h_timer_set(2);
	      if (carrier())
		 com_flush();
	      else {
		 com_dump();
		 com_purge();
	      }
	   }
	   else
	      com_purge();
	}
	else if (!remote.capabilities) {
	   h_timer t = h_timer_set(2);

	   while (!h_timer_expired(t,h_timer_get()))
		 if (com_getbyte() == H_DLE && carrier()) t = h_timer_set(2);
	   com_flush();
	}
	else if (batchesdone > 1)
	   com_flush();

	return (res);
}/*hydra()*/


/*---------------------------------------------------------------------------*/
int send_hydra (char *fname, char *alias, int doafter, int fsent, int wazoo)
{
	int fd;
	int res;
	int n;
	char *p;

	wazoo = wazoo;		/* dummy to shut up compiler warning */

	if (fsent < 0) {
	   res = hydra(NULL,NULL);
	   return (res);
	}
	else {
	   res = hydra(fname,alias);
	   if (res != XFER_OK) return (res);

	   if (remote.capabilities) {
	      kbtransfered = (int) ((txfsize + 1023L) / 1024L);
	      p = alias ? alias : fname;
	      n = (int) strlen(p) - 1;
	      if (n > 3 && (!stricmp(&p[n-3],".PKT") || is_arcmail(p,n))) {
		 remote.nrmail++;
		 remote.kbmail += kbtransfered;
	      }
	      else {
		 remote.nrfiles++;
		 remote.kbfiles += kbtransfered;
	      }
	      show_mail();
	   }

	   switch (doafter) {
		  case '-':  /*DELETE_AFTER*/
		       if (unlink(fname))
			  message(6,"!HSEND: Could not delete %s",fname);
		       else
			  message(2,"+Sent-H deleted %s",fname);
		       break;

		  case '#':  /*TRUNC_AFTER*/
		       if ((fd = dos_sopen(fname,O_WRONLY|O_CREAT|O_TRUNC,DENY_ALL)) < 0) {
			  message(6,"!HSEND: Error truncating %s",fname);
			  break;
		       }
		       dos_close(fd);
		       message(2,"+Sent-H truncated %s",fname);
		       break;

		  default:
		       message(2,"+Sent-H %s",fname);
		       break;
	   }

	   return (XFER_OK);
	}
}/*send_hydra()*/


/* end of hydra.c */
