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


#include "xenia.h"


#define CONFIG_DEBUG 0


static boolean cmdlinetask  = false;
static boolean cmdlineport  = false;
#if OS2
static boolean cmdlinesnoop = false;
#endif
static int     error  = 0,
	       ignore = 0;


/* ------------------------------------------------------------------------- */
char *myalloc (word sp)
{
    char *tmp;

    tmp = (char *)malloc(sp);
    if (tmp == NULL) {
       fprint(stderr,"             ! ! Mem error ! !                \n");
       exit(1);
    }

    return (tmp);
}


/* ------------------------------------------------------------------------- */
char *skip_blanks (char *string)
{
    register byte c;

    /*while (*string && isspace(*string)) string++;*/

    while (c = *string,
	   (c && ((c >= 0x09 && c <= 0x0d) || c == 0x20 || c == 0x7f || c >= 0xfe)))
	  string++;

    return (string);
}


/* ------------------------------------------------------------------------- */
char *skip_to_blank (char *string)
{
   register byte c;

   /*while (*string && (!isspace(*string))) string++;*/

   while (c = *string,
	  (c && ((c < 0x09 || c > 0x0d) && c != 0x20 && c != 0x7f && c < 0xfe)))
	 string++;

   return (string);
}


/* ------------------------------------------------------------------------- */
char *ctl_string (char *string)
{
    char *p, *d;

    p = skip_blanks(string);
    d = myalloc((word) (strlen(p) + 1));
    strcpy(d,p);

    return (d);
}


/* ------------------------------------------------------------------------- */
char *ctl_dir (char *string)
{
    char *p, *d, db[PATHLEN];

    strcpy(db,string);
    if ((p = strchr(db,';')) != NULL) *p = '\0';
    p = skip_blanks(db);
    d = skip_to_blank(p);
    *d = '\0';
    if (strlen(p) > 127) {
       fprint(stderr,"Error: Path too long (%s)\n",p);
       return ("");
    }
    if (p[strlen(p) - 1] != '\\') strcat(p,"\\");

    if (!create_dir(p))
       return ("");

    return (ctl_string(p));
}


/* ------------------------------------------------------------------------- */
boolean create_dir (char *path)
{
	char buffer[PATHLEN];
	char *p;

	strcpy(buffer,path);

	if ((p = strrchr(buffer,'\\')) != NULL)
	   *p = '\0';
	if (!*buffer) return (true);

	p = &buffer[strlen(buffer) - 1];
	if (*p == ':' || *p == '\\') return (true);

	p = dfirst(buffer);
	dnclose();
	if (!p && mkdir(buffer) < 0) {
	   fprint(stderr,"Error: Can't create directory (%s)\n",buffer);
	   return (false);
	}
	return (true);
}


/* ------------------------------------------------------------------------- */
#if 0
char *ctl_in(char *str)
{
	int  i, j;
	char buffer[PATHLEN];
	char *p = buffer;

	while (*str) {
	      if (*str == '\\') {
		 switch (*++str) {
			case 'n': *p++ = '\n'; str++; break;
			case 'r': *p++ = '\r'; str++; break;
			case 'b': *p++ = '\b'; str++; break;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			     j = i = 0;
			     while (j++ < 3 && *str >= '0' && *str <= '7') {
				   i *= 8;
				   i += *(str++)-'0';
			     }
			     *p++ = i;
			     break;
			default:
			     *p++= *str; str++; break;
		 }
	      }
	      else
		 *p++ = *str++;
	}

	*p = '\0';

	return (ctl_string(buffer));
}
#endif


/* ------------------------------------------------------------------------- */
static struct {
	char *tag;
	byte  col;
} coltab[] = { { "normal"    ,CHR_NORMAL    }, { "inverse"   ,CHR_INVERSE   },
	       { "bright"    ,CHR_BRIGHT    }, { "bold"      ,CHR_BOLD	    },
	       { "underline" ,CHR_UNDERLINE }, { "blink"     ,CHR_BLINK     },
	       { "black"     ,CHR_F_BLACK   }, { "blue"      ,CHR_F_BLUE    },
	       { "green"     ,CHR_F_GREEN   }, { "cyan"      ,CHR_F_CYAN    },
	       { "red"	     ,CHR_F_RED     }, { "magenta"   ,CHR_F_MAGENTA },
	       { "brown"     ,CHR_F_BROWN   }, { "white"     ,CHR_F_WHITE   },
	       { "grey"      ,CHR_F_GREY    }, { "gray"      ,CHR_F_GREY    },
	       { "yellow"    ,CHR_F_YELLOW  }, { "b_black"   ,CHR_B_BLACK   },
	       { "b_blue"    ,CHR_B_BLUE    }, { "b_green"   ,CHR_B_GREEN   },
	       { "b_cyan"    ,CHR_B_CYAN    }, { "b_red"     ,CHR_B_RED     },
	       { "b_magenta" ,CHR_B_MAGENTA }, { "b_brown"   ,CHR_B_BROWN   },
	       { "b_white"   ,CHR_B_WHITE   },
	       { NULL	     ,0 	    }
};


/* ------------------------------------------------------------------------- */
static struct {
	char *tag;
	word  keycode;
} keytab[] = { { "Esc"	      ,Esc	  }, { "Alt_Esc"       ,Alt_Esc       },
	       { "Ins"	      ,Ins	  },
	       { "Ctrl_Ins"   ,Ctrl_Ins   }, { "Alt_Ins"       ,Alt_Ins       },
	       { "Del"	      ,Del	  },
	       { "Ctrl_Del"   ,Ctrl_Del   }, { "Alt_Del"       ,Alt_Del       },
	       { "Enter"      ,Enter	  }, { "Alt_Num_Enter" ,Alt_Num_Enter },
	       { "Ctrl_Enter" ,Ctrl_Enter }, { "Alt_Enter"     ,Alt_Enter     },
	       { "Space"      ,Space	  },
	       { "BS"	      ,BS	  },
	       { "Ctrl_BS"    ,Ctrl_BS	  }, { "Alt_BS"        ,Alt_BS	      },
	       { "Tab"	      ,Tab	  }, { "Ctrl_Tab"      ,Ctrl_Tab      },
	       { "Shift_Tab"  ,Shift_Tab  }, { "Alt_Tab"       ,Alt_Tab       },
	       { "Ctrl_PrtSc" ,Ctrl_PrtSc },
	       { "Key_45"     ,Key_45	  }, { "Shift_Key_45"  ,Shift_Key_45  },
	       { "Centre"     ,Centre	  }, { "Ctrl_Centre"   ,Ctrl_Centre   },
	       { "Ctrl_Asterisk" ,Ctrl_Asterisk }, { "Alt_Asterisk" ,Alt_Asterisk },
	       { "Ctrl_Minus"	 ,Ctrl_Minus	}, { "Alt_Minus"    ,Alt_Minus	  },
	       { "Ctrl_Plus"	 ,Ctrl_Plus	}, { "Alt_Plus"     ,Alt_Plus	  },
	       { "Ctrl_Slash"	 ,Ctrl_Slash	}, { "Alt_Slash"    ,Alt_Slash	  },
	       { "Up"	      ,Up	  }, { "Down"	       ,Down	      },
	       { "Left"       ,Left	  }, { "Right"	       ,Right	      },
	       { "Home"       ,Home	  }, { "End"	       ,End	      },
	       { "PgUp"       ,PgUp	  }, { "PgDn"	       ,PgDn	      },
	       { "Ctrl_Up"    ,Ctrl_Up	  }, { "Alt_Up"        ,Alt_Up	      },
	       { "Ctrl_Down"  ,Ctrl_Down  }, { "Alt_Down"      ,Alt_Down      },
	       { "Ctrl_Left"  ,Ctrl_Left  }, { "Alt_Left"      ,Alt_Left      },
	       { "Ctrl_Right" ,Ctrl_Right }, { "Alt_Right"     ,Alt_Right     },
	       { "Ctrl_Home"  ,Ctrl_Home  }, { "Alt_Home"      ,Alt_Home      },
	       { "Ctrl_End"   ,Ctrl_End   }, { "Alt_End"       ,Alt_End       },
	       { "Ctrl_PgUp"  ,Ctrl_PgUp  }, { "Alt_PgUp"      ,Alt_PgUp      },
	       { "Ctrl_PgDn"  ,Ctrl_PgDn  }, { "Alt_PgDn"      ,Alt_PgDn      },
	       { "MKEY_LEFTMOVE"    ,MKEY_LEFTMOVE    },
	       { "MKEY_UPMOVE"	    ,MKEY_UPMOVE      },
	       { "MKEY_RIGHTMOVE"   ,MKEY_RIGHTMOVE   },
	       { "MKEY_DOWNMOVE"    ,MKEY_DOWNMOVE    },
	       { "MKEY_LEFTBUTTON"  ,MKEY_LEFTBUTTON  },
	       { "MKEY_RIGHTBUTTON" ,MKEY_RIGHTBUTTON },
	       { "MKEY_CURBUTTON"   ,MKEY_CURBUTTON   },
	       { "Alt_0" ,Alt_0 }, { "Alt_1" ,Alt_1 }, { "Alt_2" ,Alt_2 },
	       { "Alt_3" ,Alt_3 }, { "Alt_4" ,Alt_4 }, { "Alt_5" ,Alt_5 },
	       { "Alt_6" ,Alt_6 }, { "Alt_7" ,Alt_7 }, { "Alt_8" ,Alt_8 },
	       { "Alt_9" ,Alt_9 },
	       { "F10" ,F10 }, { "Ctrl_F10" ,Ctrl_F10 },
	       { "F11" ,F11 }, { "Ctrl_F11" ,Ctrl_F11 },
	       { "F12" ,F12 }, { "Ctrl_F12" ,Ctrl_F12 },
	       { "F1"  ,F1  }, { "Ctrl_F1"  ,Ctrl_F1  },
	       { "F2"  ,F2  }, { "Ctrl_F2"  ,Ctrl_F2  },
	       { "F3"  ,F3  }, { "Ctrl_F3"  ,Ctrl_F3  },
	       { "F4"  ,F4  }, { "Ctrl_F4"  ,Ctrl_F4  },
	       { "F5"  ,F5  }, { "Ctrl_F5"  ,Ctrl_F5  },
	       { "F6"  ,F6  }, { "Ctrl_F6"  ,Ctrl_F6  },
	       { "F7"  ,F7  }, { "Ctrl_F7"  ,Ctrl_F7  },
	       { "F8"  ,F8  }, { "Ctrl_F8"  ,Ctrl_F8  },
	       { "F9"  ,F9  }, { "Ctrl_F9"  ,Ctrl_F9  },
	       { "Shift_F10" ,Shift_F10 }, { "Alt_F10" ,Alt_F10 },
	       { "Shift_F11" ,Shift_F11 }, { "Alt_F11" ,Alt_F11 },
	       { "Shift_F12" ,Shift_F12 }, { "Alt_F12" ,Alt_F12 },
	       { "Shift_F1"  ,Shift_F1	}, { "Alt_F1"  ,Alt_F1	},
	       { "Shift_F2"  ,Shift_F2	}, { "Alt_F2"  ,Alt_F2	},
	       { "Shift_F3"  ,Shift_F3	}, { "Alt_F3"  ,Alt_F3	},
	       { "Shift_F4"  ,Shift_F4	}, { "Alt_F4"  ,Alt_F4	},
	       { "Shift_F5"  ,Shift_F5	}, { "Alt_F5"  ,Alt_F5	},
	       { "Shift_F6"  ,Shift_F6	}, { "Alt_F6"  ,Alt_F6	},
	       { "Shift_F7"  ,Shift_F7	}, { "Alt_F7"  ,Alt_F7	},
	       { "Shift_F8"  ,Shift_F8	}, { "Alt_F8"  ,Alt_F8	},
	       { "Shift_F9"  ,Shift_F9	}, { "Alt_F9"  ,Alt_F9	},
	       { "Ctrl_A" ,Ctrl_A }, { "Alt_A" ,Alt_A },
	       { "Ctrl_B" ,Ctrl_B }, { "Alt_B" ,Alt_B },
	       { "Ctrl_C" ,Ctrl_C }, { "Alt_C" ,Alt_C },
	       { "Ctrl_D" ,Ctrl_D }, { "Alt_D" ,Alt_D },
	       { "Ctrl_E" ,Ctrl_E }, { "Alt_E" ,Alt_E },
	       { "Ctrl_F" ,Ctrl_F }, { "Alt_F" ,Alt_F },
	       { "Ctrl_G" ,Ctrl_G }, { "Alt_G" ,Alt_G },
	       { "Ctrl_H" ,Ctrl_H }, { "Alt_H" ,Alt_H },
	       { "Ctrl_I" ,Ctrl_I }, { "Alt_I" ,Alt_I },
	       { "Ctrl_J" ,Ctrl_J }, { "Alt_J" ,Alt_J },
	       { "Ctrl_K" ,Ctrl_K }, { "Alt_K" ,Alt_K },
	       { "Ctrl_L" ,Ctrl_L }, { "Alt_L" ,Alt_L },
	       { "Ctrl_M" ,Ctrl_M }, { "Alt_M" ,Alt_M },
	       { "Ctrl_N" ,Ctrl_N }, { "Alt_N" ,Alt_N },
	       { "Ctrl_O" ,Ctrl_O }, { "Alt_O" ,Alt_O },
	       { "Ctrl_P" ,Ctrl_P }, { "Alt_P" ,Alt_P },
	       { "Ctrl_Q" ,Ctrl_Q }, { "Alt_Q" ,Alt_Q },
	       { "Ctrl_R" ,Ctrl_R }, { "Alt_R" ,Alt_R },
	       { "Ctrl_S" ,Ctrl_S }, { "Alt_S" ,Alt_S },
	       { "Ctrl_T" ,Ctrl_T }, { "Alt_T" ,Alt_T },
	       { "Ctrl_U" ,Ctrl_U }, { "Alt_U" ,Alt_U },
	       { "Ctrl_V" ,Ctrl_V }, { "Alt_V" ,Alt_V },
	       { "Ctrl_W" ,Ctrl_W }, { "Alt_W" ,Alt_W },
	       { "Ctrl_X" ,Ctrl_X }, { "Alt_X" ,Alt_X },
	       { "Ctrl_Y" ,Ctrl_Y }, { "Alt_Y" ,Alt_Y },
	       { "Ctrl_Z" ,Ctrl_Z }, { "Alt_Z" ,Alt_Z },

	       { NULL	      ,0     }
};


