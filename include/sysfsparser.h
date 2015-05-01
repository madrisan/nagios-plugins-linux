/* sysfsparser -- get informations from sysfs

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

#ifndef _SYSFSPARSER_H
#define _SYSFSPARSER_H

#include <dirent.h>
#include <limits.h>
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /* generic functions */

  bool sysfsparser_path_exist (const char *path, ...)
       _attribute_format_printf_(1, 2);
  void sysfsparser_opendir(DIR **dirp, const char *path, ...)
       _attribute_format_printf_(2, 3);
  void sysfsparser_closedir(DIR *dirp);
  struct dirent *sysfsparser_readfilename(DIR *dirp, unsigned int flags);

  char *sysfsparser_getline (const char *filename, ...)
       _attribute_format_printf_(1, 2);
  unsigned long sysfsparser_getvalue (const char *filename, ...)
       _attribute_format_printf_(1, 2);

  /* cpufreq */

  int sysfsparser_cpufreq_get_hardware_limits (unsigned int cpu,
					       unsigned long *min,
					       unsigned long *max);
  unsigned long sysfsparser_cpufreq_get_freq_kernel (unsigned int cpu);
  unsigned long sysfsparser_cpufreq_get_transition_latency (unsigned int cpu);

  char *sysfsparser_cpufreq_get_available_freqs (unsigned int cpu);
  char *sysfsparser_cpufreq_get_driver (unsigned int cpu);
  char *sysfsparser_cpufreq_get_governor (unsigned int cpu);
  char *sysfsparser_cpufreq_get_available_governors (unsigned int cpu);

  /* thermal sensors */

# define ALL_THERMAL_ZONES   UINT_MAX

  bool sysfsparser_thermal_kernel_support (void);
  int sysfsparser_thermal_get_temperature (unsigned int selected_zone,
						  unsigned int *zone, char **type);
  int sysfsparser_thermal_get_critical_temperature (unsigned int thermal_zone);

#ifdef __cplusplus
}
#endif

#endif				/* _SYSFSPARSER_H */
