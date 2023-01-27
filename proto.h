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


#ifdef __PROTO__
# define	PROTO(s) s
#include "stdarg.h"
#else
# define	PROTO(s) ()
#endif

/* xenia.c */
int Term       PROTO((int noexit));
/*
int main       PROTO((int argc, char *argv[]));
*/

/* sealink.c */
int   send_SEA	  PROTO((char *fname, char *alias, int doafter, int fsent, int wazoo));
int   xmtfile	  PROTO((char *name, char *alias));
char *rcvfile	  PROTO((char *path, char *name));
int   send_YOOHOO PROTO((void));
int   send_Hello  PROTO((void));
void  yhs_error   PROTO((int retries, char *errorstr));
int   get_YOOHOO  PROTO((int send_too));
/*static*/ void  ackchk   PROTO((void));
/*static*/ void  sendblk  PROTO((FILE *f, int blknum));
/*static*/ void  shipblk  PROTO((char *blk, int blknum, char hdr));
/*static*/ void  sendack  PROTO((int acknak, int blknum));
/*static*/ char *getblock PROTO((char *buf));

/* xfer.c */
char *rcvfname	PROTO((void));
void  rfn_error PROTO((int retries, char *errorstr));
void  unparse	PROTO((char *to, char *from));
int   batchdown PROTO((void));
void  parse	PROTO((char *to, char *from));
int   xmtfname	PROTO((char *name));
char *nextfile	PROTO((char *file, char *next));
char *batchup	PROTO((char *namelist, char pr));
void  end_batch PROTO((void));

/* script.c */
boolean  isloginchr	 PROTO((char c));
void	 script_outstr	 PROTO((char *s, char *loginname, char *password, WIN_IDX win));
boolean  password_login  PROTO((char *password));
boolean  run_script	 PROTO((SCRIPT *sp, char *loginname, char *password, WIN_IDX win));

/* misc.c */
word  crc_update     PROTO((word crc, word crc_char));
word  crc_finish     PROTO((word crc));
void  tdelay	     PROTO((int tme));
void  hangup	     PROTO((void));
void  alarm	     PROTO((void));
int   keyabort	     PROTO((void));
void  print	     PROTO((char *fmt, ... /*char *args*/));
void  win_print      PROTO((WIN_IDX win, char *fmt, ... /*char *args*/));
void  win_xyprint    PROTO((WIN_IDX win, byte horpos, byte verpos, char *fmt, ... /*char *args*/));
void  win_cyprint    PROTO((WIN_IDX win, byte verpos, char *fmt, ...));
void  splitfn	     PROTO((char *path, char *filename, char *file));
void  maketnam	     PROTO((char *name, char ch, char *out));
char *get_str	     PROTO((char *str, int len));
void  invent_pktname PROTO((char *str));
void  invent_arcname PROTO((char *str));
void  throughput     PROTO((int opt, dword btes));
void  send_can	     PROTO((void));
int   is_arcmail     PROTO((char *p, int n));
int   fexist	     PROTO((char *filename));
word  ztoi	     PROTO((char *str, int len));
int   dayofweek      PROTO((int year, int month, int day));
int   getadress      PROTO((char *str, int *zone, int *net, int *node, int *point));
#ifdef __MSDOS__
void  fmprint	     PROTO((char *buf, char *fmt, void *args));
void  doprint	     PROTO((void (*outp)(char c), char *fmt, va_list args));
#else
void  cdecl fmprint  PROTO((char *buf, char *fmt, void *args));
void  cdecl doprint  PROTO((void (*outp)(char c), char *fmt, va_list args));
#endif
void  sprint	     PROTO((char *buf, char *fmt, ... /*char *args*/));
void  fprint	     PROTO((FILE *f, char *fmt, ... /*char *args*/));
void  message	     PROTO((int level, char *fmt, ... /*char *arg*/));
void  showprod	     PROTO((word prod, word pmaj, word pmin));
void  statusmsg      PROTO((char *fmt, ... /*char *args*/));
void  filemsg	     PROTO((char *fmt, ... /*char *args*/));
char *h_revdate      PROTO((long revstamp ));
void  hydramsg	     PROTO((char *fmt, ...));
void  xfertime	     PROTO((long length));
void  last_set	     PROTO((char **s, char *str));
boolean patimat      PROTO((char *raw, char *pat));
/*static*/ void wputc	PROTO((char c));
/*static*/ void conv	PROTO((dword getal, char *p, int base, int unsign, int blank));
/*static*/ void iprint	PROTO((void (*outp )(char c ), char *fmt, ... /*char *arg*/));
/*static*/ void putSTR	PROTO((char c));
/*static*/ void putFILE PROTO((char c));

