/* include file to be used wherever io.h or share.h would normally          *
 * be used.                                                                 */

#if defined(__POSIX__) || defined(__GNUC__)
  #include <unistd.h>

  #define sopen(name, mode, share, imode) open(name, mode, imode)

  #define SH_DENYNO  0
  #define SH_DENYRW  0
  #define SH_DENYRD  0
  #define SH_DENYWR  0
  #define SH_DENYALL 0
  #define SH_COMPAT  0

  #define O_NOINHERIT 0

#else
  #include <io.h>
  #include <share.h>
#endif

