#ifndef __STAMP_H_DEFINED
#define __STAMP_H_DEFINED

#include "typedefs.h"

struct _stamp   /* DOS-style datestamp */
{
  struct
  {
    /* IBM Cset/2 is allergic to "unsigned short" when declaring bitfields! */

#ifdef __IBMC__
    unsigned int da : 5;
    unsigned int mo : 4;
    unsigned int yr : 7;
#else
/*lint -e46 */
    word da : 5;
    word mo : 4;
    word yr : 7;
/*lint -restore */
#endif
  } date;

  struct
  {
#ifdef __IBMC__
    unsigned int ss : 5;
    unsigned int mm : 6;
    unsigned int hh : 5;
#else
/*lint -e46 */
    word ss : 5;
    word mm : 6;
    word hh : 5;
/*lint -restore */
#endif
  } time;
};


struct _dos_st
{
  word date;
  word time;
};

/* Union so we can access stamp as "int" or by individual components */

union stamp_combo   
{
  dword ldate;
  struct _stamp msg_st;
  struct _dos_st dos_st;
};

typedef union stamp_combo SCOMBO;

#endif /* __STAMP_H_DEFINED */

