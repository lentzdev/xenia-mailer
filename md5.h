/* ------------------------------------------------------------------------- */
/* A derived work of the RSA Data Security,Inc MD5 Message Digest algorithm, */
/* from version 02/17/91 - 07/10/91; See MD5.DOC for a complete description. */
/* Generic C & C++ class implementation by Arjen G. Lentz (07/30/91)	     */
/* License for any use granted, provided this notice & MD5.DOC are included. */
/* ------------------------------------------------------------------------- */
#include "2types.h"


#ifdef __cplusplus
class MD5 {	 // ---------------------- MD5 Message Digest algorithm class
	dword	resbuf[4];		// Holds 4-dword result of computation
	byte	count[8];		// Number of bits processed so far
	boolean done;			// MD5 computation finished?
	byte	databuf[64];		// Buffer to remember strings <64 bytes
	byte	datalen;		// Number of bytes in databuf

	void	reverse (dword *buf);		// Private byte-order reverser
	void	block	(dword *buf);		// Private block process func

public: void	init	(void); 		// Inits computation variables
	boolean update	(byte *buf, word len);	// Do; ret true=ok, false=fail
	char   *result	(void); 		// Ret MD5 in 32 hex digit str

	MD5 (void) { init(); }			// Constructor
};
#else
typedef struct {
	dword	resbuf[4];
	byte	count[8];
	int	done;
	byte	databuf[64];
	byte	datalen;
} MD5;

void	 md5_init   (MD5 *this);
int	 md5_update (MD5 *this, byte *buf, word len);
char	*md5_result (MD5 *this);
#endif


/* end of md5.h */
