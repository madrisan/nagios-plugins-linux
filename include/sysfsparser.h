/* sysfsparser -- get informations from sysfs
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

#ifndef _SYSFSPARSER_H
#define _SYSFSPARSER_H

#ifdef __cplusplus
extern "C"
{
#endif

  extern int sysfsparser_cpufreq_get_hardware_limits (unsigned int cpu,
						      unsigned long *min,
						      unsigned long *max);
  extern unsigned long sysfsparser_cpufreq_get_freq_kernel (unsigned int cpu);
  extern unsigned long sysfsparser_cpufreq_get_transition_latency (unsigned int cpu);

  extern char *sysfsparser_cpufreq_get_driver (unsigned int cpu);
  extern char *sysfsparser_cpufreq_get_governor (unsigned int cpu);
  extern char *sysfsparser_cpufreq_get_available_governors (unsigned int cpu);

#ifdef __cplusplus
}
#endif

#endif				/* _SYSFSPARSER_H */
