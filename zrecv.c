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


static long	zr_bdd; 		/* brain dead or damaged */
static FILE    *zr_fp;
static char    *rxbuf;
static int	txtype;
static long	diskfree;
static char	attn[ZATTNLEN+1];
static int	zconv, zmanag, ztrans, zext;
static int	binary, eofseen;
#if ZCOMP
static int	special;
#endif
static long	offset, rxbytes;
static long	filesize;


get_Zmodem (void)
{
	int txtype = ZRINIT;

	for (;;) {
	    txtype = zrecv(txtype);

	    if (txtype == ZFIN) return (1);
	    if (txtype == ERROR) return (0);
	}
}


/*---------------------------------------------------------------------------*/
int zrecv (int function)	       /* (returns ZRINIT/ZCOMPL/ZFIN/ERROR) */
						  /* function: ZRINIT/ZCOMPL */
{
	register int c;
	char  rxfname[NAMELEN];
	char *rxpathname;
	long  filetime, rem_zserialnr;
	int   filemode;
	char *p, *q;

	rxbuf = (char *) malloc(Z_BUFLEN+16);
	if (!rxbuf)
	   errl_exit(2,false,"Can't allocate ZRECV buffer");

	txtype = function;
	diskfree = freespace(remote.capabilities ? inbound : download);
	attn[0] = ZPAD; attn[1] = '\0';        /* Reset the Attention string */
#if ZCOMP
	special = zc_rinit(sdmach);  /* Try for highest (wanted) compression */
#endif
	directzap = (remote.capabilities &&
		     (ourbbs.capabilities & EMSI_DZA) &&
		     (remote.capabilities & EMSI_DZA)) ? 1 : 0;


	filewincls("ZMODEM Receive",3);
	switch (c = zr_rinit()) {
	       case ZFILE:
		    binary = (zconv == ZCNL) ? 0 : 1;
#if ZCOMP
		    if (special && ztrans)
		       special = zc_rinit((((word)ztrans) >> 5) & 7);
		    else
		       special = zc_rinit(0);	      /* No special delivery */
#endif
		    for (p=(char *) rxbuf, q=rxfname; *p; p++) {
			if (*q=*p, *p=='\\' || *p==':' || *p=='/')
			   q=rxfname;
			else q++;
		    }
		    *q++='\0';
		    /*strupr(rxfname);*/

		    filesize = filetime = rem_zserialnr = 0L;
		    filemode = 0;
		    p = rxbuf + strlen(rxbuf) + 1;
		    if (*p)
		       sscanf(p,"%ld %lo %o %lo",
			      &filesize,&filetime,&filemode,&rem_zserialnr);
		    if (rem_zserialnr)
		       message(0,"=Remote ZSN: %ld",rem_zserialnr);

		    win_xyprint(file_win,29,1,"CRC-%d%s",
				crc32r ? 32 : 16, binary ? "" : " ASCII");
#if ZCOMP
		    if (special)
		       win_print(file_win," Mach %d",special);
		    else
#endif
#if 0
		       win_xyputs(file_win,31,7,"---");
#endif
		    win_xyputs(file_win,14,2,rxfname);
		    win_xyprint(file_win,14,5,"%8ld",filesize);

		    rxpathname = xfer_init(rxfname,filesize,filetime);

		    if (rxpathname) {			    /* Not skipping? */
		       if (fexist(rxpathname)) {
			  if ((zr_fp = sfopen(rxpathname,"r+b",DENY_ALL)) == NULL) {
			     message(6,"!ZRECV: Unable to re-open %s",rxpathname);
			     c = ERRFILE;
			     break;
			  }
		       }
		       else {
			  if ((zr_fp = sfopen(rxpathname,"wb",DENY_ALL)) == NULL) {
			     message(6,"!ZRECV: Unable to create %s",rxpathname);
			     c = ERRFILE;
			     break;
			  }
		       }
		    }
		    else {
		       filemsg("Already have file");
		       message(1,"+ZRECV: Already have %s",rxfname);
		       zr_fp = NULL;
		       rxbytes = filesize;
		       c = zr_fdata();
		       if (c == OK) txtype = ZRINIT;
		       break;
		    }

		    message(2,"*ZRECV%s %s %s(%ld bytes), %d min.",
			    crc32r ? "/32" : "", rxfname,
			    binary ? "" : "ASCII ", filesize,
			    (int)(filesize*10/line_speed+27)/54);
#if ZCOMP
		    if (special)
		       message(0,"+Special Delivery [Mach %d]",special);
#endif

		    fseek(zr_fp,0L,2);	/* seek to end */
		    rxbytes = offset = ftell(zr_fp);

		    if ((filesize - offset) + 10240L > diskfree) {
		       message(6,"!ZRECV: %s not enough diskspace: %ld > %ld",
			       rxfname, (filesize - offset) + 10240L, diskfree);
		       fclose(zr_fp);
		       c = ZFERR;
		       break;
		    }

		    if (offset) {
		       filemsg("Resuming from offset %ld",offset);
		       message(0,"+Resuming from offset %ld (%d min. to go)",
			       offset,(int)((filesize-offset)*10/line_speed+27)/54);
		    }
		    xfertime(filesize - rxbytes);
		    throughput(0,0L);
		    c = zr_fdata();
		    if (fclose(zr_fp) == EOF) {
		       c = ZFERR;
		       break;
		    }

		    if (c == OK) {
		       p = xfer_okay();
		       if (p) {
			  win_xyprint(file_win,28,2,"->  %s",p);
			  message(2,"+Dup file renamed: %s",p);
		       }
		       throughput(2,rxbytes - offset);
		       throughput(1,rxbytes - offset);
		       kbtransfered= (int) ((rxbytes+1023L) / 1024L);

		       if (remote.capabilities) {
			  int n;

			  n = strlen(rxfname) - 1;
			  if (n > 3 && !stricmp(&rxfname[n - 3],".PKT")) {
			     got_bundle = 1;
			     got_mail = 1;
			     extra.nrmail++;
			     extra.kbmail+= (word) kbtransfered;
			     ourbbs.nrmail++;
			     ourbbs.kbmail+= kbtransfered;
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
			     extra.kbmail+= (word) kbtransfered;
			     ourbbs.nrmail++;
			     ourbbs.kbmail+= kbtransfered;
			     show_mail();
			     show_stats();
			     p = "ARCmail";
			  }
			  else {
			     got_mail = 1;
			     extra.nrfiles++;
			     extra.kbfiles+= (word) kbtransfered;
			     ourbbs.nrfiles++;
			     ourbbs.kbfiles+= kbtransfered;
			     show_mail();
			     show_stats();
			     p = "Attached file";
			  }
		       }
		       else p = "Download";

		       message(1,"+Rcvd-Z %s %s",p,rxfname);
		       txtype = ZRINIT;
		       break;
		    }

		    /* transfer didn't go ok */
		    if (binary && filesize && xfer_bad()) {
		       message(0,"+Bad xfer recovery-info saved");
		       break;
		    }

		    /* not even a little 'bit' - tssk */
		    xfer_del();
		    message(0,"-Bad xfer - file deleted");
		    break;

	       case ZCOMMAND:
		    if (strlen(rxbuf) > 255) {
		       filemsg("ZCOMMAND too long....");
		       message(2,"-ZRECV: ZCOMMAND too long....");
		    }
		    else {
		       win_xyputs(file_win,14,2,rxbuf);
		       filemsg("ZCOMMAND");
		       message(2,"+Received ZCOMMAND '%s'",rxbuf);
		    }
		    txtype = ZCOMPL;
		    c = OK;
		    break;

	       case ZFIN:
		    win_xyputs(file_win,14,2,"End of Batch");
		    message(1,"*ZRECV: End of Batch");
		    zr_fin();
		    txtype = ZFIN;
		    c = OK;
		    break;

	       default:
		    break;
	}

#if ZCOMP
	zc_rinit(0);
#endif
	free(rxbuf);

	if (!carrier()) {
	   com_dump(); com_purge();
	}

	if (c == ZSKIP) {
	   filemsg("Skip requested");
	   message(0,"+Skipping %s",rxfname);
	   return (ZRINIT);
	}

	if (c != OK) {
	   filemsg(frametypes[c+FTOFFSET]);
	   message(3,"!ZRECV: %s",frametypes[c+FTOFFSET]);
	   com_dump(); com_purge();
	   if (carrier()) {
	      long t;

	      z_puts(attn);
	      send_can();
	      t = timerset(20);
	      while (carrier() && !timeup(t))
		    if (com_getbyte() == ZDLE) t = timerset(20);
	      send_can();
	   }
	   return (ERROR);
	}

	return (txtype);
}/*zrecv()*/


