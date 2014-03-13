#ifndef _CPUINFO_H
#define _CPUINFO_H

#include <sys/sysinfo.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* The amount of time, measured in units of USER_HZ (1/100ths of a second on
 * most architectures, use sysconf(_SC_CLK_TCK) to obtain the right value)  */

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

  struct proc_cpu
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

/* Get the number of (active) cpu.*/

  static inline int get_processor_number ()
  {
    return
#if defined (HAVE_GET_NPROCS)
      get_nprocs ();
#elif defined (HAVE_GET_NPROCS_CONF)
      get_nprocs_conf ();
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_ONLN)
      sysconf (_SC_NPROCESSORS_ONLN);
#elif defined (HAVE_SYSCONF__SC_NPROCESSORS_CONF)
      sysconf (_SC_NPROCESSORS_CONF);
#else
      -1;
#endif
  }

  /* Fill the proc_cpu structure pointed with the values found in the 
   * proc filesystem */

  extern void proc_cpu_read (struct proc_cpu * __restrict cpuinfo);

#ifdef __cplusplus
}
#endif

#endif /* _CPUINFO_H */
