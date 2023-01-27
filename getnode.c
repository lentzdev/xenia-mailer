/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 * Routines to access the translated/indexed nodelist
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
#include "nodeidx.h"


/* ------------------------------------------------------------------------- */
#define MAX_BUFFER 256

typedef struct _nlentry NLENTRY;
struct _nlentry {
	char  info[MAX_BUFFER + 1];
	char *name, *location, *sysop, *phone, *flags;
};

typedef struct _adrinfo ADRINFO;
struct _adrinfo {
	char	*domname;
	int	 zone, net, node, point;
	word	 flags;
	NLENTRY  nlentry;
	long	 used;
};
#if WINNT || OS2
#define NUM_ADRINFO 50
#else
#define NUM_ADRINFO 20
#endif

static ADRINFO *adrinfo   = NULL;
static char    *idxbase   = NULL;
static FILE    *idxfp	  = NULL;
static int	last_zi   = 0,
		last_ni   = 0,
		last_bi   = 0;

static NLHEADER nlheader;
static char    *nlname	= NULL;
static NLFILE  *nlfile	= NULL;
static NLZONE  *nlzone	= NULL;
static NLNET   *nlnet	= NULL;
static NLNODE  *nlnode	= NULL;
static NLBOSS  *nlboss	= NULL;
static NLPOINT *nlpoint = NULL;

static NLENTRY	nlentry;


/* ------------------------------------------------------------------------- */
static boolean getline (long entry)
{
	static FILE	*nlfp = NULL;
	static char	 cur_name[PATHLEN] = "";
	static FILE_OFS  cur_ofs;
	char  buffer[MAX_BUFFER + 1];
	char *p;
	word  got = 0;
	int   c;

	p = &nlname[nlfile[NL_ENTFILE(entry)].name_ofs];
	if (!nlfp || stricmp(p,cur_name)) {
	   if (nlfp) fclose(nlfp);
	   strcpy(cur_name,p);
	   if ((nlfp = sfopen(cur_name,"rb",DENY_NONE)) == NULL)
	      return (false);
	   cur_ofs = 0L;
	}

	if (NL_ENTOFS(entry) != cur_ofs) {
	   cur_ofs = NL_ENTOFS(entry);
	   fseek(nlfp,cur_ofs,SEEK_SET);
	}

	p = buffer;
	while ((c = getc(nlfp)) != EOF && !ferror(nlfp) && got < MAX_BUFFER) {
	      if (c == '\032') break;
	      got++;
	      if (c == '\r' || c == '\n') {
		 c = getc(nlfp);
		 if (ferror(nlfp)) break;
		 if (c == '\r' || c == '\n') got++;
		 else			     ungetc(c,nlfp);
		 *p = '\0';
		 cur_ofs += got;

		 for (p = buffer; *p; p++)
		     if (*p == '_') *p = ' ';
		 strcpy(nlentry.info,buffer);
		 if (strtok(nlentry.info,",") == nlentry.info)
		    strtok(NULL,",");
		 nlentry.name	  = strtok(NULL,",");
		 nlentry.location = strtok(NULL,",");
		 nlentry.sysop	  = strtok(NULL,",");
		 nlentry.phone	  = strtok(NULL,",");
		 if ((nlentry.flags = strtok(NULL,"")) == NULL)
		    nlentry.flags = "";
		 else if (*nlentry.flags == ',')
		    nlentry.flags++;

		 return (nlentry.phone ? true : false);
	      }
	      *p++ = c;
	}

	clearerr(nlfp);
	cur_ofs = ftell(nlfp);

	return (false);
}


