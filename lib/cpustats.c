/*
 * License: GPLv3+
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A library for checking the CPU utilization
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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpustats.h"
#include "messages.h"
#include "procparser.h"
#include "xalloc.h"

#define BUFFSIZE 0x1000
static char buff[BUFFSIZE];

#define PATH_PROC_STAT		"/proc/stat"
#define PATH_PROC_CPUINFO	"/proc/cpuinfo"

/* Fill the cpu_desc structure pointed with the values found in the 
 * proc filesystem */

void
cpu_desc_read (struct cpu_desc *cpudesc)
{
  char *line = NULL, *buf;
  FILE *fp;
  size_t len = 0;
  ssize_t chread;
  struct utsname utsbuf;

  if ((fp = fopen (PATH_PROC_CPUINFO,  "r")) == NULL)
    plugin_error (STATE_UNKNOWN, errno, "error opening %s", PATH_PROC_CPUINFO);

  if (uname (&utsbuf) == -1)
    plugin_error (STATE_UNKNOWN, errno, "uname() failed");
  cpudesc->arch = xstrdup (utsbuf.machine);

  cpudesc->ncpus = get_processor_number_total ();

  cpudesc->mode = 0;
#if defined(__alpha__) || defined(__ia64__)
  cpudesc->mode |= MODE_64BIT;		/* 64bit platforms only */
#endif
  /* platforms with 64bit flag in /proc/cpuinfo, define
   * 32bit default here */
#if defined(__i386__) || defined(__x86_64__) || \
    defined(__s390x__) || defined(__s390__) || defined(__sparc_v9__)
  cpudesc->mode |= MODE_32BIT;
#endif

  while ((chread = getline (&line, &len, fp)) != -1)
    {
      if (linelookup (line, "vendor", &cpudesc->vendor));
      else if (linelookup (line, "vendor_id", &cpudesc->vendor));
      else if (linelookup (line, "vendor_id", &cpudesc->vendor));
      else if (linelookup (line, "family", &cpudesc->family));
      else if (linelookup (line, "cpu family", &cpudesc->family));
      else if (linelookup (line, "model", &cpudesc->model));
      else if (linelookup (line, "model name", &cpudesc->modelname)) ;
      else if (linelookup (line, "cpu MHz", &cpudesc->mhz)) ;
      else if (linelookup (line, "flags", &cpudesc->flags)) ;	/* x86 */
      else
	continue;
    }

  if (cpudesc->flags)
    {
      size_t buflen = sizeof (cpudesc->flags) + 2;
      buf = xmalloc (buflen);

      snprintf (buf, buflen, " %s ", cpudesc->flags);
      if (strstr (buf, " svm "))
        cpudesc->virtflag = xstrdup ("svm");
      else if (strstr (buf, " vmx "))
        cpudesc->virtflag = xstrdup ("vmx");
      if (strstr (buf, " lm "))
        cpudesc->mode |= MODE_32BIT | MODE_64BIT;  /* x86_64 */
      if (strstr (buf, " zarch "))
        cpudesc->mode |= MODE_32BIT | MODE_64BIT;  /* s390x */
      if (strstr (buf, " sun4v ") || strstr (buf, " sun4u "))
        cpudesc->mode |= MODE_32BIT | MODE_64BIT;  /* sparc64 */
  
      free (buf);
    }

  free (line);
}

/* Fill the proc_cpu structure pointed with the values found in the 
 * proc filesystem */

void
cpu_stats_read (struct cpu_stats *cpustats)
{
  static int fd;
  const char *b;

  if (fd)
    lseek (fd, 0L, SEEK_SET);
  else
    {
      fd = open (PATH_PROC_STAT, O_RDONLY, 0);
      if (fd == -1)
	plugin_error (STATE_UNKNOWN, errno, "Error opening %s", PATH_PROC_STAT);
    }

  read (fd, buff, BUFFSIZE - 1);

  cpustats->iowait = 0;	/* not separated out until the 2.5.41 kernel */
  cpustats->irq = 0;	/* not separated out until the 2.6.0-test4 */
  cpustats->softirq = 0;	/* not separated out until the 2.6.0-test4 */
  cpustats->steal = 0;	/* not separated out until the 2.6.11 */
  cpustats->guest = 0;	/* since Linux 2.6.24 */
  cpustats->guestn = 0;	/* since Linux 2.6.33 */

  b = strstr (buff, "cpu ");
  if (b)
    sscanf (b, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
	    &cpustats->user, &cpustats->nice, &cpustats->system,
	    &cpustats->idle, &cpustats->iowait, &cpustats->irq,
	    &cpustats->softirq, &cpustats->steal, &cpustats->guest,
	    &cpustats->guestn);
  else
    plugin_error (STATE_UNKNOWN, errno, "Error reading %s", PATH_PROC_STAT);
}
