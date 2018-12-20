<a href='https://ko-fi.com/K3K57TH3' target='_blank'><img height='36' style='border:0px;height:36px;' src='https://az743702.vo.msecnd.net/cdn/kofi2.png?v=0' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>

<a href='https://www.amazon.fr/hz/wishlist/dl/invite/asbDKEd'>My Wish List at amazon.fr</a>

# Nagios Plugins for Linux

![Release Status](https://img.shields.io/badge/status-stable-brightgreen.svg)
[![License](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](https://spdx.org/licenses/GPL-3.0.html)
[![Download Latest Release](https://img.shields.io/badge/download-latest--tarball-blue.svg)](https://github.com/madrisan/nagios-plugins-linux/releases/download/v23/nagios-plugins-linux-23.tar.xz)
[![SayThanks](https://img.shields.io/badge/Say%20Thanks-!-1EAEDB.svg)](https://saythanks.io/to/madrisan)

[![Build Status](https://travis-ci.org/madrisan/nagios-plugins-linux.svg?branch=master)](https://travis-ci.org/madrisan/nagios-plugins-linux)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/3779/badge.svg)](https://scan.coverity.com/projects/3779)
[![Total Alerts](https://img.shields.io/lgtm/alerts/g/madrisan/nagios-plugins-linux.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/madrisan/nagios-plugins-linux/alerts/)
[![Language Grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/madrisan/nagios-plugins-linux.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/madrisan/nagios-plugins-linux/context:cpp)

---------------

![alt tag](https://madrisan.files.wordpress.com/2015/11/nagios-plugins-linux-logo-256.png)


## About

This package contains several `nagios plugins` for monitoring Linux boxes.
Nagios is an open source computer system monitoring, network monitoring and infrastructure monitoring software application (see: http://www.nagios.org/).

Here is the list of the available plugins:

* **check_clock** - returns the number of seconds elapsed between local time and Nagios server time 
* **check_cpu** - checks the CPU (user mode) utilization 
* **check_cpufreq** - displays the CPU frequency characteristics
* **check_cswch** - checks the total number of context switches across all CPUs
* **check_docker** - checks the number of running docker containers :new: (:warning: *alpha*)
* **check_fc** - monitors the status of the fiber status ports
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
in the GitHub [wiki page](https://github.com/madrisan/nagios-plugins-linux/wiki) or
[here](https://sites.google.com/site/davidemadrisan/nagios-monitoring/linux-os).


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
* gcc 4.1.2 (RHEL 5 / CentOS 5),
* gcc 4.4 (RHEL6 / CentOS 6),
* gcc 4.8.2 (RHEL7 / CentOS 7),
* gcc 5.1.1, 5.3.1, 6.3.1, 7.2.1, 7.3.1, 8.0.1 (Fedora 23 Cloud, Fedora 25 to 28),
* clang 3.7.0, 3.8.0, 5.0.1, 6.0.0 (Fedora 23 Cloud, Fedora 25 to 28),
* gcc 4.9.0-4.9.2, 5.2.0, 5.3.0, 6.2.0 (openmamba GNU/Linux 2.90+),
* clang 3.1, 3.5.1, and 3.8.1 (openmamba GNU/Linux 2.90+).

List of the Linux kernels that have been successfully tested:
* 2.6.18, 2.6.32,
* 3.10, 3.14, 3.18,
* 4.2, 4.4, 4,9, 4.14, 4.15, 4.16


## CentOS/RHEL, Debian, and Fedora Packages

The `.rpm` and `.deb` packages for CentOS/RHEL, Debian, and Fedora can be built using the following commands

Command            | Distribution
------------------ | ------------
CentOS 5           | `make -C packages centos-5`
CentOS 6           | `make -C packages centos-6`
CentOS 7           | `make -C packages centos-7`
Debian 6 (Squeeze) | `make -C packages debian-squeeze`
Debian 7 (Wheezy)  | `make -C packages debian-wheezy`
Debian 8 (Jessie)  | `make -C packages debian-jessie`
Debian 9 (Stretch) | `make -C packages debian-stretch`
Fedora 24          | `make -C packages fedora-24`
Fedora 25          | `make -C packages fedora-25`
Fedora 26          | `make -C packages fedora-26`
Fedora 27          | `make -C packages fedora-27`
Fedora 28          | `make -C packages fedora-28`
Fedora Rawhide     | `make -C packages fedora-rawhide`

in the root source folder.
The building process requires the Docker software containerization platform running on your system, and an internet connection to download the Docker images of the operating systems you want to build the packages for.


## Bugs

If you find a bug please create an issue in the project bug tracker at
[GitHub](https://github.com/madrisan/nagios-plugins-linux/issues)