/* ------------------------------------------------------------------------- */
static boolean findidx (DOMAIN *domp)
{
	FILE	*newfp;
	NLHEADER nlh, newnlh;
	char	 buffer[MAX_BUFFER + 1];
	int	 len, i;
	byte	 number;
	boolean  found = false;
	char	*p;

	if (domp->timestamp) {
	   sprint(buffer,"%s.I%02u",domp->idxbase,(word) domp->number);
	   if ((newfp = sfopen(buffer,"rb",DENY_NONE)) != NULL) {
	      if (fread(&newnlh,sizeof (NLHEADER),1,newfp) != 1 ||
		  strncmp(newnlh.idstring,NL_IDSTRING,sizeof (newnlh.idstring)) ||
		  NL_REVMAJOR(newnlh.revision) != NL_MAJOR) {
		 fclose(newfp);
	      }
	      else {
		 domp->timestamp = newnlh.timestamp;
		 memmove(&nlh,&newnlh,sizeof (NLHEADER));
		 if (idxfp) fclose(idxfp);
		 idxfp = newfp;
		 found = true;
	      }
	   }
	}

	if (!found) {
	   sprint(buffer,"%s.I??",domp->idxbase);
	   for (p = ffirst(buffer); p; p = fnext()) {
	       len = strlen(p);
	       if (!isdigit(p[len-2]) || !isdigit(p[len-1]))
		  continue;
	       number = atoi(&p[len-2]);
	       sprint(buffer,"%s.I%02u",domp->idxbase,(word) number);
	       if ((newfp = sfopen(buffer,"rb",DENY_NONE)) == NULL)
		  continue;

	       if (fread(&newnlh,sizeof (NLHEADER),1,newfp) != 1 ||
		   strncmp(newnlh.idstring,NL_IDSTRING,sizeof (newnlh.idstring)) ||
		   NL_REVMAJOR(newnlh.revision) != NL_MAJOR ||
		   newnlh.timestamp < domp->timestamp) {
		  fclose(newfp);
		  continue;
	       }

	       domp->number    = number;
	       domp->timestamp = newnlh.timestamp;
	       memmove(&nlh,&newnlh,sizeof (NLHEADER));
	       if (idxfp) fclose(idxfp);
	       idxfp = newfp;
	       found = true;
	   }
	   fnclose();
	}

	if (found) {
	   if (nlname)	{ free(nlname);  nlname  = NULL; }
	   if (nlfile)	{ free(nlfile);  nlfile  = NULL; }
	   if (nlzone)	{ free(nlzone);  nlzone  = NULL; }
	   if (nlnet)	{ free(nlnet);	 nlnet	 = NULL; }
	   if (nlnode)	{ free(nlnode);  nlnode  = NULL; }
	   if (nlboss)	{ free(nlboss);  nlboss  = NULL; }
	   if (nlpoint) { free(nlpoint); nlpoint = NULL; }

	   if (adrinfo) {
	      for (i = 0; i < NUM_ADRINFO; i++) {
		  if (adrinfo[i].used && !stricmp(adrinfo[i].domname,domp->abbrev))
		     memset(&adrinfo[i],0,sizeof (ADRINFO));
	      }
	   }

	   idxbase = domp->idxbase;
	   memmove(&nlheader,&nlh,sizeof (NLHEADER));

	   if ((nlname = (char *) malloc(nlheader.name_size)) == NULL ||
	       fseek(idxfp,nlheader.name_ofs,SEEK_SET) ||
	       fread(nlname,nlheader.name_size,1,idxfp) != 1 ||
	       (nlfile = (NLFILE *) malloc(nlheader.files * sizeof (NLFILE))) == NULL ||
	       fseek(idxfp,nlheader.file_ofs,SEEK_SET) ||
	       fread(nlfile,sizeof (NLFILE) * nlheader.files,1,idxfp) != 1) {
	      fclose(idxfp);
	      idxfp = NULL;
	      domp->errors = true;
	   }
	   else
	      domp->errors = false;
	}

	return (domp->errors ? false : true);
}


