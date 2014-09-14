![alt tag](https://sites.google.com/site/davidemadrisan/files/nagios-logos.png)

# Nagios Plugins for Linux

## About

This package contains several `nagios plugins` for monitoring Linux boxes.
Nagios is an open source computer system monitoring, network monitoring and infrastructure monitoring software application (see: http://www.nagios.org/).

Here is the list of the available plugins:

* **check_clock** - returns the number of seconds elapsed between local time and Nagios server time 
* **check_cpu** - checks the CPU (user mode) utilization 
* **check_cswch** - checks the total number of context switches across all CPUs
* **check_ifmountfs** - checks whether the given filesystems are mounted
* **check_intr** - monitors the total number of system interrupts
* **check_iowait** - monitors the I/O wait bottlenecks 
* **check_load** - checks the current system load average 
* **check_memory** - checks the memory usage 
* **check_multipath** - checks the multipath topology status 
* **check_nbprocs** - displays the number of running processes per user 
* **check_network** - displays some network interfaces statistics 
* **check_paging** - checks the memory and swap paging 
* **check_readonlyfs** - checks for readonly filesystems 
* **check_swap** - checks the swap usage 
* **check_tcpcount** - checks the tcp network usage 
* **check_temperature** - monitors the hardware's temperature 
* **check_uptime** - checks how long the system has been running 
* **check_users** - displays the number of users that are currently logged on 


## Full documentation

The full documentation of the `nagios-plugins-linux` is available online
[here](https://sites.google.com/site/davidemadrisan/nagios-monitoring/linux-os).


## Source code

The source code of the latest stable version can be found in
[this page](https://sites.google.com/site/davidemadrisan/nagios-monitoring/linux-os).


## The plugins in detail 

**The check_clock plugin**

This Nagios plugin returns the number of seconds elapsed between the host local
time and Nagios time.

*Usage*

	check_clock [-w COUNTER] [-c COUNTER] --refclock TIME
	check_clock --help

*Command line options*

* -r, --refclock TIME: the clock reference (in seconds since the Epoch)
* -w, --warning COUNTER: warning threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -c, --critical COUNTER: critical threshold
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	# $ARG1$ is the number of seconds since the Epoch --
	# "$(date '+%s')" -- provided by the Nagios poller
	check_clock -w 60 -c 120 --refclock $ARG1$


**The check_cpu plugin**

This Nagios plugin checks the CPU (user mode) utilization.

*Usage*

	check_cpu [-m] [-p] [-v] [-w PERC] [-c PERC] [delay [count]]
	check_cpu --cpuinfo
	check_cpu --help

*Command line options*

* -m, --no-cpu-model: do not display the cpu model in the output message
* -p, --per-cpu: display the utilization of each CPU
* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -i, --cpuinfo: show the CPU characteristics (for debugging)
* -h, --help: display this help and exit
* -V, --version: output version information and exit
* delay is the delay between updates in seconds (default: 1sec)
* count is the number of updates (default: 2)

*Examples*

	check_cpu -w 85% -c 95%
	cpu (CPU: Intel(R) Atom(TM) CPU N270 @ 1.60GHz) OK - cpu user 79.5% | cpu_user=79.5% cpu_system=20.5% cpu_idle=0.0% cpu_iowait=0.0% cpu_steal=0.0%

	# count = 1 means the percentages of total CPU time from boottime
	check_cpu -m -w 85% -c 95% 1 1
	cpu OK - cpu user 33.2% | cpu_user=33.2% cpu_system=6.5% cpu_idle=57.1% cpu_iowait=3.2% cpu_steal=0.0%

	check_cpu -m -p -w 85% -c 95% 1 2
	cpu OK - cpu user 20.6% | cpu_user=20.2% cpu_system=6.7% cpu_idle=66.8% cpu_iowait=6.2% cpu_steal=0.0% cpu0_user=20.8% cpu0_system=7.3% cpu0_idle=59.4% cpu0_iowait=12.5% cpu0_steal=0.0% cpu1_user=20.6% cpu1_system=5.2% cpu1_idle=74.2% cpu1_iowait=0.0% cpu1_steal=0.0%

**The check_cswch plugin**

This Nagios plugin checks the total number of context switches across all CPUs.

*Usage*

	check_cswch [-v] [-w COUNTER] -c [COUNTER] [delay [count]]
	check_cswch --help

*Command line options*

* -w, --warning COUNTER: warning threshold
* -c, --critical COUNTER: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_cswch 1 2


**The check_ifmountfs plugin**

This Nagios plugin checks whether the given filesystems are mounted.

*Usage*

	check_ifmountfs [FILESYSTEM]...
	check_ifmountfs --help

*Examples*

	check_ifmountfs /mnt/nfs-data /mnt/cdrom


**The check_intr plugin**

This Nagios plugin monitors the total number of system interrupts.

*Usage*

	check_intr [-v] [-w COUNTER] -c [COUNTER] [delay [count]]
	check_intr --help

*Command line options*

* -w, --warning COUNTER: warning threshold
* -c, --critical COUNTER: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_intr 1 2


**The check_iowait plugin**

This Nagios plugin checks for I/O wait bottlenecks.

*Usage*

	check_iowait [-v] [-w PERC] [-c PERC] [delay [count]]
	check_iowait --help

*Command line options*

* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* delay is the delay between updates in seconds (default: 1sec)
* count is the number of updates (default: 2)
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_iowait -w 10% -c 20%
	iowait OK - cpu iowait 0% | cpu_user=31%, cpu_system=13%, cpu_idle=56%, cpu_iowait=0%, cpu_steal=0%
	
	# count = 1 means the percentages of total CPU time from boottime
	check_iowait -w 10% -c 20% 1 1
	iowait OK - cpu iowait 7% | cpu_user=34%, cpu_system=11%, cpu_idle=49%, cpu_iowait=7%, cpu_steal=0%


**The check_load plugin**

This Nagios plugin tests the current system load average.

*Usage*

	check_load [-r] [--load1=w,c] [--load5=w,c] [--load15=w,c]
	check_load --help

*Command line options*

* -r, --percpu: divide the load averages by the number of CPUs
* 1, --load1=WLOAD1,CLOAD1: warning and critial thresholds for load1
* 5, --load5=WLOAD5,CLOAD5. warning and critical thresholds for load5
* L, --load15=WLOAD15,CLOAD15: warning and critical thresholds for load15
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_load -r --load1=2,3 --load15=1.5,2.5


**The check_memory and check_swap plugins**

These two nagios plugins respectivery check for memory and swap usage.

*Usage*

	check_memory [-s] [-b,-k,-m,-g] [-w PERC] [-c PERC]
	check_swap [-b,-k,-m,-g] [-w PERC] [-c PERC]
	
	check_memory --help
	check_swap --help

*Command line options*

* -a, --available: display the memory available (kernel 2.6.27+) / free (older kernels)
* -s, --vmstats: display the virtual memory perfdata
* -b,-k,-m,-g: show output in bytes, KB (the default), MB, or GB
* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_memory --vmstats -w 80% -c 90%
	memory OK: 26.05% (266580 kB) used | mem_total=1023312kB, mem_used=266580kB, mem_free=171548kB, mem_shared=51244kB, mem_buffers=34744kB, mem_cached=550440kB, mem_available=674712kB, mem_active=325136kB, mem_anonpages=240464kB, mem_committed=1704152kB, mem_dirty=604kB, mem_inactive=468904kB, vmem_pageins/s=128, vmem_pageouts/s=0, vmem_pgmajfault/s=0
	  # mem_total    : Total usable physical RAM
	  # mem_used     : Total amount of physical RAM used by the system
	  # mem_free     : Amount of RAM that is currently unused
	  # mem_shared   : Now always zero; not calculated
	  # mem_buffers  : Amount of physical RAM used for file buffers
	  # mem_cached   : In-memory cache for files read from the disk (the page cache)
	  # mem_available: kernel >= 2.6.27: memory available for starting new applications, without swapping
	  # mem_available: kernel < 2.6.27: same as 'mem_free'
	  # mem_active   : Memory that has been used more recently
	  # mem_anonpages: Non-file backed pages mapped into user-space page tables
	  # mem_committed: The amount of memory presently allocated on the system
	  # mem_dirty    : Memory which is waiting to get written back to the disk
	  # mem_inactive : Memory which has been less recently used
	  # vmem_pageins
	  # vmem_pageouts: The number of memory pages the system has written in and out to disk
	  # vmem_pgmajfault: The number of memory major pagefaults

	check_memory -a -m -w 20%: -c 10%:
	memory OK: 65.40% (653 MB) available | mem_total=999MB, mem_used=264MB, mem_free=105MB, mem_shared=50MB, mem_buffers=38MB, mem_cached=590MB, mem_available=653MB, mem_active=341MB, mem_anonpages=238MB, mem_committed=1663MB, mem_dirty=0MB, mem_inactive=495MB

	check_swap -w 40% -c 60% -m
	swap WARNING: 42.70% (895104 kB) used | swap_total=2096444kB, swap_used=895104kB, swap_free=1201340kB, swap_cached=117024kB, swap_pageins/s=97, swap_pageouts/s=73
	  # swap_total   : Total amount of swap space available
	  # swap_used    : Total amount of swap used by the system
	  # swap_free    : Amount of swap space that is currently unused
	  # swap_cached  : The amount of swap used as cache memory
	  # swap_pageins 
	  # swap_pageouts: The number of swap pages the system has brought in and out


**The check_multipath plugin**

This Nagios plugin checks the multipath topology status.

*Usage*

	check_multipath [-v]
	check_multipath --help

*Command line options*

* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_multipath


**The check_nbprocs plugin**

This Nagios plugin displays the number of running processes per user.

*Usage*

	check_nbprocs [--verbose] [--threads] [-w COUNT] [-c COUNT]
	check_nbprocs --help

*Command line options*

* -t, --threads: display the number of threads
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_nbprocs
	check_nbprocs --threads -w 1500 -c 2000


**The check_network plugin**

This Nagios plugin checks displays some network interfaces.statistics.

*Usage*

	check_network
	check_network --help

*Command line options*

* -h, --help: display this help and exit
* -V, --version: output version information and exit


**The check_paging plugin**

This Nagios plugin checks for memory and swap paging.

*Usage*

	check_paging [--paging] [--swapping]
	check_paging --help

*Command line options*

* -p, --paging: display the page reads and writes
* -s, --swapping: display the swap reads amd writes
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_paging --paging --swapping -w 10 -c 25


**The check_readonlyfs plugin**

This Nagios plugin checks for readonly filesystems.

*Usage*

	check_readonlyfs [OPTION]... [FILE]...
	check_readonlyfs --help

*Command line options*

* -l, --local: limit listing to local file systems
* -L, --list: display the list of checked file systems
* -T, --type=TYPE: limit listing to file systems of type TYPE
* -X, --exclude-type=TYPE: limit listing to file systems not of type TYPE
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_readonlyfs
	check_readonlyfs -l -T ext3 -T ext4
	check_readonlyfs -l -X vfat


**The check_tcpcount plugin**

This plugin displays TCP network and socket informations.

*Usage*

	check_tcpcount [--tcp] [--tcp6] --warning COUNTER --critical COUNTER
	check_tcpcount --help

*Command line options*

* -t, --tcp: display the statistics for the TCP protocol (the default)
* -6, --tcp6: display the statistics for the TCPv6 protocol
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_tcpcount -w 1000 -c 1500
	check_tcpcount --tcp6 -w 1000 -c 1500
	check_tcpcount --tcp --tcp6 -w 1500 -c 2000


**The check_temperature  plugin**

This Nagios plugin monitors the hardware's temperature.

*Usage*

	check_temperature [-f|-k] [-t <thermal_zone>] [-w COUNTER] [-c COUNTER]
	check_temperature -.help

*Command line options*

* -f, --fahrenheit: use fahrenheit as the temperature unit
* -k, --kelvin: use kelvin as the temperature unit
* -t, --thermal_zone: only consider a specific thermal zone
* -w, --warning COUNTER: warning threshold
* -c, --critical COUNTER: critical threshold
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_temperature -w 80 -c 90
	check_temperature -t thermal_zone0 -w 80 -c 90


**The check_uptime plugin**

This Nagios plugin checks how long the system has been running.

*Usage*

	check_uptime [-m] [--warning [@]start:end] [--critical [@]start:end]
	check_uptime --help

*Command line options*

* -m, --clock-monotonic  use the monotonic clock for retrieving the time
* -w, --warning COUNTER: warning threshold
* -c, --critical COUNTER: critical threshold
* -h, --help: display this help and exit
* -V, --version: output version information and exit

and

* start <= end
* start and ":" is not required if start=0
* if range is of format "start:" and end is not specified, assume end is infinity
* to specify negative infinity, use "~"
* alert is raised if metric is outside start and end range (inclusive of endpoints)
* if range starts with "@", then alert if inside this range (inclusive of endpoints)

*Examples*

	check_uptime
	check_uptime --warning 30: --critical 15:
	check_uptime --clock-monotonic -c 15: -w 30:


**The check_users plugin**

This Nagios plugin displays the number of users that are currently logged on.

*Usage*

	check_users [-w PERC] [-c PERC]

*Command line options*

* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold
* -v, --verbose: show details for command-line debugging (Nagios may truncate output)
* -h, --help: display this help and exit
* -V, --version: output version information and exit

*Examples*

	check_users -w 1


## Installation

This package uses `GNU autotools` for configuration and installation.

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
A C99-compliant compiler is required anyway.

This package is known to compile with
* gcc 4.1.2 (RHEL 5 / CentOS 5)
* gcc 4.4 (RHEL6 / CentOS 6),
* gcc 4.8.2, gcc 4,9,0 and clang 3.1 (openmamba GNU/Linux 2.90+).

List of the Linux kernels that have been successfully tested: 2.6.18, 2.6.32, 3.10, 3.14.


## Bugs

If you find a bug please create an issue in the project bug tracker at
[github](https://github.com/madrisan/nagios-plugins-linux/issues)
