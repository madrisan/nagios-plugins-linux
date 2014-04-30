/*
 *  debug.h - Debugging utilities
 *
 *  cpuinfo (C) 2006-2007 Gwenole Beauchesne
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DEBUG_H
#define DEBUG_H

extern void cpuinfo_set_debug_file(FILE *debug_file);
extern void cpuinfo_dprintf(const char *format, ...) attribute_hidden;

#if DEBUG
#define bug cpuinfo_dprintf
#define D(x) x
#else
#define D(x) ;
#endif

#endif /* DEBUG_H */