/* ------------------------------------------------------------------------- */
static ADRINFO *get_adrinfo (int zone, int net, int node, int point)
{
	int zi, ni, si, bi, pi;
	DOMAIN *domp;
	long oldest_stamp;
	boolean gotboss = false;
	word flags;
	int i, n;
	char *p;
	DOMZONE *zp;

	alloc_msg("pre getadrinfo");

	if (!adrinfo) {
	   if ((adrinfo = (ADRINFO *) malloc(NUM_ADRINFO * sizeof (ADRINFO))) == NULL)
	      return (NULL);
	   memset(adrinfo,0,NUM_ADRINFO * sizeof (ADRINFO));
	}

	if (idxbase) {
	   char buffer[256];
	   struct stat f;

	   for (domp = domain; domp; domp = domp->next)
	       if (domp->idxbase && !stricmp(idxbase,domp->idxbase)) break;

	   sprint(buffer,"%s.FLG",domp->idxbase);
	   if (!getstat(buffer,&f) && f.st_mtime != domp->flagstamp) {
	      domp->timestamp = 0L;
	      domp->flagstamp = f.st_mtime;
	      findidx(domp);
	   }
	}

	for (zp = domzone; zp && zone != zp->zone; zp = zp->next);
	if (!zp) return (NULL);
	domp = zp->domptr;
	if (!domp->idxbase)
	   return (NULL);

	if (domp->idxbase != idxbase) {
	   if (!findidx(domp)) {
	      alloc_msg("post getadrinfo(no idx)");
	      return (NULL);
	   }
	}
	else if (domp->errors) {
	   alloc_msg("post getadrinfo(errflag)");
	   return (NULL);
	}

	for (i = 0; i < NUM_ADRINFO; i++) {
	    if (adrinfo[i].used &&
		zone == adrinfo[i].zone && net == adrinfo[i].net &&
		node == adrinfo[i].node && point == adrinfo[i].point) {
	       adrinfo[i].used = time(NULL);
	       alloc_msg("post getadrinfo(in mem)");
	       return (&adrinfo[i]);
	    }
	}

	zi = ni = si = bi = pi = 0;

	if (point) {
	   if (!nlheader.bosses)
	      return (NULL);

	   if (!nlboss &&
	       ((nlboss = (NLBOSS *) malloc(nlheader.bosses * sizeof (NLBOSS))) == NULL ||
		fseek(idxfp,nlheader.boss_ofs,SEEK_SET) ||
		fread(nlboss,sizeof (NLBOSS) * nlheader.bosses,1,idxfp) != 1))
	      goto err;

	   for (bi = 0; bi < nlheader.bosses; bi++) {
	       if (nlboss[bi].zone > zone) break;
	       if (nlboss[bi].zone < zone) continue;
	       if (nlboss[bi].net > net) break;
	       if (nlboss[bi].net < net) continue;
	       if (nlboss[bi].node > node) break;
	       if (nlboss[bi].node < node) continue;

	       if (!nlpoint || bi != last_bi) {
		  if (nlpoint) { free(nlpoint); nlpoint = NULL; }
		  last_bi = bi;
		  if ((nlpoint = (NLPOINT *) malloc(nlboss[bi].points * sizeof (NLPOINT))) == NULL ||
		      fseek(idxfp,nlboss[bi].point_ofs,SEEK_SET) ||
		      fread(nlpoint,sizeof (NLPOINT) * nlboss[bi].points,1,idxfp) != 1)
		     goto err;
	       }

	       for (pi = 0; pi < nlboss[bi].points; pi++) {
		   if (nlpoint[pi].point == point) {
		      if (!getline(nlpoint[pi].entry)) break;
		      flags = nlpoint[pi].flags;
		      goto oki;
		   }
	       }

	       /* Point not found; now check boss in nodelist, then bosslist */
	       /* Return NULL in any case, but now we have boss info in mem  */
	       if (!get_adrinfo(zone,net,node,0) && bi < nlheader.bosses) {
		  if (getline(nlboss[bi].entry)) {
		     flags = nlboss[bi].flags;
		     gotboss = true;
		     goto oki;
		  }
	       }

	       break;
	   }

	   return (NULL);
	}

	if (!nlzone && nlheader.zones) {
	   if ((nlzone = (NLZONE *) malloc(nlheader.zones * sizeof (NLZONE))) == NULL ||
	       fseek(idxfp,nlheader.zone_ofs,SEEK_SET) ||
	       fread(nlzone,sizeof (NLZONE) * nlheader.zones,1,idxfp) != 1)
	      goto err;
	}

	for (zi = 0; zi < nlheader.zones; zi++) {
	    if (nlzone[zi].zone > zone) break;
	    if (nlzone[zi].zone < zone) continue;

	    if (!nlnet || zi != last_zi) {
	       if (nlnet) { free(nlnet); nlnet = NULL; }
	       if (nlnode) { free(nlnode); nlnode = NULL; }
	       last_zi = zi;
	       if ((nlnet = (NLNET *) malloc(nlzone[zi].nets * sizeof (NLNET))) == NULL ||
		   fseek(idxfp,nlzone[zi].net_ofs,SEEK_SET)||
		   fread(nlnet,sizeof (NLNET) * nlzone[zi].nets,1,idxfp) != 1)
		  goto err;
	    }

	    for (ni = 0; ni < nlzone[zi].nets; ni++) {
		if (nlnet[ni].net != net) continue;

		if (!nlnode || ni != last_ni) {
		   if (nlnode) { free(nlnode); nlnode = NULL; }
		   last_ni = ni;
		   if ((nlnode = (NLNODE *) malloc(nlnet[ni].nodes * sizeof (NLNODE))) == NULL ||
		       fseek(idxfp,nlnet[ni].node_ofs,SEEK_SET) ||
		       fread(nlnode,sizeof (NLNODE) * nlnet[ni].nodes,1,idxfp) != 1)
		   goto err;
		}

		for (si = 0; si < nlnet[ni].nodes; si++) {
		    if (nlnode[si].node != node) continue;

		    if (!getline(nlnode[si].entry)) goto err;
		    flags = nlnode[si].flags;
		    goto oki;
		}

		/* unlisted node in net */
		alloc_msg("post getadrinfo(not found)");
		return (NULL);
	    }

	    /* unlisted net in zone */
	    alloc_msg("post getadrinfo(not found)");
	    return (NULL);
	}

	/* unlisted zone in domain - can't happen while we map zone->domain */
	alloc_msg("post getadrinfo(not found)");
	return (NULL);

oki:	oldest_stamp = 0L;
	for (i = n = 0; i < NUM_ADRINFO; i++) {
	    if (!adrinfo[i].used) {
	       n = i;
	       break;
	    }
	    if (!oldest_stamp || adrinfo[i].used < oldest_stamp) {
	       oldest_stamp = adrinfo[i].used;
	       n = i;
	    }
	}

	adrinfo[n].domname = domp->domname;
	adrinfo[n].zone    = zone;
	adrinfo[n].net	   = net;
	adrinfo[n].node    = node;
	adrinfo[n].point   = gotboss ? 0 : point;
	adrinfo[n].flags   = flags;
	p = adrinfo[n].nlentry.info; strcpy(p,nlentry.name    ); adrinfo[n].nlentry.name     = p;
	p += strlen(p) + 1;	     strcpy(p,nlentry.location); adrinfo[n].nlentry.location = p;
	p += strlen(p) + 1;	     strcpy(p,nlentry.sysop   ); adrinfo[n].nlentry.sysop    = p;
	p += strlen(p) + 1;	     strcpy(p,nlentry.phone   ); adrinfo[n].nlentry.phone    = p;
	p += strlen(p) + 1;	     strcpy(p,nlentry.flags   ); adrinfo[n].nlentry.flags    = p;
	adrinfo[n].used    = time(NULL);

	alloc_msg("post getadrinfo(found)");

	return (gotboss ? NULL : &adrinfo[n]);

err:	if (idxfp) {
	   fclose(idxfp);
	   idxfp = NULL;
	}
	domp->errors = true;

	alloc_msg("post getadrinfo(error)");
	return (NULL);
}/*get_adrinfo()*/


