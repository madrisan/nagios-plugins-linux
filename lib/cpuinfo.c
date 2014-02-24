#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cpuinfo.h"
#include "error.h"

#define BUFFSIZE 0x1000
static char buff[BUFFSIZE];

#define PROC_STAT   "/proc/stat"

void
cpuinfo (jiff * restrict cuser, jiff * restrict cnice,
	 jiff * restrict csystem, jiff * restrict cidle,
	 jiff * restrict ciowait, jiff * restrict cirq,
	 jiff * restrict csoftirq, jiff * restrict csteal,
	 jiff * restrict cguest, jiff * restrict cguestn)
{
  static int fd;
  const char *b;

  if (fd)
    lseek (fd, 0L, SEEK_SET);
  else
    {
      fd = open (PROC_STAT, O_RDONLY, 0);
      if (fd == -1)
	plugin_error (STATE_UNKNOWN, errno, "Error opening %s", PROC_STAT);
    }

  read (fd, buff, BUFFSIZE - 1);

  *ciowait = 0;			/* not separated out until the 2.5.41 kernel */
  *cirq = 0;			/* not separated out until the 2.6.0-test4 */
  *csoftirq = 0;		/* not separated out until the 2.6.0-test4 */
  *csteal = 0;			/* not separated out until the 2.6.11 */
  *cguest = 0;			/* since Linux 2.6.24 */
  *cguestn = 0;			/* since Linux 2.6.33 */

  b = strstr (buff, "cpu ");
  if (b)
    sscanf (b, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", cuser, cnice,
	    csystem, cidle, ciowait, cirq, csoftirq, csteal, cguest, cguestn);
  else
    plugin_error (STATE_UNKNOWN, errno, "Error reading %s", PROC_STAT);
}
