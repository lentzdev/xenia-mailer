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


#include "zmodem.h"


static long	zs_bdd; 		/* brain dead or damaged */
static FILE    *zs_fp;
static char    *txbuf;
static long	offset;
static long	lastsync;
static int	synccount;
static word	txmaxblklen;
#if ZCOMP
static int	special;
#endif
static struct stat f;


				/* Until the rest of Xenia is cleaned up ;-( */
int send_Zmodem (char *fname, char *alias, int doafter, int fsent, int wazoo)
{
	int fd;
	int res;
	int n;
	char *p;

	switch (((fsent < 0) ? ZFIN : ZFILE)) {
	       case ZFIN:
		    res = zsend(ZFIN,NULL,NULL,wazoo);
		    break;

	       case ZFILE:
		    res = zsend(ZFILE,fname,alias,wazoo);
		    if (res != XFER_OK) break;

		    if (remote.capabilities) {
		       p = alias ? alias : fname;
		       n = (int) strlen(p) - 1;
		       if (n > 3 && (!stricmp(&p[n-3],".PKT") || is_arcmail(p,n))) {
			  remote.nrmail++;
			  remote.kbmail+= kbtransfered;
		       }
		       else {
			  remote.nrfiles++;
			  remote.kbfiles+= kbtransfered;
		       }
		       show_mail();
		    }

		    switch (doafter) {
			   case DELETE_AFTER:
				if (unlink(fname))
				   message(6,"!Could not delete %s",fname);
				else
				   message(1,"+Sent-Z deleted %s",fname);
				break;
			   case TRUNC_AFTER:
				if ((fd = dos_sopen(fname,O_WRONLY|O_CREAT|O_TRUNC,DENY_ALL)) < 0) {
				   message(6,"!Error truncating %s",fname);
				   break;
				}
				dos_close(fd);
				message(1,"+Sent-Z truncated %s",fname);
				break;
			   default:
				message(1,"+Sent-Z %s",fname);
				break;
		    }
		    break;
	}

	return (res);
}/*send_zmodem()*/


/*---------------------------------------------------------------------------*/
int zsend(int function,char *parm, char *alias, int wazoo)
				      /* main driver (returns ABORT,SKIP,OK) */
