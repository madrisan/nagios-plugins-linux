![project logo](nagios-plugins-linux-logo-128.png)

This project provides several plugins for monitoring physical and virtual Linux hosts with [Nagios](http://www.nagios.org/) and Nagios-compatible monitoring systems like [Icinga](https://icinga.com/learn/) and [Naemon](https://www.naemon.org/).

[Nagios](http://www.nagios.org/) is an open source computer system monitoring, network monitoring and infrastructure monitoring software application.

---

### Web sites

 * **GitHub**: [https://github.com/madrisan/nagios-plugins-linux](https://github.com/madrisan/nagios-plugins-linux)
 * **Wiki**: [https://github.com/madrisan/nagios-plugins-linux/wiki](https://github.com/madrisan/nagios-plugins-linux/wiki)

---

### Available Nagios Plugins

| Plugin Name        | Detailed Informations   |
|:------------------ |:----------------------- |
|[check_clock]       | Returns the number of seconds elapsed between local time and Nagios server time |
|[check_cpu]         | Checks the CPU (user mode) utilization |
|[check_cpufreq]     | Displays the CPU frequency characteristics |
|[check_cswch]       | Checks the total number of context switches across all CPUs |
|[check_docker]      | Checks the number of running docker containers (:warning: *pre-alpha*, requires *libcurl* version 7.40.0+) |
|[check_fc]          | Monitors the status of the fiber status ports |
|[check_filecount]   | Checks the number of files found in one or more directories :new: |
|[check_ifmountfs]   | Checks whether the given filesystems are mounted |
|[check_intr]        | Monitors the total number of system interrupts |
|[check_iowait]      | Monitors the I/O wait bottlenecks |
|[check_load]        | Checks the current system load average |
|[check_memory]      | Checks the memory usage |
|[check_multipath]   | Checks the multipath topology status |
|[check_nbprocs]     | Displays the number of running processes per user |
|[check_network]     | Displays some network interfaces statistics |
|[check_paging]      | Checks the memory and swap paging |
|[check_pressure]    | Checks Linux Pressure Stall Information (PSI) data |
|[check_podman]      | Monitor the status of podman containers (:warning: *alpha*, requires *libvarlink*) |
|[check_readonlyfs]  | Checks for readonly filesystems |
|[check_swap]        | Checks the swap usage |
|[check_tcpcount]    | Checks the tcp network usage |
|[check_temperature] | Monitors the hardware's temperature |
|[check_uptime]      | Checks how long the system has been running |
|[check_users]       | Displays the number of users that are currently logged on |

### Documentation

The full documentation and the source code are available in the project
[GitHub](https://github.com/madrisan/nagios-plugins-linux) site.

[check_clock]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_clock
[check_cpu]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cpu
[check_cpufreq]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cpufreq
[check_cswch]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cswch
[check_docker]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_docker
[check_fc]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_fc
[check_filecount]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_filecount
[check_ifmountfs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_ifmountfs
[check_intr]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_intr
[check_iowait]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_iowait
[check_load]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_load
[check_memory]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_memory
[check_multipath]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_multipath
[check_nbprocs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_nbprocs
[check_network]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_network
[check_paging]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_paging
[check_podman]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_podman
[check_pressure]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_pressure
[check_readonlyfs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_readonlyfs
[check_swap]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_swap
[check_tcpcount]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_tcpcount
[check_temperature]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_temperature
[check_uptime]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_uptime
[check_users]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_users
