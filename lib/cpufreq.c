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
#include "cpufreq.h"
#include "sysfsparser.h"
#include "xalloc.h"
#include "xasprintf.h"

struct cpufreq_available_frequencies
{
  unsigned long value;
  struct cpufreq_available_frequencies *next;
};

unsigned long
cpufreq_get_freq_kernel (unsigned int cpu)
{
  return sysfsparser_cpufreq_get_freq_kernel (cpu);
}

struct cpufreq_available_frequencies *
cpufreq_get_available_freqs (unsigned int cpu)
{
  char *freqs =
    sysfsparser_cpufreq_get_available_freqs (cpu);

  if (NULL == freqs)
    return NULL;

  struct cpufreq_available_frequencies *first = NULL, *curr = NULL;
  char *token, *str, *saveptr;

  for (str = freqs; ; str = NULL)
    {
      token = strtok_r (str, " ", &saveptr);
      if (token == NULL)
	break;

      if (curr)
	{
	  curr->next = xmalloc (sizeof (*curr));
	  curr = curr->next;
	}
      else
	{
	  first = xmalloc (sizeof (*first));
	  curr = first;
	}
      curr->next = NULL;
      curr->value = strtoul (token, NULL, 10);
    }

  return first;
}

struct cpufreq_available_frequencies *
cpufreq_get_available_freqs_next (struct cpufreq_available_frequencies *curr)
{
  return curr->next;
}

unsigned long
cpufreq_get_available_freqs_value (struct cpufreq_available_frequencies *curr)
{
  return curr->value;
}

void
cpufreq_available_frequencies_unref(struct cpufreq_available_frequencies *first)
{
  struct cpufreq_available_frequencies *tmp, *curr = first;
  while (curr)
    {
      tmp = curr;
      curr = curr->next;
      free(tmp);
    }
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
      return xasprintf ("%u.%02uGHz", ((unsigned int) freq / 1000000),
		        ((unsigned int) (freq % 1000000) / 10000));
    }
  else if (freq > 100000)
    {
      tmp = freq % 1000;
      if (tmp >= 500)
	freq += 1000;
      return xasprintf ("%uMHz", ((unsigned int) freq / 1000));
    }
  else if (freq > 1000)
    {
      tmp = freq % 100;
      if (tmp >= 50)
	freq += 100;
      return xasprintf ("%u.%01uMHz", ((unsigned int) freq / 1000),
		        ((unsigned int) (freq % 1000) / 100));
    }
  else
    return xasprintf ("%lukHz", freq);
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
      return xasprintf ("%u.%02ums", ((unsigned int) duration / 1000000),
			((unsigned int) (duration % 1000000) / 10000));
    }
  else if (duration > 100000)
    {
      tmp = duration % 1000;
      if (tmp >= 500)
	duration += 1000;
      return xasprintf ("%uus", ((unsigned int) duration / 1000));
    }
  else if (duration > 1000)
    {
      tmp = duration % 100;
      if (tmp >= 50)
        duration += 100;
      return xasprintf ("%u.%01uus", ((unsigned int) duration / 1000),
			((unsigned int) (duration % 1000) / 100));
    }
  else
    return xasprintf ("%luns", duration);
}
