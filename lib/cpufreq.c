/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the CPU frequency configuration
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "sysfsparser.h"
#include "xasprintf.h"

unsigned long
cpufreq_get_freq_kernel (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_freq_kernel (cpu);
}

int
cpufreq_get_hardware_limits (unsigned int cpu,
			     unsigned long *min, unsigned long *max)
{
  return sysfsparser_cpufreq_get_hardware_limits (cpu, min, max);
}

unsigned long
cpufreq_get_transition_latency (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_transition_latency (cpu);
}

char *
cpufreq_get_driver (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_driver (cpu);
}

char *
cpufreq_get_governor (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_governor (cpu);
}

char *
cpufreq_get_available_governors (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_available_governors (cpu);
}

char *
cpufreq_freq_to_string (unsigned long freq)
{
  unsigned long tmp;

  if (freq > 1000000)
    { 
      tmp = freq % 10000;
      if (tmp >= 5000)
	freq += 10000;
      return xasprintf ("%u.%02u GHz", ((unsigned int) freq / 1000000),
		        ((unsigned int) (freq % 1000000) / 10000));
    }
  else if (freq > 100000)
    {
      tmp = freq % 1000;
      if (tmp >= 500)
	freq += 1000;
      return xasprintf ("%u MHz", ((unsigned int) freq / 1000));
    }
  else if (freq > 1000)
    {
      tmp = freq % 100;
      if (tmp >= 50)
	freq += 100;
      return xasprintf ("%u.%01u MHz", ((unsigned int) freq / 1000),
		        ((unsigned int) (freq % 1000) / 100));
    }
  else
    return xasprintf ("%lu kHz", freq);
}

char *
cpufreq_duration_to_string (unsigned long duration)
{
  unsigned long tmp;

  if (duration > 1000000)
    {
      tmp = duration % 10000;
      if (tmp >= 5000)
	duration += 10000;
      return xasprintf ("%u.%02u ms", ((unsigned int) duration / 1000000),
			((unsigned int) (duration % 1000000) / 10000));
    }
  else if (duration > 100000)
    {
      tmp = duration % 1000;
      if (tmp >= 500)
	duration += 1000;
      return xasprintf ("%u us", ((unsigned int) duration / 1000));
    }
  else if (duration > 1000)
    {
      tmp = duration % 100;
      if (tmp >= 50)
        duration += 100;
      return xasprintf ("%u.%01u us", ((unsigned int) duration / 1000),
			((unsigned int) (duration % 1000) / 100));
    }
  else
    return xasprintf ("%lu ns", duration);
}
