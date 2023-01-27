/*# name=Main Maximus header file
*/

#ifndef __MAX_H_DEFINED
#define __MAX_H_DEFINED

#include "prog.h"

/***************************************************************************
 *    Conditional flags for the entire program, and portability stuff	   *
 ***************************************************************************/

#define CANENCRYPT	    /* Do we support encryption of passwords? */
#define MUSTENCRYPT	    /* Should we always encrypt the user's password
			     * at logon, even if they don't try to change it?
			     * (This requires CANENCRYPT.)
			     */
#define CHANGEENCRYPT	    /* Passwords are always encrypted when the user
			     * changes it or logs on for the first time */
#define TTYVIDEO	    /* Local video supported via fputc() et al */
#define MAXIMUS 	    /* Who are we? */
#define MEX		    /* Include MEX support */
#define MAX_TRACKER	    /* Use message tracking subsystem */
/*#define FLUSH_LOG*/
#define INTERNAL_PROTOCOLS
#define SHORT_MDM_CMD

#if !defined(__FLAT__) && !defined(ORACLE)
  /*#define KEY*/		/* Use key file */
#endif

#ifdef OS_2
  #define MCP

  #ifndef ORACLE
    #define MCP_VIDEO
  #endif

  #ifndef __FLAT__
    /*#define MAXSNOOP*/
  #endif
#endif


/***************************************************************************
			  Initialization of variables
 ***************************************************************************/

#define extrn extern
#define IS(x)
#define LEN(x)

#include <time.h>
#include "utime.h"


#ifdef BLINK
#undef BLINK
#endif

#ifdef DLE
#undef DLE
#endif

/***************************************************************************
			  Miscellanious Macros
 ***************************************************************************/

#ifdef __TURBOC__
#define isupsp(c) (_ctype[(c) + 1] & (_IS_UPP | _IS_SP))
#else
#define isupsp(c) (isupper(c) || isspace(c))
#endif

#define MNU(m,o)	    (((m).menuheap)+(m).o)
#define MsgAreaHasLock(a,l) ((a).msglock & (1L << (dword)(l)))
#define FileAreaHasLock(a,l)((a).filelock & (1L << (dword)(l)))
#define GEPriv(p1,p2)	    ((word)(p1) >= (word)(p2))
#define LEPriv(p1,p2)	    ((word)(p1) <= (word)(p2))

/***************************************************************************
			    Constant #define's
 ***************************************************************************/


#define FFLAG_TAG	  0x0001    /* File was T)agged 		    */
#define FFLAG_THIS1	  0x0002    /* Selected during THIS wcard expansion */
#define FFLAG_NOTIME	  0x0004    /* Don't debit time for this file       */
#define FFLAG_NOBYTES	  0x0008    /* Don't debit DL bytes for this file   */
#define FFLAG_EXP	  0x0010    /* Filename was expanded from wildcard  */
#define FFLAG_NOENT	  0x0020    /* This file is not on disk 	    */
#define FFLAG_OK	  0x0040    /* This file previously verified for DL */
#define FFLAG_NOLIST	  0x0080    /* File is not in FILES.BBS 	    */
#define FFLAG_GOT	  0x0100    /* This file was uploaded		    */
#define FFLAG_SENT	  0x0200    /* This file was downloaded 	    */
#define FFLAG_STAGE	  0x0400    /* Copy file to staging area before xfer*/
#define FFLAG_SLOW	  0x0800    /* File is from from an FA_SLOW area    */
#define FFLAG_DUPE	  0x8000    /* File is a dupe			    */

#define CMSG_PAGE	0x00   /* "You're being paged by another user!"     */
#define CMSG_ENQ	0x01   /* "Are you on this chat channel?"	    */
#define CMSG_ACK	0x02   /* "Yes, I AM on this channel!"		    */
#define CMSG_EOT	0x03   /* "I'm leaving this chat channel!"	    */
#define CMSG_CDATA	0x04   /* Text typed by used while in chat	    */
#define CMSG_HEY_DUDE	0x05   /* A normal messge.  Always displayed.	    */
#define CMSG_DISPLAY	0x06   /* Display a file to the user		    */

#define MAX_MENUNAME  PATHLEN	/* Max. length of menuname[].		    */
#define RST_VER 	    3	/* Version number of RESTARxx.BBS	    */
#define MAX_LINE	  255	/* Max. length of a .CTL file line!	    */
#define PREREGISTERED  0xfffe	/* Priv. value that means let no new	    *
				 * callers on the system.		    */