/*---------------------------------------------------------------------------*/
static int zr_rinit()					      /* send ZRINIT */
{
	register int c;
	register int retries=Z_RETRIES;

	zr_bdd = timerset(Z_BDD);

	do {
	   z_longtohdr(0L);
	   if (txtype == ZRINIT) {
	      /*if (isdnlink && arqlink) txhdr[ZF0] = ZRFLAGS & ~CANFC32;
	      else */			 txhdr[ZF0] = ZRFLAGS;
#if ZCOMP
	      if (special)
		 txhdr[ZF1] = special << 5;
#endif
	   }
	   z_puthhdr(txtype,(byte *)txhdr);
	   if (txtype != ZRINIT)
	      txtype = ZRINIT;
again:	   if (timeup(zr_bdd)) return (BDD);
	   c = z_gethdr(rxhdr);
see:	   switch (c) {
		  case TIMEOUT: 		/* they didn't say anything */
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		       break;
		  case ZRQINIT: 		/* didn't see our ZRINIT yet */
		       /* zs_challenge();	       not implemented yet   */
		       if (retries == Z_RETRIES)
			  goto skip;
		       break;
		  case BADHEX:
		  case BADZDLE:
		  case HDRCRC:
		  case DATACRC:
		  case DATALONG:
		       z_longtohdr(0L);
		       z_puthhdr(ZNAK,(byte *)txhdr);
		       if (retries > 0) {
			  zr_error(ZRINIT,retries,c);
			  retries--;
			  goto again;
		       }
		       break;
		  case RCDO:			/* BAD stuff */
		  case ZCAN:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		       return (c);
		  case ZFILE:			/* GOOD stuff */
		       zconv  = ((word)rxhdr[ZF0])&0xff;
		       zmanag = ((word)rxhdr[ZF1])&0xff;
		       ztrans = ((word)rxhdr[ZF2])&0xff;
		       zext   = ((word)rxhdr[ZF3])&0xff;
		       if ((c = z_getdata(rxbuf,Z_BUFLEN)) == GOTCRCW)
			  return (ZFILE);
		       goto see;
		  case ZSINIT:
		       if ((c = z_getdata(attn,ZATTNLEN)) == GOTCRCW) {
			  z_longtohdr(1L);
			  z_puthhdr(ZACK,(byte *)txhdr);
			  goto again;
		       }
		       goto see;
		  case ZFREECNT:
		       z_longtohdr(diskfree);
		       z_puthhdr(ZACK,(byte *)txhdr);
		       goto again;
		  case ZCOMMAND:
		       if ((c = z_getdata(rxbuf,Z_BUFLEN)) == GOTCRCW)
			  return (ZCOMMAND);
		       goto see;
		  case ZFIN:
		       return (c);
		  default:	    /* something, but no good - keep looking */
		       goto again;
	   }
	   zr_error(ZRINIT,retries,c);
skip:;	} while (--retries >= 0);

	return (ERROR);
} /* zr_rinit() */


