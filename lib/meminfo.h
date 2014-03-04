#ifndef _MEMINFO_H_
#define _MEMINFO_H_

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SU(X) ( ((unsigned long long)(X) << 10) >> shift ), units

  void meminfo (int);
  void swapinfo (void);
  void mempaginginfo (unsigned long *, unsigned long *);
  void swappaginginfo (unsigned long *, unsigned long *);

#ifdef __cplusplus
}
#endif

#endif				/* _MEMINFO_H_ */