/* ------------------------------------------------------------------------- */
static byte get_colour (char *s)
{
	byte col = 0;
	register char *p;
	register byte i;

	if (isdigit(s[0]))
	   return (atoi(s));

	for (p = strtok(s,"+|"); p; p = strtok(NULL,"+|")) {
	    for (i = 0; coltab[i].tag; i++) {
		if (!stricmp(p,coltab[i].tag)) {
		   col |= coltab[i].col;
		   break;
		}
	    }
	    if (!coltab[i].tag) {
	       fprint(stderr,"Error: Unknown colour '%s'\n",p);
	       exit (1);
	    }
	}

	return (col);
}/*get_colour()*/


/* ------------------------------------------------------------------------- */
#if CONFIG_DEBUG
static char *dump_colour (char *buf, byte col)
{
	boolean blink, bright;
	char *back, *fore;

	blink = (col & 0x80) ? true : false;
	col &= 0x7f;

	switch (col & 0x70) {
	       case CHR_B_BLUE:    back = "b_blue";    break;
	       case CHR_B_GREEN:   back = "b_green";   break;
	       case CHR_B_CYAN:    back = "b_cyan";    break;
	       case CHR_B_RED:	   back = "b_red";     break;
	       case CHR_B_MAGENTA: back = "b_magenta"; break;
	       case CHR_B_BROWN:   back = "b_brown";   break;
	       case CHR_B_WHITE:   back = "b_white";   break;
	       default: 	   back = NULL;        break;
	}
	col &= 0x0f;

	bright = (col & 0x08) ? true : false;
	col &= 0x07;

	switch (col & 0x07) {
	       case CHR_F_BLUE:    fore = "blue";    break;
	       case CHR_F_GREEN:   fore = "green";   break;
	       case CHR_F_CYAN:    fore = "cyan";    break;
	       case CHR_F_RED:	   fore = "red";     break;
	       case CHR_F_MAGENTA: fore = "magenta"; break;
	       case CHR_F_BROWN:   fore = bright ? "yellow" : "brown";
				   bright = false;
				   break;
	       case CHR_F_WHITE:   fore = "white"; break;
	       default: 	   fore = !bright ? "grey" : NULL; break;
	}

	strcpy(buf," ");
	if (back) strcat(buf,back);
	if (blink) {
	   if (buf[1]) strcat(buf,"+");
	   strcat(buf,"blink");
	}
	if (bright) {
	   if (buf[1]) strcat(buf,"+");
	   strcat(buf,"bright");
	}
	if (fore) {
	   if (buf[1]) strcat(buf,"+");
	   strcat(buf,fore);
	}

	return (buf);
}/*dump_colour()*/
#endif


/* ------------------------------------------------------------------------- */
static word get_keycode (char *s)
{
	register word i;
	word keycode;

	for (i = 0; keytab[i].tag; i++) {
	    if (!stricmp(s,keytab[i].tag))
	       return (keytab[i].keycode);
	}

	if (s[1]) {
	   if (sscanf(s,"%03hx",&keycode))
	      return (keycode);
	   return (No_Key);
	}

	return ((word) s[0]);
}/*get_keycode()*/


/* ------------------------------------------------------------------------- */
#if CONFIG_DEBUG
static char *dump_keycode (word keycode)
{
	register word i;
	static char buf[5];

	if (keycode >= 33 && keycode <= 126) {
	   sprint(buf,"%c",(short) keycode);
	   return (buf);
	}

	for (i = 0; keytab[i].tag; i++) {
	    if (keycode == keytab[i].keycode)
	       return (keytab[i].tag);
	}

	sprint(buf,"%03x",keycode);
	return (buf);
}/*dump_keycode()*/
#endif


/* ------------------------------------------------------------------------- */
boolean get_periodmask (char *s, PERIOD_MASK *pmp)
{
	char *p;
	int i;

	memset(pmp,0,sizeof (PERIOD_MASK));

	for (p = strtok(s,"+|"); p; p = strtok(NULL,"+|")) {
	    for (i = 0; i < num_periods && stricmp(p,periods[i]->tag); i++);
	    if (i == num_periods) {
	       fprint(stderr,"Error: Period tag '%s' unknown\n",p);
	       return (false);
	    }
	    PER_MASK_SETBIT(pmp,i);
	}

	return (true);
}/*get_periodmask()*/


/* ------------------------------------------------------------------------- */
static boolean mdm_addrsp (char *string, dword code)
{
	MDMRSP *mrp, **mrpp;

	if (!string || !string[0]) return (false);

	for (mrpp = &mdm.rsplist; (mrp = *mrpp) != NULL; mrpp = &mrp->next);
	if ((mrp = (MDMRSP *) calloc(1,sizeof (MDMRSP))) == NULL ||
	    (mrp->string = ctl_string(string)) == NULL) {
	   fprint(stderr,"Error: Can't allocate for mdmrsp\n");
	   return (false);
	}

	mrp->code = code;
	mrp->next = *mrpp;
	*mrpp = mrp;

	return (true);
}/*mdm_addrsp()*/


