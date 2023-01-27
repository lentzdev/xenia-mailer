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
#if PC | OS2 | WINNT
#include <share.h>
#include <io.h>
#endif


static boolean dos_sharing = false;


void dos_sharecheck (void)    /* SHARE installed? set dos_sharing true/false */
{
#if PC
	union REGS regs;

	dos_sharing = false;

	regs.x.ax = 0x1000;				/* DOS Multiplexer   */
	int86(0x2f,&regs,&regs);			/* INT 2Fh sub 1000h */
	if (regs.h.al == 0xff)
	   dos_sharing = true;
	else {	/* Following one is to compensate for LANtastic stupidity */
	   regs.x.ax = 0x0000;				/* Network functions */
	   int86(0x2a,&regs,&regs);			/* INT 2Ah sub 0000h */
	   if (regs.h.ah != 0x00)
	      dos_sharing = true;
	}

#endif
#if OS2 | WINNT
	dos_sharing = true;
#endif
}/*dos_sharecheck()*/


int dos_sopen (char *pathname, unsigned int mode, int shareflag)
{
	register int i;

	com_disk(true);

#if PC | OS2 | WINNT
	i = sopen(pathname, mode | O_BINARY | O_NOINHERIT,
		  shareflag, (mode & O_CREAT) ? S_IREAD | S_IWRITE : 0);
#endif
#ifdef __TOS__
	i = open(pathname, mode, 0);
#endif

	com_disk(false);

	return (i);
}/*dos_sopen()*/


int dos_close (int handle)
{
	register int i;

	com_disk(true);
	i = close(handle);
	com_disk(false);

	return (i);
}/*dos_close()*/


int dos_lock (int handle, long offset, long len)
{
#if PC | OS2 | WINNT
	if (dos_sharing)
	   while (!lock(handle,offset,len));
#endif

	return (0);
}/*dos_lock()*/


int dos_unlock (int handle, long offset, long len)
{
#if PC | OS2 | WINNT
	return (dos_sharing ? unlock(handle,offset,len) : 0);
#endif
#ifdef __TOS__
	return 0;
#endif
}/*dos_unlock()*/


long dos_seek (int handle, long offset, int fromwhere)
{
	return (lseek(handle,offset,fromwhere));
}/*dos_seek()*/


long dos_tell (int handle)
{
#if PC | OS2 | WINNT
	return (tell(handle));
#endif
#ifdef __TOS__
	return (lseek(handle,0L,SEEK_CUR));
#endif
}/*dos_tell()*/


int dos_read (int handle, void *buf, word len)
{
#if PC | OS2 | WINNT
	unsigned i;
	int n;

	com_disk(true);
	if (_dos_read(handle,buf,len,&i))
	   n = -1;
	else
	   n = i;
	com_disk(false);
	return (n);
#endif

#ifdef __TOS__
	int i;

	com_disk(true);
	i = (int) read(handle,buf,len);
	com_disk(false);

	return (i);
#endif
}/*dos_read()*/


int dos_write (int handle, void *buf, word len)
{
#if PC | OS2 | WINNT
	unsigned i;
	int n;

	com_disk(true);
	if (_dos_write(handle,buf,len,&i))
	   n = -1;
	else
	   n = i;
	com_disk(false);
	return (n);
#endif

#ifdef __TOS__
	int i;

	com_disk(true);
	i = (int) write(handle,buf,len);
	com_disk(false);

	return (i);
#endif
}/*dos_write()*/


/* ----------------------------------------------------------------------------
			oflags			mode
	r	O_RDONLY			don't care
	w	O_CREAT | O_WRONLY | O_TRUNC	S_IWRITE
	a	O_CREAT | O_WRONLY | O_APPEND	S_IWRITE
	r+	O_RDWR				don't care
	w+	O_RDWR | O_CREAT | O_TRUNC	S_IWRITE | S_IREAD
	a+	O_RDWR | O_CREAT | O_APPEND	S_IWRITE | S_IREAD
---------------------------------------------------------------------------- */
FILE *sfopen (char *name, char *mode, int shareflag)
{
#if PC | OS2 | WINNT
	int   fd, access, flags;
	char  *type = mode, c;
	FILE *fp;

	if ((c = *type++) == 'r') {
	   access = O_RDONLY | O_NOINHERIT;
	   flags = 0;
	}
	else if (c == 'w') {
	   access = O_WRONLY | O_CREAT | O_TRUNC | O_NOINHERIT;
	   flags = S_IWRITE;
	}
	else if (c == 'a') {
	   access = O_WRONLY | O_CREAT | O_APPEND | O_NOINHERIT;
	   flags = S_IWRITE;
	}
	else
	   return (NULL);

	c = *type++;

	if (c == '+' || (*type == '+' && (c == 't' || c == 'b'))) {
	   if (c == '+') c = *type;
	   access = (access & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
	   flags = S_IREAD | S_IWRITE;
	}

	if	(c == 't') access |= O_TEXT;
	else if (c == 'b') access |= O_BINARY;
	else		   access |= (_fmode & (O_TEXT | O_BINARY));

	com_disk(true);
	fd = sopen(name, access, shareflag, flags);
	com_disk(false);

	if (fd < 0) return (NULL);

	if ((fp = fdopen(fd,mode)) == NULL)
	   close(fd);
	else if (setvbuf(fp,NULL,_IOFBF,BUFSIZ)) {
	   fclose(fp);
	   return (NULL);
	}

	return (fp);
#endif

#ifdef __TOS__
	char *p, *q, nm[10];
	FILE fp;

	strcpy(nm,mode);
	for(p=q=nm; *p; p++) if (*p!='t') *q++=*p;
	*q='\0';

	com_disk(true);
	fp = fopen(name,nm);
	com_disk(false);
	return (fp);
#endif
}/*sfopen()*/


/* end of dos_file.c */
