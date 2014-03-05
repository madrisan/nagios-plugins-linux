#ifndef _MEMINFO_H_
#define _MEMINFO_H_

#include "config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SU(X) ( ((unsigned long long)(X) << 10) >> shift ), units

  void meminfo (bool, unsigned long *, unsigned long *, unsigned long *,
		unsigned long *, unsigned long *, unsigned long *);
  void mempaginginfo (unsigned long *, unsigned long *);

  void swapinfo (unsigned long *, unsigned long *, unsigned long*,
		 unsigned long*);
  void swappaginginfo (unsigned long *, unsigned long *);

#ifdef __cplusplus
}
#endif

#endif				/* _MEMINFO_H_ */
