## Version 27 ("Polish Landscapes")
### TBD

#### FIXES

##### Plugin check_multipath

Free memory allocated to the pattern buffer by the function `regcomp`.

##### lib/meminfo_procps, lib/vminfo_procps

Fix the
[libprocps-ng:newlib](https://gitlab.com/procps-ng/procps/-/tree/newlib)
detection at build time.
Most distro packaged version of `procps-ng` correctly report the
version of the library *libprocps*.
According to the project mailing-list and my own tests, Debian
and Ubuntu report `UNKNOWN` as library version.
But, under openSUSE Tumbleweed, Archlinux, Slackware, and Fedora
it is shown properly.
So switch from `UNKNOWN` to `libprocps >= 3.3.12` in configure.

Also add a
[documentation page](DEVELOPERS.md)
for developers because the `procps-ng:newlib`
library is still in active development and there's no stable version
available.

#### ENHANCEMENTS

##### Plugin check_network

Add a bunch of command-line options:
 * `-i,--ifname`: only display interfaces matching a POSIX Extended [Regular Expression](https://man7.org/linux/man-pages/man7/regex.7.html);
 * `--ifname-debug`: display the list of metric keys and exit (for debugging);
 * `-k,--check-link`: report an error if one or more links are down;
 * `-l,--no-loopback`: skip the loopback interface;
 * `-W,--no-wireless `: skip the wireless interfaces;
 * `-%,--perc`: return percentage metrics when possible.

Note that the percentage **can only be calculated** for links with an available
physical speed. This feature has been asked by [iam333](https://github.com/iam333).
See the issue [#55](https://github.com/madrisan/nagios-plugins-linux/issues/55).

By default *all* the counters are reported in the perdata, but it's now possible
to select a subset of them if it's preferable:
 * `-b --no-bytes`: omit the rx/tx bytes counter from perfdata;
 * `-C,--no-collisions`: omit the collisions counter from perfdata;
 * `-d --no-drops`: omit the rx/tx drop counters from perfdata;
 * `-e --no-errors`: omit the rx/tx errors counters from perfdata;
 * `-m,--no-multicast`: omit the multicast counter from perfdata;
 * `-p,--no-packets`: omit the rx/tx packets counter from perfdata.

Make it possible to set the thresholds for all the reported metrics, by using
the following new plugins (they are actually just symlinks to `check_network`):
 * `check_network_collisions`
 * `check_network_dropped`
 * `check_network_errors`
 * `check_network_multicast`

Previously it was only possible for the network traffic in bytes.

Add two extra command-line switches `-r/--rx-only` and `-t/--tx-only`
for discarding the transmission and received metrics respectively.
This should allow even more custom plugin configurations and should be
especially usefull for setting thresholds in the network traffic in bytes.

##### lib/netinfo* (check_network): switch from getifaddrs glib call to linux rtnetlink

Switch to linux
[rtnetlink](https://www.man7.org/linux/man-pages/man7/netlink.7.html)
to be able to get the required
network statistics for all the available network interfaces,
not only for the `AF_PACKET`-capable ones.

This new implementation in particular let now `check_network` return
the statistics also for the [WireGuard](https://www.wireguard.com/) interfaces
[[#58](https://github.com/madrisan/nagios-plugins-linux/issues/58)].

##### lib/netinfo-private: switch to ETHTOOL_GLINKSETTINGS when possible

Change `SIOCETHTOOL` ioctl to use `ETHTOOL_GLINKSETTINGS`instead of the
obsolete `ETHTOOL_GSET`, when determining the network interface speed.
This requires Linux kernel 4.9+.
In case of failure revert to the obsolete `ETHTOOL_GSET`.

See the
[kernel commit](https://github.com/torvalds/linux/commit/3f1ac7a700d039c61d8d8b99f28d605d489a60cf)
(net: ethtool: add new ETHTOOL_xLINKSETTINGS API) if you need deeper technical informations.

#### LICENSE

Add to all files containing C code the
[SPDX License Identifier](https://spdx.org/licenses/)
for the GPL 3.0+ license.
```
// SPDX-License-Identifier: GPL-3.0-or-later
```

### GIT DIFF
```
-- git diff --stat 532861c 2de9564
.codeclimate.yml                            |   2 +-
.gitattributes                              |   1 -
AUTHORS                                     |  25 +++---
DEVELOPERS.md                               |  68 -----------------
NEWS                                        | 661 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++-
NEWS-OLD                                    | 660 ------------------------------------------------------------------------------------------------------------------------------------------------------------
NEWS.md                                     | 324 -----------------------------------------------------------------------------
README.md                                   |  20 ++---
check_skel.c.sample                         |   5 +-
configure.ac                                |  23 +-----
include/Makefile.am                         |   1 -
include/collection.h                        |   1 -
include/common.h                            |   4 +-
include/container_docker.h                  |   1 -
include/container_podman.h                  |   1 -
include/cpudesc.h                           |   1 -
include/cpufreq.h                           |   1 -
include/cpustats.h                          |   1 -
include/cputopology.h                       |   1 -
include/getenv.h                            |   1 -
include/interrupts.h                        |   1 -
include/jsmn.h                              |   1 -
include/json_helpers.h                      |   1 -
include/kernelver.h                         |   1 -
include/logging.h                           |   1 -
include/meminfo.h                           |   1 -
include/messages.h                          |   1 -
include/mountlist.h                         |   1 -
include/netinfo-private.h                   |  59 --------------
include/netinfo.h                           |  79 ++++---------------
include/perfdata.h                          |   1 -
include/processes.h                         |   1 -
include/procparser.h                        |   1 -
include/progname.h                          |   1 -
include/progversion.h                       |   1 -
include/string-macros.h                     |  10 ---
include/sysfsparser.h                       |   1 -
include/system.h                            |   6 +-
include/tcpinfo.h                           |   1 -
include/testutils.h                         |   1 -
include/thresholds.h                        |   1 -
include/units.h                             |   1 -
include/url_encode.h                        |   1 -
include/vminfo.h                            |   1 -
include/xalloc.h                            |   1 -
include/xasprintf.h                         |   1 -
include/xstrton.h                           |   1 -
lib/Makefile.am                             |   4 +-
lib/collection.c                            |   1 -
lib/container_docker_count.c                |   1 -
lib/container_docker_memory.c               |   1 -
lib/container_podman.c                      |   1 -
lib/container_podman_count.c                |   1 -
lib/container_podman_stats.c                |   1 -
lib/cpudesc.c                               |   1 -
lib/cpufreq.c                               |   1 -
lib/cpustats.c                              |   1 -
lib/cputopology.c                           |   1 -
lib/interrupts.c                            |   1 -
lib/json_helpers.c                          |   1 -
lib/kernelver.c                             |   1 -
lib/meminfo.c                               |   1 -
lib/meminfo_procps.c                        |   3 +-
lib/messages.c                              |   1 -
lib/mountlist.c                             |   1 -
lib/netinfo-private.c                       | 454 ------------------------------------------------------------------------------------------------------------
lib/netinfo.c                               | 303 +++++++++++++++++++-----------------------------------------------------
lib/perfdata.c                              |   1 -
lib/processes.c                             |   1 -
lib/procparser.c                            |   1 -
lib/progname.c                              |   1 -
lib/sysfsparser.c                           |   1 -
lib/tcpinfo.c                               |   1 -
lib/thresholds.c                            |   1 -
lib/url_encode.c                            |   1 -
lib/vminfo.c                                |   1 -
lib/vminfo_procps.c                         |  30 ++++++--
lib/xasprintf.c                             |   1 -
lib/xmalloc.c                               |   1 -
lib/xstrton.c                               |   1 -
packages/specs/nagios-plugins-linux.spec.in |   2 +-
plugins/Makefile.am                         |  17 +----
plugins/check_clock.c                       |   1 -
plugins/check_cpu.c                         |   8 +-
plugins/check_cpufreq.c                     |   1 -
plugins/check_cswch.c                       |   1 -
plugins/check_docker.c                      |   1 -
plugins/check_fc.c                          |   1 -
plugins/check_ifmountfs.c                   |   1 -
plugins/check_intr.c                        |   1 -
plugins/check_load.c                        |   1 -
plugins/check_memory.c                      |   1 -
plugins/check_multipath.c                   |   3 -
plugins/check_nbprocs.c                     |   1 -
plugins/check_network.c                     | 423 +++++++---------------------------------------------------------------------------------------------
plugins/check_paging.c                      |   1 -
plugins/check_podman.c                      |   1 -
plugins/check_readonlyfs.c                  |   1 -
plugins/check_swap.c                        |   1 -
plugins/check_tcpcount.c                    |   1 -
plugins/check_temperature.c                 |   1 -
plugins/check_uptime.c                      |   1 -
plugins/check_users.c                       |   1 -
tests/Makefile.am                           |   2 +-
tests/testutils.c                           |   1 -
tests/tsclock_thresholds.c                  |   1 -
tests/tscswch.c                             |   1 -
tests/tsintr.c                              |   1 -
tests/tslib_uname.c                         |   1 -
tests/tslibcontainer_docker_count.c         |   1 -
tests/tslibcontainer_docker_memory.c        |   1 -
tests/tslibkernelver.c                      |   1 -
tests/tslibmeminfo_conversions.c            |   1 -
tests/tslibmeminfo_interface.c              |   5 +-
tests/tslibmeminfo_procparser.c             |   1 -
tests/tslibmessages.c                       |   1 -
tests/tslibperfdata.c                       |   1 -
tests/tsliburlencode.c                      |   1 -
tests/tslibvminfo.c                         |   1 -
tests/tsload_normalize.c                    |   1 -
tests/tsload_thresholds.c                   |   1 -
tests/tspaging.c                            |   1 -
tests/tstemperature.c                       |   1 -
tests/{tstestutils.c => tststestutils.c}    |   1 -
tests/tsuptime.c                            |   1 -
125 files changed, 852 insertions(+), 2447 deletions(-)
```

## Version 26 ("Lockdown")
### May 5th, 2020

#### FIXES

##### Fix build with musl by including limits.h when PATH_MAX is used.

Bug reported and patched by Louis Sautier.
See also: [Gentoo bug #717038](https://bugs.gentoo.org/717038)

##### Fix build when `-fno-common` is added to CFLAGS.

The build with `-fno-common` failed with the error message:
```
(.bss+0x8): multiple definition of `program_name'
(.bss+0x0): multiple definition of `program_name_short'
```
This flag will be apparently enabled by default in gcc 10.
Bug reported by [sbraz](https://github.com/sbraz).

##### Fixed travis CI build

Fix by [sbraz](https://github.com/sbraz).

#### ENHANCEMENTS / CHANGES

##### New plugin check_podman

New plugin **check_podman** for checking some runtime metric of [podman](https://podman.io/) containers.

##### Package creation

Add Fedora 32 and CentOS 8 to the supported distributions.

#### GIT DIFF
```
-- git diff --stat f5c0edc 6c41cc9

.travis.yml                                                       |   2 +-
AUTHORS                                                           |   2 +
README.md                                                         |  30 ++++++++---
configure.ac                                                      |  30 ++++++++++-
debian/copyright                                                  |   2 +-
include/Makefile.am                                               |   8 +--
include/collection.h                                              |   2 +-
include/{container.h => container_docker.h}                       |   8 +--
include/container_podman.h                                        | 117 ++++++++++++++++++++++++++++++++++++++++
include/{json.h => jsmn.h}                                        |   0
include/json_helpers.h                                            |  35 ++++++++++++
include/progname.h                                                |   4 +-
include/testutils.h                                               |  10 +++-
include/xalloc.h                                                  |   2 +
include/{xstrtol.h => xstrton.h}                                  |   9 ++--
lib/Makefile.am                                                   |  15 ++++--
lib/collection.c                                                  |  25 ++++++---
lib/{container_count.c => container_docker_count.c}               |  34 +++---------
lib/{container_memory.c => container_docker_memory.c}             |   2 +-
lib/container_podman.c                                            | 373 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
lib/container_podman_count.c                                      | 114 +++++++++++++++++++++++++++++++++++++++
lib/container_podman_stats.c                                      | 179 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
lib/json_helpers.c                                                |  66 +++++++++++++++++++++++
lib/processes.c                                                   |   1 +
lib/xmalloc.c                                                     |  17 ++++--
lib/{xstrtol.c => xstrton.c}                                      |  20 ++++++-
packages/Makefile.am                                              |   8 +--
packages/multibuild.sh                                            |  16 +++++-
packages/specs/nagios-plugins-linux.spec.in                       |   6 +++
plugins/Makefile.am                                               |  13 ++++-
plugins/check_clock.c                                             |   2 +-
plugins/check_cpu.c                                               |   2 +-
plugins/check_cswch.c                                             |   2 +-
plugins/check_docker.c                                            |   4 +-
plugins/check_fc.c                                                |   3 +-
plugins/check_intr.c                                              |   2 +-
plugins/check_podman.c                                            | 261 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
tests/Makefile.am                                                 |  16 +++---
tests/{ts_container.data => ts_container_docker.data}             |   0
tests/ts_container_podman_GetContainerStats.data                  |   1 +
tests/ts_container_podman_ListContainers.data                     |   1 +
tests/{tslibcontainer_count.c => tslibcontainer_docker_count.c}   |   6 +--
tests/{tslibcontainer_memory.c => tslibcontainer_docker_memory.c} |   8 +--
43 files changed, 1362 insertions(+), 96 deletions(-)
```


## Version 25 ("Gentoo")
### May 5th, 2020

#### FIXES

##### Fix issues reported by lgtm, clang, and Codacy

Fix two security issues reported by lgtm analyzer, an issue reported by the clang static analyser v8, and some issues reported by Codacy.

##### Library sysfsparser

Fix debug messages in `sysfsparser_thermal_get_temperature()`.

##### Plugin check_memory

Add `min`, `max`, `warning` and `critical` to perfdata for `mem_available` and `mem_used` as asked by [sbraz](https://github.com/sbraz).

Minor code cleanup, and typo fixes.

##### Build system

Fix compilation when *libcurl* headers are not installed.

#### ENHANCEMENTS / CHANGES

Udate the external *jsmn* library.

Move some functions to the new library *perfdata*.

##### Build system

Fix a warning message about obsolete `AC_PROG_RANLIB`.

Add a build option to disable libcurl: `--disable-libcurl`.

##### Package creation

Make rpm packages for Fedora 30 and Debian 10 (Buster) packages.

Drop support for the Linux distribution Fedora 24-27 and Debian 6 (Squeeze).

##### Test framework

New unit test for `lib/perfdata.c`.

#### GIT DIFF
```
-- git diff --stat 440eefa c4a58b5

AUTHORS                                               |   6 ++
README.md                                             |  16 ++--
configure.ac                                          |  17 +++--
debian/changelog                                      |  32 ++++++++
include/Makefile.am                                   |   1 +
include/json.h                                        | 490 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++--------
include/perfdata.h                                    |  37 +++++++++
include/string-macros.h                               |   1 +
include/thresholds.h                                  |   9 ++-
lib/Makefile.am                                       |   2 +-
lib/container_count.c                                 |   7 +-
lib/container_memory.c                                |   8 +-
lib/cputopology.c                                     |   3 +-
lib/json.c                                            | 417 ------------------------------------------------------------------------------------------------------
lib/perfdata.c                                        |  85 +++++++++++++++++++++
lib/sysfsparser.c                                     |  41 +++++-----
lib/thresholds.c                                      |  26 +++++--
packages/Makefile.am                                  |  25 ++++---
packages/docker-shell-helpers/docker-shell-helpers.sh |   6 +-
packages/multibuild.sh                                |  30 +++-----
packages/specs/nagios-plugins-linux.spec.in           |  14 +++-
plugins/check_clock.c                                 |  10 +--
plugins/check_cpu.c                                   |   3 +
plugins/check_memory.c                                |  55 ++++++++++++--
plugins/check_swap.c                                  |   3 +
plugins/check_uptime.c                                |   8 +-
plugins/check_users.c                                 |   3 +-
tests/Makefile.am                                     |   9 ++-
tests/tslibcontainer_count.c                          |   2 +-
tests/tslibkernelver.c                                |   7 +-
tests/tslibperfdata.c                                 | 111 +++++++++++++++++++++++++++
31 files changed, 925 insertions(+), 559 deletions(-)
```


## Version 24
### January 13th, 2019

#### FIXES

##### Plugin check_cpufreq

The frequences returned by sysfs are in KHz.
This issue has been reported by [sbraz](https://github.com/sbraz).
Thanks!

#### ENHANCEMENTS / CHANGES

##### Plugin check_uptime

Add warn, crit, and min values to perfdata.
Based on a merge request opened by [magmax](https://github.com/magmax).
Thanks!

##### Plugin check_cpufreq

Make it possible to output the values in Hz/kHz/mHz/gHz
by adding some new command-line switches:
  * `-H,--Hz`
  * `-K,--kHz`
  * `-M,--mHz`
  * `-G,--gHz`

##### Build system

Check for the compiler flag `-Wstringop-truncation` availability.

Remove the autotools-generated file `libtool`.

Fix unsupported warning options for clang (7.0.0):
  * `-Wformat-signedness`
  * `-Wstringop-truncation`

Support Fedora 29 packaging (rpm packages generation).

#### GIT DIFF
```
-- git diff --stat 0a3b90b 0b7e7ea

.github/ISSUE_TEMPLATE/bug_report.md |    26 +
.gitignore                           |     1 +
README.md                            |     8 +-
configure.ac                         |     3 +-
libtool                              | 11645 --------------------------------
m4/cc_try_cflags.m4                  |     4 +-
packages/Makefile.am                 |     7 +-
plugins/check_cpufreq.c              |    35 +-
plugins/check_uptime.c               |     9 +-
9 files changed, 73 insertions(+), 11665 deletions(-)
```