/*t function;			   /+ function: xfer ZFILE, ZCOMMAND or ZFIN */
/*ar *parm;			   /+ file or command to transfer (or NULL)  */
/*ar *alias;			   /+ NULL || filename to send to other side */
/*t wazoo;			   /+ Non-zero for WaZoo sessions	     */
{
	register int c;
	register char *q;
	char fpath[PATHLEN], fname[NAMELEN];

	crc32t = 0;		       /* Assume CRC-16 transfer (no CRC-32) */

	directzap = (remote.capabilities &&
		     (ourbbs.capabilities & EMSI_DZA) &&
		     (remote.capabilities & EMSI_DZA)) ? 1 : 0;

	filewincls("ZMODEM Transmit",3);
	switch (function) {
	       /* case ZCOMMAND:			 not implemented yet */
	       default:
		    return (XFER_ABORT);

	       case ZFILE:
		    if ((zs_fp = sfopen(parm,"rb",DENY_NONE)) == NULL) {
		       filemsg("Can't open file");
		       message(6,"-ZSEND: Unable to open %s",strupr(parm));
		       return (XFER_SKIP);
		    }

		    if ((c = zs_rqinit()) == ZCHALLENGE)
		       c = zs_challenge();

		    if (c==ZRINIT || c==ZCOMPL) {
		       rxflags	= ((word)rxhdr[ZF0])&0xff;
#if ZCOMP
		       special	= (((word)rxhdr[ZF1]) >> 5) & 7;
#endif
		       rxbuflen = ((((word)rxhdr[ZP1])&0xff) << 8) | (((word)rxhdr[ZP0])&0xff);
		       /*if (!isdnlink || !arqlink)*/
			  crc32t = rxflags & CANFC32;

		       txbuf = (char *) malloc(Z_BUFLEN+16);
		       if (!txbuf)
			  errl_exit(2,false,"Can't allocate ZSEND buffer");

		       /* zs_sinit();			 not implemented yet */

#if 0 /*OS2*/
		       splitfn(fpath, fname, (alias != NULL) ? alias : parm);
		       MakeShortName(fname, txbuf);
#else
		       splitfn(fpath, txbuf, (alias != NULL) ? alias : parm);
		       strlwr(txbuf);
#endif
		       q = &txbuf[strlen(txbuf) + 1];
		       getstat(parm,&f);
		       sprint(q, "%lu %lo %o",
			      f.st_size, f.st_mtime, (word) f.st_mode);
		       kbtransfered= (int) ((f.st_size+1023L)/1024L);

		       if (alias != NULL) strupr(alias);
		       message(2,"*ZSEND%s %s%s%s (%ldb), %d min.",
			       crc32t ? "/32" : "", strupr(parm),
			       alias != NULL ? " -> " : "", alias != NULL ? alias : "",
			       f.st_size, (int)(f.st_size*10/line_speed+27)/54);
#if ZCOMP
		       if (special) {
			  special = zc_sinit(special);
			  if (special)
			     message(0,"+Special Delivery [Mach %d]",special);
		       }
#endif

		       win_xyprint(file_win,30,1,"CRC-%d",crc32t ? 32 : 16);
#if ZCOMP
		       if (special)
			  win_print(file_win," Mach %d",special);
		       else
#endif
#if 0
			  win_xyputs(file_win,31,7,"---");
#endif
		       splitfn(fpath,fname,parm);
		       win_xyputs(file_win,14,2,fname);
		       if (alias != NULL)
			  win_print(file_win,"  ->  %s",alias);

		       /* zs_freecnt(); 		 not implemented yet */

		       c = zs_file(2+strlen(txbuf)+strlen(q));
		       if (c == ZRPOS) {
			  if (offset == f.st_size) {
			     filemsg("They already have file");
			     message(0,"+They already have %s",parm);
			  }
			  else if (offset) {
			     filemsg("Transmitting from offset %ld",offset);
			     message(0,"+Transmitting from offset %ld (%d min. to go)",
				     offset,(int)((f.st_size-offset)*10/line_speed+27)/54);
			  }
			  win_xyprint(file_win,14,5,"%8ld",f.st_size);
			  xfertime(f.st_size - txpos);
			  throughput(0,0L);
			  c = zs_fdata(wazoo);
		       }
#if ZCOMP
		       zc_sinit(0);
#endif
		       free(txbuf);
		    } /* if (ZRINIT||ZCOMPL) */

		    fclose(zs_fp);

		    if (c == OK)
		       return (XFER_OK);
		    if (c == ZSKIP) {
		       filemsg("Skip requested");
		       message(0,"+Skipping %s",parm);
		       return (XFER_SKIP);
		    }

		    filemsg(frametypes[c+FTOFFSET]);
		    message(3,"!ZSEND: %s",frametypes[c+FTOFFSET]);
		    com_dump(); com_purge();
		    if (carrier())
		       send_can();
		    return (XFER_ABORT);

	       case ZFIN:
		    win_xyputs(file_win,14,2,"End of Batch");
		    if ((c = zs_rqinit()) == ZCHALLENGE)
		       c = zs_challenge();
		    message(1,"*ZSEND: End of Batch");
		    if (c==ZRINIT || c==ZCOMPL)
		       zs_fin();
		    if (!carrier()) {
		       com_dump(); com_purge();
		    }
		    return (XFER_OK);
	}
} /* zsend() */


