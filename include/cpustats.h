/* cpustats.h -- a library for checking the CPU utilization
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
 *            (since Linux 2.6.33)
 * ctxt      The number of context switches that the system underwent 	  */

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
    jiff ctxt;
  };

  /* Fill the cpu_stats structure pointed with the values found in the 
   * proc filesystem */
  extern void cpu_stats_read (struct cpu_stats * __restrict cpustats);

#ifdef __cplusplus
}
#endif

#endif		/* _CPUSTATS_H */
