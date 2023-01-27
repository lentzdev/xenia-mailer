/*# name=Structs for utime()
*/

#ifndef __UTIME_H_DEFINED
#define __UTIME_H_DEFINED

#include <time.h>

#if defined(__TURBOC__)
  struct utimbuf
  {
    time_t actime;  /* access time */
    time_t modtime; /* modification time */
  };
#elif defined(__POSIX__)
  #include <utime.h>
#else
  #include <sys/utime.h>
#endif

#endif

