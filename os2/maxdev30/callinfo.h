/*
 * Structure of Callers.Bbs
 * Maximus (optional) caller information & activity log
 */

#ifndef CALLINFO_H_DEFINED
#define CALLINFO_H_DEFINED

#define CALL_LOGON      0x8000                  /* Caller was logged on! */
#define CALL_CARRIER    0x0001                    /* Caller lost carrier */
#define CALL_EJECTED    0x0002    /* Caller was ejected by sysop or priv */
#define CALL_PAGED_AH   0x0004               /* Caller paged after hours */
#define CALL_DLEXCEED   0x0008         /* Caller attempted to exceed d/l */
#define CALL_EXPIRED    0x0010          /* Caller's subscription expired */
#define CALL_TIMELIMIT  0x0020             /* Caller exceeded time limit */
#define CALL_NERD       0x0040                     /* Caller was nerd'ed */
#define CALL_BARRPWD    0x0080             /* Barricade password failure */

struct callinfo
{
  byte              name[36];     /* User's name/alias                 0*/
  byte              city[36];     /* User's city                      36*/
  union stamp_combo login;        /* Time logged on                   72*/
  union stamp_combo logoff;       /* Time logged off                  76*/
  word              task;         /* Which node                       80*/
  word              flags;        /* Call flags                       82*/
  word              logon_priv;   /* Priv level on login              84*/
  word              logoff_priv;  /* Priv level on logoff             86*/
  dword             logon_xkeys;  /* Keys on login                    88*/
  dword             logoff_xkeys; /* Keys on logoff                   92*/
  word              filesup;      /* Number of files uploaded         96*/
  word              filesdn;      /* Number of files dnloaded         98*/
  word              kbup;         /* kb uploaded                     100*/
  word              kbdn;         /* kb dnloaded                     102*/
  word              calls;        /* Number of previous calls + 1    104*/
  word              read;         /* Number of messages read         106*/
  word              posted;       /* Number of messages posted       108*/
  word              paged;        /* Number of times user paged      110*/
  sword             added;        /* Time added during call          112*/
  byte              reserved[14];                                 /* 114*/
};                                                                /* 128*/

#endif

