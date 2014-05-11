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

struct cpufreq_available_frequencies;

#ifdef __cplusplus
extern "C"
{
#endif

  extern int cpufreq_get_hardware_limits (unsigned int cpu,
				   	  unsigned long *min,
					  unsigned long *max);
  extern unsigned long cpufreq_get_transition_latency (unsigned int cpu);
  extern unsigned long cpufreq_get_freq_kernel (unsigned int cpu);

  extern struct cpufreq_available_frequencies *
	cpufreq_get_available_freqs (unsigned int cpu);
  extern struct cpufreq_available_frequencies *
        cpufreq_get_available_freqs_next (struct cpufreq_available_frequencies *curr);
  extern unsigned long cpufreq_get_available_freqs_value (
			 struct cpufreq_available_frequencies *curr);

  extern char *cpufreq_get_driver (unsigned int cpu);
  extern char *cpufreq_get_governor (unsigned int cpu);
  extern char *cpufreq_get_available_governors (unsigned int cpu);

  extern char* cpufreq_freq_to_string (unsigned long freq);
  extern char* cpufreq_duration_to_string (unsigned long duration);

#ifdef __cplusplus
}
#endif

#endif				/* _CPUFREQ_H */
