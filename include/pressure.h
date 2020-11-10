// SPDX-License-Identifier: GPL-3.0-or-later
/* pressure.h -- Linux Pressure Stall Information (PSI) parser

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _PRESSURE_H_
#define _PRESSURE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define PATH_PROC_PRESSURE	"/proc/pressure"
#define PATH_PSI_PROC_CPU     PATH_PROC_PRESSURE "/cpu"
#define PATH_PSI_PROC_IO	PATH_PROC_PRESSURE "/io"
#define PATH_PSI_PROC_MEMORY  PATH_PROC_PRESSURE "/memory"

  enum linux_psi_id		/* Linux PSI modes */
  {
    LINUX_PSI_NONE = 0,
    LINUX_PSI_CPU,
    LINUX_PSI_IO,
    LINUX_PSI_MEMORY
  };

  /* Structure for pressure-stall cpu statistics.
   *  - avg* fields: percentage of time in the last 10, 60 and 300 seconds
   *           respectively that processes were starved of CPU,
   *  - total: total time in microseconds that processes were starved of CPU.
   */
  struct proc_psi_oneline
  {
    unsigned long long total;
    double avg10;
    double avg60;
    double avg300;
  };

  /* Structure for pressure-stall io and memory statistics
   */
  struct proc_psi_twolines
  {
    unsigned long long some_total;
    unsigned long long full_total;
    double some_avg10;
    double some_avg60;
    double some_avg300;
    double full_avg10;
    double full_avg60;
    double full_avg300;
  };

  int proc_psi_read_cpu (struct proc_psi_oneline **psi_cpu,
			 unsigned long long *starvation);
  int proc_psi_read_io (struct proc_psi_twolines **psi_io);
  int proc_psi_read_memory (struct proc_psi_twolines **psi_memory);

#ifdef __cplusplus
}
#endif

#endif				/* pressure.h */