/*---------------------------------------------------------------------------*/
static int zr_fdata()					    /* send filedata */
{
	register int c;
	register int retries=Z_RETRIES;

	eofseen = 0;
	zr_bdd = timerset(Z_BDD);

	do {
	   com_purge();
	   z_longtohdr(rxbytes);
	   z_puthhdr(ZRPOS,(byte *)txhdr);
again:	   if (timeup(zr_bdd)) return (BDD);
	   switch (c=z_gethdr(rxhdr)) {
		  case ZDATA:
		       if (rxpos != rxbytes) {
			  filemsg("Bad file pos %ld/%ld (retry %d)",
				  rxbytes,rxpos,(int) (Z_RETRIES-retries));
			  message(0,"-Bad file pos %ld/%ld (retry %d)",
				  rxbytes,rxpos,(int) (Z_RETRIES-retries));
			  z_puts(attn);
			  break;
		       }
#if ZCOMP
	   if (special)
	      dlza_init();    /* Init decompressor at start of new DATAframe */
#endif

moredata:	       switch (c = z_getdata(rxbuf,Z_BUFLEN)) {
			      case ZCAN:
			      case RCDO:
				   return (c);
			      default:
				   break;
			      case GOTCRCW:
				   if (rxcount != 0)
				      retries = Z_RETRIES;
				   zr_bdd = timerset(Z_BDD);
				   if ((c = zr_save()) != OK)
				      return (c);
				   z_longtohdr(rxbytes);
				   z_puthhdr(ZACK,(byte *)txhdr);
				   com_putbyte(XON);
				   goto again;
			      case GOTCRCQ:
				   retries = Z_RETRIES;
				   zr_bdd = timerset(Z_BDD);
				   if ((c = zr_save()) != OK)
				      return (c);
#if ZCOMP
				   if (special)
				      dlza_init();
#endif
				   z_longtohdr(rxbytes);
				   z_puthhdr(ZACK,(byte *)txhdr);
				   goto moredata;
			      case GOTCRCG:
				   retries = Z_RETRIES;
				   zr_bdd = timerset(Z_BDD);
				   if ((c = zr_save()) != OK)
				      return (c);
				   goto moredata;
			      case GOTCRCE:
				   retries = Z_RETRIES;
				   zr_bdd = timerset(Z_BDD);
				   if ((c = zr_save()) != OK)
				      return (c);
				   goto again;
		       } /* case ZDATA: switch */
		       /* fallthrough */
		  case TIMEOUT: 		/* they didn't say anything */
		  case ZNAK:			/* didn't get our msg clear */
		  case HDRZCRCW:
		  case GARBAGE: 		/* even double-dutch to me! */
		  case BADHEX:
		  case BADZDLE:
		  case HDRCRC:
		  case ZFILE:
		       if (c == ZFILE) z_getdata(rxbuf,Z_BUFLEN);
		       else	       z_puts(attn);
		       filemsg("Byte %ld retry %d: %s",
			       rxbytes,(int) (Z_RETRIES-retries),frametypes[c+FTOFFSET]);
		       message(0,"-Byte %ld retry %d: %s",
			       rxbytes,(int) (Z_RETRIES-retries),frametypes[c+FTOFFSET]);
		       break;
		  case ZFIN:			/* BAD stuff */
		  case RCDO:
		  case ZCAN:
		  case SYSABORT:
		  case ZFERR:
		  case ERROR:
		       return (c);
		  case ZEOF:			/* GOOD stuff - relatively */
		       if (rxbytes != rxpos)
			  goto again;
		       return (OK);
		  case ZSKIP:
		       return (c);
		  default:	    /* something, but no good - keep looking */
		       goto again;
	   }
	} while (--retries >= 0);

	return (ERROR);
} /* zr_fdata() */



