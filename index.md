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
|[check_clock]       | Returns the number of seconds elapsed between local time and Nagios server time | 
|[check_cpu]         | Checks the CPU (user mode) utilization | 
|[check_cpufreq]     | Displays the CPU frequency characteristics |
|[check_cswch]       | Checks the total number of context switches across all CPUs |
|[check_docker]      | Checks the number of running docker containers :new: (:warning: *alpha*, requires *libcurl* version 7.40.0+) |
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
|[check_podman]      | Monitor the status of podman containers :new: (:warning: *beta*, requires *libvarlink*) |
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

The plugin `check_multipath` grabs the status of each path by opening a
connection to the multipathd socket.  The default value is currently set to
the Linux abstract socket namespace `@/org/kernel/linux/storage/multipathd`,
but can be modified at build time by using the option `--with-socketfile`.

Example (RHEL5 and RHEL6 and other old distributions):

        ./configure --with-socketfile=/var/run/multipathd.sock

After `./configure` has completed successfully run `make install` and
you're done!

You can also run the (still incomplete) set of bundled unit tests by entering
the command `make check` (or `VERBOSE=1 make check`) and, if the llvm tool
`scan-build` is installed on your system, a `make -C tests check-clang-checker`
to get a static code analysis report (for developers only).

_Note_: you can also pass the _experimental_ option `--enable-libprocps` to
`configure` for getting the informations about memory and swap usage through
the API of the library `libprocps.so.5`
([procps newlib](https://gitlab.com/procps-ng/procps/tree/newlib)).
This library is still under active development and no stable version has
been released yet.

## Supported Platforms

This package is written in plain C, making as few assumptions as possible, and
sticking closely to ANSI C/POSIX. 
A C99-compliant compiler is required anyway.

This package is known to compile with:
* gcc 4.1 (RHEL 5 / CentOS 5),
* gcc 4.4 (RHEL6 / CentOS 6),
* gcc 4.8 (RHEL7 / CentOS 7),
* gcc 3.x, 5.1, 5.3, 6.3, 7.x, 8.x, 9.x, 10.x (openmamba GNU/Linux, Debian 8+, Fedora 25+),
* clang 3.7, 3.8, 4.9, 5.x, 6.x, 7.0, 8.0, 10.0.0 (openmamba GNU/Linux, Fedora 25+),

List of the Linux kernels that have been successfully tested:
* 2.6.18, 2.6.32,
* 3.10, 3.14, 3.18,
* 4.2, 4.4, 4,9, 4.14, 4.15, 4.16, 4.19
* 5.6, 5.7

## CentOS/RHEL, Debian, and Fedora Packages

The `.rpm` and `.deb` packages for CentOS/RHEL, Debian, and Fedora can be built using the following commands

Command            | Distribution
------------------ | ------------
CentOS 5           | `make -C packages centos-5`
CentOS 6           | `make -C packages centos-6`
CentOS 7           | `make -C packages centos-7`
CentOS 8           | `make -C packages centos-8`
Debian 8 (Jessie)  | `make -C packages debian-jessie`
Debian 9 (Stretch) | `make -C packages debian-stretch`
Debian 10 (Buster) | `make -C packages debian-buster`
Fedora 30          | `make -C packages fedora-30`
Fedora 31          | `make -C packages fedora-31`
Fedora 32          | `make -C packages fedora-32`
Fedora Rawhide     | `make -C packages fedora-rawhide`

in the root source folder.
The building process requires the _Docker_ software containerization platform running on your system, and an internet connection to download the Docker images of the operating systems you want to build the packages for.

On *Fedora 31* and *Fedora 32* you can use the native *Podman* pod manager along with the Docker CLI emulation script:

    sudo install dnf podman podman-docker

_Note_: the previous make commands can end with a `permission denied` error if *selinux* is configured in enforcing mode.
In this case you can temporarily disable *selinux* by executing as root the command `setenforce 0`.

## Gentoo Package

The plugins are available [in the Gentoo tree](https://packages.gentoo.org/packages/net-analyzer/nagios-plugins-linux-madrisan). They can be installed by running:
```
emerge -av net-analyzer/nagios-plugins-linux-madrisan
```
The USE flags `curl` and `varlink` are required to respectively build `check_docker` and `check_podman`.

## Bugs

If you find a bug please create an issue in the project bug tracker at
[github](https://github.com/madrisan/nagios-plugins-linux/issues)


[check_clock]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_clock
[check_cpu]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cpu
[check_cpufreq]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cpufreq
[check_cswch]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_cswch
[check_docker]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_docker
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
[check_podman]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_podman
[check_readonlyfs]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_readonlyfs
[check_swap]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_swap
[check_tcpcount]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_tcpcount
[check_temperature]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_temperature
[check_uptime]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_uptime
[check_users]: https://github.com/madrisan/nagios-plugins-linux/wiki/Nagios-Plugin-check_users
