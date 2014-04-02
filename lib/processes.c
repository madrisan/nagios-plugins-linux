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

#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "messages.h"
#include "xalloc.h"

#define PROC_ROOT  "/proc"
#define MAX_LINE   1000

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
  long nbr;		/* number of the occurrences */
  char *username;
  struct procs_list_node *next;
};

long
procs_list_node_get_nbr (struct procs_list_node *node)
{
  return node->nbr;
}

char *
procs_list_node_get_username (struct procs_list_node *node)
{
  return node->username;
}

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
procs_list_node_add (uid_t uid, struct procs_list_node *plist)
{
  struct procs_list_node *p = plist;

  //printf ("DEBUG: *** procs_list_node_add (uid %d)\n", uid);
  while (p != p->next)
    {
      p = p->next;
      //printf ("DEBUG: iterate (uid = %d)\n", p->uid);
      if (p->uid == uid)
	{
	  p->nbr++;
	  //printf ("DEBUG: found uid %d (now #%ld)\n", uid, p->nbr);
	  plist->nbr++;
	  return p;
	}
    }

  plist->nbr++;

  struct procs_list_node *new = xmalloc (sizeof (struct procs_list_node));
  //printf ("DEBUG: new uid --> append uid %d #1\n", uid);
  new->uid = uid;
  new->nbr = 1;
  new->username = xstrdup (uid_to_username (uid));
  new->next = new;
  p->next = new;

  return new;
}

/* Parses /proc/PID/status files to produce a list of the running processes */

struct procs_list_node *
procs_list_getall (bool verbose)
{
  DIR *dirp;
  FILE *fp;
  size_t len = 0;
  ssize_t chread;
  struct dirent *dp;

  bool gotname, gotuid;
  char path[PATH_MAX];
  char *line, cmd[MAX_LINE];
  char *p;
  uid_t uid = -1;
  struct procs_list_node *plist = NULL;

  if ((dirp = opendir (PROC_ROOT)) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "Cannot open %s", PROC_ROOT);

  procs_list_node_init (&plist);

  /* Scan entries under /proc directory */
  for (;;)
    {
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

      gotname = gotuid = false;
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

	  /* The "Uid:" line contains the real, effective, saved set-,
	     and file-system user IDs */
	  if (strncmp (line, "Uid:", 4) == 0)
	    {
	      uid = strtol (line + 4, NULL, 10);
	      procs_list_node_add (uid, plist);
	      gotuid = true;
	    }

	  if (gotname && gotuid)
	    break;
	}

      fclose (fp);

      if (gotname && gotuid && verbose)
	printf ("%12s:  pid: %5s  cmd: %s", uid_to_username (uid),
		dp->d_name, cmd);
    }

  free (line);
  return plist;
}
