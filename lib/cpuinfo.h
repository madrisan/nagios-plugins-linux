#ifndef _CPUINFO_H
#define _CPUINFO_H

#include "config.h"
#include <sys/sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get the number of (active) cpu.*/
static inline int get_processor_number ()
{
  return
#if defined (HAVE_GET_NPROCS)
    get_nprocs();
#elif defined (HAVE_GET_NPROCS_CONF)
    get_nprocs_conf();
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_ONLN)
    sysconf(_SC_NPROCESSORS_ONLN);
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_CONF)
    sysconf(_SC_NPROCESSORS_CONF);
#else
    -1;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _CPUINFO_H */
