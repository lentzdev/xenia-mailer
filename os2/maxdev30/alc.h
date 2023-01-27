/*# name=Include file to pick between MALLOC.H and ALLOC.H
*/

#ifndef __ALC_H_DEFINED
#define __ALC_H_DEFINED

#include "compiler.h"

#if defined(_lint) || defined(__MSC__) || defined(__WATCOMC__) || defined(__IBMC__) || defined(__TOPAZ__)
  #include <malloc.h>

  #ifdef __FARDATA__

  /* for some insane reason the turbo-c coreleft() function changes
   * it's return value based on the memory model.
   */

    unsigned long cdecl coreleft   (void);
  #else
    unsigned cdecl coreleft        (void);
  #endif

#elif defined(__TURBOC__)
  #include <alloc.h>
#else
  #include <string.h>
#endif

#ifdef __TURBOC__
#define halloc(x,y) ((char far *)farmalloc(x*y))
#define hfree(p)    farfree(p)
#endif

#endif /* __ALC_H_DEFINED */

