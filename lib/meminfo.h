#ifndef _MEMINFO_H_
#define _MEMINFO_H_

#include "config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct memory_status
  {
    unsigned long used;
    unsigned long total;
    unsigned long free;
    unsigned long shared;
    unsigned long buffers;
    unsigned long cached;
  } memory_status_struct;

  typedef struct swap_status
  {
    unsigned long used;
    unsigned long total;
    unsigned long free;
    unsigned long cached;
  } swap_status_struct;


#define SU(X) ( ((unsigned long long)(X) << 10) >> shift ), units

  void get_meminfo (bool, struct memory_status **);
  void get_mempaginginfo (unsigned long *, unsigned long *);

  void get_swapinfo (struct swap_status **);
  void get_swappaginginfo (unsigned long *, unsigned long *);

#ifdef __cplusplus
}
#endif

#endif				/* _MEMINFO_H_ */