/* modem.c */
void	 mdm_putc      PROTO((char c));
void	 mdm_puts      PROTO((char *str));
void	 mdm_reset     PROTO((void));
dword	 mdm_result    PROTO((char *str));
char	*mdm_gets      PROTO((int tenths, boolean show));
dword	 mdm_command   PROTO((char *str, int code, int time1, int time2));
dword	 dail	       PROTO((char *pstring, char *number, char *nodeflags));
dword	 got_caller    PROTO((int forced));
int	 init_modem    PROTO((void));
void	 reset_modem   PROTO((void));
void	 mdm_busy      PROTO((void));
void	 mdm_terminit  PROTO((void));
void	 mdm_aftercall PROTO((void));

/* faxrecv.c */
boolean fax_receive  PROTO((void));
boolean fax_transmit PROTO((void));

/* getnode.c */
void		  nlidx_init	 PROTO((void));
char		 *nlidx_getphone PROTO((int zone, int net, int node, int point, char **phpp, char **flpp));
int		  nodetoinfo	 PROTO((struct _nodeinfo *nodeinfo));
struct _nodeinfo *strtoinfo	 PROTO((char *str));
char		 *get_passw	 PROTO((int zone, int net, int node, int point));
void		  set_alias	 PROTO((int zone, int net, int node, int point));
void		  remap 	 PROTO((int *zone, int *net, int *node, int *point, int *pointnet));
word		  getreqlevel	 PROTO((void));

/* tmailer.c */
int   get_numbers PROTO((int poll));
void  mailout	  PROTO((int poll));
long  sendmail	  PROTO((void));
int   do_mail	  PROTO((int zone, int net, int node, int point));

/* msession.c */
int	recv_session  PROTO((void));
int	startbbs      PROTO((int mailmode));
int	xmt_session   PROTO((void));
int	FTSC_sender   PROTO((int smart));
int	FTSC_receiver PROTO((int smart));
void	do_request    PROTO((void));
int	FTSC_sendmail PROTO((int smart));
int	FTSC_recvmail PROTO((int smart));
int	WaZOO	      PROTO((int call, int emsi));
void	flag_session  PROTO((boolean set));
boolean flag_set      PROTO((int zone, int net, int node, int point));
boolean flag_test     PROTO((int zone, int net, int node, int point));

/* mailer.c */
int   handle_inbound_mail PROTO((int forced));
void  start_unatt	  PROTO((void));
void  unattended	  PROTO((void));
void  receive_exit	  PROTO((void));
void  errl_exit 	  PROTO((int n, boolean online, char *fmt, ...));
long  random_time	  PROTO((int x));
int   bad_call		  PROTO((int bzone, int bnet, int bnode, int bpoint, int rwd));
char *holdarea		  PROTO((int uzone));
char *holdpoint 	  PROTO((int point));
boolean createhold	  PROTO((int zone, int net, int node, int point));
char *pointnr		  PROTO((int point));
char *zonedomabbrev	  PROTO((int zone));
char *zonedomname	  PROTO((int zone));
void  flag_shutdown	  PROTO((void));

/* outblist.c */
OUTBLIST *add_outentry	  PROTO((int zone, int net, int node, int point));
boolean   nlflag_isonline PROTO((int zone, char *flags, long curgmt));
void	  scan_outbound   PROTO((void));
int	  xmit_next	  PROTO((int *zone, int *net, int *node, int *point));
void	  show_outbound   PROTO((word keycode));
dword	  check_kbmail	  PROTO((boolean iscaller, int zone, int net, int node, int point));

/* period.c */
int	period_parsetime PROTO((char *av[], byte base []));
void	period_read	 PROTO((void ));
void	period_write	 PROTO((void ));
long	period_calculate PROTO((void ));
boolean period_active	 PROTO((PERIOD_MASK *pmp));

/* sched.c */
char *start_time    PROTO((EVENT *e, char *p));
char *end_time	    PROTO((EVENT *e, char *p));
int   parse_event   PROTO((char *e_line));
void  find_event    PROTO((void));
void  read_sched    PROTO((void));
void  write_sched   PROTO((void));
int   time_to_next  PROTO((void));
word  time_to_noreq PROTO((void));