#define RESTART_MENU	  0x00	/* If restarting at a menu		  */
#define RESTART_DOTBBS	  0x01	/* If restarting in middle of .BBS file   */

#define NUM_MENU	   256	/* Max. # of options in a menu file	  */


/* Defines for *.MNU: */

#define AREATYPE_LOCAL	  0x01	/* Tells which type (and which type ONLY)  */
#define AREATYPE_MATRIX   0x02	/* that command can be used in. 	   */
#define AREATYPE_ECHO	  0x04
#define AREATYPE_CONF	  0x08

#define AREATYPE_ALL	  (AREATYPE_LOCAL | AREATYPE_MATRIX |	\
			   AREATYPE_ECHO | AREATYPE_CONF)

#define HEADER_NONE	  0x00	/* The header type for each menu.	   */
#define HEADER_MESSAGE	  0x01
#define HEADER_FILE	  0x02
#define HEADER_CHANGE	  0x03
#define HEADER_CHAT	  0x04

#define OFLAG_NODSP	  0x0001 /* Don't display menu option on MENU, but */
				 /* accept it as a command.		   */
#define OFLAG_CTL	  0x0002 /* Produce a .CTL file for this xtern cmd */
#define OFLAG_NOCLS	  0x0004 /* Don't do a CLS for this display_menu   */
#define OFLAG_THEN	  0x0008 /* Do only if last IF equation was true   */
#define OFLAG_ELSE	  0x0010 /* Do only if last IF equation was FALSE  */
#define OFLAG_ULOCAL	  0x0020 /* Only display for local users	   */
#define OFLAG_UREMOTE	  0x0040 /* Only display for remote useres	   */
#define OFLAG_REREAD	  0x0080 /* Re-read LASTUSER.BBS upon re-entry	   */
#define OFLAG_STAY	  0x0100 /* Don't perform menu clean-up operations */
#define OFLAG_RIP	  0x0200 /* Available for RIP callers only	   */
#define OFLAG_NORIP	  0x0400 /* Available for non-RIP callers only	   */

/***************************************************************************
		       Structure Definitions
 ***************************************************************************/

typedef struct _netaddr NETADDR;

struct _netaddr
{
  word zone;
  word net;
  word node;
  word point;
};



struct _maxcol
{
  byte status_bar;	  /* black on white */
  byte status_cht;	  /* blinking black on white */
  byte status_key;	  /* blinking black on white */

  byte pop_text;	  /* white on blue */
  byte pop_border;	  /* yellow on blue */
  byte pop_high;	  /* yellow on blue */
  byte pop_list;	  /* black on grey */
  byte pop_lselect;	  /* grey on red */

  byte wfc_stat;	  /* white on blue */
  byte wfc_stat_bor;	  /* yellow on blue */
  byte wfc_modem;	  /* gray on blue */
  byte wfc_modem_bor;	  /* lgreen on blue */
  byte wfc_keys;	  /* yellow on blue */
  byte wfc_keys_bor;	  /* white on blue */
  byte wfc_activ;	  /* white on blue */
  byte wfc_activ_bor;	  /* lcyan on blue */
  byte wfc_name;	  /* yellow on black */
  byte wfc_line;	  /* white on black */
};



/* Typedefs */

typedef word zstr;	/* Offset of string within area heap */

#define CLS_ID	0x8f7c9321L

#include "uclass.h"
#include "max_u.h"	  /* USER.BBS structure */
#include "option.h"	  /* Menu option enumeration */
#include "callinfo.h"	  /* Caller information log */

/* An individual menu option.  There are many of these contained in one    *
 * _menu file, following the _menu data header, optionally with some	   *
 * NULL-terminated strings between each _opt structure, for the argument.  */

struct _opt
{
#ifdef __FLAT__
  /* force enum to be 16 bits */
  word type;
#else
  option type;	/* What this menu option does				   */
#endif

  zstr priv;	/* Priv level required to execute this command		   */
  dword rsvd;	/* Bit-field locks for this particular menu option	   */
  word flag;	/* See the OFLAG_xxx contants for more info.		   */
  zstr name;	/* The menu option, as it appears to user		   */
  zstr keypoke; /* Auto-keypoke string					   */
  zstr arg;	/* The argument for this menu option			   */
  byte areatype;/* If this particular option can only be used if the user  *
		 * is in a certain message-area type.			   */
  byte fill1;	/* Reserved by Maximus for future use			   */

