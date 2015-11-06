/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin that displays TCP network and socket informations.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "messages.h"
#include "progname.h"
#include "progversion.h"
#include "tcpinfo.h"
#include "thresholds.h"

static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">\n";

static struct option const longopts[] = {
  {(char *) "tcp", no_argument, NULL, 't'},
  {(char *) "tcp6", no_argument, NULL, '6'},
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
  fputs ("This plugin displays TCP network and socket informations.\n", out);
  fputs (program_copyright, out);
  fputs (USAGE_HEADER, out);
  fprintf (out, "  %s [--tcp] [--tcp6] [-w COUNTER] [-c COUNTER]\n",
	   program_name);
  fputs (USAGE_OPTIONS, out);
  fputs ("  -t, --tcp       display the statistics for the TCP protocol "
	 "(the default)\n", out);
  fputs ("  -6, --tcp6      display the statistics for the TCPv6 protocol\n",
	 out);
  fputs ("  -w, --warning COUNTER   warning threshold\n", out);
  fputs ("  -c, --critical COUNTER   critical threshold\n", out);
  fputs ("  -v, --verbose   show details for command-line debugging "
	 "(Nagios may truncate output)\n", out);
  fputs (USAGE_HELP, out);
  fputs (USAGE_VERSION, out);
  fputs (USAGE_EXAMPLES, out);
  fprintf (out, "  %s --tcp -w 1000 -c 1500    # TCPv4 only (the default)\n",
	   program_name);
  fprintf (out, "  %s --tcp --tcp6 -w 1500 -c 2000   # TCPv4 and TCPv6\n",
	   program_name);
  fprintf (out, "  %s --tcp6 -w 1500 -c 2000   # TCPv6 only\n", program_name);

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

int
main (int argc, char **argv)
{
  int c, err;
  bool verbose = false;
  unsigned int tcp_flags = TCP_UNSET;
  char *critical = NULL, *warning = NULL;
  nagstatus status = STATE_OK;
  thresholds *my_threshold = NULL;

  struct proc_tcptable *tcptable = NULL;
  unsigned long tcp_established;
  unsigned long tcp_syn_sent;
  unsigned long tcp_syn_recv;
  unsigned long tcp_fin_wait1;
  unsigned long tcp_fin_wait2;
  unsigned long tcp_time_wait;
  unsigned long tcp_close;
  unsigned long tcp_close_wait;
  unsigned long tcp_last_ack;
  unsigned long tcp_listen;
  unsigned long tcp_closing;

  set_program_name (argv[0]);

  while ((c = getopt_long (argc, argv,
			   "t6c:w:v" GETOPT_HELP_VERSION_STRING,
			   longopts, NULL)) != -1)
    {
      switch (c)
	{
	default:
	  usage (stderr);
	case 't':
	  tcp_flags |= TCP_v4;
	  break;
	case '6':
	  tcp_flags |= TCP_v6;
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

  if (tcp_flags == TCP_UNSET)
    tcp_flags = TCP_v4;

  if (verbose)
    tcp_flags |= TCP_VERBOSE;

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  err = proc_tcptable_new (&tcptable);
  if (err < 0)
    plugin_error (STATE_UNKNOWN, err, "memory exhausted");

  proc_tcptable_read (tcptable, tcp_flags);

  tcp_established = proc_tcp_get_tcp_established (tcptable);
  tcp_syn_sent    = proc_tcp_get_tcp_syn_sent (tcptable);
  tcp_syn_recv    = proc_tcp_get_tcp_syn_recv (tcptable);
  tcp_fin_wait1   = proc_tcp_get_tcp_fin_wait1 (tcptable);
  tcp_fin_wait2   = proc_tcp_get_tcp_fin_wait2 (tcptable);
  tcp_time_wait   = proc_tcp_get_tcp_time_wait (tcptable);
  tcp_close       = proc_tcp_get_tcp_close (tcptable);
  tcp_close_wait  = proc_tcp_get_tcp_close_wait (tcptable);
  tcp_last_ack    = proc_tcp_get_tcp_last_ack (tcptable);
  tcp_listen      = proc_tcp_get_tcp_listen (tcptable);
  tcp_closing     = proc_tcp_get_tcp_closing (tcptable);

  proc_tcptable_unref (tcptable);

  status = get_status (tcp_established, my_threshold);
  free (my_threshold);

  printf ("%s %s - %lu tcp established | "
	  "tcp_established=%lu, tcp_syn_sent=%lu, tcp_syn_recv=%lu, "
	  "tcp_fin_wait1=%lu, tcp_fin_wait2=%lu, tcp_time_wait=%lu, "
	  "tcp_close=%lu, tcp_close_wait=%lu, tcp_last_ack=%lu, "
	  "tcp_listen=%lu, tcp_closing=%lu\n",
	  program_name_short, state_text (status), tcp_established,
	  tcp_established, tcp_syn_sent, tcp_syn_recv,
	  tcp_fin_wait1, tcp_fin_wait2, tcp_time_wait,
	  tcp_close, tcp_close_wait, tcp_last_ack,
	  tcp_listen, tcp_closing);

  return status;
}
