/* cpufreq.h -- a library for checking the CPU frequency configuration
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

#ifndef _CPUFREQ_H
#define _CPUFREQ_H

#ifdef __cplusplus
extern "C"
{
#endif

  extern long cpufreq_get_freq_min (void);
  extern long cpufreq_get_freq_max (void);

#ifdef __cplusplus
}
#endif

#endif		/* _CPUFREQ_H */
