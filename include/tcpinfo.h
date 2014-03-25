/* Library for getting TCP network and socket informations

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _TCPINFO_H_
#define _TCPINFO_H_

#include <stdbool.h>

#define TCP_UNSET  0
#define TCP_v4     1
#define TCP_v6     2

#ifdef __cplusplus
extern "C"
{
#endif

  struct proc_tcptable;

  /* Allocates space for a new tcptable object.
   * Returns 0 if all went ok. Errors are returned as negative values.  */
  int proc_tcptable_new (struct proc_tcptable **tcptable);

  /* Fill the proc_tcptable structure pointed with the values found in the
   * proc filesystem.  */
  void proc_tcptable_read (struct proc_tcptable *tcptable, int flags);

  /* Drop a reference of the tcptable library context. If the refcount of
   * reaches zero, the resources of the context will be released.  */
  struct proc_cputable *proc_tcptable_unref (struct proc_tcptable *tcptable);

  /* Accessing the values from proc_tcptable */

  unsigned long proc_tcp_get_tcp_established (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_syn_sent (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_syn_recv (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_fin_wait1 (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_fin_wait2 (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_time_wait (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_close (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_close_wait (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_last_ack (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_listen (struct proc_tcptable *tcptable);
  unsigned long proc_tcp_get_tcp_closing (struct proc_tcptable *tcptable);

#ifdef __cplusplus
}
#endif

#endif				/* _TCPINFO_H_ */