/* ------------------------------------------------------------------------- */
void read_conf (char *cfgname)
{
    FILE *conf; 		    /* filepointer for config-file */
    char buffer[256];		    /* buffer for 1 line */
    char *p;			    /* workhorse */
    int count;			    /* just a counter... */
    int line = 0;		    /* line in configfile */
    long btmp;			    /* var for speed (bps) */

    if ((conf = sfopen(cfgname,"rt",DENY_NONE)) == NULL) {
       fprint(stderr,"Error: Config file '%s' not found\n",cfgname);
       error++;
       return;
    }

    while (fgets(buffer, 250, conf)) {
	line++;

	p = skip_blanks(buffer);
	if (p != buffer) strcpy(buffer,p);
	while ((p = strchr(buffer,'\t')) != NULL) *p = ' ';
	for (p = buffer; (p = strchr(p,';')) != NULL; p++) {
	    if (p == buffer || p[-1] != '\\') {
	       *p= '\0';
	       break;
	    }
	}
	if ((count = (int) strlen(buffer)) < 3) continue;
	p = &buffer[count];
	while (p != buffer && isspace(*--p)) *p = '\0';

	if (!strnicmp(buffer,"unattended",10))
	{
	    un_attended=1;
	    continue;
	}

	if (!strnicmp(buffer,"ignore",6))
	{
	    ignore = 1;
	    continue;
	}

	if (!strnicmp(buffer,"tasknumber",10)) {
	   if (cmdlinetask) continue;

	   count = atoi(&buffer[10]);
	   /* AGL: task > 255 might actually work, check rest of the code */
	   if (count < 1 || count > 255) {
	      fprint(stderr,"Error: Invalid tasknumber '%s'\n",&buffer[10]);
	      goto err;
	   }
	   if (!task)
	      task = (word) count;
	   continue;
	}

	if (!strnicmp(buffer,"domain",6)) {
	   DOMAIN *domp, **dompp;
	   DOMPAT *patp, **patpp;
	   char *domname, *abbrev, *idxbase;
	   int i;

	   if (nakas) {
	      fprint(stderr,"Error: Domains must be specified BEFORE addresses\n");
	      goto err;
	   }

	   domname = strtok(&buffer[6]," ");
	   abbrev  = strtok(NULL," ");
	   idxbase = strtok(NULL," ");
	   if (!domname || !abbrev || !idxbase) {
	      fprint(stderr,"Error: Invalid number of domain parms\n");
	      goto err;
	   }
	   if (strlen(abbrev) > 8) abbrev[8] = '\0';

	   for (dompp = &domain; (domp = *dompp) != NULL; dompp = &domp->next) {
	       i = stricmp(domp->domname,domname);
	       if (!i) {
		  fprint(stderr,"Error: Duplicate domain name '%s'\n",domname);
		  goto err;
	       }
	       if (i > 0) break;	  /* sort low to high */
	   }
	   domp = (DOMAIN *) calloc (1,sizeof (DOMAIN));
	   if (!domp) {
	      fprint(stderr,"Error: Can't allocate space for domain struct\n");
	      goto err;
	   }
	   domp->domname   = ctl_string(domname);
	   domp->abbrev    = ctl_string(abbrev);
	   domp->idxbase   = stricmp(idxbase,"nolist") ? ctl_string(idxbase) : NULL;
	   domp->timestamp = 0L;
	   domp->flagstamp = 0L;
	   domp->number    = 0;
	   domp->errors    = false;
	   domp->next	   = *dompp;
	   *dompp = domp;

	   while ((p = strtok(NULL," ")) != NULL) {
		 for (patpp = &dompat; (patp = *patpp) != NULL; patpp = &patp->next) {
		     i = stricmp(patp->pattern,p);
		     if (i <= 0) break;     /* sort high to low !! */
		 }
		 if (!i) continue;	    /* Duplicate pattern.. */
		 patp = (DOMPAT *) calloc (1,sizeof (DOMPAT));
		 if (!patp) {
		    fprint(stderr,"Error: Can't allocate space for dompat struct\n");
		    goto err;
		 }
		 patp->pattern = ctl_string(p);
		 patp->domptr  = domp;
		 patp->next    = *patpp;
		 *patpp = patp;
	   }

	   continue;
	}

	if (!strnicmp(buffer,"nodomhold",9)) {
	   nodomhold = true;
	   continue;
	}

#if 0
	if (!strnicmp(buffer,"sdmach",6)) {
	   p=skip_blanks(&buffer[6]);
	   machbps = sdmach = limitmach = 0;
	   sscanf(p,"%d %lu %d",&sdmach,&machbps,&limitmach);
	   continue;
	}
#endif

	if (!strnicmp(buffer,"fkey",4)) {
	   p = skip_blanks(&buffer[4]);
	   count = atoi(p);
	   if (count < 1 || count > 12) {
	      fprint(stderr,"Error: Invalid fkey '%s'\n",p);
	      goto err;
	   }
	   p = skip_blanks(skip_to_blank(p));
	   funkeys[count] = ctl_string(p);
	   continue;
	}

	if (!strnicmp(buffer,"video",5)) {
	   p = skip_blanks(&buffer[5]);
	   if (!strnicmp(p,"bios",4))
	      win_setvdudirect(false);
	   else if (!strnicmp(p,"direct",6))
	      win_setvdudirect(true);
	   else {
	      fprint(stderr,"Error: Unknown video option '%s'\n",p);
	      goto err;
	   }
	   continue;
	}

	if (!strnicmp(buffer,"screenblank",11)) {
	   blank_secs = atoi(skip_blanks(&buffer[11]));
	   continue;
	}

	if (!strnicmp(buffer,"colors",6) || !strnicmp(buffer,"colours",7)) {
	   char *tag[7];
	   byte  col[7];
	   byte  n, i;
	   struct _win_colours *wincolptr;

	   p = skip_to_blank(buffer);
	   for (n = 0, p = strtok(p," "); p && n < 7; p = strtok(NULL," "))
	       tag[n++] = p;

	   for (i = 1; i < n; i++)
	       col[i] = get_colour(tag[i]);
	   for (i = n; i < 7; i++)
	       col[i] = col[1];

	   if (!stricmp(tag[0],"menu")) {
	      xenwin_colours.menu.normal = col[1]; xenwin_colours.menu.hotnormal = col[2];
	      xenwin_colours.menu.select = col[3]; xenwin_colours.menu.inactive  = col[4];
	      xenwin_colours.menu.border = col[5]; xenwin_colours.menu.title	 = col[6];
	      continue;
	   }
	   else if (!stricmp(tag[0],"select")) {
	      xenwin_colours.select.normal    = col[1]; xenwin_colours.select.select	   = col[2];
	      xenwin_colours.select.border    = col[3]; xenwin_colours.select.title	   = col[4];
	      xenwin_colours.select.scrollbar = col[5]; xenwin_colours.select.scrollselect = col[6];
	      continue;
	   }

	   if	   (!stricmp(tag[0],"desktop"))  wincolptr = &xenwin_colours.desktop;
	   else if (!stricmp(tag[0],"top"))	 wincolptr = &xenwin_colours.top;
	   else if (!stricmp(tag[0],"schedule")) wincolptr = &xenwin_colours.schedule;
	   else if (!stricmp(tag[0],"stats"))	 wincolptr = &xenwin_colours.stats;
	   else if (!stricmp(tag[0],"status"))	 wincolptr = &xenwin_colours.status;
	   else if (!stricmp(tag[0],"outbound")) wincolptr = &xenwin_colours.outbound;
	   else if (!stricmp(tag[0],"modem"))	 wincolptr = &xenwin_colours.modem;
	   else if (!stricmp(tag[0],"log"))	 wincolptr = &xenwin_colours.log;
	   else if (!stricmp(tag[0],"mailer"))	 wincolptr = &xenwin_colours.mailer;
	   else if (!stricmp(tag[0],"term"))	 wincolptr = &xenwin_colours.term;
	   else if (!stricmp(tag[0],"statline")) wincolptr = &xenwin_colours.statline;
	   else if (!stricmp(tag[0],"file"))	 wincolptr = &xenwin_colours.file;
	   else if (!stricmp(tag[0],"script"))	 wincolptr = &xenwin_colours.script;
	   else if (!stricmp(tag[0],"help"))	 wincolptr = &xenwin_colours.help;
	   else {
	      fprint(stderr,"Error: Unknown window name (colours) '%s'\n",tag[0]);
	      goto err;
	   }

	   wincolptr->normal	= col[1];
	   wincolptr->border	= col[2];
	   wincolptr->title	= col[3];
	   wincolptr->template	= col[4];
	   wincolptr->hotnormal = col[5];
	   continue;
	}

	if (!strnicmp(buffer,"mailer",6)) {
	   for (p = strtok(&buffer[6]," "); p; p = strtok(NULL," ")) {
	       if      (!stricmp(p,"no-hydra"	  )) ourbbs.capabilities &= ~DOES_HYDRA;
	       else if (!stricmp(p,"opt-xonxoff"  )) hydra_options |= HOPT_XONXOFF;
	       else if (!stricmp(p,"opt-telenet"  )) hydra_options |= HOPT_TELENET;
	       else if (!stricmp(p,"opt-ctlchrs"  )) hydra_options |= HOPT_CTLCHRS;
	       else if (!stricmp(p,"opt-highctl"  )) hydra_options |= HOPT_HIGHCTL;
	       else if (!stricmp(p,"opt-highbit"  )) hydra_options |= HOPT_HIGHBIT;
	       else if (!stricmp(p,"no-reqfirst"  )) ourbbs.options &= ~EOPT_REQFIRST;

	       else if (!stricmp(p,"no-directzap" )) ourbbs.capabilities &= ~EMSI_DZA;
	       else if (!stricmp(p,"no-zedzap"	  )) ourbbs.capabilities &= ~ZED_ZAPPER;
	       else if (!stricmp(p,"no-zedzip"	  )) ourbbs.capabilities &= ~ZED_ZIPPER;
	       else if (!stricmp(p,"no-sealink"   )) ourbbs.capabilities &= ~EMSI_SLK;

	       else if (!stricmp(p,"no-smart"	  )) ourbbsflags |= NO_SMART_MAIL;
	       else if (!stricmp(p,"no-emsi"	  )) ourbbsflags |= NO_EMSI;
	       else if (!stricmp(p,"no-pwforce"   )) ourbbsflags |= NO_PWFORCE;
	       else if (!stricmp(p,"no-pickup"	  )) pickup=0;
	       else if (!stricmp(p,"no-overdrive" )) no_overdrive = 1;

	       else if (!stricmp(p,"no-init"	  )) ourbbsflags |= NO_INIT;
	       else if (!stricmp(p,"no-setdcb"	  )) ourbbsflags |= NO_SETDCB;
	       else if (!stricmp(p,"no-dirscan"   )) ourbbsflags |= NO_DIRSCAN;

	       else if (!stricmp(p,"collide"	  )) ourbbsflags &= ~NO_COLLIDE;
	       else {
		  fprint(stderr,"Error: Unknown mailer flag '%s'",p);
		  goto err;
	       }

	       if (!(ourbbs.capabilities & ZED_ZAPPER) || hydra_options)
		  ourbbs.capabilities &= ~EMSI_DZA;
	   }
	   continue;
	}

	if (!strnicmp(buffer,"scan_semaphore",14)) {
	   sema_secs = atoi(skip_blanks(&buffer[14])) * 10;
	   continue;
	}

	if (!strnicmp(buffer,"emsi_addresses",14)) {
	   p = skip_blanks(&buffer[14]);
	   emsi_adr = atoi(p);
	   continue;
	}

	if (!strnicmp(buffer,"emsi_ident",10)) {
	   char *ident_name,  *ident_city,  *ident_sysop,
		*ident_phone, *ident_speed, *ident_flags;
	   char *q;

	   p = skip_blanks(&buffer[10]);
	   while ((q = strchr(p,'_')) != NULL) *q = ' ';
	   sprint(buffer,p,task,task,task);
	   ident_name  = strtok(buffer,",");
	   ident_city  = strtok(NULL,",");
	   ident_sysop = strtok(NULL,",");
	   ident_phone = strtok(NULL,",");
	   ident_speed = strtok(NULL,",");
	   ident_flags = strtok(NULL,"\n");	 /* flags contains , itself! */
	   if (ident_name && ident_city && ident_sysop &&
	       ident_phone && ident_speed && ident_flags) {
	      emsi_ident.name  = ctl_string(ident_name);
	      emsi_ident.city  = ctl_string(ident_city);
	      emsi_ident.sysop = ctl_string(ident_sysop);
	      emsi_ident.phone = ctl_string(ident_phone);
	      emsi_ident.speed = ctl_string(ident_speed);
	      emsi_ident.flags = ctl_string(ident_flags);
	   }
	   else {
	      fprint(stderr,"Error: Bad/missing emsi_ident field\n");
	      goto err;
	   }

	   continue;
	}

	if (!strnicmp(buffer,"iemsi_profile",13)) {
	   char *iname, *ialias, *ilocation, *idata, *ivoice, *ibirthdate;
	   int iday, imonth, iyear;
	   struct tm t;
	   char *q;

	   p = skip_blanks(&buffer[13]);
	   while ((q = strchr(p,'_')) != NULL) *q = ' ';
	   iname      = strtok(p,",");
	   ialias     = strtok(NULL,",");
	   ilocation  = strtok(NULL,",");
	   idata      = strtok(NULL,",");
	   ivoice     = strtok(NULL,",");
	   ibirthdate = strtok(NULL,",");
	   if (iname && ialias && ilocation && idata && ivoice && ibirthdate) {
	      iemsi_profile.name      = ctl_string(iname);
	      iemsi_profile.alias     = ctl_string(ialias);
	      iemsi_profile.location  = ctl_string(ilocation);
	      iemsi_profile.data      = ctl_string(idata);
	      iemsi_profile.voice     = ctl_string(ivoice);

	      sscanf(ibirthdate,"%d-%d-%d",&iday,&imonth,&iyear);
	      if (iyear < 1900) iyear += 1900;
	      t.tm_mday = iday; t.tm_mon = imonth - 1; t.tm_year = iyear - 1900;
	      t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
	      t.tm_isdst = 0;
	      iemsi_profile.birthdate = mktime(&t);
	   }
	   else {
	      fprint(stderr,"Error: Bad/missing iemsi_profile field\n");
	      goto err;
	   }

	   continue;
	}

	if (!strnicmp(buffer,"iemsi_password",14)) {
	   IEMSI_PWD *ip, **ipp;
	   char *ipassword, *iptr = NULL;

	   if ((ipassword = strtok(&buffer[14]," ")) == NULL ||
	       (p = strtok(NULL," ")) == NULL) {
	      fprint(stderr,"Error: Bad/missing iemsi_password field\n");
	      goto err;
	   }

	   do {
	      for (ipp = &iemsi_pwd; (ip = *ipp) != NULL; ipp = &ip->next);
	      if ((ip = (IEMSI_PWD *) calloc(1,sizeof (IEMSI_PWD))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for iemsi_password\n");
		 goto err;
	      }

	      ip->pattern = ctl_string(p);
	      if (iptr) ip->password = iptr;
	      else	ip->password = iptr = ctl_string(ipassword);
	      ip->next = *ipp;
	      *ipp = ip;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"alloc_debug",11)) {
	   alloc_debug = true;
	   continue;
	}

	if (!strnicmp(buffer,"hydra_debug",11)) {
	   hydra_debug = true;
	   continue;
	}

	if (!strnicmp(buffer,"hydra_speeds",12)) {
	   sscanf(&buffer[12],"%lu %lu",&hydra_minspeed,&hydra_maxspeed);
	   continue;
	}

	if (!strnicmp(buffer,"hydra_nofdx",11)) {
	   hydra_nofdx = ctl_string(&buffer[11]);
	   continue;
	}

	if (!strnicmp(buffer,"hydra_fdx",9)) {
	   hydra_fdx = ctl_string(&buffer[9]);
	   continue;
	}

	if (!strnicmp(buffer,"hydra_txwindow",14)) {
	   hydra_txwindow = atol(&buffer[14]);
	   continue;
	}

	if (!strnicmp(buffer,"hydra_rxwindow",14)) {
	   hydra_rxwindow = atol(&buffer[14]);
	   continue;
	}

	if (!strnicmp(buffer,"zmodem_txwindow",15)) {
	   zmodem_txwindow = atol(&buffer[15]);
	   continue;
	}

	if (!strnicmp(buffer,"ptt_cost",8) || !strnicmp(buffer,"ptt_tick",8)) {
#if PC
	   sscanf(&buffer[8],"%x %hu %hu %s",
		  &ptt_port,&ptt_irq,&ptt_cost,ptt_valuta);
	   if (ptt_irq > 15) {
	      fprint(stderr,"Error: Invalid ptt_cost IRQ %u\n",ptt_irq);
	      goto err;
	   }
#endif
	   continue;
	}

	if (!strnicmp(buffer,"request-on-us",13))
	{
	    ourbbsflags &= ~NO_REQ_ON;
	    continue;
	}

	if (!strnicmp(buffer,"address",7))
	{
	    if (nakas == 50) {
	       fprint(stderr,"Error: Too many address statements\n");
	       goto err;
	    }

	    p = skip_blanks(&buffer[7]);
	    count = getaddress(p,&aka[nakas].zone, &aka[nakas].net,
				 &aka[nakas].node, &aka[nakas].point);
	    if (count != 3 && count != 7) {
	       fprint(stderr,"Error: Invalid address '%s'\n",p);
	       goto err;
	    }
	    p = skip_blanks(skip_to_blank(p));

	    /* special HCC kludge too make sure there won't be 500/-1 anymore */
	    if (*p == '*') {
	       aka[nakas].usepnt = 1;
	       p = skip_blanks(skip_to_blank(p));
	    }
	    else if (*p == '+') {
	       aka[nakas].usepnt = 2;
	       p=skip_blanks(skip_to_blank(p));
	    }
	    else
	       aka[nakas].usepnt = 0;

	    aka[nakas].pointnet     = 0;
	    if (*p && isdigit(*p)) {
	       if (!(aka[nakas].pointnet = atoi(p)) &&
		   (!aka[nakas].point || aka[nakas].usepnt != 2)) {
		  fprint(stderr,"Error: Invalid pointnet '%s'\n",p);
		  goto err;
	       }
	    }
	    if (!aka[nakas].pointnet)
	       aka[nakas].usepnt = 2;

	    if (!nakas) {
	       ourbbs.zone  = aka[0].zone;
	       ourbbs.net   = aka[0].net;
	       ourbbs.node  = aka[0].node;
	       ourbbs.point = aka[0].point;
	    }
	    nakas++;

	    continue;
	}

	if (!strnicmp(buffer,"password",8)) {
	   PWD *pp, **ppp;
	   char *password, *pptr = NULL;
	   int zone, net, node, point;

	   if ((password = strtok(&buffer[8]," ")) == NULL ||
	       (p = strtok(NULL," ")) == NULL) {
	      fprint(stderr,"Error: Bad/missing password field\n");
	      goto err;
	   }
	   if (strlen(password) > 39) {
	      fprint(stderr,"Error: Password too long\n");
	      goto err;
	   }

	   do {
	      if (!getaddress(p,&zone,&net,&node,&point)) {
		 fprint(stderr,"Error: Bad password address '%s'\n",p);
		 goto err;
	      }

	      for (ppp = &pwd; (pp = *ppp) != NULL; ppp = &pp->next);
	      if ((pp = (PWD *) calloc(1,sizeof (PWD))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for password\n");
		 goto err;
	      }

	      pp->zone	   = zone;
	      pp->net	   = net;
	      pp->node	   = node;
	      pp->point    = point;
	      if (pptr) pp->password = pptr;
	      else	pp->password = pptr = ctl_string(password);
	      pp->next = *ppp;
	      *ppp = pp;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"alias",5)) {
	   ALIAS *ap, **app;
	   char *q;
	   int zone, net, node, point;
	   int my_aka;
	   int i;

	   if ((p = strtok(&buffer[5]," ")) == NULL ||
	       !getaddress(p,&zone,&net,&node,&point) ||
	       (p = strtok(NULL," ")) == NULL) {
	      fprint(stderr,"Error: Missing alias field\n");
	      goto err;
	   }

	   my_aka = -1;
	   for (i = 0; i < nakas; i++) {
	       if (aka[i].zone == zone && aka[i].net == net &&
		   aka[i].node == node && aka[i].point == point) {
		  my_aka = i;
		  break;
	       }
	   }
	   if (my_aka < 0) {
	      fprint(stderr,"Error: Alias %d:%d/%d%s not declared\n",
			    zone, net, node, pointnr(point));
	      error++;
	      goto err;
	   }

	   do {
	      for (app = &alias; (ap = *app) != NULL && strcmp(ap->pattern,"*"); app = &ap->next);
	      if ((ap = (ALIAS *) calloc(1,sizeof (ALIAS))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for alias\n");
		 goto err;
	      }

	      ap->my_aka  = my_aka;
	      if ((i = strlen(p)) > 2 && !strcmp(&p[i-2],".0")) p[i-2] = '\0';
	      strlwr(p);
	      while ((q = strstr(p,"all")) != NULL) {
		    *q = '*';
		    strcpy(q + 1,q + 3);
	      }
	      ap->pattern = ctl_string(p);
	      ap->next = *app;
	      *app = ap;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"phone",5)) {
	   PHONE *pp, **ppp;
	   int zone, net, node, point;
	   int dummy;
	   char *q;

	   if ((p = strtok(&buffer[5]," ")) == NULL ||
	       ((count = getaddress(p,&zone,&net,&node,&point)) != 3 && count != 7) ||
	       (p = strtok(NULL," ")) == NULL) {
	      fprint(stderr,"Error: Bad/missing phone field\n");
	      goto err;
	   }

	   do {
	      if (strpbrk(p,":/") &&
		  (count = getaddress(p,&dummy,&dummy,&dummy,&dummy)) != 3 && count != 7) {
		 fprint(stderr,"Error: Bad/missing phone field\n");
		 goto err;
	      }

	      for (ppp = &phone; (pp = *ppp) != NULL; ppp = &pp->next);
	      if ((pp = (PHONE *) calloc(1,sizeof (PHONE))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for phone\n");
		 goto err;
	      }

	      pp->zone	 = zone;
	      pp->net	 = net;
	      pp->node	 = node;
	      pp->point  = point;
	      pp->number = ctl_string(p);
	      for (q = pp->number; (q = strchr(q,',')) != NULL; q++) {
		  if (q == pp->number || q[-1] != '\\') {
		     *q++ = '\0';
		     if (*q)	   /* part of pp->number, already allocated! */
			pp->nodeflags = q;
		     break;
		  }
	      }
	      pp->next = *ppp;
	      *ppp = pp;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"okdial",6)) {
	   OKDIAL *np, **npp;
	   boolean allowed;
	   char *q;

	   if ((p = strtok(&buffer[6]," ")) == NULL) {
	      fprint(stderr,"Error: Missing okdial field\n");
	      goto err;
	   }

	   do {
	      for (npp = &okdial; (np = *npp) != NULL; npp = &np->next);
	      if ((np = (OKDIAL *) calloc(1,sizeof (OKDIAL))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for okdial\n");
		 goto err;
	      }

	      while ((q = strstr(p,"All")) != NULL) {
		    q[0] = '*';
		    strcpy(q + 1,q + 3);
	      }
	      if (*p == '-') {
		 allowed = false;
		 p++;
	      }
	      else
		 allowed = true;
	      if (!*p) continue;
	      if (isalpha(*p) || (*p != '*' && !strpbrk(p,":/.@"))) {
		 sprint(buffer,"*%s*",p);
		 p = buffer;
	      }
	      np->allowed = allowed;
	      np->pattern = ctl_string(p);
	      np->next = *npp;
	      *npp = np;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"dialback",8)) {
	   DIALBACK *dp, **dpp;
	   byte dflags = 0;
	   PERIOD_MASK dpermask;
	   char *dperstr, *dnumber, *dstring;

	   if ((p = strtok(&buffer[8]," ")) != NULL) {
	      if (!strnicmp(p,"bbs",3))
		 dflags |= APP_BBS;
	      else if (!strnicmp(p,"mail",4) || !strnicmp(p,"always",6))
		 dflags |= APP_MAIL;
	   }
	   dnumber = strtok(NULL," ");
	   if ((dstring = strtok(NULL,"")) != NULL)
	      dstring = skip_blanks(dstring);

	   memset(&dpermask,0xff,sizeof (PERIOD_MASK));
	   if (p && (dperstr = strchr(p,',')) != NULL)
	      dperstr++;
	   if (!p || (dperstr && !get_periodmask(dperstr,&dpermask)) ||
	       !dflags || !dnumber || !dstring || !dstring[0]) {
	      fprint(stderr,"Error: Missing dialback field\n");
	      goto err;
	   }

	   for (dpp = &dialback; (dp = *dpp) != NULL; dpp = &dp->next);
	   if ((dp = (DIALBACK *) calloc(1,sizeof (DIALBACK))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for dialback\n");
	      goto err;
	   }

	   dp->flags  = dflags;
	   memmove(&dp->per_mask,&dpermask,sizeof (PERIOD_MASK));
	   dp->number = ctl_string(dnumber);
	   if ((p = strchr(dp->number,',')) != NULL) {
	      *p++ = '\0';
	      if (*p)		   /* part of dp->number, already allocated! */
		 dp->nodeflags = p;
	   }

	   for (p = dstring; *p; p++) if (*p & 0x80) *p &= 0x7f;
	   if ((p = strchr(dstring,',')) != NULL) {
	      *p++ = '\0';
	      dp->password = ctl_string(p);
	   }
	   sprint(buffer,strupr(dstring));
	   if (dp->password) strcat(buffer,"\r");
	   dp->string	   = ctl_string(buffer);

	   dp->next = *dpp;
	   *dpp = dp;

	   continue;
	}

	if (!strnicmp(buffer,"freepoll_dial",13)) {
	   FREEDIAL *np, **npp;

	   if ((p = strtok(&buffer[13]," ")) == NULL) {
	      fprint(stderr,"Error: Missing freepoll_dial field\n");
	      goto err;
	   }

	   do {
	      for (npp = &mdm.freedial; (np = *npp) != NULL; npp = &np->next);
	      if ((np = (FREEDIAL *) calloc(1,sizeof (FREEDIAL))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for freepoll_dial\n");
		 goto err;
	      }

	      np->address = ctl_string(p);
	      np->next = *npp;
	      *npp = np;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"freepoll_answer",15)) {
	   FREEANSWER *np, **npp;

	   if ((p = strtok(&buffer[15]," ")) == NULL) {
	      fprint(stderr,"Error: Missing freepoll_answer field\n");
	      goto err;
	   }

	   do {
	      for (npp = &mdm.freeanswer; (np = *npp) != NULL; npp = &np->next);
	      if ((np = (FREEANSWER *) calloc(1,sizeof (FREEANSWER))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for freepoll_answer\n");
		 goto err;
	      }

	      np->address = ctl_string(p);
	      if ((p = strchr(np->address,',')) == NULL) {
		 fprint(stderr,"Error: Missing freepoll_answer callerid field\n");
		 goto err;
	      }
	      *p++ = '\0';
	      np->callerid = p;
	      if ((p = strchr(np->callerid,',')) != NULL) {
		 *p++ = '\0';
		 np->minbytes = atol(p) * 1024UL;
	      }

	      np->next = *npp;
	      *npp = np;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"freepoll_pattern",16))
	{
	    mdm.freepattern=ctl_string(&buffer[16]);
	    continue;
	}

#if 0
	if (!strnicmp(buffer,"returncall",10)) {
	   RETURNCALL *rp, **rpp;
	   char *rpattern, *rstring;

	   if ((rpattern = strtok(&buffer[10]," ")) != NULL &&
	       (rstring = strtok(NULL,"")) != NULL)
	      rstring = skip_blanks(rstring);

	   if (!rpattern || !rstring || !rstring[0]) {
	      fprint(stderr,"Error: Missing returncall field\n");
	      goto err;
	   }

	   for (rpp = &returncall; (rp = *rpp) != NULL; rpp = &rp->next);
	   if (!(rp = (RETURNCALL *) calloc(1,sizeof (RETURNCALL)))) {
	      fprint(stderr,"Error: Can't allocate for returncall\n");
	      goto err;
	   }

	   rp->pattern = ctl_string(rpattern);
	   rp->string  = ctl_string(rstring);
	   rp->next = *rpp;
	   *rpp = rp;

	   continue;
	}
#endif

	if (!strnicmp(buffer,"prefixsuffix",12)) {
	   PREFIXSUFFIX *pp, **ppp;
	   char *ppattern, *pprefix, *psuffix;

	   if ((p = strtok(&buffer[12]," ")) == NULL || !strchr(p,'/')) {
	      fprint(stderr,"Error: Bad/missing prefixsuffix field\n");
	      goto err;
	   }

	   pprefix = p;
	   psuffix = strchr(p,'/');
	   *psuffix++ = '\0';

	   ppattern = strtok(NULL," ");

	   do {
	      for (ppp = &prefixsuffix; (pp = *ppp) != NULL && strcmp(pp->pattern,"*"); ppp = &pp->next);
	      if ((pp = (PREFIXSUFFIX *) calloc(1,sizeof (PREFIXSUFFIX))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for prefixsuffix\n");
		 goto err;
	      }

	      if (*pprefix) pp->prefix = ctl_string(pprefix);
	      if (*psuffix) pp->suffix = ctl_string(psuffix);

	      if (ppattern) {
		 while ((p = strstr(ppattern,"All")) != NULL) {
		       p[0] = '*';
		       strcpy(p + 1,p + 3);
		 }
		 if (isalpha(*ppattern)) {
		    sprint(buffer,"*%s*",ppattern);
		    ppattern = buffer;
		 }
		 pp->pattern = ctl_string(ppattern);
	      }
	      else
		 pp->pattern = "*";

	      pp->next	  = *ppp;
	      *ppp = pp;
	   } while ((ppattern = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"xlatdash",8)) {
	   xlatdash = ctl_string(&buffer[8]);
	   continue;
	}

	if (!strnicmp(buffer,"dial",4)) {
	   DIALXLAT *dp, **dpp;
	   char *xlatnumber, *xlatprefix, *xlatsuffix, *xlatpattern;

	   if ((p = strtok(&buffer[4]," ")) != NULL && strchr(p,'/'))
	      xlatnumber = NULL;
	   else {
	      xlatnumber = p;
	      if ((p = strtok(NULL," ")) == NULL || !strchr(p,'/')) {
		 fprint(stderr,"Error: Bad/missing dial field\n");
		 goto err;
	      }
	   }

	   xlatpattern = strtok(NULL," ");

	   do {
	      for (dpp = &dialxlat; (dp = *dpp) != NULL && dp->number; dpp = &dp->next);
	      if ((dp = (DIALXLAT *) calloc(1,sizeof (DIALXLAT))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for dial\n");
		 goto err;
	      }

	      xlatprefix = p;
	      xlatsuffix = strchr(p,'/');
	      *xlatsuffix++ = '\0';

	      if (xlatnumber)  dp->number = ctl_string(xlatnumber);
	      if (*xlatprefix) dp->prefix = ctl_string(xlatprefix);
	      if (*xlatsuffix) dp->suffix = ctl_string(xlatsuffix);

	      if (xlatpattern) {
		 while ((p = strstr(xlatpattern,"All")) != NULL) {
		       p[0] = '*';
		       strcpy(p + 1,p + 3);
		 }
		 if (isalpha(*xlatpattern)) {
		    sprint(buffer,"*%s*",xlatpattern);
		    xlatpattern = buffer;
		 }
		 dp->pattern = ctl_string(xlatpattern);
	      }
	      else
		 dp->pattern = "*";

	      dp->next = *dpp;
	      *dpp = dp;
	   } while ((xlatpattern = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"ftpmail",7))
	{
	   FTPMAIL *mp, **mpp;
	   int zone, net, node, point;
	   char *host, *userid, *passwd;

	   if ((p = strtok(&buffer[7]," ")) == NULL ||
	       ((count = getaddress(p,&zone,&net,&node,&point)) != 3 && count != 7) ||
	       (host = strtok(NULL," ")) == NULL ||
	       (userid = strtok(NULL," ")) == NULL) {
	      fprint(stderr,"Error: Bad/missing ftpmail field\n");
	      goto err;
	   }

	   passwd = strtok(NULL," ");

	   for (mpp = &ftpmail; (mp = *mpp) != NULL; mpp = &mp->next);
	   if ((mp = (FTPMAIL *) calloc(1,sizeof (FTPMAIL))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for ftpmail\n");
	      goto err;
	   }

	   mp->zone   = zone;
	   mp->net    = net;
	   mp->node   = node;
	   mp->point  = point;
	   if ((p = strchr(host,':')) != NULL) {
	      mp->port = atoi(p + 1);
	      *p = '\0';
	   }
	   mp->host   = ctl_string(host);
	   mp->userid = ctl_string(userid);
	   if (passwd)
	      mp->passwd = ctl_string(passwd);
	   else {
	      if ((p = get_passw(zone,net,node,point)) == NULL) {
		 fprint(stderr,"Error: Missing password for ftpmail entry\n");
		 goto err;
	      }
	      mp->passwd = p;
	   }

	   mp->next = *mpp;
	   *mpp = mp;

	   continue;
	}

	if (!strnicmp(buffer,"script",6))
	{
	   SCRIPT *sp, **spp;
	   char *q;

	   p = skip_blanks(&buffer[6]);
	   q = skip_to_blank(p);
	   if (!*p || !*q || !*skip_blanks(q)) {
	      fprint(stderr,"Error: Missing script field\n");
	      goto err;
	   }
	   *q++ = '\0';
	   q = skip_blanks(q);

	   for (spp = &script; (sp = *spp) != NULL && stricmp(p,sp->tag); spp = &sp->next);
	   if (sp) {			  /* space */	   /* NUL */
	      p = myalloc((word) (strlen(sp->string) + 1 + strlen(q) + 1));
	      strcpy(p,sp->string);		/* append new string to old */
	      strcat(p," ");			/* with space to separate   */
	      strcat(p,q);
	      free(sp->string); 		/* replace and free old     */
	      sp->string = p;
	   }
	   else {
	      if ((sp = (SCRIPT *) calloc(1,sizeof (SCRIPT))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for script\n");
		 goto err;
	      }

	      sp->tag	 = ctl_string(p);
	      sp->string = ctl_string(q);

	      sp->next = *spp;
	      *spp = sp;
	   }

	   continue;
	}

	if (!strnicmp(buffer,"mailscript",10))
	{
	   MAILSCRIPT *sp, **spp;
	   SCRIPT *sptr;
	   char *tag, *loginname, *password;

	   if ((tag = strtok(&buffer[10]," ")) == NULL) {
	      fprint(stderr,"Error: Missing mailscript field\n");
	      goto err;
	   }
	   if ((loginname = strchr(tag,',')) != NULL) {
	      *loginname++ = '\0';
	      if ((password = strchr(loginname,',')) != NULL)
		 *password++ = '\0';
	      while ((p = strchr(loginname,'_')) != NULL) *p = ' ';
	   }
	   else
	      password = NULL;

	   for (sptr = script; sptr; sptr = sptr->next)
	       if (!stricmp(tag,sptr->tag)) break;
	   if (!sptr) {
	      fprint(stderr,"Error: Undefined script tag '%s'\n",tag);
	      goto err;
	   }

	   p = strtok(NULL," ");

	   do {
	      for (spp = &mailscript; (sp = *spp) != NULL && strcmp(sp->pattern,"*"); spp = &sp->next);
	      if ((sp = (MAILSCRIPT *) calloc(1,sizeof (MAILSCRIPT))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for mailscript\n");
		 goto err;
	      }

	      if (loginname) sp->loginname = ctl_string(loginname);
	      if (password)  sp->password  = ctl_string(password);
	      sp->pattern   = p ? ctl_string(p) : "*";
	      sp->scriptptr = sptr;

	      sp->next = *spp;
	      *spp = sp;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"termscript",10))
	{
	   TERMSCRIPT *sp, **spp;
	   SCRIPT *sptr;
	   char *tag, *loginname, *password;

	   if ((tag = strtok(&buffer[10]," ")) == NULL) {
	      fprint(stderr,"Error: Missing mailscript field\n");
	      goto err;
	   }
	   if ((loginname = strchr(tag,',')) != NULL) {
	      *loginname++ = '\0';
	      if ((password = strchr(loginname,',')) != NULL)
		 *password++ = '\0';
	      while ((p = strchr(loginname,'_')) != NULL) *p = ' ';
	   }
	   else
	      password = NULL;

	   for (sptr = script; sptr; sptr = sptr->next)
	       if (!stricmp(tag,sptr->tag)) break;
	   if (!sptr) {
	      fprint(stderr,"Error: Undefined script tag '%s'\n",tag);
	      goto err;
	   }

	   p = strtok(NULL," ");

	   do {
	      for (spp = &termscript; (sp = *spp) != NULL && strcmp(sp->pattern,"*"); spp = &sp->next);
	      if ((sp = (TERMSCRIPT *) calloc(1,sizeof (TERMSCRIPT))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for termscript\n");
		 goto err;
	      }

	      if (loginname) sp->loginname = ctl_string(loginname);
	      if (password)  sp->password  = ctl_string(password);
	      sp->pattern   = p ? ctl_string(p) : "*";
	      sp->scriptptr = sptr;

	      sp->next = *spp;
	      *spp = sp;
	   } while ((p = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"request",7))
	{
	    if (!*(reqpath = ctl_dir(&buffer[8]))) goto err;
	    sprint(buffer,"%sREQUEST.IDX",reqpath);
	    count=0;
	    if (fexist(buffer)) {
	       sprint(buffer,"%sREQUEST.DIR",reqpath);
	       if (fexist(buffer))
		  count++;
	    }
	    if (!count) {
	       fprint(stderr,"Error: Request control files not found\n");
	       free(reqpath);
	       reqpath = NULL;
	       goto err;
	    }
	    continue;
	}

	if (!strnicmp(buffer,"extreq",6))
	{
	    sprint(buffer,&buffer[6],task,task,task);
	    reqprog = ctl_string(buffer);
	    continue;
	}

	if (!strnicmp(buffer,"servreq",7)) {
	   SERVREQ *dp, **dpp;
	   char *filepat, *prog, *description;

	   filepat = strtok(&buffer[7]," ");
	   prog = strtok(NULL," ");
	   if ((description = strtok(NULL,"")) != NULL)
	      description = skip_blanks(description);

	   if (!filepat || !prog || !description || !description[0]) {
	      fprint(stderr,"Error: Missing servreq field\n");
	      goto err;
	   }

	   for (dpp = &servreq; (dp = *dpp) != NULL; dpp = &dp->next);
	   if ((dp = (SERVREQ *) calloc(1,sizeof (SERVREQ))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for servreq\n");
	      goto err;
	   }

	   dp->filepat	   = ctl_string(filepat);
	   dp->progname    = ctl_string(prog);
	   dp->description = ctl_string(description);
	   dp->next = *dpp;
	   *dpp = dp;

	   continue;
	}

	if (!strnicmp(buffer,"timesync",8)) {
	   TIMESYNC *dp, **dpp;
	   int zone, net, node, point;
	   char *timediff, *maxdiff, *dstchange;

	   p = strtok(&buffer[8]," ");
	   timediff  = strtok(NULL," ");
	   maxdiff   = strtok(NULL," ");
	   dstchange = strtok(NULL," ");

	   if (!p || ((count = getaddress(p,&zone,&net,&node,&point)) != 3 && count != 7)) {
	      fprint(stderr,"Error: Missing or invalid timesync field\n");
	      goto err;
	   }

	   for (dpp = &timesync; (dp = *dpp) != NULL; dpp = &dp->next);
	   if ((dp = (TIMESYNC *) calloc(1,sizeof (TIMESYNC))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for timesync\n");
	      goto err;
	   }

	   dp->zone	   = zone;
	   dp->net	   = net;
	   dp->node	   = node;
	   dp->point	   = point;
	   if (timediff) {
	      if (sscanf(timediff,"%ld:%d",&dp->timediff,&count) == 2)
		 dp->timediff = dp->timediff * 60L + count;
	      dp->timediff *= 60L;
	      if (maxdiff) {
		 if (sscanf(maxdiff,"%ld:%d",&dp->maxdiff,&count) == 2)
		    dp->maxdiff = dp->maxdiff * 60L + count;
		 dp->maxdiff *= 60L;
		 if (dstchange && !stricmp(dstchange,"DST"))
		    dp->dstchange = true;
	      }
	   }
	   dp->next = *dpp;
	   *dpp = dp;

	   continue;
	}

	if (!strnicmp(buffer,"gmtdiff",7)) {
	   if (sscanf(&buffer[7],"%ld:%d",&gmtdiff,&count) == 2) {
	      gmtdiff *= 60L;
	      if (gmtdiff >= 0) gmtdiff += count;
	      else		gmtdiff -= count;
	   }
	   gmtdiff *= 60L;
	   continue;
	}

	if (!strnicmp(buffer,"period",6)) {
	   static char *per_field[] = { "day-of-week", "day-of-month", "month",
					"year", "hh:mm", "hour", "minute" };
	   PERIOD *perp;
	   char *av[13];
	   int ac = 0;

	   if (num_periods == MAX_PERIODS) {
	      fprint(stderr,"Error: Maximum number of periods reached\n");
	      goto err;
	   }

	   for (av[ac] = strtok(&buffer[6]," "); ac < 13 && av[ac]; av[ac] = strtok(NULL," ")) ac++;
	   if (ac != 7 && ac != 8 && ac != 12) {	/* 2 + date [+ date] */
	      fprint(stderr,"Error: Invalid number of period parameters (%d)\n",ac);
	      goto err;
	   }

	   for (count = 0; count < num_periods; count++) {
	       if (!stricmp(av[0],periods[count]->tag)) {
		  fprint(stderr,"Error: Duplicate period tag '%s'\n",av[0]);
		  goto err;
	       }
	   }

	   if ((perp = (PERIOD *) calloc(1,sizeof (PERIOD))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for period\n");
	      goto err;
	   }

	   strncpy(perp->tag,av[0],8);
	   av[0][8] = '\0';

	   if (!stricmp(av[1],"Local"))
	      perp->flags |= PER_LOCAL;
	   else if (stricmp(av[1],"GMT") && stricmp(av[1],"UTC")) {
	      fprint(stderr,"Error: Invalid period timezone %s\n",av[1]);
	      goto err;
	   }

	   /* end first, because parse_pertime fiddles with av[] */
	   if (ac == 12) {
	      if ((count = period_parsetime(&av[7],perp->defend)) != 7) {
		 fprint(stderr,"Error: Invalid period end %s field\n",per_field[count]);
		 goto err;
	      }
	   }
	   else if (ac == 7)
	      perp->length = SEC_DAY;
	   else if (ac == 8) {
	      if (av[7][0] != '+') {
		 fprint(stderr,"Error: Invalid period length specification\n");
		 goto err;
	      }
	      if (sscanf(&av[7][1],"%lu:%d",&perp->length,&count) == 2)
		 perp->length = (perp->length * 60UL) + count;
	      perp->length *= SEC_MIN;
	   }

	   if ((count = period_parsetime(&av[2],perp->defbegin)) != 7) {
	      fprint(stderr,"Error: Invalid period begin %s field\n",per_field[count]);
	      goto err;
	   }

	   periods[num_periods++] = perp;

	   continue;
	}

	if (!strnicmp(buffer,"mailhour",8)) {
	   MAILHOUR *mp, **mpp;
	   PERIOD_MASK mpermask;
	   int mzone;

	   if (sscanf(&buffer[8],"%d %s", &mzone, buffer) != 2 ||
	       !get_periodmask(buffer,&mpermask)) {
	      fprint(stderr,"Error: Missing or invalid mailhour field\n");
	      goto err;
	   }

	   for (mpp = &mailhour; (mp = *mpp) != NULL; mpp = &mp->next);
	   if ((mp = (MAILHOUR *) calloc(1,sizeof (MAILHOUR))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for mailhour\n");
	      goto err;
	   }

	   mp->zone  = mzone;
	   memmove(&mp->per_mask,&mpermask,sizeof (PERIOD_MASK));
	   mp->next = *mpp;
	   *mpp = mp;

	   continue;
	}

	if (!strnicmp(buffer,"appkey",6)) {
	   char *hotkey, *menukey, *cmd, *text;

	   hotkey  = strtok(&buffer[6]," ");
	   menukey = strtok(NULL," ");
	   cmd	   = strtok(NULL," ");
	   text    = strtok(NULL,"");
	   if (!hotkey || !menukey || !cmd || !text) {
	      fprint(stderr,"Error: Invalid number of appkey parameters\n");
	      goto err;
	   }

	   while ((p = strchr(cmd,'_')) != NULL) *p = ' ';
	   text = skip_blanks(text);
	   while ((p = strchr(text,'_')) != NULL) *p = ' ';
	   if (!add_appkey(get_keycode(hotkey),get_keycode(menukey),cmd,text))
	      fprint(stderr,"Warning: appkey table full, ignoring '%s'\n", text);

	   continue;
	}

	if (!strnicmp(buffer,"loglevel",8))
	{
	    p=skip_blanks(&buffer[8]);
	    if (sscanf(p,"%d", &count)!=1 || count>6 || count<0)
	    {
		fprint(stderr,"Error: Invalid loglevel '%s'\n", p);
		goto err;
	    }
	    else loglevel=count;
	    continue;
	}

	if (!strnicmp(buffer,"cominfo",7))
	{
#if PC
	   word prt, adr, irq;

	   p = skip_blanks(&buffer[7]);
	   if (sscanf(p,"%hu=%hx,%hu",&prt,&adr,&irq) != 3 ||
	       !com_setport(prt,adr,irq)) {
	      fprint(stderr,"Error: Invalid cominfo settings '%s'\n",p);
	      goto err;
	   }
#endif
	   continue;
	}

	if (!strnicmp(buffer,"port",4)) {
	   if (cmdlineport) continue;

	   p = skip_blanks(&buffer[4]);
	   if (!*p) {
	      fprint(stderr,"Error: Port without number or device name\n");
	      goto err;
	   }

	   sprint(buffer,p,task);
	   if (isdigit(*buffer)) {
	      count = atoi(buffer);
#if PC
	      if (count < 0 || count > 64) {
#else
	      if (count < 1 || count > 64) {
#endif
		 fprint(stderr,"Error: Invalid com port '%s'\n",buffer);
		 goto err;
	      }
	      else {
		 mdm.port   = count;
		 mdm.comdev = NULL;
	      }
	   }
	   else {
	      mdm.port	 = 0;
	      mdm.comdev = ctl_string(strupr(buffer));
	   }

	   continue;
	}

	if (!strnicmp(buffer,"snoop",5)) {
#if OS2
	   if (cmdlinesnoop) continue;

	   p = skip_blanks(&buffer[5]);
	   if (!*p) {
	      fprint(stderr,"Error: Snoop without \\pipe\\name\n");
	      goto err;
	   }

	   sprint(buffer,p,task);
	   pipename = ctl_string(buffer);
#endif

	   continue;
	}

	if (!strnicmp(buffer,"mcp",3)) {
#if OS2
	   char *q;

	   p = strtok(&buffer[3]," ");
	   q = strtok(NULL," ");
	   if (!p || !q) {
	      fprint(stderr,"Error: Invalid number of mcp parameters\n");
	      goto err;
	   }

	   MCPbase  = ctl_string(p);
	   MCPtasks = (word) atoi(q);
#endif

	   continue;
	}

	if (!strnicmp(buffer,"speed",5)) {
	   p = strtok(&buffer[5]," ");
	   if (!strnicmp(p,"lock",4)) {
	      mdm.lock = true;
	      mdm.handshake |= 0x02;   /* Enable hardware handshaking */
	      p = strtok(NULL," ");
	    }

	    if (!p || (btmp = atol(p)) <= 0L) {
	       fprint(stderr,"Error: Invalid speed '%s'\n",p ? p : "");
	       goto err;
	    }
	    else {
		mdm.speed = btmp;
		p = strtok(NULL," ");
		if (p && isdigit(*p)) {
		   if ((count = atoi(p)) <= 0) {
		      fprint(stderr,"Error: Invalid speed divisor '%s'\n",p);
		      goto err;
		   }
		   else
		      mdm.divisor = (word) count;
		}
		else
		   mdm.divisor = 1;
	    }

	    continue;
	}

	if (!strnicmp(buffer,"shareport",9)) {
	   mdm.shareport = true;
	   continue;
	}

	if (!strnicmp(buffer,"slowmodem",9)) {
	   mdm.slowmodem = true;
	   continue;
	}

#if 0	/* replaced by 'RINGS <n>' below */
	if (!strnicmp(buffer,"firstring",9)) {
	   mdm.rings = 1;
	   continue;
	}
#endif

	if (!strnicmp(buffer,"rings",5)) {
	   count = atoi(&buffer[5]);
	   if (count < 1 || count > 50) {
	      fprint(stderr,"Error: Invalid rings '%s'\n",&buffer[5]);
	      goto err;
	   }
	   mdm.rings = (word) count;
	   continue;
	}

	if (!strnicmp(buffer,"slowdisk",8)) {
	   mdm.slowdisk = true;
	   continue;
	}

	if (!strnicmp(buffer,"nofossil",8)) {
	   mdm.nofossil = true;
	   continue;
	}

	if (!strnicmp(buffer,"winfossil",9)) {
	   mdm.winfossil = true;
	   continue;
	}

	if (!strnicmp(buffer,"dcdmask",7)) {
	   mdm.dcdmask = atoi(&buffer[7]);
	   continue;
	}

	if (!strnicmp(buffer,"handshake",9)) {
	   p = skip_blanks(&buffer[9]);
	   if	   (!strnicmp(p,"none",4)) mdm.handshake = 0;
	   else if (!strnicmp(p,"soft",4)) mdm.handshake |= 0x09;
	   else if (!strnicmp(p,"hard",4)) mdm.handshake |= 0x02;
	   else if (!strnicmp(p,"both",4)) mdm.handshake |= 0x0b;
	   else {
	      fprint(stderr,"Error: Invalid handshake flag '%s'\n",p);
	      goto err;
	   }
	   if (mdm.handshake & 0x09)
	      hydra_options |= HOPT_XONXOFF;
	   continue;
	}

	if (!strnicmp(buffer,"fifo",4)) {
	   p = &buffer[4];
	   mdm.fifo = !strnicmp(p,"off",3) ? 0 : atoi(p);
	   continue;
	}

	if (!strnicmp(buffer,"maxrings",8)) {
	   mdm.maxrings = (word) atoi(&buffer[8]);
	   continue;
	}

	if (!strnicmp(buffer,"mdmtimeout",10)) {
	   count = atoi(&buffer[10]);
	   if (count < 10 || count > 300) {
	      fprint(stderr,"Error: Invalid mdmtimeout '%s'\n",&buffer[10]);
	      goto err;
	   }
	   mdm.timeout = (word) (count * 10);
	   continue;
	}

	if (!strnicmp(buffer,"banner",6)) {
	   strcat(buffer,"\r\n");
	   banner = ctl_string(&buffer[6]);
	   continue;
	}

	if (!strnicmp(buffer,"apperrorlevel",13)) {
	   count = atoi(&buffer[13]);
	   if (count != 3 && (count < 8 || count > 255)) {
	      fprint(stderr,"Error: Invalid apperrorlevel '%s'\n",&buffer[13]);
	      goto err;
	   }
	   apperrorlevel = (word) count;
	   continue;
	}

	if (!strnicmp(buffer,"apptext",7)) {
	   strcat(buffer,"\\r");
	   apptext = ctl_string(&buffer[7]);
	   continue;
	}

	if (!strnicmp(buffer,"noapptext",9)) {
	   strcat(buffer,"\\r");
	   noapptext = ctl_string(&buffer[9]);
	   continue;
	}

	if (!strnicmp(buffer,"loginprompt",11)) {
	   loginprompt = ctl_string(&buffer[11]);
	   continue;
	}

	if (!strnicmp(buffer,"extapp",6)) {
	   EXTAPP *ep, **epp;
	   byte extflags = 0;
	   PERIOD_MASK extpermask;
	   char *extperstr, *extstring, *extopt, *extbatch, *extdesc;

	   if ((p = strtok(&buffer[6]," ")) != NULL) {
	      if (!strnicmp(p,"modem",5))
		 extflags |= APP_MODEM;
	      else if (!strnicmp(p,"bbs",3))
		 extflags |= APP_BBS;
	      else if (!strnicmp(p,"mail",4) || !strnicmp(p,"always",6))
		 extflags |= APP_MAIL;
	      else goto badapp;
	   }
	   extstring = strtok(NULL," ");
	   if ((extopt = strtok(NULL," ")) != NULL) {
	      if      (!stricmp(extopt,"spawn")) extflags |= APP_SPAWN;
	      else if (!stricmp(extopt,"batch")) extflags |= APP_BATCH;
	      else if (!stricmp(extopt,"exit"))  extflags |= APP_EXIT;
	      else goto badapp;
#if 0 /*OS2*/
	      if (extflags & (APP_BATCH | APP_EXIT)) {
		 fprint(stderr,"Error: extapp BATCH/EXIT not allowed under OS/2\n");
		 goto err;
	      }
#endif
	   }
	   extbatch  = strtok(NULL," ");
	   extdesc   = strtok(NULL,"");
	   memset(&extpermask,0xff,sizeof (PERIOD_MASK));
	   if (p && (extperstr = strchr(p,',')) != NULL)
	      extperstr++;
	   if (!p || (extperstr && !get_periodmask(extperstr,&extpermask)) ||
	       !extstring || !extopt || !extbatch || !extdesc) {
badapp:       fprint(stderr,"Error: Bad/missing extapp field\n");
	      goto err;
	   }

	   for (epp = &extapp; (ep = *epp) != NULL; epp = &ep->next);
	   if ((ep = (EXTAPP *) calloc(1,sizeof (EXTAPP))) == NULL) {
	      fprint(stderr,"Error: Can't allocate for extapp\n");
	      goto err;
	   }

	   ep->flags	   = extflags;
	   memmove(&ep->per_mask,&extpermask,sizeof (PERIOD_MASK));

	   for (p = extstring; *p; p++) {
	       if      (*p & 0x80)  *p &= 0x7f;
	       else if (*p == '\\') *p = 27;
	   }
	   if ((p = strchr(extstring,',')) != NULL) {
	      *p++ = '\0';
	      ep->password = ctl_string(p);
	   }
	   sprint(buffer,strupr(extstring));
	   if (ep->password) strcat(buffer,"\r");
	   ep->string	   = ctl_string(buffer);

	   ep->batchname   = ctl_string(extbatch);
	   ep->description = ctl_string(extdesc);
	   ep->next = *epp;
	   *epp = ep;

	   continue;
	}

	if (!strnicmp(buffer,"ringapp",7)) {
	   ringapp = ctl_string(&buffer[7]);
	   continue;
	}

	if (!strnicmp(buffer,"mailpack",8)) {
	   mailpack = ctl_string(&buffer[8]);
	   continue;
	}

	if (!strnicmp(buffer,"mailprocess",11)) {
	   mailprocess = ctl_string(&buffer[11]);
	   continue;
	}

	if (!strnicmp(buffer, "statuslog",9))
	{
	    p = skip_blanks(&buffer[9]);
	    *skip_to_blank(p) = '\0';
	    sprint(buffer,p,task);
	    logname= ctl_string(buffer);
	    if (!create_dir(logname)) goto err;
	    if ((log = sfopen(logname,"at",DENY_WRITE)) == NULL) {
	       fprint(stderr,"Error: Could not create logfile '%s'!\n",logname);
	       free(logname);
	       logname = NULL;
	       goto err;
	    }
	    setvbuf(log,NULL,_IOLBF,256);
	    continue;
	}

	if (!strnicmp(buffer, "timelog",7))
	{
	    p = skip_blanks(&buffer[7]);
	    *skip_to_blank(p) = '\0';
	    sprint(buffer,p,task);
	    p = ctl_string(buffer);
	    if (!create_dir(p)) goto err;
	    if ((cost = sfopen(p,"at",DENY_WRITE)) == NULL) {
	       fprint(stderr,"Error: Could not create timelog '%s'!\n",p);
	       goto err;
	    }
	    free(p);
	    setvbuf(cost,NULL,_IOLBF,256);
	    continue;
	}

	if (!strnicmp(buffer,"capture",7))
	{
	    if (capture) free(capture);
	    p = skip_blanks(&buffer[7]);
	    *skip_to_blank(p) = '\0';
	    sprint(buffer,p,task);
	    capture = ctl_string(buffer);
	    continue;
	}

	if (!strnicmp(buffer,"system",6))
	{
	    sprint(buffer,skip_blanks(&buffer[6]),task);
	    strncpy(ourbbs.name,buffer,60);
	    continue;
	}

	if (!strnicmp(buffer,"sysop",5))
	{
	    p=skip_blanks(&buffer[5]);
	    strncpy(ourbbs.sysop,p,40);
	    continue;
	}

	if (!strnicmp(buffer,"hold",4))
	{
	    if (!*(hold=ctl_dir(&buffer[4]))) goto err;
	    hold[strlen(hold)-1]='\0';	/* get rid of trailing \ */
	    continue;
	}

	if (!strnicmp(buffer,"netfile",7))
	{
	    if (!*(filepath=ctl_dir(&buffer[7]))) goto err;
	    continue;
	}

	if (!strnicmp(buffer,"safenetfile",11))
	{
	    if (!*(safepath=ctl_dir(&buffer[11]))) goto err;
	    continue;
	}

	if (!strnicmp(buffer,"localnetfile",12))
	{
	    if (!*(localpath=ctl_dir(&buffer[12]))) goto err;
	    continue;
	}

	if (!strnicmp(buffer,"faxindir",8))
	{
	    if (!*(fax_indir=ctl_dir(&buffer[8]))) goto err;
	    continue;
	}

	if (!strnicmp(buffer,"faxdial",7))
	{
	    mdm.faxdial=ctl_string(&buffer[7]);
	    continue;
	}

	if (!strnicmp(buffer,"fax",3)) {
	   for (p = strtok(&buffer[3]," "); p; p = strtok(NULL," ")) {
	       if      (!stricmp(p,"off"))	 fax_options |= FAXOPT_DISABLE;
	       else if (!stricmp(p,"lock"))	 fax_options |= FAXOPT_LOCK;
	       else if (!stricmp(p,"carrier"))	 fax_options |= FAXOPT_DCD;
	       else if (!stricmp(p,"noswap"))	 fax_options |= FAXOPT_NOSWAP;
	       else if (!stricmp(p,"eolpad"))	 fax_options |= FAXOPT_EOLPAD;
	       else if (!stricmp(p,"on"))	 fax_options &= ~FAXOPT_DISABLE;
	       else if (!stricmp(p,"nolock"))	 fax_options &= ~FAXOPT_LOCK;
	       else if (!stricmp(p,"nocarrier")) fax_options &= ~FAXOPT_DCD;
	       else if (!stricmp(p,"swap"))	 fax_options &= ~FAXOPT_NOSWAP;
	       else if (!stricmp(p,"noeolpad"))  fax_options &= ~FAXOPT_EOLPAD;
	       else if (!stricmp(p,"zfax"))	 fax_format = FAXFMT_ZFAX;
	       else if (!stricmp(p,"qfx"))	 fax_format = FAXFMT_QFX;
	       else if (!stricmp(p,"raw"))	 fax_format = FAXFMT_RAW;
	       else {
		  fprint(stderr,"Error: Unknown fax flag '%s'",p);
		  goto err;
	       }
	   }
	   continue;
	}

	if (!strnicmp(buffer,"netmail",7))
	{
	   continue;	    /* not used in the mailer, for pack etc. */
	}

	if (!strnicmp(buffer,"download",8))
	{
	    if (!*(download=ctl_dir(&buffer[8]))) goto err;
	    continue;
	}

	if (!strnicmp(buffer,"flagdir",7))
	{
	    if (!*(flagdir = ctl_dir(&buffer[7]))) goto err;
	    continue;
	}

	if (!strnicmp(buffer,"bbs",3))
	{
	    char *q;

	    bbstype = BBSTYPE_NONE;
	    if ((p = strtok(&buffer[3]," ")) != NULL) {
	       if      (!stricmp(p,"Maximus"))	    bbstype = BBSTYPE_MAXIMUS;
	       else if (!stricmp(p,"RemoteAccess")) bbstype = BBSTYPE_RA;
	    }
	    q = strtok(NULL," ");
	    if (!p || !q || !bbstype) {
	       fprint(stderr,"Error: Bad/missing bbs field\n");
	       bbstype = BBSTYPE_NONE;
	       goto err;
	    }

	    if (!*(bbspath = ctl_dir(q))) goto err;

	    continue;
	}

	if (!strnicmp(buffer,"reject",6))
	{
	    mdm.reject=ctl_string(&buffer[6]);
	    continue;
	}

	if (!strnicmp(buffer,"hangup",6))
	{
	    mdm.hangup=ctl_string(&buffer[6]);
	    continue;
	}

	if (!strnicmp(buffer,"init",4))
	{
	    mdm.setup=ctl_string(&buffer[4]);
	    continue;
	}

	if (!strnicmp(buffer,"terminit",8))
	{
	    mdm.terminit=ctl_string(&buffer[8]);
	    continue;
	}

	if (!strnicmp(buffer,"reset",5))
	{
	    mdm.reset=ctl_string(&buffer[5]);
	    continue;
	}

	if (!strnicmp(buffer,"busy",4))
	{
	    mdm.busy=ctl_string(&buffer[4]);
	    continue;
	}

	if (!strnicmp(buffer,"answer",6))
	{
	   ANSWER *ap, **app;
	   boolean quote = false;
	   char *q, *pcommand, *ppattern;

	   p = skip_blanks(&buffer[6]);
	   if (!*p) {
	      fprint(stderr,"Error: Bad/missing answer field\n");
	      goto err;
	   }

	   for (q = p; *q && (*q != ' ' || quote); q++) {
	       /* this walk-through works like mdm_puts/mdm_putc in modem.c! */
	       if (*q == '\"') quote = quote ? false : true;
	       else if (*q == '\\' && !quote) q++;
	   }
	   if (*q) *q++ = '\0';

	   pcommand = ctl_string(p);
	   ppattern = strtok(q," ");

	   do {
	      for (app = &mdm.answer; (ap = *app) != NULL && strcmp(ap->pattern,"*"); app = &ap->next);
	      if ((ap = (ANSWER *) calloc(1,sizeof (ANSWER))) == NULL) {
		 fprint(stderr,"Error: Can't allocate for answer\n");
		 goto err;
	      }

	      if (stricmp(pcommand,"ignore"))  /* so if 'ignore', leave NULL */
		 ap->command = pcommand;

	      if (ppattern) {
		 sprint(buffer,"*%s*",ppattern);
		 ap->pattern = ctl_string(buffer);
	      }
	      else
		 ap->pattern = "*";

	      ap->next	  = *app;
	      *app = ap;
	   } while ((ppattern = strtok(NULL," ")) != NULL);

	   continue;
	}

	if (!strnicmp(buffer,"data",4))
	{
	    mdm.data=ctl_string(&buffer[4]);
	    continue;
	}

	if (!strnicmp(buffer,"aftercall",9))
	{
	    mdm.aftercall=ctl_string(&buffer[9]);
	    continue;
	}

	if (!strnicmp(buffer,"mdmrsp",6))
	{
	   dword code;

	   p = skip_blanks(&buffer[6]);
	   if (isdigit(*p))
	      code = atol(p);
	   else if (!strnicmp(p,"ignore" ,6)) code = MDMRSP_IGNORE;
	   else if (!strnicmp(p,"error"  ,5)) code = MDMRSP_ERROR;
	   else if (!strnicmp(p,"ok"	 ,2)) code = MDMRSP_OK;
	   else if (!strnicmp(p,"ring"	 ,4)) code = MDMRSP_RING;
	   else if (!strnicmp(p,"nodial" ,6)) code = MDMRSP_NODIAL;
	   else if (!strnicmp(p,"noconn" ,6)) code = MDMRSP_NOCONN;
	   else if (!strnicmp(p,"badcall",7)) code = MDMRSP_BADCALL;
	   else if (!strnicmp(p,"connect",7)) code = MDMRSP_CONNECT;
	   else if (!strnicmp(p,"set"	 ,3)) code = MDMRSP_SET;
	   else if (!strnicmp(p,"fax"	 ,3)) code = MDMRSP_FAX;
	   else if (!strnicmp(p,"data"	 ,4)) code = MDMRSP_DATA;
	   else {
	      fprint(stderr,"Error: Unknown mdmrsp code '%s'",p);
	      goto err;
	   }

	   p = skip_blanks(skip_to_blank(p));
	   strupr(p);

	   if (!mdm_addrsp(p,code))
	      goto err;
	   continue;
	}

	if (!strnicmp(buffer,"mdmcause",8))
	{
	   MDMCAUSE *mcp, **mcpp;
	   word cause;
	   char *desc;

	   p	= strtok(&buffer[8]," ");
	   desc = strtok(NULL,"");
	   if (!p || !desc ||
	       sscanf(p,"%hx",&cause) != 1 || !*(desc = skip_blanks(desc))) {
	      fprint(stderr,"Error: Bad/missing mdmcause field\n");
	      goto err;
	   }
	   for (mcpp = &mdm.causelist; (mcp = *mcpp) != NULL; mcpp = &mcp->next);
	   if ((mcp = (MDMCAUSE *) calloc(1,sizeof (MDMCAUSE))) == NULL ||
	       (mcp->description = ctl_string(desc)) == NULL) {
	      fprint(stderr,"Error: Can't allocate for mdmcause\n");
	      goto err;
	   }

	   mcp->cause = cause;
	   mcp->next  = *mcpp;
	   *mcpp = mcp;

	   continue;
	}

	if (!strnicmp(buffer,"local-cost",10))
	{
	    p=skip_blanks(&buffer[10]);
	    sscanf(p ,"%d", &local_cost);
	    continue;
	}

	if (!strnicmp(buffer,"include",7)) {
	   p = skip_blanks(&buffer[7]);
	   *skip_to_blank(p) = '\0';
	   sprint(buffer,p,task);
	   read_conf(buffer);
	   continue;
	}

	if (!strnicmp(buffer,"application",11) ||
	    !strnicmp(buffer,"msgareas",8) || !strnicmp(buffer,"fileareas",9))
	   continue;

err:	if (!ignore) fprint(stderr,"-Syntax error in '%s' line %d\n",cfgname,line);
	else fprint(stderr,"-Ignoring possible error in '%s' line %d\n",cfgname,line);
	if (!ignore) error++;
    }

    fclose(conf);
}/*read_conf()*/


init_conf(int argc,char *argv[])
{
    FILE *conf; 		    /* filepointer for config-file */
    char  fname[256];
    char  buffer[256];		    /* buffer for 1 line */
    char *p, *q;		    /* workhorse */
    int   count;		    /* just a counter... */
    word  vdutype;
    ANSWER *ap, **app;
    PREFIXSUFFIX *pp, **ppp;
    extern polled_last;

    /* set all to default values */

    loglevel = 2;
    rts_out(1);
    dtr_out(1);
#if JAC
    phone_out(0);
#endif
    task = 0;	// to make sure the user specifies a task number
    memset(&mdm,0,sizeof (struct _modem));
    mdm.rings = 2;
    mdm.timeout = 600;
    mdm.speed = 2400;
    mdm.divisor = 1;
    mdm.port   = 1;
    mdm.comdev = NULL;
#if OS2 || WINNT
    mdm.shareport = true;
#endif
    mdm.fifo = 8;
    mdm.dcdmask = 0x80;
    mdmconnect[0] = errorfree[0] = errorfree2[0] = callerid[0] = '\0';
    memset(&clip,0,sizeof (CLIP));
    arqlink = isdnlink = iplink = false;
    memset(&ourbbs,0,sizeof (struct _nodeinfo));
    ourbbs.zone=ourbbs.net=ourbbs.node=ourbbs.point=0;
    ourbbs.capabilities= (ZED_ZAPPER | ZED_ZIPPER | DIETIFNA | EMSI_DZA);
    /*ourbbs.capabilities= (ZED_ZAPPER | ZED_ZIPPER | DIETIFNA | EMSI_SLK | EMSI_DZA);*/
#ifndef NOHYDRA
    ourbbs.capabilities|=DOES_HYDRA;
#endif
    ourbbs.options |= EOPT_REQFIRST;
    zmodem_txwindow = 0L;
    hydra_minspeed = hydra_maxspeed = 0UL;
    hydra_nofdx = "HST";
    hydra_fdx = NULL;
    hydra_options = HDEF_OPTIONS;
    hydra_txwindow = hydra_rxwindow = 0L;
    ptt_port = ptt_irq = ptt_units = ptt_cost = 0U;
    strcpy(ptt_valuta,"$");
    sprint(ourbbs.name,"System using Xenia%s %s",
		       XENIA_SFX, /*xenpoint_ver ? "Point" :*/ "Mailer");
    pickup=1;
    mail_only=command_line_un=un_attended=no_overdrive=0;
    for (count=0; count<20; count++) dial[count].zone=-2;
    ourbbsflags |= NO_REQ_ON | NO_COLLIDE;
    memset(&emsi_ident,0,sizeof (struct _emsi_ident));
    memset(&iemsi_profile,0,sizeof (IEMSI_PROFILE));
    memset(&extra,0,sizeof (struct _extrainfo));

    if (!win_init(50,CUR_NORMAL,CON_COOKED|CON_WRAP|CON_SCROLL|CON_UNBLANK,
		  xenwin_colours.desktop.normal, KEY_RAW)) {
	fprint(stderr,"Can't initialize window system!\n");
	exit (1);
    }
    win_deinit();

    memset(&xenwin_colours,CHR_NORMAL,sizeof (struct _xenwin_colours));

    vdutype = win_getvdutype();
    if (vdutype >= 2 && vdutype <= 4) {
    /* From Dennis Eshelman (1:2205/1@fidonet) Net 2205 NEC (CM), Seymour IN */
       xenwin_colours.desktop.normal	  = CHR_F_CYAN;
       xenwin_colours.top.normal	  = CHR_B_BLUE | CHR_F_WHITE;
       xenwin_colours.schedule.normal	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.schedule.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.schedule.title	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.schedule.template   = CHR_F_RED;
       xenwin_colours.schedule.hotnormal  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.stats.normal	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.stats.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.stats.title	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.stats.template	  = CHR_F_RED;
       xenwin_colours.stats.hotnormal	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.status.normal	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.status.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.status.title	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.status.template	  = CHR_F_RED;
       xenwin_colours.status.hotnormal	  = CHR_F_YELLOW;
       xenwin_colours.outbound.normal	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.outbound.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.outbound.title	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.outbound.template   = CHR_F_RED;
       xenwin_colours.outbound.hotnormal  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.modem.normal	  = CHR_F_WHITE;
       xenwin_colours.modem.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.modem.title	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.modem.template	  = CHR_F_WHITE;
       xenwin_colours.modem.hotnormal	  = CHR_F_YELLOW;
       xenwin_colours.log.normal	  = CHR_F_CYAN;
       xenwin_colours.log.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.log.title 	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.log.hotnormal	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.mailer.normal	  = CHR_BRIGHT | CHR_F_CYAN;
       xenwin_colours.mailer.border	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.mailer.title	  = CHR_BLINK | CHR_F_YELLOW;
       xenwin_colours.mailer.template	  = CHR_F_CYAN;
       xenwin_colours.mailer.hotnormal	  = CHR_BRIGHT | CHR_F_WHITE;
       xenwin_colours.term.normal	  = CHR_F_CYAN;
       xenwin_colours.term.border	  = CHR_BRIGHT | CHR_F_BLUE;
       xenwin_colours.term.title	  = CHR_BRIGHT | CHR_F_GREEN;
       xenwin_colours.statline.normal	  = CHR_B_BLUE | CHR_F_WHITE;
       xenwin_colours.file.normal	  = CHR_BRIGHT | CHR_F_CYAN;
       xenwin_colours.file.border	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.file.title	  = CHR_F_YELLOW;
       xenwin_colours.file.template	  = CHR_F_CYAN;
       xenwin_colours.file.hotnormal	  = CHR_BRIGHT | CHR_F_WHITE;
       xenwin_colours.script.normal	  = CHR_BRIGHT | CHR_F_CYAN;
       xenwin_colours.script.border	  = CHR_BRIGHT | CHR_F_RED;
       xenwin_colours.script.title	  = CHR_F_YELLOW;
       xenwin_colours.script.template	  = CHR_F_CYAN;
       xenwin_colours.script.hotnormal	  = CHR_BRIGHT | CHR_F_WHITE;
       xenwin_colours.help.normal	  = CHR_B_BLUE | CHR_F_WHITE;

       xenwin_colours.menu.normal	  = CHR_B_CYAN;
       xenwin_colours.menu.hotnormal	  = CHR_B_CYAN | CHR_F_RED;
       xenwin_colours.menu.select	  = CHR_BRIGHT | CHR_F_WHITE;
       xenwin_colours.menu.inactive	  = CHR_B_WHITE | CHR_F_GREY;
       xenwin_colours.menu.border	  = CHR_B_CYAN;
       xenwin_colours.menu.title	  = CHR_B_WHITE;

       xenwin_colours.select.normal	  = CHR_B_CYAN;
       xenwin_colours.select.select	  = CHR_BRIGHT | CHR_F_WHITE;
       xenwin_colours.select.border	  = CHR_B_CYAN;
       xenwin_colours.select.title	  = CHR_B_WHITE;
       xenwin_colours.select.scrollbar	  = CHR_F_WHITE;
       xenwin_colours.select.scrollselect = CHR_F_RED;
    }
    else {
       xenwin_colours.top.normal	  = CHR_INVERSE;
       xenwin_colours.help.normal	  = CHR_INVERSE;
       xenwin_colours.statline.normal	  = CHR_INVERSE;
       xenwin_colours.menu.normal	  = CHR_INVERSE;
#if ATARIST
       xenwin_colours.menu.hotnormal	  = CHR_NORMAL;
#else
       xenwin_colours.menu.hotnormal	  = CHR_INVERSE | CHR_BRIGHT;
#endif
       xenwin_colours.menu.select	  = CHR_NORMAL | CHR_BRIGHT;
       xenwin_colours.menu.inactive	  = CHR_INVERSE | CHR_BRIGHT;
       xenwin_colours.menu.border	  = CHR_INVERSE;
       xenwin_colours.menu.title	  = CHR_INVERSE;
       xenwin_colours.select.normal	  = CHR_INVERSE;
       xenwin_colours.select.select	  = CHR_NORMAL | CHR_BRIGHT;
       xenwin_colours.select.border	  = CHR_INVERSE;
       xenwin_colours.select.title	  = CHR_INVERSE;
       xenwin_colours.select.scrollbar	  = CHR_INVERSE;
       xenwin_colours.select.scrollselect = CHR_INVERSE | CHR_BRIGHT;
    }

    /* install dummy event */
    e_ptrs[0] = (EVENT *) calloc (sizeof (EVENT), 1);
    e_ptrs[0]->days	 = 0x7fff;
    e_ptrs[0]->wait_time = 120;
    e_ptrs[0]->max_conn  = 3;
    e_ptrs[0]->max_noco  = 34;
    e_ptrs[0]->minute	 = 0;
    e_ptrs[0]->length	 = 1440;
    e_ptrs[0]->behavior  = MAT_NOOUT | MAT_OUTONLY | MAT_CBEHAV;
    num_events = 1;

    while (--argc > 0) {
	  ++argv;
	  if (!strnicmp(argv[0],"HOTRECV",7)) {
	     if (argc >= 3) {
		if	(!stricmp(argv[1],"TSYNC"))  hotstart = 0x01;
		else if (!stricmp(argv[1],"YOOHOO")) hotstart = 0x02;
		else if (!stricmp(argv[1],"EMSI"))   hotstart = 0x04;
	     }

	     if (!hotstart) {
		fprint(stderr,"Error: Hotstart with too few or invalid options\n");
		return (1);
	     }
	     for (p = argv[2]; isdigit(*p); p++);
	     arqlink = isdnlink = iplink = false;
	     strcpy(errorfree,p);
	     cur_speed = atol(argv[2]);
	     if (cur_speed == 64000UL)
		isdnlink = true;
	     if (errorfree[0]) {
		if (strstr(errorfree,"ARQ") || strstr(errorfree,"MNP") ||
		    strstr(errorfree,"V42") || strstr(errorfree,"V.42") ||
		    strstr(errorfree,"LAPM") ||
		    strstr(errorfree,"X75") ||strstr(errorfree,"V120"))
		   arqlink = true;
		if (strstr(errorfree,"X75")  || strstr(errorfree,"T90") ||
		    strstr(errorfree,"V110") || strstr(errorfree,"V120") ||
		    strstr(errorfree,"X.75")  || strstr(errorfree,"T.90") ||
		    strstr(errorfree,"V.110") || strstr(errorfree,"V.120") ||
		    strstr(errorfree,"ISDN"))
		   isdnlink = true;
		if (strstr(errorfree,"TEL") || strstr(errorfree,"VMP"))
		   iplink = true;
		if (cur_speed == 64000UL ||
		    strstr(errorfree,"X75") || strstr(errorfree,"T90") ||
		    strstr(errorfree,"X.75") || strstr(errorfree,"T.90") ||
		    strstr(errorfree,"V120"))
		   line_speed += cur_speed / 6;
		else if (strstr(errorfree,"MNP") || strstr(errorfree,"V42") ||
			 strstr(errorfree,"V.42") || strstr(errorfree,"LAPM"))
		   line_speed += cur_speed / 7;
	     }
	     else if (cur_speed == 64000UL)
		line_speed += cur_speed / 6;
	     sprint(mdmconnect,"%lu%s",cur_speed,errorfree);
	     argv += 2;
	     argc -= 2;
	  }
	  else if (!strnicmp (argv[0], "force", 5))
	     force = 1;
	  else if (!strnicmp (argv[0], "share", 5))
	     mdm.shareport = true;
	  else if (!strnicmp(argv[0],"dynam",5))
	     redo_dynam++;
	  else if (!strnicmp(argv[0],"unattended",10)) {
	     un_attended = 1;
	     command_line_un = 1;
	  }
	  else if (!strnicmp(argv[0],"clear",5))
	     extra.calls = 0xffff;
	  else if (!strnicmp(argv[0],"ignore",6))
	     ignore = 1;
	  else if (!strnicmp(argv[0],"answercall",10))
	     e_ptrs[0]->behavior &= ~MAT_OUTONLY;
	  else if (!strnicmp(argv[0],"term",4))
	     cmdlineterm = 1;
	  else if (!strnicmp(argv[0],"answer",6))
	     cmdlineanswer = 1;
	  else if (!strnicmp(argv[0],"exitdynam",9))
	     e_ptrs[0]->behavior |= MAT_EXITDYNAM;
	  else if (!strnicmp(argv[0],"poll",4)) {
	     if (argc >= 2) {
		for (count = 0; count < 20; count++)
		    if (dial[count].zone == -2) break;
		if (count == 20) {
		   fprint(stderr,"Error: Too many poll requests\n");
		   return (1);
		}
		dial[count].point = 0;
		if (sscanf(argv[1], "%d:%d/%d.%d",
			   &dial[count].zone, &dial[count].net,
			   &dial[count].node, &dial[count].point) < 3) {
		   fprint(stderr,"Error: Invalid poll nodenumber '%s'\n",argv[1]);
		   return (1);
		}
		dial[count].phone[0] = '\0';
		dial[count].speed    = 0;
		polled_last = 1;
	     }
	     else {
		fprint(stderr,"Error: Poll without a nodenumber\n");
		return (1);
	     }
	     argv++;
	     argc--;
	  }
	  else if (!strnicmp(argv[0],"task",4)) {
	     if (argc >= 2) {
		int tmptask = atoi(argv[1]);

		if (tmptask < 1 || tmptask > 255) {
		   fprint(stderr,"Error: Invalid task number '%s'\n",argv[1]);
		   return (1);
		}
		task = (word) tmptask;
		cmdlinetask = true;
	     }
	     else {
		fprint(stderr,"Error: Task without number\n");
		return (1);
	     }
	     argv++;
	     argc--;
	  }
	  else if (!strnicmp(argv[0],"port",4)) {
	     if (argc >= 2) {
		if (isdigit(*argv[1])) {
		   count = atoi(argv[1]);
#if PC
		   if (count < 0 || count > 64) {
#else
		   if (count < 1 || count > 64) {
#endif
		      fprint(stderr,"Error: Invalid com port '%s'\n",argv[1]);
		      return (1);
		   }
		   else {
		      mdm.port	 = count;
		      mdm.comdev = NULL;
		      cmdlineport = true;
		   }
		}
		else {
		   mdm.port   = 0;
		   mdm.comdev = ctl_string(strupr(argv[1]));
		   cmdlineport = true;
		}
	     }
	     else {
		fprint(stderr,"Error: Port without number or device name\n");
		return (1);
	     }
	     argv++;
	     argc--;
	  }
	  else if (!strnicmp(argv[0],"snoop",5)) {
#if OS2
	     if (argc >= 2) {
		pipename = ctl_string(argv[1]);
		cmdlinesnoop = true;
	     }
	     else {
		fprint(stderr,"Error: Snoop without \\pipe\\name\n");
		return (1);
	     }
#endif
	     argv++;
	     argc--;
	  }
#if 0
	  else if (!strnicmp(argv[0],"XENPOINT",8)) {
	       xenpoint_ver = ctl_string(&argv[0][8]);
	       xenpoint_reg = strchr(xenpoint_ver,'/');
	       *xenpoint_reg++ = '\0';
	  }
#endif
	  else {
	     fprint(stderr,"Error: Unknown option: %s\n",argv[0]);
	     return (1);
	  }
    }

    sprint(buffer,"%sXENIA%.0u.CFG",homepath,task);
    if (!fexist(buffer) && task)
       sprint(buffer,"%sXENIA.CFG",homepath);
    read_conf(buffer);

    if (!task) {
       fprint(stderr,"Error: No task number specified!\n");
       return (1);
    }

#if OS2
    { HMTX hmtx = 0UL;

      sprint(buffer,"\\SEM32\\XENIA%u",task);
      if (DosCreateMutexSem((byte *) buffer,&hmtx,0UL,TRUE) != NO_ERROR) {
	 fprint(stderr,"Another Xenia/2 with task number %u appears to be already running!\n",task);
	 return (1);
      }
    }
#endif

#if 0
    sprint(buffer,"%sXENIA%u.BAN",homepath,task);
    if ((conf = sfopen(buffer,"rb",DENY_NONE)) == NULL) {
       sprint(buffer,"%sXENIA.BAN",homepath);
       conf = sfopen(buffer,"rb",DENY_NONE);
    }
    if (conf != NULL) {
       if (banner) free(banner);
       banner = myalloc(4097);
	count = fread(banner,(int) sizeof (char),4096,conf);
	fclose(conf);
	banner[count] = '\0';
	if (count && (p = strchr(banner,'\032')) != NULL) {
	   *p = '\0';
	   count = strlen(banner);
	}
	banner = (char *) realloc(banner,count + 1);
    }
#endif

    read_sched();	/* get the schedule */

    /* If we haven't got a schedule yet then read it NOW */

    if (!got_sched) {
       sprint(buffer,"%sXENIA%.0u.EVT",homepath,noevttask ? 0 : task);
       if ((conf=sfopen(buffer,"rt",DENY_NONE))!=NULL) { /* open event file */
	  while(fgets(buffer,250,conf)) {		  /* read a line     */
	       p = skip_blanks(buffer);
	       if (*p == ';' || strlen(p) < 3) continue;
	       if ((p = strchr(buffer,'\n')) != NULL) *p = '\0';
	       if (parse_event(buffer)) error++;
	   }
	   fclose(conf);
       }
       else if (!aka[0].point /* && !xenpoint_ver */ ) {
	  fprint(stderr,"Error: Event file not found, please check!!\n");
	  error++;
       }
    }

    if (!capture) {
       sprint(buffer,"XENIA%u.CAP",task);
       capture=ctl_string(buffer);
    }
    if(!noapptext)  noapptext = "Processing mail only - please hang up\\r";
    if(!apptext)    apptext   = "Press <ESC> twice for BBS\\r";
    if(!flagdir)    flagdir   = ctl_dir(homepath);
    if(!safepath)   safepath  = filepath;
    if(!localpath)  localpath = safepath;
    if(!fax_indir)  fax_indir = localpath;

    if (!mdm.comdev) {
       sprint(buffer,"COM%d",mdm.port);
       mdm.comdev = ctl_string(buffer);
    }
    else if (!strcmp(mdm.comdev,"NONE"))
       mdm.port = -1;
    else if (!mdm.port && !strncmp(mdm.comdev,"COM",3) && atoi(&mdm.comdev[3]))
       mdm.port = atoi(&mdm.comdev[3]);
    if (!mdm.hangup)	mdm.hangup    = "!`v~~^`!!``";
    if (!mdm.setup)	mdm.setup     = "AT!";
    if (!mdm.reset)	mdm.reset     = "AT!";
    if (!mdm.busy)	mdm.busy      = "AT!";
    if (!mdm.faxdial)	mdm.faxdial   = "AT+FCLASS=2|";
    /* if (!mdm.terminit)  mdm.terminit  = "AT!"; */
    /* if (!mdm.data)	   mdm.data	 = "ATO|"; */

    mdm_addrsp("CONNECT FAX",(fax_options & FAXOPT_DISABLE) ? MDMRSP_FAX : MDMRSP_IGNORE);
    mdm_addrsp("FAX"	    ,(fax_options & FAXOPT_DISABLE) ? MDMRSP_FAX : MDMRSP_IGNORE);
    mdm_addrsp("+FCO"	    ,MDMRSP_FAX    );
    mdm_addrsp("SET TO"     ,MDMRSP_SET    );
    mdm_addrsp("CONNECT"    ,MDMRSP_CONNECT);
    mdm_addrsp("DATA"	    ,mdm.data ? MDMRSP_DATA : MDMRSP_IGNORE );
    mdm_addrsp("NO CARRIER" ,MDMRSP_NOCONN );
    mdm_addrsp("+FHNG"	    ,MDMRSP_NOCONN );
    mdm_addrsp("+FHS"	    ,MDMRSP_NOCONN );
    mdm_addrsp("BUSY"	    ,MDMRSP_NOCONN );
    mdm_addrsp("VOICE"	    ,MDMRSP_NOCONN );
    mdm_addrsp("NO ANSWER"  ,MDMRSP_NOCONN );
    mdm_addrsp("NO TONE"    ,MDMRSP_NODIAL );
    mdm_addrsp("NO DIAL"    ,MDMRSP_NODIAL );
    mdm_addrsp("ERROR"	    ,MDMRSP_ERROR  );
    mdm_addrsp("RING"	    ,MDMRSP_RING   );
    mdm_addrsp("RRING"	    ,MDMRSP_RING   );
    mdm_addrsp("OK"	    ,MDMRSP_OK	   );

    for (app = &mdm.answer; (ap = *app) != NULL && strcmp(ap->pattern,"*"); app = &ap->next);
    if (!ap) {
       if ((ap = (ANSWER *) calloc(1,sizeof (ANSWER))) == NULL) {
	  fprint(stderr,"Error: Can't allocate for answer\n");
	  return (1);
       }

       ap->pattern = "*";
       ap->command = "ATA|";
       ap->next    = *app;
       *app = ap;
    }

    /*if (mdm.answer->next)
       mdm.firstring = true;*/

    for (ppp = &prefixsuffix; (pp = *ppp) != NULL && strcmp(pp->pattern,"*"); ppp = &pp->next);
    if (!pp) {
       if ((pp = (PREFIXSUFFIX *) calloc(1,sizeof (PREFIXSUFFIX))) == NULL) {
	  fprint(stderr,"Error: Can't allocate for prefixsuffix\n");
	  return (1);
       }
       pp->pattern = "*";
       pp->prefix  = "ATDT";
       pp->suffix  = "|";
       pp->next    = *ppp;
       *ppp = pp;
    }

    if (okdial) {
       OKDIAL *op;

       for (op = okdial; op->next; op = op->next);
       if ((op->next = (OKDIAL *) calloc(1,sizeof (OKDIAL))) == NULL) {
	  fprint(stderr,"Error: Can't allocate for okdial\n");
	  return (1);
       }
       op->next->allowed = op->allowed ? false : true;
       op->next->pattern = "*";
    }

    if (!ourbbs.sysop[0])
       sprint(ourbbs.sysop,"Sysop of %d:%d/%d%s%s",
			   ourbbs.zone, ourbbs.net, ourbbs.node,
			   pointnr(ourbbs.point), zonedomabbrev(ourbbs.zone));
    if (emsi_adr > nakas) emsi_adr = nakas;

    if (!iemsi_profile.location) iemsi_profile.location = emsi_ident.city;
    if (!iemsi_profile.data)	 iemsi_profile.data	= emsi_ident.phone;
    if (!emsi_ident.name)	 emsi_ident.name	= ourbbs.name;
    if (!emsi_ident.city)	 emsi_ident.city	= "Somewhere";
    if (!emsi_ident.sysop)	 emsi_ident.sysop	= ourbbs.sysop;
    if (!iemsi_profile.name)	 iemsi_profile.name	= emsi_ident.sysop;
    if (!emsi_ident.phone)	 emsi_ident.phone	= "-Unpublished-";
    if (!emsi_ident.speed) {
       sprint(buffer,"%lu", (mdm.speed >= 9600UL ? 9600UL :
			    (mdm.speed > 2400UL ? 2400UL : mdm.speed)));
       emsi_ident.speed = ctl_string(buffer);
    }
    if (!emsi_ident.flags) emsi_ident.flags = "XA";

    if (*hold && hold[strlen(hold) - 1] != '.') {
       if (!dfirst(hold)) {
	  fprint(stderr,"Error: Outbound holding dir '%s' does not exist!",hold);
	  error++;
       }
       dnclose();
    }

    sprint(fname,"%sXENIA%u.NVR",homepath,task);
    if ((conf = sfopen(fname,"rt",DENY_WRITE)) != NULL) {     /* NVRAM file */
       while (fgets(buffer,250,conf)) {
	     if ((p = strchr(buffer,';')) != NULL) *p = '\0';
	     if ((p = strtok(buffer," \t\n")) == NULL ||
		 (q = strtok(NULL,"\t\n")) == NULL ||
		 !*(q = skip_blanks(q)))
		continue;

	     if (!stricmp(p,"POLL")) {
		for (count = 0; count < 20; count++)
		    if (dial[count].zone == -2) break;
		if (count == 20) continue;
		dial[count].point = 0;
		if (sscanf(q,"%d:%d/%d.%d",
			     &dial[count].zone, &dial[count].net,
			     &dial[count].node, &dial[count].point) < 3)
		   dial[count].zone = -2;
		else {
		   dial[count].phone[0] = '\0';
		   dial[count].speed	= 0;
		   polled_last = 1;
		}
	     }
	     else if (!stricmp(p,"STATS_CALLS"))
		sscanf(q,"%hu %hu %hu %hu",
			 &extra.calls,&extra.human,&extra.mailer,&extra.called);
	     else if (!stricmp(p,"STATS_MOVED"))
		sscanf(q,"%hu %hu %hu %hu",
			 &extra.nrmail,&extra.kbmail,&extra.nrfiles,&extra.kbfiles);
	     else if (!stricmp(p,"LAST_IN"))
		last_in  = ctl_string(q);
	     else if (!stricmp(p,"LAST_BBS"))
		last_bbs = ctl_string(q);
	     else if (!stricmp(p,"LAST_OUT"))
		last_out = ctl_string(q);
       }
       fclose(conf);
    }
    if (polled_last == 1 && fexist(fname)) unlink(fname);

    /*read_bbsinfo();*/

    sprint(buffer,"%sXMHANGUP.%u",flagdir,task);
    if (fexist(buffer)) unlink(buffer);

    return (error);			/* signal OK?? */
}


int getint(char **p, int *i)		 /* Mind the calling parms! */
{
	char	*q;
	long	 temp;
	boolean  sign = false;

	q = *p;
	if (*q == '-') {
	   sign = true;
	   q++;
	}
	if (!isdigit(*q)) return (-1);

	temp = 0;
	do {
	   temp *= 10;
	   temp += (*q++ - '0');
	}
	while (isdigit(*q));
	if (temp > 32767) return (-1);
	*p = q;
	*i = (int) (sign ? -temp : temp);

	return (0);
}


int getaddress (char *str, int *zone, int *net, int *node, int *point)
		  /* Decode zz:nn/mm.pp number				     */
		  /* 0=error | 1=node | 2=net/node | 3=zone:net/node | 4=pnt */
{
	int retcode = 0;

	*zone  = aka[0].zone;
	*net   = aka[0].net;
	*node  = aka[0].node;
	*point = 0;

	if (isalpha(*str)) {
	   while (isalpha(*++str));
	   if (*str != '#') return (0);
	   str++;
	}

	if (*str=='.') goto pnt;
	retcode++;
	if (getint(&str,node)) {
	   if (strnicmp(str,"ALL",3)) return (0);
	   *zone = *net = *node = -1;
	   return (10);
	}
	if (!*str) return (retcode);
	if (*str == '.') goto pnt;
	retcode++;
	if (*str == ':') {
	   str++;
	   *zone = *node;
	   *node = -1;
	   if (!*str) return (0);
	   if (getint(&str,node)) {
	      if (strnicmp(str,"ALL",3)) return (0);
	      *net = *node = -1;
	      return (10);
	   }
	   retcode++;
	}
	if (*str!='/') return (0);
	str++;
	*net = *node;
	*node = aka[0].node;
	if (getint(&str,node)) {
	   if (strnicmp(str,"ALL",3)) return (0);
	   *node = -1;
	   return (10);
	}
	if (*str=='.') goto pnt;
	return (retcode);

pnt:	str++;
	if (getint(&str,point)) {
	   if (strnicmp(str,"ALL",3)) return (0);
	   *point = -1;
	   return (10);
	}

	return (4 + retcode);
}


/* end of tconf.c */