/* ------------------------------------------------------------------------- */
void nlidx_init (void)
{
	DOMAIN	*domp;
	DOMZONE *zp, **zpp;
	int	 zi;
	char	 buffer[PATHLEN];
	char	 *p;
	boolean  indexes = false;

	strcpy(buffer,hold);
	if ((p = strrchr(buffer,'\\')) == NULL && (p = strrchr(buffer,':')) == NULL)
	   p = buffer;
	else
	   p++;

	for (domp = domain; domp; domp = domp->next) {
	    if (!domp->idxbase)
	       goto noidx;

	    indexes = true;

	    if (domp->idxbase != idxbase) {
	       if (!findidx(domp)) goto noidx;
	    }
	    else if (domp->errors)
	       goto noidx;

	    if (!nlzone && nlheader.zones) {
	       if ((nlzone = (NLZONE *) malloc(nlheader.zones * sizeof (NLZONE))) == NULL ||
		   fseek(idxfp,nlheader.zone_ofs,SEEK_SET) ||
		   fread(nlzone,sizeof (NLZONE) * nlheader.zones,1,idxfp) != 1)
		  goto err;
	    }

	    for (zi = 0; zi < nlheader.zones; zi++) {
		for (zpp = &domzone; (zp = *zpp) != NULL; zpp = &zp->next)
		    if (nlzone[zi].zone == zp->zone) break;
		if (!zp) {
		   if ((zp = (DOMZONE *) calloc(1,sizeof (DOMZONE))) == NULL)
		      continue;

		   zp->zone   = nlzone[zi].zone;
		   zp->domptr = domp;
		   zp->next = *zpp;
		   *zpp = zp;

		   if (!domp->hold && (zp->zone == aka[0].zone || nodomhold))
		      domp->hold = hold;
		}
	    }
	    goto noidx;

err:	    if (idxfp) {
	       fclose(idxfp);
	       idxfp = NULL;
	    }
	    domp->errors = true;

noidx:	    if (!domp->hold) {
	       if (nodomhold)
		  domp->hold = hold;
	       else {
		  strcpy(p,domp->abbrev);
		  domp->hold = ctl_string(buffer);
	       }
	    }
	}

	if (domain && indexes)
	   message(-1,"+Nodelist indexes initialized");
}/*nlidx_init()*/