  byte rsvd2[8]; /* Reserved for future uses */
};

#define DEFAULT_OPT_WIDTH 20

struct _menu
{
  word header,	    /* What to display when the user enters menu, such as  *
		     * "The MESSAGE Section", "The CHG SETUP Section", etc */
       num_options, /* Total number of options (struct _opt's) in menu     */
       menu_length, /* Number of lines long the .?BS menu file is!	   */
       opt_width;   /* Option width override				   */

  sword hot_colour; /* What colour to display if a user uses hotkeys to    *
		     * bypass a .?BS menu display, before displaying the   *
		     * key.  -1 == display nothing.			   */

  word title;	    /* Length of the title string, not counting \0.	   */
  word headfile;    /* Length of the header filename, not counting \0	   */
  word dspfile;     /* Name of file to display for menu, instead of	   *
		     * generating menu from .Mnu file.			   */
  word flag;	    /* See MFLAG_XXX in MAX.H.				   */
};



/* Menu structure used INTERNALLY within Max itself */

typedef struct _amenu
{
  struct _menu m;
  struct _opt *opt;
  char *menuheap;
} AMENU, *PAMENU;


  #define MFLAG_MF_NOVICE   0x0001u /* MenuFile for these levels only */
  #define MFLAG_MF_REGULAR  0x0002u
  #define MFLAG_MF_EXPERT   0x0004u
  #define MFLAG_MF_RIP	    0x0400u
  #define MFLAG_MF_ALL	    (MFLAG_MF_NOVICE | MFLAG_MF_REGULAR | \
			     MFLAG_MF_EXPERT /*| MFLAG_MF_HOTFLASH*/)

  #define MFLAG_HF_NOVICE   0x0010u /* HeaderFile for these levels only */
  #define MFLAG_HF_REGULAR  0x0020u
  #define MFLAG_HF_EXPERT   0x0040u
  #define MFLAG_HF_RIP	    0x0800u

  #define MFLAG_HF_ALL	    (MFLAG_HF_NOVICE | MFLAG_HF_REGULAR | \
			     MFLAG_HF_EXPERT /*| MFLAG_HF_HOTFLASH*/)

  #define MFLAG_SILENT	    0x0100u /* Silent menuheader option */
  #define MFLAG_RESET	    0x0200u /* Reset term size on display */




#define STATS_VER	    1	/* Version number of the BBSTATxx.BBS file */

struct _bbs_stats
{
  byte	  version;	/* Version number of BBSTATxx.BBS file */
  dword   num_callers;
  dword   quote_pos;
  dword   msgs_written;
  time_t  online_date;
  dword   total_dl;
  dword   total_ul;
  sword   today_callers;
  union stamp_combo date;
  byte	  lastuser[36];
};



/* Structure for entries in PROTOCOL.MAX */

struct _proto
{
  #define P_ISPROTO 0x01  /* This bit always set			    */
  #define P_BATCH   0x02  /* Can handle batch transfers 		    */
  #define P_OPUS    0x04  /* Write an Opus-style .CTL file		    */
  #define P_ERL     0x08  /* Exit with xtern_erlvl			    */
  #define P_BI	    0x10  /* Bidirectional transfer			    */

  word flag;

  char desc[40];
  char log[PATHLEN];
  char ctl[PATHLEN];
  char dlcmd[PATHLEN];
  char ulcmd[PATHLEN];
  char dlstr[40];
  char ulstr[40];
  char dlkey[40];
  char ulkey[40];
    
  word fnamword;
  word descword;
};



#include "prm.h"    /* MAX.PRM structure */
#include "newarea.h"


/* IPCxx.BBS header structure */

struct _cstat
{
  word avail;

  byte username[36];
  byte status[80];

#ifndef MCP
  word msgs_waiting;

  dword next_msgofs;
  dword new_msgofs;
#endif
};



/* Data element in IPCxx.BBS file (see MAX_CHAT.C) */

struct _cdat
{
  word tid;
  word type;
  word len;

#ifdef MCP
  word dest_tid;
  dword rsvd1;
#else
  dword rsvd1;
  word	rsvd2;
#endif
};

/* Handle for saving CHAT status.  Mainly used internally, but also	   *
 * in RESTARxx.BBS.							   */

struct _css
{
  word avail;
  byte status[80];
};


