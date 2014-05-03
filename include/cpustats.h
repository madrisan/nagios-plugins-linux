#ifndef _CPUSTATS_H
#define _CPUSTATS_H

#include <sys/sysinfo.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef unsigned long long jiff;

/* cuser:    Time spent running non-kernel code. (user time, including nice time)
 * cnice:    Time spent in user mode with low priority (nice)
 * csystem:  Time spent running kernel code. (system time)
 * cidle:    Time spent idle. Prior to Linux 2.5.41, this includes IO-wait time
 * ciowait:  Time spent waiting for IO. Prior to Linux 2.5.41, included in idle
 * cirq:     Time servicing interrupts. (since Linux 2.6.0-test4)
 * csoftirq: Time servicing softirqs.i (since Linux 2.6.0-test4)
 * csteal;   Stolen time, which is the time spent in other operating systems
 *            when running in a virtualized environment. (since Linux 2.6.11)
 * cguest    Time spent running a virtual CPU for guest operating systems
 *            under the control of the Linux kernel. (since Linux 2.6.24)
 * cguestn   Time spent running a niced guest (virtual CPU for guest
 *            operating systems under the control of the Linux kernel).
 *            (since Linux 2.6.33)	*/

  struct cpu_stats
  {
    jiff user;
    jiff nice;
    jiff system;
    jiff idle;
    jiff iowait;
    jiff irq;
    jiff softirq;
    jiff steal;
    jiff guest;
    jiff guestn;
  };

  struct cpu_desc;

/* Get the number of total and active cpus */

  static inline int get_processor_number_total ()
  {
    return
#if defined (HAVE_GET_NPROCS_CONF)
      get_nprocs_conf ();
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_CONF)
      sysconf (_SC_NPROCESSORS_CONF);
#else
      -1;
#endif
  }

  static inline int get_processor_number_online ()
  {
    return
#if defined (HAVE_GET_NPROCS)
      get_nprocs ();
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_ONLN)
      sysconf (_SC_NPROCESSORS_ONLN);
#else
      -1;
#endif
  }

  /* Allocates space for a new cpu_desc object.
   * Returns 0 if all went ok. Errors are returned as negative values.  */
  extern int cpu_desc_new (struct cpu_desc **cpudesc);

  /* Fill the cpu_desc structure pointed with the values found in the 
   * proc filesystem */
  extern void cpu_desc_read (struct cpu_desc * __restrict cpudesc);

  /* Drop a reference of the cpu_desc library context. If the refcount of
   * reaches zero, the resources of the context will be released.  */
  extern struct cpu_desc *cpu_desc_unref (struct cpu_desc *cpudesc);

  /* Accessing the values from cpu_desc */
  extern char *cpu_desc_get_architecture (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_vendor (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_family (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_model (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_model_name (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_virtualization_flag (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_mhz (struct cpu_desc *cpudesc);
  extern char *cpu_desc_get_flags (struct cpu_desc *cpudesc);

  enum		/* CPU modes */
  {
    MODE_32BIT = (1 << 1),
    MODE_64BIT = (1 << 2)
  };
  extern int cpu_desc_get_mode (struct cpu_desc *cpudesc);

  extern int cpu_desc_get_number_of_cpus (struct cpu_desc *cpudesc);

  /* Fill the cpu_stats structure pointed with the values found in the 
   * proc filesystem */
  extern void cpu_stats_read (struct cpu_stats * __restrict cpustats);

#ifdef __cplusplus
}
#endif

#endif /* _CPUSTATS_H */