/*---------------------------------------------------------------------------*/
static int zr_save()			   /* save received filedata to disk */
{
	static char lastc;
	register char *p;
	register int   i;

	if (keyabort())
	   return (SYSABORT);

	if (!zr_fp) return (OK);	  /* Already have him, so not opened */

	if (binary) {
#if ZCOMP
	   if (special) {
	      win_setpos(file_win,31,7);
	      if (rxcount) {
		 if ((i = zc_fwrite((byte *)rxbuf,rxcount,zr_fp)) < 0)
		    return (i);
		 rxbytes = ftell(zr_fp);
		 rxcount--;  /* for the user, don't show the extra s.d. byte */
		 win_print(file_win,"%d",(int) (*rxbuf));
		 win_clreol(file_win);
	      }
	      else
		 win_puts(file_win,"Synchronizing");  /* 0-length datasubpkt */
	   }
	   else
#endif
	      {
	      com_disk(true);
	      i = fwrite(rxbuf,rxcount,1,zr_fp);
	      com_disk(false);
	      rxbytes += rxcount;
	   }
	   if (i < 0) return (ZFERR);
	}
	else {
	   if (eofseen) return (OK);
	   for (i=rxcount, p=rxbuf; --i >= 0; ++p) {
	      if (*p == CPMEOF) {
		 eofseen = 1;
		 return (OK);
	      }

	      if (*p == '\n') {
		 if (lastc != '\r' && putc('\r',zr_fp) == EOF)
		    return (ZFERR);
	      }
	      else {
		 if (lastc == '\r' && putc('\n',zr_fp) == EOF)
		    return (ZFERR);
	      }

	      if (putc((lastc = *p),zr_fp) == EOF)
		 return (ZFERR);
	   }
	   rxbytes += rxcount;
	}

	win_xyprint(file_win,14,6,"%8ld",rxbytes);
	win_xyprint(file_win,14,7,"%8d",rxcount);
	xfertime(filesize - rxbytes);
	throughput(2,rxbytes - offset);

	return (OK);
} /* zr_save() */


/*---------------------------------------------------------------------------*/
static void zr_fin()					/* process/send ZFIN */
{
	z_longtohdr(0L);
	z_puthhdr(ZFIN,(byte *)txhdr);
	/* Try to get (but don't wait too long for) Over & Out from xmitter */
	if (z_get7(Z_TIMEOUT) == 'O')
	    z_get7(1); /* get 2nd 'O' */
	return;
} /* zr_fin() */


/*---------------------------------------------------------------------------*/
static void zr_error(int function,int retries,int error)
						    /* print error condition */
{
	filemsg("%s retry %d: %s", frametypes[function+FTOFFSET],
		(int) (Z_RETRIES-retries), frametypes[error+FTOFFSET]);

	message(0,"-ZR_%s retry %d: %s",
		frametypes[function+FTOFFSET]+1,
		(int) (Z_RETRIES-retries),
		frametypes[error+FTOFFSET]);
}


/* end of zrecv.c */
