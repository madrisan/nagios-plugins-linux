/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check the CPU utilization.
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
 * This software is based on the source code of the tool "vmstat".
 */

 /* Definition of I/O wait time:
  *   The I/O wait time is the time during which a CPU was idle and
  *   there was at least one outstanding (disk or network) I/O operation
  *   requested by a task scheduled on that CPU.  */

#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpudesc.h"
#include "cpufreq.h"
#include "cpustats.h"
#include "cputopology.h"
#include "logging.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"
#include "system.h"
#include "xalloc.h"
#include "xasprintf.h"
#include "xstrtol.h"

/* by default one iteration with 1sec delay */
#define DELAY_DEFAULT	1
#define COUNT_DEFAULT	2

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static const char *program_shorthelp = NULL;

static struct option const longopts[] = {
  {(char *) "cpuinfo", no_argument, NULL, 'i'},
  {(char *) "cpufreq", no_argument, NULL, 'f'},
  {(char *) "no-cpu-model", no_argument, NULL, 'm'},
  {(char *) "per-cpu", no_argument, NULL, 'p'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "verbose", no_argument, NULL, 'v'},
  {(char *) "help", no_argument, NULL, GETOPT_HELP_CHAR},
  {(char *) "version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static _Noreturn void
usage (FILE * out)
{
  fprintf (out, "%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs (program_shorthelp, out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out,
	   "  %s [-v] [-f] [-m] [-p] [-w PERC] [-c PERC] [delay [count]]\n",
	   program_name);
  fprintf (out, "  %s --cpuinfo\n", program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -f, --cpufreq   show the CPU frequency characteristics\n", out);
  fputs ("  -m, --no-cpu-model  "
	 "do not display the CPU model in the output message\n", out);
  fputs ("  -p, --per-cpu   display the utilization of each CPU\n", out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
         "(Nagios may truncate output)\n", out);
  fputs ("  -i, --cpuinfo   show the CPU characteristics (for debugging)\n",
	 out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  delay is the delay between updates in seconds "
           "(default: %dsec)\n", DELAY_DEFAULT);
  fprintf (out, "  count is the number of updates "
           "(default: %d)\n", COUNT_DEFAULT);
  fputs ("\t1 means the percentages of total CPU time from boottime.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -m -p -w 85%% -c 95%%\n", program_name);
  fprintf (out, "  %s -f -w 85%% -c 95%% 1 2\n", program_name);
  fprintf (out, "  %s --cpuinfo\n", program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static _Noreturn void
print_version (void)
{
  printf ("%s (" PACKAGE_NAME ") v%s\n", program_name, program_version);
  fputs (program_copyright, stdout);
  fputs (GPLv3_DISCLAIMER, stdout);

  exit (STATE_OK);
}

/* output formats "<key>:  <value>" */
#define print_n(_key, _val)  printf ("%-30s%d\n", _key, _val)
#define print_s(_key, _val)  printf ("%-30s%s\n", _key, _val)
#define print_key_s(_key)    printf ("%-30s", _key)
#define print_range_s(_key, _val1, _val2) \
        printf ("%-30s%s - %s\n", _key, _val1, _val2)

static void cpu_desc_summary (struct cpu_desc *cpudesc)
{
  printf ("-= CPU Characteristics =-\n");

  print_s("Architecture:", cpu_desc_get_architecture (cpudesc));

  int cpu_mode = cpu_desc_get_mode (cpudesc);
  if (cpu_mode)
    {
      char mbuf[32], *p = mbuf;
      if (cpu_mode & MODE_32BIT)
        {
          strcpy (p, "32-bit, ");
          p += 8;
        }
      if (cpu_mode & MODE_64BIT)
        {
          strcpy (p, "64-bit, ");
          p += 8;
        }
      *(p - 2) = '\0';

      print_s("CPU op-mode(s):", mbuf);
    }

#if !defined(WORDS_BIGENDIAN)
  print_s("Byte Order:", "Little Endian");
#else
  print_s("Byte Order:", "Big Endian");
#endif

  int cpu, ncpus = cpu_desc_get_ncpus (cpudesc);

  print_n("CPU(s):", ncpus);

  unsigned int nsockets, ncores, nthreads;
  get_cputopology_read (&nsockets, &ncores, &nthreads);

  print_n("Thread(s) per core:", nthreads);
  print_n("Core(s) per socket:", ncores);
  print_n("Socket(s):", nsockets);

  print_s("Vendor ID:", cpu_desc_get_vendor (cpudesc));
  print_s("CPU Family:", cpu_desc_get_family (cpudesc));
  print_s("Model:", cpu_desc_get_model (cpudesc));
  print_s("Model name:", cpu_desc_get_model_name (cpudesc));

  for (cpu = 0; cpu < ncpus; cpu++)
    {
      printf ("-CPU%d-\n", cpu);

      bool cpu_hot_pluggable = get_processor_is_hot_pluggable (cpu);
      int cpu_online = get_processor_is_online (cpu);

      print_s ("CPU is Hot Pluggable:", cpu_hot_pluggable ?
		 (cpu_online ? "yes (online)" : "yes (offline)") : "no");

      unsigned long latency = cpufreq_get_transition_latency (cpu);
      if (latency)
	print_s("Maximum Transition Latency:",
		cpufreq_duration_to_string (latency));

      unsigned long freq_kernel = cpufreq_get_freq_kernel (cpu);
      if (freq_kernel > 0)
	print_s("Current CPU Frequency:", cpufreq_freq_to_string (freq_kernel));

      struct cpufreq_available_frequencies *curr;
      curr = cpufreq_get_available_freqs (cpu);
      if (curr)
	{
	  print_key_s("Available CPU Frequencies:");
	  while (curr)
	    {
	      printf ("%s ",
		      cpufreq_freq_to_string
		      (cpufreq_get_available_freqs_value (curr)));
	      curr = cpufreq_get_available_freqs_next (curr);
	    }
	  printf ("\n");
	}

      unsigned long freq_min, freq_max;
      if (0 == cpufreq_get_hardware_limits (cpu, &freq_min, &freq_max))
	{
	  char *min_s = cpufreq_freq_to_string (freq_min),
	       *max_s = cpufreq_freq_to_string (freq_max);
	  print_range_s("Hardware Limits:", min_s, max_s);
	  free (min_s);
	  free (max_s);
	}

      char *freq_governor = cpufreq_get_governor (cpu);
      if (freq_governor)
	{
	  print_s ("CPU Freq Current Governor:", freq_governor);
	  free (freq_governor);
	}

      char *freq_governors = cpufreq_get_available_governors (cpu);
      if (freq_governors)
	{
	  print_s ("CPU Freq Available Governors:", freq_governors);
	  free (freq_governors);
	}

      char *freq_driver = cpufreq_get_driver (cpu);
      if (freq_driver)
	{
	  print_s ("CPU Freq Driver:", freq_driver);
	  free (freq_driver);
	}
    }

  char *cpu_virtflag = cpu_desc_get_virtualization_flag (cpudesc); 
  if (cpu_virtflag)
    print_s("Virtualization:", cpu_virtflag);
}

int
main (int argc, char **argv)
{
  int c, err;
  bool verbose, show_freq, cpu_model, per_cpu_stats;
  unsigned long i, len, delay, count;
  char *critical = NULL, *warning = NULL;
  char *p = NULL, *cpu_progname;
  nagstatus currstatus, status;
  thresholds *my_threshold = NULL;

  float cpu_perc;
  unsigned int sleep_time = 1,
               tog = 0;		/* toggle switch for cleaner code */
  struct cpu_desc *cpudesc = NULL;

  set_program_name (argv[0]);

  len = strlen (program_name);
  if (len > 6 && !strncmp (program_name, "check_", 6))
    p = (char *) program_name + 6;
  else
    plugin_error (STATE_UNKNOWN, 0,
		  "bug: the plugin does not have a standard name");

  if (!strncmp (p, "iowait", 6))	/* check_iowait --> cpu_iowait */
    {
      cpu_progname = xstrdup ("iowait");
      program_shorthelp =
        xstrdup ("This plugin checks I/O wait bottlenecks\n");
    }
  else				/* check_cpu --> cpu_user (the default) */
    {
      cpu_progname = xstrdup ("user");;
      program_shorthelp =
        xstrdup ("This plugin checks the CPU (user mode) utilization\n");
    }

  err = cpu_desc_new (&cpudesc);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  /* default values */
  verbose = show_freq = false;
  cpu_model = true;
  per_cpu_stats = false;

  while ((c = getopt_long (
		argc, argv, "c:w:vifmp"
		GETOPT_HELP_VERSION_STRING, longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 'i':
	  cpu_desc_read (cpudesc);
	  cpu_desc_summary (cpudesc);
	  return STATE_UNKNOWN;
	case 'f':
	  show_freq = true;
	  break;
	case 'm':
	  cpu_model = false;
	  break;
	case 'p':
	  per_cpu_stats = true;
	  break;
	case 'c':
	  critical = optarg;
	  break;
	case 'w':
	  warning = optarg;
	  break;
	case 'v':
	  verbose = true;
	  break;

	case_GETOPT_HELP_CHAR
	case_GETOPT_VERSION_CHAR

	}
    }

  delay = DELAY_DEFAULT, count = COUNT_DEFAULT;
  if (optind < argc)
    {
      delay = strtol_or_err (argv[optind++], "failed to parse argument");

      if (delay < 1)
	plugin_error (STATE_UNKNOWN, 0, "delay must be positive integer");
      else if (UINT_MAX < delay)
	plugin_error (STATE_UNKNOWN, 0, "too large delay value");

      sleep_time = delay;
    }

  if (optind < argc)
    count = strtol_or_err (argv[optind++], "failed to parse argument");

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  int ncpus = per_cpu_stats ? get_processor_number_total () + 1 : 1;

  jiff duser[ncpus], dsystem[ncpus], didle[ncpus],
       diowait[ncpus], dsteal[ncpus], ratio[ncpus];
  int debt[ncpus];			/* handle idle ticks running backwards */
  struct cpu_time cpuv[2][ncpus];
  jiff *cpu_value = strncmp (p, "iowait", 6) ? duser : diowait;

  cpu_stats_get_time (cpuv[0], ncpus);

  for (c = 0; c < ncpus; c++)
    {
      duser[c]   = cpuv[0][c].user + cpuv[0][c].nice;
      dsystem[c] = cpuv[0][c].system + cpuv[0][c].irq + cpuv[0][c].softirq;
      didle[c]   = cpuv[0][c].idle;
      diowait[c] = cpuv[0][c].iowait;
      dsteal[c]  = cpuv[0][c].steal;

      debt[c] = 0;
      ratio[c] = duser[c] + dsystem[c] + didle[c] + diowait[c] + dsteal[c];
      if (!ratio[c])
	ratio[c] = 1, didle[c] = 1;
    }

  for (i = 1; i < count; i++)
    {
      sleep (sleep_time);
      tog = !tog;
      cpu_stats_get_time (cpuv[tog], ncpus);

      for (c = 0; c < ncpus; c++)
	{
	  duser[c] =
	    cpuv[tog][c].user - cpuv[!tog][c].user +
	    cpuv[tog][c].nice - cpuv[!tog][c].nice;
	  dsystem[c] =
	    cpuv[tog][c].system  - cpuv[!tog][c].system +
	    cpuv[tog][c].irq     - cpuv[!tog][c].irq +
	    cpuv[tog][c].softirq - cpuv[!tog][c].softirq;
	  didle[c]   = cpuv[tog][c].idle   - cpuv[!tog][c].idle;
	  diowait[c] = cpuv[tog][c].iowait - cpuv[!tog][c].iowait;
	  dsteal[c]  = cpuv[tog][c].steal  - cpuv[!tog][c].steal;

	  /* idle can run backwards for a moment -- kernel "feature" */
	  if (debt[c])
	    {
	      didle[c] = (int) didle[c] + debt[c];
	      debt[c] = 0;
	    }
	  if ((int) didle[c] < 0)
	    {
	      debt[c] = (int) didle[c];
	      didle[c] = 0;
	    }

	  ratio[c] = duser[c] + dsystem[c] + didle[c] + diowait[c] + dsteal[c];
	  if (!ratio[c])
	    ratio[c] = 1, didle[c] = 1;

	  if (verbose)
	    printf
	     ("cpu%d_user=%.1f%%, cpu%d_system=%.1f%%, cpu%d_idle=%.1f%%, "
	      "cpu%d_iowait=%.1f%%, cpu%d_steal=%.1f%%\n"
	       , c, 100.0 * duser[c]   / ratio[c]
	       , c, 100.0 * dsystem[c] / ratio[c]
	       , c, 100.0 * didle[c]   / ratio[c]
	       , c, 100.0 * diowait[c] / ratio[c]
	       , c, 100.0 * dsteal[c]  / ratio[c]);
	}
    }

  for (c = 0, status = STATE_OK; c < ncpus; c++)
    {
      cpu_perc = (100.0 * (cpu_value[c]) / ratio[c]);
      currstatus = get_status (cpu_perc, my_threshold);
      if (currstatus > status)
	status = currstatus;
    }

  cpu_desc_read (cpudesc);
  char *cpu_model_str =
    cpu_model ?	xasprintf ("(%s) ",
			   cpu_desc_get_model_name (cpudesc)) : NULL;

  printf ("%s %s%s - cpu %s %.1f%% |"
	  , program_name_short, cpu_model ? cpu_model_str : ""
	  , state_text (status), cpu_progname, cpu_perc);
  for (c = 0; c < ncpus; c++)
    {
      const char *cpuname = cpuv[0][c].cpuname;
      unsigned long freq_min, freq_max, freq_kernel;
      if (cpuname)
        printf (" %s_user=%.1f%% %s_system=%.1f%% %s_idle=%.1f%%"
		" %s_iowait=%.1f%% %s_steal=%.1f%%"
		, cpuname, 100.0 * duser[c]   / ratio[c]
		, cpuname, 100.0 * dsystem[c] / ratio[c]
		, cpuname, 100.0 * didle[c]   / ratio[c]
		, cpuname, 100.0 * diowait[c] / ratio[c]
		, cpuname, 100.0 * dsteal[c]  / ratio[c]);
      if (show_freq && ((ncpus > 1) && (c > 1))
	  && (0 == cpufreq_get_hardware_limits (c - 1, &freq_min, &freq_max)))
	{
	  freq_kernel = cpufreq_get_freq_kernel (c - 1);
	  /* expected format for the Nagios performance data:
	   *   'label'=value[UOM];[warn];[crit];[min];[max]	*/
	  if (freq_kernel)
	    printf (" cpu%d_freq=%luHz;;;%lu;%lu",
		    c - 1, freq_kernel, freq_min, freq_max);
	}
    }
  putchar ('\n');

/*
  dbg ("sum (cpu_*) = %.1f%%\n", (100.0 * duser / ratio) +
       (100.0 * dsystem / ratio) + (100.0 * didle  / ratio) +
       (100.0 * diowait / ratio) + (100.0 * dsteal / ratio));
*/
  cpu_desc_unref (cpudesc);
  return status;
}
