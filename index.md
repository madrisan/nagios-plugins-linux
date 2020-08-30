![project logo](https://madrisan.files.wordpress.com/2015/11/nagios-plugins-linux-logo-256.png)

### Overview.
[Nagios](http://www.nagios.org/) is an open source computer system monitoring, network monitoring and infrastructure monitoring software application. Nagios, originally created under the name NetSaint, was written and is currently maintained by Ethan Galstad along with a group of developers who are actively maintaining both the official and unofficial plugins.

The _Nagios Plugins_ for Linux are intended to be run by _NRPE_, the Nagios Remote Plugin Executor, that "allows you to remotely execute Nagios plugins on other Linux/Unix machines. This allows you to monitor remote machine metrics (disk usage, CPU load, etc.)."

These plugins have been reported to also work with 
[Icinga](https://www.icinga.org/) and
[Icinga2](https://www.icinga.org/icinga/icinga-2/).

### Available Nagios Plugins

| Plugin Name        | Detailed Informations   | 
|:------------------ |:----------------------- |
|[check_clock]       | Returns the number of seconds elapsed between local time and Nagios time | 
|[check_cpu]         | Checks the CPU (user mode) utilization     | 
|[check_cpufreq]     | Displays the CPU frequency characteristics |
|[check_cswch]       | Checks the total number of context switches across all CPUs |
|[check_fc]          | Monitors the status of the fiber status ports |
|[check_ifmountfs]   | Checks whether the given filesystems are mounted |
|[check_intr]        | Monitors the total number of system interrupts |
|[check_iowait]      | Monitors the I/O wait bottlenecks |
|[check_load]        | Checks the current system load average |
|[check_memory]      | Checks the memory usage |
|[check_multipath]   | Checks the multipath topology status |
|[check_nbprocs]     | Displays the number of running processes per user |
|[check_network]     | Displays some network interfaces statistics |
|[check_paging]      | Checks the memory and swap paging |
|[check_readonlyfs]  | Checks for readonly filesystems |
|[check_swap]        | Checks the swap usage |
|[check_tcpcount]    | Checks the tcp network usage |
|[check_temperature] | Monitors the hardware's temperature |
|[check_uptime]      | Checks how long the system has been running |
|[check_users]       | Displays the number of users that are currently logged on |

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
* gcc 4.8.2 (RHEL7 / CentOS 7),
* gcc 4.9.0-4.9.2,5.2.0 clang 3.1 and 3.5.1 (openmamba GNU/Linux 2.90+,3.0.1).

List of the Linux kernels that have been successfully tested: 2.6.18, 2.6.32, 3.10, 3.14, 3.18.


## Bugs

If you find a bug please create an issue in the project bug tracker at
[github](https://github.com/madrisan/nagios-plugins-linux/issues)


[check_clock]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_clock
[check_cpu]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cpu
[check_cpufreq]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cpufreq
[check_cswch]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cswch
[check_fc]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_fc
[check_ifmountfs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_ifmountfs
[check_intr]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_intr
[check_iowait]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_iowait
[check_load]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_load
[check_memory]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_memory
[check_multipath]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_multipath
[check_nbprocs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_nbprocs
[check_network]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_network
[check_paging]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_paging
[check_readonlyfs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_readonlyfs
[check_swap]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_swap
[check_tcpcount]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_tcpcount
[check_temperature]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_temperature
[check_uptime]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_uptime
[check_users]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_users