/* ------------------------------------------------------------------------- */
char *nlidx_getphone (int zone, int net, int node, int point, char **phpp, char **flpp)
{
	ADRINFO *ap = get_adrinfo(zone,net,node,point);
	char *p;
	static char phonebuf[60];

	if (phpp) *phpp = NULL;
	if (flpp) *flpp = NULL;

	if (!ap || !strpbrk(ap->nlentry.phone,"0123456789"))
	   return (NULL);

	strcpy(phonebuf,"+");
	strcat(phonebuf,ap->nlentry.phone);
	if (phpp) *phpp = phonebuf;

	p = ap->nlentry.flags[0] ? ap->nlentry.flags : NULL;
	if (flpp) *flpp = p;

	return (p);
}


/* ------------------------------------------------------------------------- */
static void phone_override (struct _nodeinfo *nip,
			    int zone, int net, int node, int point)
{
    PHONE *pp;

    for (pp = phone; pp; pp = pp->next) {
	if (zone == pp->zone && net   == pp->net &&
	    node == pp->node && point == pp->point) {
	   char *p, *q;

	   if (strpbrk(pp->number,":/")) {
	      int tzone, tnet, tnode, tpoint;

	      getaddress(pp->number,&tzone,&tnet,&tnode,&tpoint);
	      nlidx_getphone(tzone,tnet,tnode,tpoint,&q,&p);
	      if (!q) continue;
	      strcpy(nip->phone,q);
	      if (pp->nodeflags) p = pp->nodeflags;
	   }
	   else {
	      strcpy(nip->phone,pp->number);
	      p = pp->nodeflags ? pp->nodeflags : nlidx_getphone(zone,net,node,point,NULL,NULL);
	   }

	   if (p && isdigit(*p)) {
	      nip->speed = atol(p);
	      if (nip->speed > mdm.speed)
		 nip->speed = mdm.speed;
	   }
	   break;
	}
    }
}


