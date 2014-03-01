# Nagios Plugins for Linux

This package contains several nagios plugins for monitoring Linux boxes.

* check_cpu
* check_ifmountfs
* check_iowait
* check_load
* check_memory
* check_multipath
* check_readonlyfs
* check_swap
* check_uptime
* check_user


## check_cpu

This Nagios plugin checks the CPU (user mode) utilization.

Usage

	check_cpu [-v] [-w PERC] [-c PERC] [delay [count]]
	check_cpu --help

Where

* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* delay is the delay between updates in seconds (default: 1sec)
* count is the number of updates (default: 2)

Examples

	check_cpu -w 85% -c 95%
	# count = 1 means the percentages of total CPU time from boottime
	check_cpu -w 85% -c 95% 1 1


## check_ifmountfs

This Nagios plugin checks whether the given filesystems are mounted.

Usage

	check_ifmountfs [FILESYSTEM]...
	check_ifmountfs --help

Examples

	check_ifmountfs /mnt/nfs-data /mnt/cdrom


## check_iowait

This Nagios plugin checks for I/O wait bottlenecks.

Usage

	check_iowait [-v] [-w PERC] [-c PERC] [delay [count]]
	check_iowait --help

Where

* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* delay is the delay between updates in seconds (default: 1sec)
* count is the number of updates (default: 2)

Examples

	check_iowait -w 10% -c 20%
	# count = 1 means the percentages of total CPU time from boottime
	check_iowait -w 10% -c 20% 1 1


## check_load

This Nagios plugin tests the current system load average.

Usage

	check_load [-r] [--load1=w,c] [--load5=w,c] [--load15=w,c]
	check_load --help

Where

* -r, --percpu: divide the load averages by the number of CPUs
* 1, --load1=WLOAD1,CLOAD1: warning and critial thresholds for load1
* 5, --load5=WLOAD5,CLOAD5. warning and critical thresholds for load5
* L, --load15=WLOAD15,CLOAD15: warning and critical thresholds for load15

Examples

	check_load -r --load1=2,3 --load15=1.5,2.5


## check_memory, check_swap

These two nagios plugins respectivery check for memory and swap usage.

Usage

	check_memory [-C] [-b,-k,-m,-g] [-w PERC] [-c PERC]
	check_swap [-b,-k,-m,-g] [-w PERC] [-c PERC]
	
	check_memory --help
	check_swap --help

Where

* -C, --caches: count buffers and cached memory as free memory
* -b,-k,-m,-g: show output in bytes, KB (the default), MB, or GB
* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold

Examples

	check_memory -C -m -w 80% -c 90%
	OK: 79.22% (810964 kB) used | mem_total=999MB, mem_used=791MB, mem_free=207MB, mem_shared=0MB, mem_buffers=1MB, mem_cached=190MB, mem_pageins=33803MB, mem_pageouts=18608MB
	  # mem_total    : Total usable physical RAM
	  # mem_used     : Total amount of physical RAM used by the system
	  # mem_free     : Amount of RAM that is currently unused
	  # mem_shared   : Now always zero; not calculated
	  # mem_buffers  : Amount of physical RAM used for file buffers
	  # mem_cached   : In-memory cache for files read from the disk (the page cache)
	  # mem_pageins
	  # mem_pageouts : The number of memory pages the system has written in and out to disk

	check_swap -w 40% -c 60% -m
	WARNING: 42.70% (895104 kB) used | swap_total=2096444kB, swap_used=895104kB, swap_free=1201340kB, swap_cached=117024kB, swap_pageins=1593302kB, swap_pageouts=1281649kB
	  # swap_total   : Total amount of swap space available
	  # swap_used    : Total amount of swap used by the system
	  # swap_free    : Amount of swap space that is currently unused
	  # swap_cached  : The amount of swap used as cache memory
	  # swap_pageins 
	  # swap_pageouts: The number of swap pages the system has brought in and out


## check_multipath

This Nagios plugin checks the multipath topology status.

Usage

	check_multipath [-v]
	check_multipath --help

Where 

* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -h, --help: display this help and exit

Examples

	check_multipath


## check_readonlyfs

This Nagios plugin checks for readonly filesystems.

Usage

	check_readonlyfs [OPTION]... [FILE]...
	check_readonlyfs --help

Options 

	-l, --local               limit listing to local file systems
	-L, --list                display the list of checked file systems
	-T, --type=TYPE           limit listing to file systems of type TYPE
	-X, --exclude-type=TYPE   limit listing to file systems not of type TYPE
	-h, --help                display this help and exit

Examples

	check_readonlyfs
	check_readonlyfs -l -T ext3 -T ext4
	check_readonlyfs -l -X vfat


## check_uptime

This Nagios plugin checks how long the system has been running.

Usage

	check_uptime [--warning [@]start:end] [--critical [@]start:end]
	check_uptime --help

Where

* start <= end
* start and ":" is not required if start=0
* if range is of format "start:" and end is not specified, assume end is infinity
* to specify negative infinity, use "~"
* alert is raised if metric is outside start and end range (inclusive of endpoints)
* if range starts with "@", then alert if inside this range (inclusive of endpoints)

Examples

	check_uptime
	check_uptime --warning 30: --critical 15:


## check_users

This Nagios plugin displays the number of users that are currently logged on.

Usage:

	check_users [-w PERC] [-c PERC]

Where

* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold

Examples:

	check_users -w 1


## Source code

The source code can be also found at https://sites.google.com/site/davidemadrisan/opensource


## Installation

This package uses GNU autotools for configuration and installation.

If you have cloned the git repository then you will need to run
`autoreconf --install` to generate the required files.

Run `./configure --help` to see a list of available install options.
The plugin will be installed by default into `LIBEXECDIR`.

It is highly likely that you will want to customise this location to
suit your needs, i.e.:

        ./configure --libexecdir=/usr/lib/nagios/plugins

After `./configure` has completed successfully run `make install` and
you're done!


## Supported Platforms

This package is written in plain C, making as few assumptions as possible, and
sticking closely to ANSI C/POSIX.
Virtually all Linux distribution are supported.


## Bugs

If you find a bug please create an issue in the project bug tracker at
https://github.com/madrisan/nagios-plugins-linux/issues
