/* ------------------------------------------------------------------------- */
/* A derived work of the RSA Data Security,Inc MD5 Message Digest algorithm, */
/* from version 02/17-91 - 07/10/91; See MD5.DOC for a complete description. */
/* Generic C & C++ class implementation by Arjen G. Lentz (07/30/91)	     */
/* License for any use granted, provided this notice & MD5.DOC are included. */
/* ------------------------------------------------------------------------- */
#include "xenia.h"
#include "md5.h"
#define LOWBYTEFIRST 1	     /* Set to 0 for CPUs with high byte first order */


/* ------------------------------------------------------------------------- */
#define I0	0x67452301UL			/* Initial values for resbuf */
#define I1	0xefcdab89UL
#define I2	0x98badcfeUL
#define I3	0x10325476UL
#define fs1	 7				/* round 1 shift amounts */
#define fs2	12
#define fs3	17
#define fs4	22
#define gs1	 5				/* round 2 shift amounts */
#define gs2	 9
#define gs3	14
#define gs4	20
#define hs1	 4				/* round 3 shift amounts */
#define hs2	11
#define hs3	16
#define hs4	23
#define is1	 6				/* round 4 shift amounts */
#define is2	10
#define is3	15
#define is4	21

#define f(X,Y,Z) ((X & Y) | (~X & Z))
#define g(X,Y,Z) ((X & Z) | (Y & ~Z))
#define h(X,Y,Z) (X ^ Y ^ Z)
#define i(X,Y,Z) (Y ^ (X | ~Z))
#define rot(X,N) ((X << N) | (X >> (32 - N)))
#define ff(A,B,C,D,x,s,AC) { A += f(B,C,D) + buf[x] + AC; A = rot(A,s); A += B; }
#define gg(A,B,C,D,x,s,AC) { A += g(B,C,D) + buf[x] + AC; A = rot(A,s); A += B; }
#define hh(A,B,C,D,x,s,AC) { A += h(B,C,D) + buf[x] + AC; A = rot(A,s); A += B; }
#define ii(A,B,C,D,x,s,AC) { A += i(B,C,D) + buf[x] + AC; A = rot(A,s); A += B; }


/* ------------------------------------------------------------------------- */
#if !LOWBYTEFIRST
#ifdef __cplusplus
void MD5 :: reverse (dword *buf)
#else
static void md5_reverse (dword *buf)
#endif
{
	register dword tmp;

#define revbuf { tmp = (*buf << 16) | (*buf >> 16); \
		 *buf++ = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); }

	revbuf; revbuf; revbuf; revbuf; revbuf; revbuf; revbuf; revbuf;
	revbuf; revbuf; revbuf; revbuf; revbuf; revbuf; revbuf; revbuf;
}/*reverse()*/
#endif