/* ------------------------------------------------------------------------- */
nodetoinfo(struct _nodeinfo *nip)	   /* get node information from list */
{
    int i;				   /* multi purpose counter	     */
    ADRINFO *ap;

    remap(&nip->zone,&nip->net,&nip->node,&nip->point,&nip->pointnet);

    strcpy(nip->name,"noname");
    nip->phone[0]  = '\0';
    nip->speed	   = mdm.speed;
    nip->cost	   = 0;
    nip->nodeflags = 0;

    for (i = 0; i < nakas; i++) {
	if (!nip->point && aka[i].point != 0 &&
	    aka[i].zone == nip->zone &&
	    aka[i].net == nip->net && aka[i].node == nip->node) {
	   strcpy(nip->name,"BossNode");
	   break;
	}
    }

    phone_override(nip,nip->zone,nip->net,nip->node,nip->point);
    ap = get_adrinfo(nip->zone,nip->net,nip->node,nip->point);
    if ((!ap || !isdigit(ap->nlentry.phone[0])) && !nip->phone[0] && nip->point) {
       phone_override(nip,nip->zone,nip->net,nip->node,0);
       ap = get_adrinfo(nip->zone,nip->net,nip->node,0);
    }
    if (ap) {
       strcpy(nip->name,ap->nlentry.name);
       strcpy(nip->sysop,ap->nlentry.sysop);
       if (nip->point && !ap->point)
	  nip->nodeflags |= VIABOSS;
       if (!nip->phone[0]) {
	  switch (NL_TYPEMASK(ap->flags)) {
	      case NL_DOWN:
		   nip->nodeflags |= DOWN;
		   return (1);

	      case NL_PVT:
	      case NL_HOLD:
		   phone_override(nip,nip->zone,nip->net,0,0);
		   ap = get_adrinfo(nip->zone,nip->net,0,0);
		   if (!ap) break;
		   nip->nodeflags |= VIAHOST;
		   /* fallthrough to default */

	      default:
		   if (strpbrk(ap->nlentry.phone,"0123456789")) {
		      strcpy(nip->phone,"+");
		      strcat(nip->phone,ap->nlentry.phone);
		   }
		   if (isdigit(*ap->nlentry.flags)) {
		      nip->speed = atol(ap->nlentry.flags);
		      if (nip->speed > mdm.speed)
			 nip->speed = mdm.speed;
		   }
		   break;
	  }
       }
    }

    if (nip->phone[0]) return (1);
    nip->name[0]   = '\0';
    nip->speed	   = 0;
    nip->cost	   = 0;
    nip->nodeflags = 0;
    return (0);
}


/* ------------------------------------------------------------------------- */
struct _nodeinfo *strtoinfo(char *str)	 /* find info about the node in str */
/*char *str;				      /+ node 'x:yyyy/zzzz' */
{
    int n;

    n=getaddress(str,&remote.zone,&remote.net,&remote.node,&remote.point);
    if (n==0 || n>7) return NULL; /* string not correct! */

    nodetoinfo(&remote);
    return &remote;
}


