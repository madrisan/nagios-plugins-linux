/* cpustats.h -- a library for checking the CPU utilization

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _CPUSTATS_H
#define _CPUSTATS_H

#include <sys/sysinfo.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef unsigned long long jiff;

  struct cpu_time
  {
    /* The name of the cpu found in /proc/stat: cpu, cpu0, cpu1, ... */
    const char *cpuname;
    /* Time spent running non-kernel code. (user time, including nice time) */
    jiff user;
    /* Time spent in user mode with low priority (nice) */
    jiff nice;
    /* Time spent running kernel code. (system time) */
    jiff system;
    /* Time spent idle. Prior to Linux 2.5.41, this includes IO-wait time */
    jiff idle;
    /* Time spent waiting for IO. Prior to Linux 2.5.41, included in idle */
    jiff iowait;
    /* Time servicing interrupts. (since Linux 2.6.0-test4) */
    jiff irq;
    /* Time servicing softirqs.i (since Linux 2.6.0-test4) */
    jiff softirq;
    /* Stolen time, which is the time spent in other operating systems
     * when running in a virtualized environment. (since Linux 2.6.11) */
    jiff steal;
    /* Time spent running a virtual CPU for guest operating systems
     * under the control of the Linux kernel. (since Linux 2.6.24) */
    jiff guest;
    /* Time spent running a niced guest (virtual CPU for guest
     * operating systems under the control of the Linux kernel).
     * (since Linux 2.6.33) */
    jiff guestn;
  };

  /* Get the cpu time statistics
   *  lines = 1 --> 'cpu' only
   *  lines = 3 --> 'cpu', 'cpu0', 'cpu1'
   * and so on  */
  void cpu_stats_get_time (struct cpu_time * __restrict cputime,
			   unsigned int lines);

  /* Get the number of context switches that the system underwent */
  unsigned long long cpu_stats_get_cswch ();

  /* Get the number of interrupts serviced since boot time, for each of the
   * possible system interrupts, including unnumbered architecture specific
   * interrupts (since Linux 2.6.0-test4)  */
  unsigned long long cpu_stats_get_intr ();

  /* Get the total of softirqs the system has experienced
   * (since Linux 2.6.0-test4) */
  unsigned long long cpu_stats_get_softirq ();

#ifdef __cplusplus
}
#endif

#endif		/* _CPUSTATS_H */