/* NOTE: The following structure is not completely stable.  Unless	   *
 * rst.rst_ver is equal to RST_VER, then the ONLY items you're guaranteed  *
 * to be able to read are those marked with "*STABLE*".  Those items	   *
 * are guaranteed to be stored at those offsets for all future versions    *
 * of Maximus, regardless of the version number.  However, everything	   *
 * else is likely to change at a moment's notice.                          */

struct _restart
{
  byte rst_ver; /* Version number of restart data		  *STABLE* */

  sdword timeon;  /* Date user got on system, seconds since 1970  *STABLE* */
  sdword timeoff; /* Date user must be OFF system, secs since '70 *STABLE* */
  sdword restart_offset; /* Offset in .BBS file to restart at	  *STABLE* */

  dword baud;		  /* User's baud rate                   *STABLE*   */
  dword max_time;	  /* Max time, as given by '-t' param	*STABLE*   */

  sword port;		  /* Current COM port, 0=COM1, 1=COM2,	*STABLE*   */

  char written_echomail;  /* 0=user HASN'T written echomail     *STABLE*   */
  char written_matrix;	  /* 0=user HASN'T entered matrix msg   *STABLE*   */
  char local;		  /* 0=NOT local			*STABLE*   */

  struct _stamp laston;   /* Time the user was last on system	*STABLE*   */
  
  word steady_baud;	  /* Locked baud rate of user		*STABLE*   */

  sdword starttime;	  /* Start time, for external protocol		   */
  sdword timestart;	  /* Time when MAX.EXE was started		   */
  sdword ultoday;	  /* KB's the user has uploaded today              */

  union stamp_combo next_ludate;
  
  byte restart_type;	  /* 1 if started via .BBS file, 0 otherwise	   */
  char restart_name[PATHLEN]; /* Name of .BBS file to restart in	   */
  char menupath[PATHLEN]; /* The current menu path			   */
  char firstname[36];	  /* The user's first name                         */
  char last_onexit[PATHLEN]; /* The 'onexit' filename for current .BBS file*/
  char parm[PATHLEN];	  /* Parms for external program, if any 	   */
  char fix_menupath[PATHLEN]; /* Readjust menu name			   */

  char lastmenu;	  /* Last ^oR menu choice			   */
  char snoop;		  /* If snoop is currently on or off		   */

  char locked;		  /* If priv is locked via keyboard 'L' command    */

  char keyboard;	  /* If the Sysop's keyboard is turned on          */
  char protocol_letter;   /* Letter representing current protocol choice   */

  char chatreq; 	  /* If user wanted to chat with SysOp		   */
  char mn_dirty;	  /* If menuname buf is dirty			   */

  char barricade_ok;	  /* If current barricade area is OK		   */
  char no_zmodem;	  /* If zmodem not allowed			   */

  sword usr_time;	  /* User's usr.time value                         */
  sword rsvdxx1;	  /* reserved for future use			   */
  sword lockpriv;	  /* If rst.locked (above), then this is real priv */
  sword ctltype;	  /* Control-file type (for xternal protocol)	   */

  word current_baud;	  /* User's baud rate, as a mask for mdm_baud() */

  /* Bit flags for ECHOTOSS.LOG */
  char rsvd[(1296/CHAR_BITS)+1];
  char rsvd2[1120]; /* old area data structure */
  struct _css css;
  
  char log_name[80];

  struct _usr origusr;	  /* User record as it was at log-on */

  char event_num;
  char rsvd3;
  
  sword last_protocol;
  long getoff;
  char returning[PATHLEN];
  long steady_baud_l;		  /* Locked baud rate (as integer) */
  SCOMBO date_newfile;		  /* User's last newfiles date */
};


/***************************************************************************
		      Global Variables and Constant Strings
 ***************************************************************************/

#define PROTOCOL_NONE	     (sword)-1 /* Protocol to use for sending/receiving   */
#define PROTOCOL_XMODEM      (sword)-2 /* files.  If we're using an EXTERNAL      */
#define PROTOCOL_YMODEM      (sword)-3 /* protocol, `protocol' will have a value  */
#define PROTOCOL_XMODEM1K    (sword)-4 /* >= 0. 				  */
#define PROTOCOL_SEALINK     (sword)-5
#define PROTOCOL_ZMODEM      (sword)-6
#define PROTOCOL_YMODEMG     (sword)-7

#define  EXPERT      (byte)0x02
#define  REGULAR     (byte)0x04
#define  NOVICE      (byte)0x06

#endif /* __MAX_H_DEFINED */