/* ------------------------------------------------------------------------- */
char *get_passw (int zone, int net, int node, int point)
{
    PWD *pw;
    int pointnet;

    remap(&zone,&net,&node,&point,&pointnet);

    for (pw = pwd; pw; pw = pw->next) {
	if ((pw->zone  == -1 || pw->zone  == zone) &&
	    (pw->net   == -1 || pw->net   == net) &&
	    (pw->node  == -1 || pw->node  == node) &&
	    (pw->point == -1 || pw->point == point)) {
	   return (pw->password);
	}
    }

    return ("");  /* Note: not NULL! */
}


/* ------------------------------------------------------------------------- */
void set_alias (int zone, int net, int node, int point)
{
    char aaddress[128];
    ALIAS *ap;
    int pointnet;
    int my_aka;

    remap(&zone,&net,&node,&point,&pointnet);

    sprint(aaddress,"%d:%d/%d%s", zone, net, node, pointnr(point));
    for (ap = alias; ap; ap = ap->next) {
	if (patimat(aaddress,ap->pattern) ||
	    (callerid[0] && patimat(aaddress,ap->pattern)))
	   break;
    }
    my_aka = ap ? ap->my_aka : 0;
    ourbbs.zone     = aka[my_aka].zone;
    ourbbs.net	    = aka[my_aka].net;
    ourbbs.node     = aka[my_aka].node;
    ourbbs.point    = aka[my_aka].point;
    ourbbs.pointnet = aka[my_aka].pointnet;
    ourbbs.usepnt   = aka[my_aka].usepnt;
    if (zone)
       message(0,"+Assuming ID %d:%d/%d%s%s",
		 ourbbs.zone, ourbbs.net, ourbbs.node,
		 pointnr(ourbbs.point), zonedomabbrev(ourbbs.zone));
}


/* ------------------------------------------------------------------------- */
void remap (int *zone, int *net, int *node, int *point, int *pointnet)
{
    int i;

    *pointnet = 0;

    if (*point) {    /* 4D address: check whether there is a pointnet number */
       for (i = 0; i < nakas; i++) {
	   if (!aka[i].point &&
	       aka[i].zone == *zone &&
	       aka[i].net == *net && aka[i].node == *node) {
	      *pointnet = aka[i].pointnet;
	      break;
	   }
       }
    }
    else {	      /* 3D address: check for pointnet use, and remap to 4D */
       for (i = 0; i < nakas; i++) {
	   if (!aka[i].point && aka[i].pointnet &&
	       aka[i].zone == *zone && aka[i].pointnet == *net) {
	      *pointnet = *net;
	      *net	= aka[i].net;
	      *point	= *node;
	      *node	= aka[i].node;
	      break;
	   }
       }
    }
}


/* ------------------------------------------------------------------------- */
word getreqlevel (void)
{
	int i, j;
	word level = 0;
	boolean unlisted = false;
	DOMZONE *zp;

	if (ourbbs.password[0] && remote.password[0])
	   return (5);			   /* password protected session = 5 */

	for (i = 0; i < num_rem_adr; i++) {
	    if (level < 2 && !rem_adr[i].point) {
	       for (j = 0; j < nakas; j++) {
		   if (aka[j].point && aka[j].zone == rem_adr[i].zone &&
		       aka[j].net == rem_adr[i].net && aka[j].node == rem_adr[i].node) {
		      level = 2;		/* known, a boss of ours = 2 */
		      break;
		  }
	       }
	   }

	   if (level < 2 &&
	       get_adrinfo(rem_adr[i].zone,rem_adr[i].net,rem_adr[i].node,rem_adr[i].point))
	      level = 2;

	   if (!level) {
	      for (zp = domzone; zp; zp = zp->next) {
		  if (rem_adr[i].zone == zp->zone) {
		     unlisted = true;
		     break;
		  }
	      }
	   }
	}

	if (!level && !unlisted && domzone)
	   level = 1;

	return (level);
}/*getreqlevel()*/


/* end of getnode.c */
