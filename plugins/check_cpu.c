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
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpuinfo.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "thresholds.h"

/* by default one iteration with 1sec delay */
#define DELAY_DEFAULT	1
#define COUNT_DEFAULT	2

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static const char *program_shorthelp = NULL;

static struct option const longopts[] = {
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
  fprintf (out, "  %s [-v] [-w PERC] [-c PERC] [delay [count]]\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -w, --warning PERCENT   warning threshold\n", out);
  fputs ("  -c, --critical PERCENT   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
         "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fprintf (out, "  delay is the delay between updates in seconds "
           "(default: %dsec)\n", DELAY_DEFAULT);
  fprintf (out, "  count is the number of updates "
           "(default: %d)\n", COUNT_DEFAULT);
  fputs ("\t1 means the percentages of total CPU time from boottime.\n", out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s -w 10%% -c 20%% 1 2\n", program_name);

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

/*
 * same as strtol(3) but exit on failure instead of returning crap
 */
long
strtol_or_err (const char *str, const char *errmesg)
{
  long num;
  char *end = NULL;

  if (str != NULL && *str != '\0')
    {
      errno = 0;
      num = strtol (str, &end, 10);
      if (errno == 0 && str != end && end != NULL && *end == '\0')
	return num;
    }

  plugin_error (STATE_UNKNOWN, errno, "%s: '%s'", errmesg, str);
  return 0;
}


int
main (int argc, char **argv)
{
  int c;
  bool verbose = false;
  unsigned long i, len, delay, count;
  enum nagios_status status = STATE_OK;
  char *critical = NULL, *warning = NULL;
  char *p, *cpu_progname, *CPU_PROGNAME;
  thresholds *my_threshold = NULL;

  struct proc_cpu cpu[2];
  jiff duser, dsystem, didle, diowait, dsteal, div, divo2;
  jiff *cpu_value;
  unsigned int cpu_perc, sleep_time = 1,
               tog = 0;		/* toggle switch for cleaner code */
  int debt = 0;			/* handle idle ticks running backwards */

  set_program_name (argv[0]);

  len = strlen (program_name);
  if (len > 6 && !strncmp (program_name, "check_", 6))
    p = (char *) program_name + 6;

  if (!strncmp (p, "iowait", 6))	/* check_iowait --> cpu_iowait */
    {
      cpu_progname = strdup ("iowait");
      cpu_value = &diowait;
      program_shorthelp =
	strdup ("This plugin checks I/O wait bottlenecks\n");
    }
  else				/* check_cpu --> cpu_user (the default) */
    {
      cpu_progname = strdup ("user");;
      cpu_value = &duser;
      program_shorthelp =
	strdup ("This plugin checks the CPU (user mode) utilization\n");
    }
  CPU_PROGNAME = strdup (cpu_progname);
  for (i = 0; i < strlen (cpu_progname); i++)
    CPU_PROGNAME[i] = toupper (CPU_PROGNAME[i]);

  while ((c = getopt_long (argc, argv,
			   "c:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
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

  proc_cpu_read (&cpu[0]);

  duser   = cpu[0].user + cpu[0].nice;
  dsystem = cpu[0].system + cpu[0].irq + cpu[0].softirq;
  didle   = cpu[0].idle;
  diowait = cpu[0].iowait;
  dsteal  = cpu[0].steal;

  div = duser + dsystem + didle + diowait + dsteal;
  if (!div)
    div = 1, didle = 1;
  divo2 = div / 2UL;

  for (i = 1; i < count; i++)
    {
      sleep (sleep_time);

      tog = !tog;
      proc_cpu_read (&cpu[tog]);

      duser = cpu[tog].user - cpu[!tog].user +
	cpu[tog].nice - cpu[!tog].nice;
      dsystem =
	cpu[tog].system  - cpu[!tog].system +
	cpu[tog].irq     - cpu[!tog].irq +
	cpu[tog].softirq - cpu[!tog].softirq;
      didle   = cpu[tog].idle   - cpu[!tog].idle;
      diowait = cpu[tog].iowait - cpu[!tog].iowait;
      dsteal  = cpu[tog].steal  - cpu[!tog].steal;

      /* idle can run backwards for a moment -- kernel "feature" */
      if (debt)
	{
	  didle = (int) didle + debt;
	  debt = 0;
	}
      if ((int) didle < 0)
	{
	  debt = (int) didle;
	  didle = 0;
	}

      div = duser + dsystem + didle + diowait + dsteal;
      if (!div)
	div = 1, didle = 1;
      divo2 = div / 2UL;

      if (verbose)
        printf
          ("cpu_user=%u%%, cpu_system=%u%%, cpu_idle=%u%%, cpu_iowait=%u%%, "
           "cpu_steal=%u%%\n"
           , (unsigned) ((100 * duser   + divo2) / div)
           , (unsigned) ((100 * dsystem + divo2) / div)
           , (unsigned) ((100 * didle   + divo2) / div)
           , (unsigned) ((100 * diowait + divo2) / div)
           , (unsigned) ((100 * dsteal  + divo2) / div));
    }

  cpu_perc = (unsigned) ((100 * (*cpu_value) + divo2) / div);
  status = get_status (cpu_perc, my_threshold);

  printf
    ("%s %s - cpu %s %u%% | "
     "cpu_user=%u%%, cpu_system=%u%%, cpu_idle=%u%%, cpu_iowait=%u%%, "
     "cpu_steal=%u%%\n"
     , CPU_PROGNAME, state_text (status), cpu_progname, cpu_perc
     , (unsigned) ((100 * duser   + divo2) / div)
     , (unsigned) ((100 * dsystem + divo2) / div)
     , (unsigned) ((100 * didle   + divo2) / div)
     , (unsigned) ((100 * diowait + divo2) / div)
     , (unsigned) ((100 * dsteal  + divo2) / div));

  return status;
}