/*---------------------------------------------------------------------------*/
static int zs_rqinit()					     /* send ZRQINIT */
{
	register int c = 0;
	register int retries = Z_RETRIES;

	zs_bdd = timerset(Z_BDD);

	do {
	   if (c!=BADHEX && c!=HDRCRC) {
	      z_puts("rz\r");
	      z_longtohdr(0L);
	      z_puthhdr(ZRQINIT,(byte *)txhdr);
	   }
again:	   if (timeup(zs_bdd)) return (BDD);
	   c = z_gethdr(rxhdr);
	   com_purge();
	   switch (c) {
		  case BADHEX:
		  case HDRCRC:
		       z_longtohdr(0L);
		       z_puthhdr(ZNAK,(byte *)txhdr);
		  case TIMEOUT: 		/* they didn't say anything */
		       if (retries == Z_RETRIES)	/* Ah, only 1st try */
			  goto enddo;
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		       break;
		  case RCDO:			/* BAD stuff */
		  case ZCAN:
		  case ZABORT:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		  case ZRINIT:			/* GOOD stuff */
		  case ZCHALLENGE:
		  case ZCOMPL:
		       return (c);
		  default:	    /* something, but no good - keep looking */
		       goto again;
	   }
	   zs_error(ZRQINIT,retries,c);
enddo:	   ;
	} while (--retries >= 0);

	return (ERROR);
} /* zs_rqinit() */


/*---------------------------------------------------------------------------*/
static int zs_challenge()			       /* process ZCHALLENGE */
{
	register int c = 0;
	register int retries = Z_RETRIES;

	zs_bdd = timerset(Z_BDD);

	do {
	   if (c!=BADHEX && c!=HDRCRC) {
	      z_longtohdr(rxpos);
	      z_puthhdr(ZACK,(byte *)txhdr);
	   }
again:	   if (timeup(zs_bdd)) return (BDD);
	   c = z_gethdr(rxhdr);
	   com_purge();
	   switch (c) {
		  case BADHEX:
		  case HDRCRC:
		       z_longtohdr(0L);
		       z_puthhdr(ZNAK,(byte *)txhdr);
		  case TIMEOUT: 		/* they didn't say anything */
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		  case ZCHALLENGE:		/* they timed out...	    */
		       break;
		  case RCDO:			/* BAD stuff */
		  case ZCAN:
		  case ZABORT:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		  case ZRINIT:			/* GOOD stuff */
		       return (c);
		  default:	    /* something, but no good - keep looking */
		       goto again;
	   }
	   zs_error(ZCHALLENGE,retries,c);
	} while (--retries >= 0);

	return (ERROR);
} /* zs_challenge() */


/*---------------------------------------------------------------------------*/
static int zs_file(int blen)				       /* send ZFILE */
{
	register int c = 0;
	register int retries = Z_RETRIES;
	register long crc;

	zs_bdd = timerset(Z_BDD);

	do {
	   if (c!=BADHEX && c!=HDRCRC) {
	      com_purge();
	      txhdr[ZF0] = LZCONV;
	      txhdr[ZF1] = LZMANAG;
#if ZCOMP
	      if (special)
		 txhdr[ZF2] = special << 5;
	      else
#endif
		 txhdr[ZF2] = LZTRANS;
	      txhdr[ZF3] = LZEXT;
	      z_putbhdr(ZFILE,(byte *)txhdr);
	      z_putdata((byte *)txbuf,blen,ZCRCW);
	   }
again:	   if (timeup(zs_bdd)) return (BDD);
	   switch (c=z_gethdr(rxhdr)) {
		  case BADHEX:
		  case HDRCRC:
		       z_longtohdr(0L);
		       z_puthhdr(ZNAK,(byte *)txhdr);
		  case TIMEOUT: 		/* they didn't say anything */
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		  case ZCOMPL:
		       break;
		  case ZRINIT:			/* They didn't get it yet?? */
		       if (retries == Z_RETRIES) {     /* We both were fast */
			  retries--;
			  goto again;
		       }
		       break;
		  case ZCRC:			/* Receiver wants to check */
		       crc = 0xffffffffl;
		       while (((c=getc(zs_fp)) != EOF) && --rxpos)
			     crc = z_updcrc32(((word) c)&0xff,crc);
		       if (ferror(zs_fp)) return (ERRFILE);
		       rewind(zs_fp);
		       crc = ~crc;
		       z_longtohdr(crc);
		       z_putbhdr(ZCRC,(byte *)txhdr);
		       goto again;
		  case ZFIN:			/* BAD stuff */
		  case RCDO:
		  case ZCAN:
		  case ZABORT:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		       return (c);
		  case ZRPOS:			/* GOOD stuff */
		       if (rxpos && fseek(zs_fp,rxpos,0))
			  return (ERRFILE);
		       offset = txpos = rxpos;
		       lastsync = -1L;
		       synccount = Z_RETRIES;
		       return (c);
		  case ZSKIP:
		       return (c);
		  default:	     /* something, but no good - keep looking */
		       goto again;
	   }
	   zs_error(ZFILE,retries,c);
	} while (--retries >= 0);

	return (ERROR);
} /* zs_file() */


