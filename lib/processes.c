// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for getting informations on the running processes
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <sys/resource.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "messages.h"
#include "system.h"
#include "xalloc.h"

#define PROC_ROOT	"/proc"
#ifndef RLIM_INFINITY
# define RLIM_INFINITY	65535
#endif

#define NBPROCS_NONE	0x00
#define NBPROCS_VERBOSE	0x01
#define NBPROCS_THREADS	0x02

/* Return name corresponding to 'uid', or NULL on error */

char *
uid_to_username (uid_t uid)
{
  struct passwd *pwd = getpwuid (uid);
  return (pwd == NULL) ? "<no-user>" : pwd->pw_name;
}

struct procs_list_node
{
  uid_t uid;
  char *username;
  long nbr;		/* number of the occurrences */
#ifdef RLIMIT_NPROC
  rlim_t rlimit_nproc_soft;	/* ulimit -Su */
  rlim_t rlimit_nproc_hard;	/* ulimit -Hu */
#endif
  struct procs_list_node *next;
};

char *
procs_list_node_get_username (struct procs_list_node *node)
{
  return node->username;
}

long
procs_list_node_get_nbr (struct procs_list_node *node)
{
  return node->nbr;
}

#ifdef RLIMIT_NPROC
unsigned long
procs_list_node_get_rlimit_nproc_soft (struct procs_list_node *node)
{
  return node->rlimit_nproc_soft;
}

unsigned long
procs_list_node_get_rlimit_nproc_hard (struct procs_list_node *node)
{
  return node->rlimit_nproc_hard;
}
#endif

struct procs_list_node *
procs_list_node_get_next (struct procs_list_node *node)
{
  return node->next;
}

long procs_list_node_get_total_procs_nbr (struct procs_list_node *list)
{
  return list->nbr;
}

void
procs_list_node_init (struct procs_list_node **list)
{
  struct procs_list_node *new = xmalloc (sizeof (struct procs_list_node));

  /* The list's head points to itself if empty */
  new->uid = -1;
  new->nbr = 0;		/* will hold the total number of processes */
  new->next = new;
  *list = new;
}

struct procs_list_node *
procs_list_node_add (uid_t uid, unsigned long inc,
		     struct procs_list_node *plist)
{
  struct procs_list_node *p = plist;

  dbg ("procs_list_node_add (uid %u, #threads %lu)\n", uid,
       plist->nbr + inc);
  while (p != p->next)
    {
      p = p->next;
      dbg (" - iteration: uid = %u\n", p->uid);
      if (p->uid == uid)
	{
	  p->nbr += inc;
	  plist->nbr += inc;
	  dbg (" - found uid %u: now #%ld\n", uid, p->nbr);
	  return p;
	}
    }

  struct procs_list_node *new = xmalloc (sizeof (struct procs_list_node));
  dbg ("new uid --> append uid %u #1\n", uid);
  new->uid = uid;
  new->nbr = inc;
  plist->nbr += inc;
  new->username = xstrdup (uid_to_username (uid));
#ifdef RLIMIT_NPROC
  struct rlimit rlim;
  int res;

  if ((res = getrlimit (RLIMIT_NPROC, &rlim)) < 0)
    new->rlimit_nproc_soft = new->rlimit_nproc_hard = RLIM_INFINITY;
  else
    {
#ifdef LIBC_MUSL
      dbg (" - with rlimits: %llu %llu\n", rlim.rlim_cur, rlim.rlim_max);
#else
      dbg (" - with rlimits: %lu %lu\n", rlim.rlim_cur, rlim.rlim_max);
#endif
      new->rlimit_nproc_soft = rlim.rlim_cur;
      new->rlimit_nproc_hard = rlim.rlim_max;
    }
#endif
  new->next = new;
  p->next = new;

  return new;
}

/* Parses /proc/PID/status files to produce a list of the running processes */

struct procs_list_node *
procs_list_getall (unsigned int flags)
{
  DIR *dirp;
  FILE *fp;
  size_t len = 0;
  bool gotname, gotuid, gotthreads,
       threads = (flags & NBPROCS_THREADS) ? true : false,
       verbose = (flags & NBPROCS_VERBOSE) ? true : false;
  char *line = NULL, *p, path[PATH_MAX];
  uid_t uid = -1;
  unsigned long threads_nbr;
  struct procs_list_node *plist = NULL;

#define MAX_LINE   128
  char *cmd = xmalloc (MAX_LINE);

  if ((dirp = opendir (PROC_ROOT)) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "Cannot open %s", PROC_ROOT);

  procs_list_node_init (&plist);

  /* Scan entries under /proc directory */
  for (;;)
    {
      struct dirent *dp;
      ssize_t chread;
      errno = 0;

      if ((dp = readdir (dirp)) == NULL)
	{
	  if (errno != 0)
	    plugin_error (STATE_UNKNOWN, errno, "readdir() failure");
	  else
	    break;		/* end-of-directory */
	}

      /* Since we are looking for /proc/PID directories, skip entries
         that are not directories, or don't begin with a digit. */

      if (dp->d_type != DT_DIR || !isdigit ((unsigned char) dp->d_name[0]))
	continue;

      snprintf (path, PATH_MAX, "/proc/%s/status", dp->d_name);

      if ((fp = fopen (path, "r")) == NULL)
	continue;		/* Ignore errors: fopen() might fail if
				   process has just terminated */

      gotname = gotuid = gotthreads = false;
      threads_nbr = 0;
      while ((chread = getline (&line, &len, fp)) != -1)
	{
	  /* The "Name:" line contains the name of the command that
	     this process is running */
	  if (strncmp (line, "Name:", 5) == 0)
	    {
	      for (p = line + 5; *p != '\0' && isspace ((unsigned char) *p);)
		p++;
	      strncpy (cmd, p, MAX_LINE - 1);
	      cmd[MAX_LINE - 1] = '\0';	/* Ensure null-terminated */
	      gotname = true;
	    }

	  /* The "Threads:" line contains the number of threads in process
	     containing this thread: check with 'ps -e -o pid,user,nlwp' */
	  if (strncmp (line, "Threads:", 8) == 0)
	    {
	      threads_nbr = strtol (line + 8, NULL, 10);
	      gotthreads = true;
	    }

	  /* The "Uid:" line contains the real, effective, saved set-,
	     and file-system user IDs */
	  if (strncmp (line, "Uid:", 4) == 0)
	    {
	      uid = strtol (line + 4, NULL, 10);
	      gotuid = true;
	    }

	  if (gotname && gotuid && gotthreads)
	    {
	      procs_list_node_add (uid, (threads ? threads_nbr : 1), plist);
	      break;
	    }
	}

      fclose (fp);

      if (gotname && gotuid && gotthreads && verbose)
	printf ("%12s:  pid: %5s  threads: %5lu, cmd: %s",
		uid_to_username (uid), dp->d_name, threads_nbr, cmd);
    }

  closedir (dirp);
  free (cmd);
  free (line);

  return plist;
}