/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
void MD5 :: block (dword *buf)
#else
static void md5_block (MD5 *this, dword *buf)
#endif
{
	register dword A, B, C, D;

#if !LOWBYTEFIRST
#ifdef __cplusplus
	reverse(buf);
#else
	md5_reverse(buf);
#endif
#endif

	A = this->resbuf[0];
	B = this->resbuf[1];
	C = this->resbuf[2];
	D = this->resbuf[3];

	/* Now update the message digest buffer */

	ff(A,B,C,D, 0,fs1,3614090360UL); ff(D,A,B,C, 1,fs2,3905402710UL); /*1*/
	ff(C,D,A,B, 2,fs3, 606105819UL); ff(B,C,D,A, 3,fs4,3250441966UL);
	ff(A,B,C,D, 4,fs1,4118548399UL); ff(D,A,B,C, 5,fs2,1200080426UL);
	ff(C,D,A,B, 6,fs3,2821735955UL); ff(B,C,D,A, 7,fs4,4249261313UL);
	ff(A,B,C,D, 8,fs1,1770035416UL); ff(D,A,B,C, 9,fs2,2336552879UL);
	ff(C,D,A,B,10,fs3,4294925233UL); ff(B,C,D,A,11,fs4,2304563134UL);
	ff(A,B,C,D,12,fs1,1804603682UL); ff(D,A,B,C,13,fs2,4254626195UL);
	ff(C,D,A,B,14,fs3,2792965006UL); ff(B,C,D,A,15,fs4,1236535329UL);

	gg(A,B,C,D, 1,gs1,4129170786UL); gg(D,A,B,C, 6,gs2,3225465664UL); /*2*/
	gg(C,D,A,B,11,gs3, 643717713UL); gg(B,C,D,A, 0,gs4,3921069994UL);
	gg(A,B,C,D, 5,gs1,3593408605UL); gg(D,A,B,C,10,gs2,  38016083UL);
	gg(C,D,A,B,15,gs3,3634488961UL); gg(B,C,D,A, 4,gs4,3889429448UL);
	gg(A,B,C,D, 9,gs1, 568446438UL); gg(D,A,B,C,14,gs2,3275163606UL);
	gg(C,D,A,B, 3,gs3,4107603335UL); gg(B,C,D,A, 8,gs4,1163531501UL);
	gg(A,B,C,D,13,gs1,2850285829UL); gg(D,A,B,C, 2,gs2,4243563512UL);
	gg(C,D,A,B, 7,gs3,1735328473UL); gg(B,C,D,A,12,gs4,2368359562UL);

	hh(A,B,C,D, 5,hs1,4294588738UL); hh(D,A,B,C, 8,hs2,2272392833UL); /*3*/
	hh(C,D,A,B,11,hs3,1839030562UL); hh(B,C,D,A,14,hs4,4259657740UL);
	hh(A,B,C,D, 1,hs1,2763975236UL); hh(D,A,B,C, 4,hs2,1272893353UL);
	hh(C,D,A,B, 7,hs3,4139469664UL); hh(B,C,D,A,10,hs4,3200236656UL);
	hh(A,B,C,D,13,hs1, 681279174UL); hh(D,A,B,C, 0,hs2,3936430074UL);
	hh(C,D,A,B, 3,hs3,3572445317UL); hh(B,C,D,A, 6,hs4,  76029189UL);
	hh(A,B,C,D, 9,hs1,3654602809UL); hh(D,A,B,C,12,hs2,3873151461UL);
	hh(C,D,A,B,15,hs3, 530742520UL); hh(B,C,D,A, 2,hs4,3299628645UL);

	ii(A,B,C,D, 0,is1,4096336452UL); ii(D,A,B,C, 7,is2,1126891415UL); /*4*/
	ii(C,D,A,B,14,is3,2878612391UL); ii(B,C,D,A, 5,is4,4237533241UL);
	ii(A,B,C,D,12,is1,1700485571UL); ii(D,A,B,C, 3,is2,2399980690UL);
	ii(C,D,A,B,10,is3,4293915773UL); ii(B,C,D,A, 1,is4,2240044497UL);
	ii(A,B,C,D, 8,is1,1873313359UL); ii(D,A,B,C,15,is2,4264355552UL);
	ii(C,D,A,B, 6,is3,2734768916UL); ii(B,C,D,A,13,is4,1309151649UL);
	ii(A,B,C,D, 4,is1,4149444226UL); ii(D,A,B,C,11,is2,3174756917UL);
	ii(C,D,A,B, 2,is3, 718787259UL); ii(B,C,D,A, 9,is4,3951481745UL);

	this->resbuf[0] += A;
	this->resbuf[1] += B;
	this->resbuf[2] += C;
	this->resbuf[3] += D;

#if !LOWBYTEFIRST
	if (buf != this->databuf)   /* No need to re-reverse internal buffer */
#ifdef __cplusplus
	   reverse(buf);
#else
	   md5_reverse(buf);
#endif
#endif
}/*block()*/


