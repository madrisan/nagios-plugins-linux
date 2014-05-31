/* cputopology.h -- a library for checking the CPU topology
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

#ifndef _CPUTOPOLOGY_H
#define _CPUTOPOLOGY_H

#ifdef __cplusplus
extern "C"
{
#endif

  /* Get the number of total and active cpus. */
  extern int get_processor_number_total ();
  extern int get_processor_number_online ();

  /* Get the maximum cpu index allowed by the kernel configuration. */
  extern int get_processor_number_kernel_max ();

  /* Get the number of threads. */
  extern int get_cputopology_nthreads ();

#ifdef __cplusplus
}
#endif

#endif		/* _CPUTOPOLOGY_H */
