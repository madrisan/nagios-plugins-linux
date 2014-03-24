/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking TCP network and socket informations
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

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "messages.h"

#define PROC_TCPINFO  "/proc/net/tcp"
/*#define PROC_TCP6INFO  "/proc/net/tcp6"*/

typedef enum tcp_status
{
  TCP_ESTABLISHED = 1,
  TCP_SYN_SENT,
  TCP_SYN_RECV,
  TCP_FIN_WAIT1,
  TCP_FIN_WAIT2,
  TCP_TIME_WAIT,
  TCP_CLOSE,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK,
  TCP_LISTEN,
  TCP_CLOSING			/* now a valid state */
} tcp_status;

typedef struct proc_tcptable_data
{
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
} proc_tcptable_data_t;

typedef struct proc_tcptable
{
  int refcount;
  struct proc_tcptable_data *data;
} proc_tcptable_t;

/* Allocates space for a new tcptable object.
 * Returns 0 if all went ok. Errors are returned as negative values. */

int
proc_tcptable_new (struct proc_tcptable **tcptable)
{
  struct proc_tcptable *t;

  t = calloc (1, sizeof (struct proc_tcptable));
  if (!t)
    return -ENOMEM;

  t->refcount = 1;
  t->data = calloc (1, sizeof (struct proc_tcptable_data));
  if (!t->data)
    {
      free (t);
      return -ENOMEM;
    }

  *tcptable = t;
  return 0;
}

/* Fill the proc_tcptable structure pointed will the values found in the
 * proc filesystem. */

void
proc_tcptable_read (struct proc_tcptable *tcptable)
{
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  char local_addr[128], rem_addr[128];
  int linenum = 1;
  int slot, num, local_port, rem_port, state, timer_run, uid, timeout;
  unsigned long rxq, txq, time_len, retr, inode;

  if (tcptable == NULL)
    return;

  struct proc_tcptable_data *data = tcptable->data;

  if ((fp = fopen (PROC_TCPINFO, "r")) < 0)
    plugin_error (STATE_UNKNOWN, errno, "Error: /proc must be mounted");

  /* sl local_addr:local_port rem_addr:rem_port st ... */
  while ((nread = getline (&line, &len, fp)) != -1)
    {
      num =
	sscanf (line,
		"%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X "
		"%lX:%lX %X:%lX %lX %d %d %lu %*s\n",
		&slot, local_addr, &local_port, rem_addr, &rem_port, &state,
		&txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout,
		&inode);

      if ((linenum > 1) /* Skip the heading line */
	   && (num < 11))
	fprintf (stderr, "warning, got bogus tcp line.\n%s\n", line);

      switch (state)
	{
	case TCP_ESTABLISHED:
	  data -> tcp_established++;
	  break;
	case TCP_SYN_SENT:
	  data -> tcp_syn_sent++;
	  break;
	case TCP_SYN_RECV:
	  data -> tcp_syn_recv++;
	  break;
	case TCP_FIN_WAIT1:
	  data -> tcp_fin_wait1++;
	  break;
	case TCP_FIN_WAIT2:
	  data -> tcp_fin_wait2++;
	  break;
	case TCP_TIME_WAIT:
	  data -> tcp_time_wait++;
	  break;
	case TCP_CLOSE:
	  data -> tcp_close++;
	  break;
	case TCP_CLOSE_WAIT:
	  data -> tcp_close_wait++;
	  break;
	case TCP_LAST_ACK:
	  data -> tcp_last_ack++;
	  break;
	case TCP_LISTEN:
	  data -> tcp_listen++;
	  break;
	case TCP_CLOSING:
	  data -> tcp_closing++;
	  break;
	}

      linenum++;
    }

  free (line);
}

struct proc_tcptable *
proc_tcptable_unref (struct proc_tcptable *tcptable)
{
  if (tcptable == NULL)
    return NULL;

  tcptable->refcount--;
  if (tcptable->refcount > 0)
    return tcptable;

  free (tcptable->data);
  free (tcptable);
  return NULL;
}

#define proc_tcp_get(arg) \
unsigned long proc_tcp_get_tcp_ ## arg (struct proc_tcptable *p) \
  { return (p == NULL) ? 0 : p->data->tcp_ ## arg; }

proc_tcp_get (established)
proc_tcp_get (syn_sent)
proc_tcp_get (syn_recv)
proc_tcp_get (fin_wait1)
proc_tcp_get (fin_wait2)
proc_tcp_get (time_wait)
proc_tcp_get (close)
proc_tcp_get (close_wait)
proc_tcp_get (last_ack)
proc_tcp_get (listen)
proc_tcp_get (closing)