/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
void MD5 :: init (void)
#else
void md5_init (register MD5 *this)
#endif
{
	this->resbuf[0] = I0;
	this->resbuf[1] = I1;
	this->resbuf[2] = I2;
	this->resbuf[3] = I3;
	memset(this->count, 0, 8 * sizeof (byte));
	this->datalen = 0;
	this->done = false;
}/*init()*/


/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
boolean MD5 :: update (register byte *buf, register word len)
#else
int md5_update (register MD5 *this, register byte *buf, register word len)
#endif
{
	if (this->done)
	    return (false);

	if (len > 0) {
	   register byte  *p;
	   register dword  tmp;

	   tmp = len << 3;		    /* Add new length to bit counter */

	   /* This is a simple add operation, but using an 8 byte variable!  */
	   p = this->count;
	   while (tmp) {
		 tmp   += *p;
		 *p++	= (byte) tmp;
		 tmp  >>= 8;
	   }

	   /* This three-part stuff looks funny, but it's faster then having */
	   /* to memmove() all the data into the work buffer		     */

	   if (this->datalen > 0) {		/* Already stuff in our buf? */
	      register byte movelen;

	      movelen = 64 - this->datalen;	/* How many free bytes left? */
	      if (len < movelen)		/* Do we have less to add?   */
		 movelen = len; 		/* Apparently, so use that   */
	      memmove(this->databuf + this->datalen, buf, movelen);  /* Move */
	      this->datalen += movelen; 	/* Add to workbuffer len     */
	      if (this->datalen < 64)		/* Do we have full 64 bytes? */
		 return (true); 		/* Nope, so return now	     */
#ifdef __cplusplus
	      block((dword *) databuf); 	/* Process the full buffer   */
#else
	      md5_block(this, (dword *) this->databuf);
#endif
	      buf += movelen;			/* Move along buffer ptr     */
	      len -= movelen;			/* Substract from len left   */
	   }

	   while (len >= 64) {			/* Can we do more 64 chunks? */
#ifdef __cplusplus
		 block((dword *) buf);		/* Process without moving!!! */
#else
		 md5_block(this, (dword *) buf);
#endif
		 buf += 64;			/* Move along buffer ptr     */
		 len -= (word) 64;		/* Substract from len left   */
	   }

	   if (len > 0) {			/* Anything left in buffer?  */
	      memmove(this->databuf, buf, len); /* Ya, so move to remember   */
	      this->datalen = len;		/* Note length for next time */
	   }
	   else
	      this->datalen = 0;		/* Nowt to remember....      */
	}

	return (true);
}/*update()*/


#ifdef __cplusplus
char * MD5 :: result (void)
#else
char *md5_result (MD5 *this)
#endif
{
	if (!this->done) {
	   register byte i, j, ptr;

	   /* This part looks different from original because that was a     */
	   /* bit-based system, but I assume we have a multiple of 8 bits... */
	   /* If it isn't, you need some masking & shifting on the last byte */

	   this->databuf[this->datalen++] = 0x80;	/* Add padding 1 bit */
	   if (this->datalen <= 56)	       /* Room enough for bit count? */
	      memset(this->databuf + this->datalen, 0, 56 - this->datalen);
	   else {
	      memset(this->databuf + this->datalen, 0, 64 - this->datalen);
#ifdef __cplusplus
	      block((dword *) databuf);
#else
	      md5_block(this, (dword *) this->databuf);
#endif
	      memset(this->databuf, 0, 56);
	   }

	   memmove(this->databuf + 56, this->count, 8 * sizeof (byte));
#ifdef __cplusplus
	   block((dword *) databuf);
#else
	   md5_block(this, (dword *) this->databuf);
#endif

	   for (ptr = 0, i = 0; i < 4; i++)
	       for (j = 0; j < 32; ptr += 2, j += 8)
		   sprint(((char *) this->databuf) + ptr, "%02x",
			  (word) (this->resbuf[i] >> j) & 0xff);

	   this->done = true;
	}

	return ((char *) this->databuf);
}/*result()*/


/* end of md5.c */