/* bbs.c */
void extapp_handler PROTO((EXTAPP *ep));
void read_bbsinfo   PROTO((void));
void write_clip     PROTO((char *username, char *city,
			   char *nodenumber, char *systemname));
void check_clip     PROTO((void));
void dos_shell	    PROTO((int no_modem));

/* wzsend.c */
int send_WaZOO PROTO((int fsent));
int SendFiles  PROTO((int fsent));

/* tconf.c */
char	*skip_blanks   PROTO((char *string));
char	*skip_to_blank PROTO((char *string));
char	*ctl_string    PROTO((char *string));
char	*ctl_dir       PROTO((char *string));
boolean  create_dir    PROTO((char *path));
char	*ctl_in        PROTO((char *str));
char	*myalloc       PROTO((word sp));
int	 init_conf     PROTO((int argc, char *argv[]));
#if 0
void	 parsekey      PROTO((char *p, int line));
#endif
int	 getint        PROTO((char **p, int *i));
int	 getaddress    PROTO((char *str, int *zone, int *net, int *node, int *point));

/* request.c */
void req_close	       PROTO((void));
int  process_req_files PROTO((int fsent));
void rm_requests       PROTO((void));
void process_bark_req  PROTO((char *req));
int  do_reqline        PROTO((char *reqline));
int  qf_send	       PROTO((int fsent,int tech));
char *qm_finish        PROTO((void));
/*static*/ int	req_open	PROTO((void));
/*static*/ int	proc_req	PROTO((void));
/*static*/ void prepfname	PROTO((char *dst, char *src));
/*static*/ int	wildmatch	PROTO((char FAR *s1, char FAR *s2));
/*static*/ int	exactmatch	PROTO((char FAR *s1, char FAR *s2));
/*static*/ int	rsp_check	PROTO((int rsp_comp, int rsp_type));
/*static*/ void process_service PROTO((void));
/*static*/ int	qf_write	PROTO((char FAR *fdir, char FAR *fname));
/*static*/ int	qf_send 	PROTO((int fsent, int tech));
/*static*/ void qm_write	PROTO((void));
/*static*/ void req_macro	PROTO((char FAR *dst, int *ofs, char FAR *src));

/* cdog.c */
int SEA_sendreq PROTO((void));
int SEA_recvreq PROTO((void));
int req_out	PROTO((char *name, char *rtime, char *pw));
int get_req_str PROTO((char *req));

/* get.c */
int  getnames PROTO((FILE *fd, int pwd));
void makelist PROTO((int fl));

/* fmisc.c */
void	 unique_name PROTO((char *pathname));
char	*xfer_init   PROTO((char *realname, long fsize, long ftime));
boolean  xfer_bad    PROTO((void));
char	*xfer_okay   PROTO((void));
void	 xfer_del    PROTO((void));

/* twindow.c */
void logwinopen      PROTO((void));
void mailerwinopen   PROTO((void));
void show_mail	     PROTO((void));
void filewinopen     PROTO((void));
void filewincls      PROTO((char *protocol, int prottype));
void show_help	     PROTO((int mail));
void show_stats      PROTO((void));
void event0	     PROTO((void));
void statuswinopen   PROTO((void));
void modemwinopen    PROTO((void));
void topwinopen      PROTO((void));
void helpwinopen     PROTO((void));

/* tmenu.c */
int  cdecl xenia_info	 PROTO((WIN_MENU *mp, word keycode));
int  cdecl mode_term	 PROTO((WIN_MENU *mp, word keycode));
int  cdecl mode_jump	 PROTO((WIN_MENU *mp, word keycode));
int  cdecl mode_exit	 PROTO((WIN_MENU *mp, word keycode));
int  cdecl handle_appkey PROTO((WIN_MENU *mp, word keycode));
int  add_appkey 	 PROTO((word hotkey, word menukey, char *cmd, char *text));
void init_mailermenu	 PROTO((void));
word call_mailermenu	 PROTO((void));