/*---------------------------------------------------------------------------*/
static int zs_fdata(int wazoo)				    /* send filedata */
{
	register int c, e;
	int newcnt, blklen, goodblks=1, goodneeded=4;
	word garbage;
	int incount, endfile;
	char *p;
	int startsub;

	if (cur_speed < 300)
	   txmaxblklen = 128;
	if (cur_speed >= 9600UL)
	   txmaxblklen = Z_MAXBLKLEN;
	else {
	   txmaxblklen = (word) (cur_speed / 300 * 256);
	   if (txmaxblklen > Z_MAXBLKLEN) txmaxblklen = Z_MAXBLKLEN; /* macro */
	}

	if (remote.capabilities && txmaxblklen > KSIZE &&
	    (!(remote.capabilities&ZED_ZAPPER) || !(ourbbs.capabilities&ZED_ZAPPER)) &&
	    (!(remote.capabilities&EMSI_DZA) || !(ourbbs.capabilities&EMSI_DZA)))
	   txmaxblklen = KSIZE;

	if (rxbuflen && txmaxblklen > rxbuflen)
	   txmaxblklen = (word) rxbuflen;
#if ZCOMP
	zc_maxblklen = (rxbuflen && Z_MAXBLKLEN > rxbuflen) ? rxbuflen : Z_MAXBLKLEN;
#endif
	p = getenv("ZBLK");	  /* Fix for Polish lines, start at 32 or so */
	startsub = ((p!=NULL) ? atoi(p) : 0);
	if (startsub > 0) {
	   if (startsub < 32) startsub = 32;
	   blklen = ((startsub < txmaxblklen) ? startsub : txmaxblklen);
	}
	else if (isdnlink || arqlink)
	   blklen = txmaxblklen;
	else
	   blklen = ((txmaxblklen < 512) ? txmaxblklen : 512);

	com_purge();

	if (com_scan() && (((c=(com_getbyte()&0x7f))==ZPAD) || (c==CAN))) {
waitack:   c = zs_sync(0);
see:	   switch (c) {
		  case ZACK:
		       if (zmodem_txwindow && (txpos >= (rxpos + zmodem_txwindow))) /*AGL:16jul95*/
			  goto waitack; 			/*AGL:16jul95*/
		       break;
		  case ZRPOS:
		       if (synccount < Z_RETRIES-2) {	 /* give'm 2 chances */
			  blklen = ((blklen >> 2) > 32) ? (blklen >> 2) : 32;
			  goodneeded = ((goodneeded << 1) > 16) ? 16 : (goodneeded << 1);
		       }
		       goodblks = 0;
#if ZCOMP
		       if (special >= 2) {	       /* timeout prevention */
			  win_xyputs(file_win,31,7,"Synchronizing");
			  com_putbyte(XON);
			  z_longtohdr(txpos);
			  z_putbhdr(ZDATA,(byte *)txhdr);
			  z_putdata((byte *)txbuf,0,ZCRCW);
			  goto waitack;
		       }
#endif
		       break;
		  case ZSKIP:
		  default:
		       return (c);
	   }
	   com_putbyte(XON);
	}

#if ZCOMP
	if (special >= 2 && blklen > 2048)
	   blklen = 2048;
	if (special)
	   lza_init();	     /* Compressor reset at start of a new DATAframe */
#endif
	garbage = Z_GARBAGE;
	newcnt = rxbuflen;
	z_longtohdr(txpos);
	z_putbhdr(ZDATA,(byte *)txhdr);

	do {
sendwait:  if (keyabort())
	      return (SYSABORT);
	   if (!carrier())
	      return (RCDO);

#if ZCOMP
	   if (special) {
	      if ((c = zc_fread((byte *)txbuf,blklen,
				zs_fp,&incount,&endfile)) == 0)
		 return (ERRFILE);
	   }
	   else
#endif
	      {
	      com_disk(true);
	      incount = c = fread(txbuf,1,blklen,zs_fp);
	      com_disk(false);
	      if (incount < 0)
		 return (ERRFILE);
	      endfile = ((incount<blklen) ? 1 : 0);  /* shortcount means EOF */
	   }

	   if (endfile)  /* our own EOF flag, gets set during read operation */
	      e = ZCRCE;
	   else if (!garbage || (rxbuflen && (newcnt-=c) <= 0)) {
	      e = ZCRCW;
#if ZCOMP
	      if (special) {	      /* Special handling for ZCRCW support  */
		 c = lza_flush();     /* Finish LZA ands set right buf count */
		 win_xyputs(file_win,31,7,"Synchronizing");
	      }
#endif
	   }
	   else if (zmodem_txwindow)				/*AGL:16jul95*/
	      e = ZCRCQ;					/*AGL:16jul95*/
	   else 						/*AGL:16jul95*/
	      e = ZCRCG;

	   z_putdata((byte *)txbuf,c,e);
	   txpos += incount;
#if ZCOMP
	   if (special) {
	      c--;	       /* Don't show the extra s.d. byte to the user */
	      win_xyprint(file_win,31,7,"%d",(int) (*txbuf));
	      win_clreol(file_win);
	   }
#endif
	   win_xyprint(file_win,14,6,"%8ld",txpos);
	   win_xyprint(file_win,14,7,"%8d",(int) c);
	   xfertime(f.st_size - txpos);
	   throughput(2,txpos - offset);

	   if (blklen < txmaxblklen && ++goodblks > goodneeded) {
	      blklen = ((blklen << 1) < txmaxblklen) ? (blklen << 1) : txmaxblklen;
	      goodblks = 1;
	   }

	   if (e == ZCRCW ||
	       (zmodem_txwindow && (txpos >= (rxpos + zmodem_txwindow)))) /*AGL:16jul95*/
	      goto waitack;
	   if (com_scan()) {
	      do {
		 switch (com_getbyte()&0x7f) {
			case CAN:
			case ZPAD:
			     if (!zmodem_txwindow) {		/*AGL:16jul95*/
				filemsg("Byte %ld: Got interruption",txpos);
				message(0,"?Byte %ld: Interruption detected...",
					txpos);
				com_dump();
				z_putdata((byte *)txbuf,0,ZCRCE);
			     }					/*AGL:16jul95*/
			     goto waitack;
			default:
			     if (--garbage > 0) break;
			     filemsg("Byte %ld: Garbage, req rsp",txpos);
			     message(0,"?Byte %ld: Much garbage, response req...",txpos);
			     goto sendwait;
		 }
	      } while (carrier() && com_scan() && !keyabort());
	   }
	} while (e == ZCRCG || e == ZCRCQ);			/*AGL:16jul95*/

	switch (c=zs_sync(1)) {
	       case ZRPOS:
		    goto see;
	       case ZRINIT:
		    throughput(2,txpos - offset);
		    throughput(1,txpos - offset);
		    return (OK);
	       case ZSKIP:
	       default:
		    return (c);
	}
} /* zs_fdata() */


