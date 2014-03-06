#ifndef _MEMINFO_H_
#define _MEMINFO_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  enum unit_shift
  {
    b_shift = 0, k_shift = 10, m_shift = 20, g_shift = 30
  };

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

#define SU(X) ( ((unsigned long long)(X) << k_shift) >> shift ), units

  void get_meminfo (bool, struct memory_status **);
  void get_mempaginginfo (unsigned long *, unsigned long *);

  void get_swapinfo (struct swap_status **);
  void get_swappaginginfo (unsigned long *, unsigned long *);

#ifdef __cplusplus
}
#endif

#endif				/* _MEMINFO_H_ */
