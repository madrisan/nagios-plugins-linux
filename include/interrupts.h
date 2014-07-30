/* interrupts.h -- a library for checking the system interrupts

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

#ifndef _CPUDESC_H
#define _CPUDESC_H

#ifdef __cplusplus
extern "C"
{
#endif

  /* Return an array containing the number of interrupts per cpu per IO device
   * Since Linux 2.6.24, for the i386 and x86_64 architectures, at least, this
   * also includes interrupts internal to the system (that is, not associated
   * with a device as such).  */
  extern unsigned long *proc_interrupts_get_nintr_per_cpu (unsigned int *ncpus);

#ifdef __cplusplus
}
#endif

#endif		/* _INTERRUPTS_H */