/*---------------------------------------------------------------------------*/
static int zs_sync(int endfile) 	   /* sync with receiver & send ZEOF */
{
	register int c = 0;
	register int retries = Z_RETRIES;

	zs_bdd = timerset(Z_BDD);

	do {
	   if (endfile && (c!=BADHEX && c!=HDRCRC)) {
	      z_longtohdr(txpos);
	      z_putbhdr(ZEOF,(byte *)txhdr);
	   }
again:	   if (timeup(zs_bdd)) return (BDD);
	   c = z_gethdr(rxhdr);
	   if (!zmodem_txwindow)				/*AGL:16jul96*/
	      com_purge();
	   switch (c) {
		  case TIMEOUT: 		/* they didn't say anything */
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		       if (endfile) break;	/* shouldn't do both NAK&EOF */
		  case BADHEX:
		  case HDRCRC:
		       z_longtohdr(0L);
		       z_puthhdr(ZNAK,(byte *)txhdr);
						/* just to wake them up.... */
		       break;
		  case ZFIN:			/* BAD stuff */
		  case RCDO:
		  case ZCAN:
		  case ZABORT:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		       return (c);
		  case ZACK:			/* GOOD stuff (comparitivly) */
		       if (endfile ||
			   (!zmodem_txwindow && txpos != rxpos) ||		      /*AGL:16jul95*/
			   (zmodem_txwindow && (txpos >= (rxpos + zmodem_txwindow)))) /*AGL:16jul95*/
			  goto again;
		       return (c);
		  case ZRPOS:
		       if (rxpos == lastsync) {
			  if (--synccount < 0) return (ERROR);
		       }
		       else {
			  synccount = (Z_RETRIES-1);
		       }
		       rewind(zs_fp);
		       if (fseek(zs_fp,rxpos,0))
			  return (ERRFILE);
		       lastsync = txpos = rxpos;
		       filemsg("Resending from %ld (Sync %d)",
			       txpos,(int) (Z_RETRIES-synccount));
		       message(0,"-Resending from %ld (Sync %d)",
			       txpos,(int) (Z_RETRIES-synccount));
		       com_dump();
		       return (c);
		  case ZRINIT:
		       if (!endfile)
			  break;
		  case ZSKIP:
		       return (c);
		  default:	    /* something, but no good - keep looking */
		       goto again;
	   }
	   filemsg("Byte %ld retry %d: %s",
		   txpos,(int) (Z_RETRIES-retries),frametypes[c+FTOFFSET]);
	   message(0,"-Byte %ld retry %d: %s",
		   txpos,(int) (Z_RETRIES-retries),frametypes[c+FTOFFSET]);
	} while (--retries >= 0);

	return (ERROR);
} /* zs_sync() */


