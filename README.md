![Nagios-compatible Plugins for Linux](nagios-plugins-linux-logo-128.png) Nagios-compatible Plugins for Linux&nbsp;
------------------
#### A suite of Nagios-compatible Plugins for monitoring Linux hosts.

![Release Status](https://img.shields.io/badge/status-stable-brightgreen.svg)
[![License](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](https://spdx.org/licenses/GPL-3.0-only.html)
[![Download Latest Release](https://img.shields.io/badge/download-latest--tarball-blue.svg)](https://github.com/madrisan/nagios-plugins-linux/releases/download/v32/nagios-plugins-linux-32.tar.xz)
[![Say Thanks!](https://img.shields.io/badge/Say%20Thanks-!-1EAEDB.svg)](https://saythanks.io/to/madrisan)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/3779.svg)](https://scan.coverity.com/projects/madrisan-nagios-plugins-linux)
![GitHub All Releases](https://img.shields.io/github/downloads/madrisan/nagios-plugins-linux/total.svg?style=flat-square)

<a href='https://ko-fi.com/K3K57TH3' target='_blank'><img height='36' style='border:0px;height:36px;' src='https://az743702.vo.msecnd.net/cdn/kofi2.png?v=0' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>
<a href='http://amzn.eu/8axPDQ1'><img height='36' src='https://images.freeimages.com/fic/images/icons/2229/social_media_mini/48/amazon.png' border='0' alt='Wish List at Amazon.fr' /></a>

*This project provides several plugins for monitoring physical and virtual Linux hosts with [Nagios](http://www.nagios.org/) and Nagios-compatible monitoring systems like [Icinga](https://icinga.com/learn/) and [Naemon](https://www.naemon.org/).*

Here is the list of the available plugins:

* **check_clock** - returns the number of seconds elapsed between local time and Nagios server time
* **check_container** - checks docker/podman containers :warning: *pre-alpha*, requires *libcurl* version 7.40.0+
* **check_cpu** - checks the CPU (user mode) utilization
* **check_cpufreq** - displays the CPU frequency characteristics
* **check_cswch** - checks the total number of context switches across all CPUs
* **check_fc** - monitors the status of the fiber status ports
* **check_filecount** - checks the number of files found in one or more directories
* **check_ifmountfs** - checks whether the given filesystems are mounted
* **check_intr** - monitors the total number of system interrupts
* **check_iowait** - monitors the I/O wait bottlenecks
* **check_load** - checks the current system load average
* **check_memory** - checks the memory usage
* **check_multipath** - checks the multipath topology status
* **check_nbprocs** - displays the number of running processes per user
* **check_network** - displays some network interfaces statistics. The following plugins are symlinks to *check_network*:
  * check_network_collisions
  * check_network_dropped
  * check_network_errors
  * check_network_multicast
* **check_paging** - checks the memory and swap paging
* **check_pressure** - checks Linux Pressure Stall Information (PSI) data
* **check_readonlyfs** - checks for readonly filesystems
* **check_selinux** - checks if SELinux is enabled :new:
* **check_swap** - checks the swap usage
* **check_tcpcount** - checks the tcp network usage
* **check_temperature** - monitors the hardware's temperature
* **check_uptime** - checks how long the system has been running
* **check_users** - displays the number of users that are currently logged on

## Full documentation

The full documentation of the `nagios-plugins-linux` is available online
in the GitHub [wiki page](https://github.com/madrisan/nagios-plugins-linux/wiki).

## Get and configure the source code

This package uses `GNU autotools` for configuration and installation.

If you have cloned the git repository

        git clone --recursive https://github.com/madrisan/nagios-plugins-linux.git

then you will need to run `autoreconf --install` to generate the required files.

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

If you want to compile the code with a C compiler different from the system default,
you can set the environment variable CC accordingly. Here are two examples:

        CC=clang-17 ./configure --libexecdir=/usr/lib/nagios/plugins
        CC=clang-18 ./configure --libexecdir=/usr/lib/nagios/plugins

## Installation

After `./configure` has completed successfully run `make install` and
you're done!

You can also run the (still incomplete) set of bundled unit tests by entering
the command `make check` (or `VERBOSE=1 make check`) and, if the llvm tool
`scan-build` is installed on your system, a `make -C tests check-clang-checker`
to get a static code analysis report (for developers only).

_Note_: you can also pass the option `--enable-libprocps` to `configure` for
getting the informations about memory and swap usage through the API of the
library `libproc2.so` [procps](https://gitlab.com/procps-ng/procps/)).

## Supported Platforms and Linux distributions

This package is written in plain C, making as few assumptions as possible, and
sticking closely to ANSI C/POSIX.
A C99-compliant compiler is required anyway.

This package is known to compile with:
* gcc 4.1 (RHEL 5 / CentOS 5),
* gcc 4.4 (RHEL6 / CentOS 6),
* gcc 4.8 (RHEL7 / CentOS 7),
* gcc 3.x, 5.1, 5.3, 6.3, 7-15 (openmamba GNU/Linux, Debian 8+, Fedora 25+),
* clang 3.7, 3.8, 4.9, 5, 6, 7, 8, 10-20 (openmamba GNU/Linux, Fedora 25+),

List of the Linux kernels that have been successfully tested:
* 2.6.18, 2.6.32,
* 3.10, 3.14, 3.18,
* 4.2, 4.4, 4,9, 4.14, 4.15, 4.16, 4.19
* 5.6, 5.7, 5.8, 5.12-5.18, 6.1-6.11

The Nagios Plugins Linux are regularly tested on
 * Alpine Linux (musl libc),
 * Debian, Fedora, Gentoo, and Ubuntu (GNU C Library (glibc)).

## Alpine, CentOS Stream, Debian, and Fedora Packages

The `.apk`, `.rpm` and `.deb` packages for Alpine, CentOS/RHEL, Debian, and Fedora can be built using the following commands

Command              | Distribution
-------------------- | ------------
Alpine 3.20          | `make -C packages alpine-3.20`
Alpine 3.21          | `make -C packages alpine-3.21`
Alpine 3.22          | `make -C packages alpine-3.22`
CentOS Stream 8      | `make -C packages centos-stream-8`
CentOS Stream 9      | `make -C packages centos-stream-9`
Debian 10 (Buster)   | `make -C packages debian-buster`
Debian 11 (Bullseye) | `make -C packages debian-bullseye`
Debian 12 (Bookworm) | `make -C packages debian-bookworm`
Fedora 41            | `make -C packages fedora-41`
Fedora 42            | `make -C packages fedora-42`
Fedora 43            | `make -C packages fedora-43`
Fedora Rawhide       | `make -C packages fedora-rawhide`
Rocky Linux 8        | `make -C packages rockylinux-8`
Rocky Linux 9        | `make -C packages rockylinux-9`

in the root source folder.
The building process requires the _Docker_ software containerization platform running on your system, and an internet connection to download the Docker images of the operating systems you want to build the packages for.

On *Fedora* (and all the distributions shipping *Podman*) you can use the native *Podman* pod manager along with the Docker CLI emulation script:

    sudo install dnf podman podman-docker

_Note_: the previous make commands can end with a `permission denied` error if *selinux* is configured in enforcing mode.
In this case you can temporarily disable *selinux* by executing as root the command `setenforce 0`
(or maybe share a better solution!).

## Debian Package `nagios-plugins-contrib`

The `Nagios Plugin Linux` source code has been merged with the Debian package [`nagios-plugins-contrib`](https://sources.debian.org/src/nagios-plugins-contrib/)
but only the `check_memory` plugin is provided by the binary package shipped by Debian,
starting with the [Debian Bookworm](https://packages.debian.org/bookworm/nagios-plugins-contrib) version.

For more details, see GitHub [discussion #147](https://github.com/madrisan/nagios-plugins-linux/discussions/147).

## Gentoo Package

The plugins are available [in the Gentoo tree](https://packages.gentoo.org/packages/net-analyzer/nagios-plugins-linux-madrisan). They can be installed by running:
```
emerge -av net-analyzer/nagios-plugins-linux-madrisan
```
The USE flag `curl` is required to build `check_container`.

## Bugs

If you find a bug please create an issue in the project bug tracker at
[GitHub](https://github.com/madrisan/nagios-plugins-linux/issues)