/* zmisc.c */
void z_longtohdr PROTO((long parm));
long z_hdrtolong PROTO((byte *hdr));
void z_puts	 PROTO((char *str));
void z_putc	 PROTO((int c));
int  z_get	 PROTO((int timeout));
int  z_get7	 PROTO((int timeout));
int  z_getc	 PROTO((int timeout));
int  z_gethex	 PROTO((int timeout));
void z_puthhdr	 PROTO((int type, byte *hdr));
void z_putbhdr	 PROTO((int type, byte *hdr));
int  z_gethdr	 PROTO((char *hdr));
void z_putdata	 PROTO((byte *buf, int length, int frameend));
int  z_getdata	 PROTO((char *buf, int length));
/*static*/ void z_putb32hdr PROTO((int type, char *hdr));
/*static*/ int	z_gethhdr   PROTO((char *hdr));
/*static*/ int	z_getbhdr   PROTO((char *hdr));
/*static*/ int	z_getb32hdr PROTO((char *hdr));
/*static*/ void z_put32data PROTO((byte *buf, int length, int frameend));
/*static*/ int	z_get32data PROTO((char *buf, int length));

/* zrecv.c */
int get_Zmodem PROTO((void));
int zrecv      PROTO((int function));
/*static*/ int	zr_rinit PROTO((void));
/*static*/ int	zr_fdata PROTO((void));
/*static*/ int	zr_save  PROTO((void));
/*static*/ void zr_fin	 PROTO((void));
/*static*/ void zr_error PROTO((int function, int retries, int error));

/* zsend.c */
int send_Zmodem PROTO((char *fname, char *alias, int doafter, int fsent, int wazoo));
int zsend	PROTO((int function, char *parm, char *alias, int wazoo));
/*static*/ int	zs_rqinit    PROTO((void));
/*static*/ int	zs_challenge PROTO((void));
/*static*/ int	zs_file      PROTO((int blen));
/*static*/ int	zs_fdata     PROTO((int wazoo));
/*static*/ int	zs_sync      PROTO((int endfile));
/*static*/ void zs_fin	     PROTO((void));
/*static*/ void zs_error     PROTO((int function, int retries, int error));

/* zcomp.c */
int  zc_sinit	PROTO((int cap));
int  zc_rinit	PROTO((int cap));
word lza_alloc	PROTO((word wantmaxbits));
void lza_init	PROTO((void));
int  lza_flush	PROTO((void));
word dlza_alloc PROTO((word wantmaxbits));
void dlza_init	PROTO((void));
int  zc_fread	PROTO((byte *buf, int size, FILE *stream, int *incount, int *endfile));
int  zc_fwrite	PROTO((byte *buf, int size, FILE *stream));

/* sys*.c */
long	 timerset     (word t);
int	 timeup       (long t);
int	 rts_out      (int x);
void	 dtr_out      (byte flag);
void	 com_flow     (byte flags);
void	 com_disk     (boolean diskaccess);
void	 setspeed     (dword speed, boolean force);
#if PC
boolean  com_setport  (word comport, word adr, word irq);
#endif
void	 sys_init     (void);
void	 sys_reset    (void);
int	 carrier      (void);
void	 com_flush    (void);
void	 com_purge    (void);
void	 com_dump     (void);
void	 com_putbyte  (byte c);
void	 com_putblock (byte *s, word len);
int	 com_outfull  (void);
void	 com_bufbyte  (byte c);
void	 com_bufflush (void);
int	 com_scan     (void);
int	 com_getc     (int time);
int	 com_getbyte  (void);
void	 com_break    (void);
void	 setstamp     (char *name, long tim);
int	 getstat      (char *pathname, struct stat *statbuf);
long	 freespace    (char *drivepath);
char	*ffirst       (char *filespec);
char	*fnext	      (void);
void	 fnclose      (void);
char	*dfirst       (char *filespec);
char	*dnext	      (void);
void	 dnclose      (void);
char	*dfirst2      (char *filespec);
char	*dnext2       (void);
void	 dnclose2     (void);
void	 MakeShortName (char *Name, char *ShortName);
void	 SetLongName   (char *fname, char *longfname);
#if 0
int	 rename       (const char *oldname, const char *newname);
#endif
int	 execute      (char *cmd);
int	 shell	      (char *cmd);
void	 ptt_enable   (void);
void	 ptt_disable  (void);
void	 ptt_reset    (void);
void	 mcp_sleep    (void);
#if	TURBOST
long intell(long);
word inteli(word);
#endif

/*dos_file.c*/
void  dos_sharecheck (void);
int   dos_sopen   (char *pathname, unsigned int mode, int shareflag);
int   dos_close   (int handle);
long  dos_seek	  (int handle, long offset, int fromwhere);
long  dos_tell	  (int handle);
int   dos_read	  (int handle, void *buf, word len);
int   dos_write   (int handle, void *buf, word len);
FILE *sfopen	  (char *file, char *mode, int shareflag);


#undef PROTO