/*---------------------------------------------------------------------------*/
static void zs_fin()						/* send ZFIN */
{
	register int c = 0;
	register int retries = Z_RETRIES;

	zs_bdd = timerset(Z_BDD);

	do {
	   if (c!=BADHEX && c!=HDRCRC) {
	      z_longtohdr(0L);
	      z_puthhdr(ZFIN,(byte *)txhdr);
	   }
again:	   if (timeup(zs_bdd)) return;
	   c = z_gethdr(rxhdr);
	   com_purge();
	   switch (c) {
		  case BADHEX:
		  case HDRCRC:
		       z_longtohdr(0L);
		       z_puthhdr(ZNAK,(byte *)txhdr);
		  case TIMEOUT: 		/* they didn't say anything */
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		       break;
		  case RCDO:			/* BAD stuff */
		  case ZCAN:
		  case ZABORT:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		  case ZFIN:			/* GOOD stuff */
		  case ZRQINIT: 				/*AGL:18apr93*/
		       com_putbyte('O'); com_putbyte('O'); com_flush();
		       return;
		  default:	    /* something, but no good - keep looking */
		       goto again;
	   }
	   zs_error(ZFIN,retries,c);
	} while (--retries >= 9);
} /* zs_fin() */


/*---------------------------------------------------------------------------*/
static void zs_error(int function, int retries, int error)
						    /* print error condition */
{
	filemsg("%s retry %d: %s", frametypes[function+FTOFFSET],
		(int) (Z_RETRIES-retries), frametypes[error+FTOFFSET]);

	message(0,"-ZS_%s retry %d: %s",
		frametypes[function+FTOFFSET]+1,
		(int) (Z_RETRIES-retries),
		frametypes[error+FTOFFSET]);
}


/* end of zsend.c */
