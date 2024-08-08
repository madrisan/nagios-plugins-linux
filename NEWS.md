## Version 34 ("Heatwaves")
### Aug 8th, 2024

#### FIXES

##### Build

 * Missing header `npl_selinux.h` in Makefile (`noinst_HEADERS`).

##### Libraries

 * `lib/container`: docker API versions before v1.24 are deprecated, so 1.24 is set as the minimum version required.
 * `lib/sysfsparser`: fix gcc warning: ‘crit_temp’ may be used uninitialized.
 * `lib/sysfsparser`: better signature for function `sysfsparser_getvalue`.

##### Contrib (Icinga2)

 * Fix Icinga2 config for check_clock by Lorenz Kästle.
   Previously the time reference value was evaluated only during the startup of Icinga 2 and therefore a fixed point in time.
   This change makes it a function which gets evaluated every time the check is executed.

#### ENHANCEMENTS

##### Plugin check_ifmount

 * Add the cmdline switch -l|--list to list the mounted filesystems. Same output as the 'mount' command executed without options).

##### Plugin check_selinux

 * New plugin `check_selinux` that checks if SELinux is enabled.

##### Package creation

 * Add Linux Alpine 3.20 and drop version 3.17
 * Add Fedora 40, drop Fedora 38

##### Documentation

 * Fix typo
 * Add a link to discussion #147
 * Add a note on the Debian package nagios-plugins-contrib

### GIT DIFF
```
$ git diff --stat 366a9d745fb62ccd64e05ea5916eb4988ec55d2b HEAD
 .github/workflows/build-checks.yml          |   4 ++--
 README.md                                   |  21 +++++++++++++++------
 contrib/icinga2/CheckCommands.conf          |   2 +-
 debian/Makefile.am                          |   3 ++-
 debian/control                              |  13 ++++++++++++-
 debian/copyright                            |   2 +-
 debian/nagios-plugins-linux-selinux.install |   1 +
 include/Makefile.am                         |   1 +
 include/mountlist.h                         |   1 +
 include/npl_selinux.h                       |  27 +++++++++++++++++++++++++++
 include/sysfsparser.h                       |   4 ++--
 include/testutils.h                         |   1 -
 lib/Makefile.am                             |   1 +
 lib/cpudesc.c                               |   5 ++++-
 lib/cputopology.c                           |   5 ++++-
 lib/meminfo.c                               |   4 ++--
 lib/mountlist.c                             |  24 ++++++++++++++++++++++++
 lib/npl_selinux.c                           |  61 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 lib/sysfsparser.c                           |  41 ++++++++++++++++++++++++++---------------
 packages/Makefile.am                        |  10 +++++-----
 packages/multibuild.sh                      |   8 +++++---
 packages/specs/nagios-plugins-linux.spec.in |  12 ++++++++++++
 plugins/Makefile.am                         |   3 +++
 plugins/check_fc.c                          |  14 +++++++++++---
 plugins/check_ifmountfs.c                   |  28 ++++++++++++++++++++++++----
 plugins/check_network.c                     |   2 +-
 plugins/check_selinux.c                     | 142 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 plugins/check_temperature.c                 |   7 ++++---
 tests/Makefile.am                           |   3 +--
 tests/ts_sysdockermemstat.data              |  40 ----------------------------------------
 30 files changed, 395 insertions(+), 95 deletions(-)
```

## Version 33 ("Śmigus-Dyngus")
### Apr 1st, 2024

#### BREAKING CHANGES

##### Build

 * lib/container: Podman 3.0+ API support.
   This library and the check_container plugin are to be considered a PoC, i.e. not production-ready code.
   Note that now varlink is no more a build requirement.
   The plugins `check_docker` and `check_podman` were merged into `check_container`.
 * rename `--with-systemd` to `--enable-systemd` for consistency with the other optional boolean options.

#### FIXES

##### Plugins

 * check_container: passing on non running `--image` no longer produces a program core dump.
 * check_memory: remove comma from perfdata (issues#140).
   Thanks to [Grischa Zengel (ggzengel)](https://github.com/ggzengel) for reporting this problem.

##### Libraries

 * lib/*info_procps: fix build with `--enable-libprocps`.

##### Tests

 * tests: fix tests tslibxstrton_sizetollint and tslibpressure on 32-bit architectures.
   See: https://bugs.gentoo.org/927490

#### ENHANCEMENTS / CHANGES

 * check_container: print short container names in the plugin perfdata.
   Example: `docker.io/traefik:v2.9.8` becomes `traefik:v2.9.8`.

##### Test framework

 * ci: add gentoo to os matrix in github workflows.

### GIT DIFF
```
 .github/workflows/build-checks.yml                               |  16 ++---
 DEVELOPERS.md                                                    |  74 --------------------
 README.md                                                        |  15 ++---
 configure.ac                                                     |  91 +++++++++++--------------
 debian/Makefile.am                                               |   2 +-
 debian/changelog                                                 |   6 ++
 debian/control                                                   |   8 +--
 debian/nagios-plugins-linux-container.install                    |   1 +
 debian/nagios-plugins-linux-docker.install                       |   1 -
 include/Makefile.am                                              |   5 +-
 include/{container_docker.h => container.h}                      |   8 ++-
 include/container_podman.h                                       | 118 --------------------------------
 include/json_helpers.h                                           |   2 +
 include/xstrton.h                                                |   4 +-
 lib/Makefile.am                                                  |  14 +---
 lib/collection.c                                                 |   1 +
 lib/container.c                                                  | 428 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 lib/container_docker_count.c                                     | 256 ---------------------------------------------------------------------
 lib/container_docker_memory.c                                    | 175 ------------------------------------------------
 lib/container_podman.c                                           | 374 -----------------------------------------------------------------------------------------------------
 lib/container_podman_count.c                                     | 115 -------------------------------
 lib/container_podman_stats.c                                     | 180 -------------------------------------------------
 lib/json_helpers.c                                               | 128 ++++++++++++++++++++++++++++++++++-
 lib/meminfo_procps.c                                             |   4 +-
 lib/pressure.c                                                   |   1 +
 lib/vminfo_procps.c                                              |   4 +-
 lib/xstrton.c                                                    |  20 +++---
 packages/specs/nagios-plugins-linux.spec.in                      |  48 +++++--------
 plugins/Makefile.am                                              |  21 ++----
 plugins/{check_docker.c => check_container.c}                    | 150 +++++++++++++++++------------------------
 plugins/check_filecount.c                                        |  10 +--
 plugins/check_podman.c                                           | 262 -----------------------------------------------------------------------
 tests/Makefile.am                                                |  23 +++----
 tests/ts_container_podman_GetContainerStats.data                 |   1 -
 tests/ts_container_podman_ListContainers.data                    |   1 -
 tests/ts_procpressurecpu.data                                    |   2 +-
 tests/{tslibcontainer_docker_count.c => tslibcontainer_count.c}  |  43 ++++++------
 tests/tslibcontainer_docker_memory.c                             | 116 --------------------------------
 tests/tslibpressure.c                                            |   2 +-
```

## Version 32 ("Gematria")
### Jan 25th, 2024

#### FIXES

##### Build

 * configure: do not silently ignore missing libcurl and libvarlink.

##### Libraries

 * lib/netinfo-private: don't enforce nl_pid.
   Thanks to [Yuri Konotopov (nE0sIghT)](https://github.com/nE0sIghT) for reporting and solving this problem in containerised environments.
 * lib/netinfo-private: fix a Clang 17 warning.

##### Plugin check_users

 * check_users: fix an issue related to the Y2038 Unix bug.
 * check_cpufreq: wrong factor in check_cpufreq for -G.
   Thanks to [Grischa Zengel (ggzengel)](https://github.com/ggzengel) for the bug report.

#### ENHANCEMENTS / CHANGES

##### Package creation

 * Add Fedora 39 and drop support for Fedora 36.
 * Add Linux Alpine 3.18 and 3.19 and drop support for Linux Alpine 3.14-3.16.
 * Add Rocky Linux distribution.
 * Fix build of debian packages.

##### Test framework

 * Enable systemd library requirement in the GitHub workflow.
 * Update Linux releases for tests execution in the GitHub workflows.

### GIT DIFF
```
 .github/workflows/build-checks.yml          | 17 +++++++----------
 AUTHORS                                     |  5 +++++
 README.md                                   | 37 +++++++++++++++++++++----------------
 configure.ac                                | 22 ++++++++++++++++++++--
 debian/rules                                |  2 +-
 lib/netinfo-private.c                       |  4 ++--
 packages/Makefile.am                        | 35 +++++++++++++++++++++++------------
 packages/docker-shell-helpers               |  2 +-
 packages/multibuild.sh                      | 20 +++++++++++++++-----
 packages/specs/nagios-plugins-linux.spec.in |  8 ++++++--
 plugins/check_cpufreq.c                     |  2 +-
 plugins/check_users.c                       | 70 ++++++++++++++++++++++++++++++++++++++++++++++++----------------------
 12 files changed, 150 insertions(+), 74 deletions(-)
```

## Version 31 ("Counter-intuitive")
### Aug 28th, 2022

#### FIXES

##### Libraries

 * lib/container_docker_memory: fix an issue reported by clang-analyzer.
 * Make sure sysfs is mounted in the plugins that require it.

#### ENHANCEMENTS / CHANGES

##### Plugin check_filecount

 * New plugin `check_filecount` that returns the number of files found in one or more directories.

##### Plugin check_memory

 * check_memory: support new units kiB/MiB/GiB.
   Feature asked by [mdicss](https://github.com/mdicss).
   See the discussion [#120](https://github.com/madrisan/nagios-plugins-linux/discussions/120).

##### contrib/icinga2/CheckCommands.conf

 * Contribution from Lorenz [RincewindsHat](https://github.com/RincewindsHat): add icinga2 command configurations.

##### Build

 * configure: ensure libprocps is v4.0.0 or better if the experimental option `--enable-libprocps` is passed to `configure`.

##### Test framework

 * Add some unit tests for `lib/xstrton`.
 * New unit tests `tslibfiles_{filecount,hiddenfile,size}`.

##### Package creation

 * Add Linux Alpine 3.16 and remove version 3.13.
 * Do not package experimental plugins in the rpm `nagios-plugins-linux-all`.
 * Add Fedora 36 and drop Fedora 33 support.
 * CentOS 8 died a premature death at the end of 2021. Add packages for CentOS Stream 8 and 9.

##### GitHub workflows

 * Build the Nagios Plugins Linux on the LTS Ubuntu versions only. The version 21 seems dead.
 * Add build tests for all the supported oses.
 * Update the os versions used in tests.
 * CentOS 8 died a premature death at the end of 2021. Remove it from the list of test oses.
 * Add CodeQL analysis

### GIT DIFF
```
$ git diff --stat db94dc16 6edcf3aa
 .github/workflows/build-checks.yml          |   9 +-
 .github/workflows/codeql-analysis.yml       |  59 ++++++++++
 .gitignore                                  |   3 +
 AUTHORS                                     |   6 ++
 DEVELOPERS.md                               |  14 ++-
 README.md                                   |  50 ++++-----
 SECURITY.md                                 |  24 +++++
 configure.ac                                |  15 ++-
 contrib/README.md                           |   6 ++
 contrib/icinga2/CheckCommands.conf          | 923 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 include/Makefile.am                         |   1 +
 include/common.h                            |   2 +
 include/cpudesc.h                           |   2 +-
 include/files.h                             |  56 ++++++++++
 include/procparser.h                        |   2 +-
 include/sysfsparser.h                       |   3 +
 include/xstrton.h                           |  16 +--
 lib/Makefile.am                             |   1 +
 lib/container_docker_memory.c               |   8 +-
 lib/container_podman_count.c                |   2 +-
 lib/cpudesc.c                               |   8 +-
 lib/cpufreq.c                               |   2 +-
 lib/cputopology.c                           |   5 +-
 lib/files.c                                 | 252 +++++++++++++++++++++++++++++++++++++++++++
 lib/interrupts.c                            |   2 +-
 lib/meminfo.c                               |   2 +-
 lib/messages.c                              |   2 +-
 lib/processes.c                             |   2 +-
 lib/sysfsparser.c                           |  30 ++++--
 lib/thresholds.c                            |   4 +-
 lib/xmalloc.c                               |   2 +-
 lib/xstrton.c                               | 148 ++++++++++++++++++++++++--
 nagios-plugins-linux-logo-128.png           | Bin 0 -> 10505 bytes
 packages/Makefile.am                        |  22 ++--
 packages/docker-shell-helpers               |   2 +-
 packages/multibuild.sh                      |   7 +-
 packages/specs/APKBUILD.in                  |   2 +-
 packages/specs/Makefile.am                  |   2 +
 packages/specs/nagios-plugins-linux.spec.in |  10 +-
 plugins/Makefile.am                         |   3 +
 plugins/check_cpu.c                         |   9 +-
 plugins/check_cpufreq.c                     |   7 +-
 plugins/check_fc.c                          |   9 +-
 plugins/check_filecount.c                   | 292 ++++++++++++++++++++++++++++++++++++++++++++++++++
 plugins/check_load.c                        |   8 +-
 plugins/check_memory.c                      |  51 ++++++---
 plugins/check_network.c                     |   2 +-
 plugins/check_swap.c                        |   2 +-
 plugins/check_temperature.c                 |   6 +-
 tests/Makefile.am                           |  34 ++++--
 tests/testutils.c                           |   2 +-
 tests/tslibfiles_age.c                      |  85 +++++++++++++++
 tests/tslibfiles_filecount.c                | 227 +++++++++++++++++++++++++++++++++++++++
 tests/tslibfiles_hiddenfile.c               |  72 +++++++++++++
 tests/tslibfiles_size.c                     |  79 ++++++++++++++
 tests/tslibkernelver.c                      |   2 +-
 tests/{tslib_uname.c => tslibuname.c}       |   0
 tests/tslibxstrton_agetoint64.c             |  92 ++++++++++++++++
 tests/tslibxstrton_sizetoint64.c            |  91 ++++++++++++++++
 59 files changed, 2638 insertions(+), 141 deletions(-)
```

## Version 30 ("Low Pressure")
### Jan 25th, 2022

#### FIXES

##### Plugin check_pressure

 * Display values per second regardless of the delay, and ensure the delta of "some" is calculated correctly.
 * Thanks to [Christian Bryn (epleterte)](https://github.com/epleterte) for reporting and fixing a typo in the git clone command.

##### Package creation

 * Fix Debian packages creation.

#### ENHANCEMENTS / CHANGES

##### Libraries

 * lib/netinfo: fix a LGTM static analyzer alert.

##### Packages

Release updates:

 * Add Debian 11 and drop Debian 8,
 * Add Fedora 35 and drop Fedora 32,
 * Add Linux Alpine 3.15 and drop version 3.12.

### GIT DIFF
```
$ git diff --stat 69770573 ce7d9f69
 .github/workflows/build-checks.yml        |  2 +-
 AUTHORS                                   | 17 +++++++++++++----
 README.md                                 | 38 +++++++++++++++++++-------------------
 debian/Makefile.am                        | 27 ++++++++++++++++++++++++++-
 debian/changelog                          |  8 +++++++-
 debian/copyright                          |  2 +-
 debian/nagios-plugins-linux-network.links |  4 ++++
 lib/pressure.c                            | 29 +++++++++++++++--------------
 packages/Makefile.am                      | 18 +++++++++---------
 packages/multibuild.sh                    |  8 ++++----
 10 files changed, 99 insertions(+), 54 deletions(-)
```

## Version 29 ("High Temperatures")
### Jul 20th, 2021

#### FIXES

##### Plugin check_temperature

 * Do not display thermal ranges if `-t|--thermal_zone` in not set at command-line.

#### ENHANCEMENTS

##### Plugin check_temperature

 * Improve the output message by adding the thermal device when available;
 * List also the devices that display a temperature of 0°C to be more consistent with the tool 'sensors';
 * Improve the help message;
 * New command-line option `-l|--list` for displyaing the all the thermal sensors reported by the kernel.

##### Package creation

 * Update the list of supported platforms by adding Alpine 3.13 and 3.14 and Fedora 34;
 * Several improvements to the Debian packaging (thanks to [Vincent Olivert-Riera](https://github.com/vincent-olivert-riera) for the PR);
 * Build Debian multi-packages instead of a single package providing all binary plugins.
   This will permit these plugins to cohexist with the Nagios native ones.

##### Test framework

 * Switch to the latest stable OSes in GitHub workflow.

##### Documentation

 * Update the documentation for linking against procps-ng newlib;
 * Thanks to [Christian Bryn (epleterte)](https://github.com/epleterte) for reporting and fixing a typo in the git clone command.

### GIT DIFF
```
$ git diff --stat c57373a9 4e0d46e8
 .github/workflows/build-checks.yml              |   6 ++--
 DEVELOPERS.md                                   |   4 +--
 NEWS.md                                         |  75 +++++++++++++++++++++++++++++++++++++++++++
 README.md                                       |  23 +++++++-------
 VERSION                                         |   1 +
 configure.ac                                    |  22 ++++++++++++-
 debian/changelog                                |   9 +++++-
 debian/compat                                   |   2 +-
 debian/control                                  | 262 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----
 debian/nagios-plugins-linux-clock.install       |   1 +
 debian/nagios-plugins-linux-cpu.install         |   1 +
 debian/nagios-plugins-linux-cpufreq.install     |   1 +
 debian/nagios-plugins-linux-cswch.install       |   1 +
 debian/nagios-plugins-linux-docker.install      |   1 +
 debian/nagios-plugins-linux-fc.install          |   1 +
 debian/nagios-plugins-linux-ifmountfs.install   |   1 +
 debian/nagios-plugins-linux-intr.install        |   1 +
 debian/nagios-plugins-linux-iowait.install      |   1 +
 debian/nagios-plugins-linux-load.install        |   1 +
 debian/nagios-plugins-linux-memory.install      |   1 +
 debian/nagios-plugins-linux-multipath.install   |   1 +
 debian/nagios-plugins-linux-nbprocs.install     |   1 +
 debian/nagios-plugins-linux-network.install     |   1 +
 debian/nagios-plugins-linux-paging.install      |   1 +
 debian/nagios-plugins-linux-pressure.install    |   1 +
 debian/nagios-plugins-linux-readonlyfs.install  |   1 +
 debian/nagios-plugins-linux-swap.install        |   1 +
 debian/nagios-plugins-linux-tcpcount.install    |   1 +
 debian/nagios-plugins-linux-temperature.install |   1 +
 debian/nagios-plugins-linux-uptime.install      |   1 +
 debian/nagios-plugins-linux-users.install       |   1 +
 debian/nagios-plugins-linux.dirs                |   1 +
 debian/rules                                    |   2 +-
 include/string-macros.h                         |   2 ++
 include/sysfsparser.h                           |   3 ++
 lib/meminfo_procps.c                            |   2 +-
 lib/sysfsparser.c                               |  86 ++++++++++++++++++++++++++++++++++++++++++-------
 lib/vminfo_procps.c                             |   2 +-
 packages/Makefile.am                            |  10 +++---
 packages/multibuild.sh                          |   2 +-
 packages/specs/nagios-plugins-linux.spec.in     |  17 ++++++++--
 plugins/Makefile.am                             |  11 ++++---
 plugins/check_temperature.c                     |  32 ++++++++++++++-----
 43 files changed, 536 insertions(+), 60 deletions(-)
```

## Version 28 ("Alpine Hike")
### Dec 12th, 2020

#### FIXES

A few Clang and GCC warnings have been fixed.

#### ENHANCEMENTS

##### Plugin check_pressure

New plugin `check_pressure` that reports the Linux Pressure Stall Information (PSI) exported by Linux kernels 4.20+ (in the `/proc/pressure/` folder).

    check_pressure --cpu      return the cpu pressure metrics
    check_pressure --io       return the io (block layer/filesystems) pressure metrics
    check_pressure --memory   return the memory pressure metrics

##### Build system

Here are some notable news:

 * Integrate *Nagios Plugins for Linux* with *GitHub Workflow* tests;
 * Add [Linux Alpine](https://alpinelinux.org/) and [Ubuntu](https://ubuntu.com/)
   to the list of supported and automatically tested Linux distributions;
 * Update the list of supported platforms by adding Alpine 3.12 and Fedora 33;
 * Make the `packages/docker-shell-helpers` folder a git submodule.

##### Package creation

Create also the native package for Alpine 3.12.

### GIT DIFF
```
$ git diff --stat bef83bc ee0fbd5
 .github/ISSUE_TEMPLATE/bug_report.md                  |  17 +++++-----
 .github/ISSUE_TEMPLATE/feature_request.md             |  22 +++++++++++++
 .github/workflows/build-checks.yml                    |  59 ++++++++++++++++++++++++++++++++++
 .gitmodules                                           |   3 ++
 NEWS.md                                               | 230 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++------------------------------------------------------------------
 README.md                                             |  69 ++++++++++++++++++++++++----------------
 configure.ac                                          |  14 ++++++++
 include/Makefile.am                                   |   1 +
 include/pressure.h                                    |  85 +++++++++++++++++++++++++++++++++++++++++++++++++
 include/system.h                                      |   3 --
 include/testutils.h                                   |   2 ++
 lib/Makefile.am                                       |   1 +
 lib/netinfo-private.c                                 |   4 +--
 lib/pressure.c                                        | 198 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 lib/processes.c                                       |   4 +++
 packages/Makefile.am                                  |  28 ++++++++++------
 packages/docker-shell-helpers                         |   1 +
 packages/docker-shell-helpers/LICENSE                 | 201 ------------------------------------------------------------------------------------------------------------------
 packages/docker-shell-helpers/README.md               |  74 ------------------------------------------
 packages/docker-shell-helpers/__generate-doc.sh       |  26 ---------------
 packages/docker-shell-helpers/docker-shell-helpers.sh | 183 --------------------------------------------------------------------------------------------------------
 packages/docker-shell-helpers/images/docker.png       | Bin 28288 -> 0 bytes
 packages/multibuild.sh                                |  51 +++++++++++++++++++++++++----
 packages/specs/APKBUILD.in                            |  44 +++++++++++++++++++++++++
 packages/specs/Makefile.am                            |   2 +-
 plugins/Makefile.am                                   |   2 ++
 plugins/check_multipath.c                             |   1 +
 plugins/check_network.c                               |   2 +-
 plugins/check_pressure.c                              | 230 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 tests/Makefile.am                                     |   6 ++++
 tests/ts_procpressurecpu.data                         |   1 +
 tests/ts_procpressureio.data                          |   2 ++
 tests/tslib_uname.c                                   |  14 ++++++++
 tests/tslibpressure.c                                 | 125 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 34 files changed, 1046 insertions(+), 659 deletions(-)
```

## Version 27 ("Polish Landscapes")
### Aug 8th, 2020

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
-- git diff --stat 2de9564 532861c
.codeclimate.yml                            |   2 +-
.gitattributes                              |   1 +
AUTHORS                                     |  25 +++---
DEVELOPERS.md                               |  68 ++++++++++++++
NEWS                                        | 661 +--------------------------------------------------------------------------------------------------------------------------------------
NEWS-OLD                                    | 660 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NEWS.md                                     | 324 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
README.md                                   |  20 ++---
check_skel.c.sample                         |   5 +-
configure.ac                                |  23 ++++-
include/Makefile.am                         |   1 +
include/collection.h                        |   1 +
include/common.h                            |   4 +-
include/container_docker.h                  |   1 +
include/container_podman.h                  |   1 +
include/cpudesc.h                           |   1 +
include/cpufreq.h                           |   1 +
include/cpustats.h                          |   1 +
include/cputopology.h                       |   1 +
include/getenv.h                            |   1 +
include/interrupts.h                        |   1 +
include/jsmn.h                              |   1 +
include/json_helpers.h                      |   1 +
include/kernelver.h                         |   1 +
include/logging.h                           |   1 +
include/meminfo.h                           |   1 +
include/messages.h                          |   1 +
include/mountlist.h                         |   1 +
include/netinfo-private.h                   |  59 ++++++++++++
include/netinfo.h                           |  79 +++++++++++++----
include/perfdata.h                          |   1 +
include/processes.h                         |   1 +
include/procparser.h                        |   1 +
include/progname.h                          |   1 +
include/progversion.h                       |   1 +
include/string-macros.h                     |  10 +++
include/sysfsparser.h                       |   1 +
include/system.h                            |   6 +-
include/tcpinfo.h                           |   1 +
include/testutils.h                         |   1 +
include/thresholds.h                        |   1 +
include/units.h                             |   1 +
include/url_encode.h                        |   1 +
include/vminfo.h                            |   1 +
include/xalloc.h                            |   1 +
include/xasprintf.h                         |   1 +
include/xstrton.h                           |   1 +
lib/Makefile.am                             |   4 +-
lib/collection.c                            |   1 +
lib/container_docker_count.c                |   1 +
lib/container_docker_memory.c               |   1 +
lib/container_podman.c                      |   1 +
lib/container_podman_count.c                |   1 +
lib/container_podman_stats.c                |   1 +
lib/cpudesc.c                               |   1 +
lib/cpufreq.c                               |   1 +
lib/cpustats.c                              |   1 +
lib/cputopology.c                           |   1 +
lib/interrupts.c                            |   1 +
lib/json_helpers.c                          |   1 +
lib/kernelver.c                             |   1 +
lib/meminfo.c                               |   1 +
lib/meminfo_procps.c                        |   3 +-
lib/messages.c                              |   1 +
lib/mountlist.c                             |   1 +
lib/netinfo-private.c                       | 454 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
lib/netinfo.c                               | 303 +++++++++++++++++++++++++++++++++++++++++++++-----------------
lib/perfdata.c                              |   1 +
lib/processes.c                             |   1 +
lib/procparser.c                            |   1 +
lib/progname.c                              |   1 +
lib/sysfsparser.c                           |   1 +
lib/tcpinfo.c                               |   1 +
lib/thresholds.c                            |   1 +
lib/url_encode.c                            |   1 +
lib/vminfo.c                                |   1 +
lib/vminfo_procps.c                         |  30 ++-----
lib/xasprintf.c                             |   1 +
lib/xmalloc.c                               |   1 +
lib/xstrton.c                               |   1 +
packages/specs/nagios-plugins-linux.spec.in |   2 +-
plugins/Makefile.am                         |  17 +++-
plugins/check_clock.c                       |   1 +
plugins/check_cpu.c                         |   8 +-
plugins/check_cpufreq.c                     |   1 +
plugins/check_cswch.c                       |   1 +
plugins/check_docker.c                      |   1 +
plugins/check_fc.c                          |   1 +
plugins/check_ifmountfs.c                   |   1 +
plugins/check_intr.c                        |   1 +
plugins/check_load.c                        |   1 +
plugins/check_memory.c                      |   1 +
plugins/check_multipath.c                   |   3 +
plugins/check_nbprocs.c                     |   1 +
plugins/check_network.c                     | 423 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++------
plugins/check_paging.c                      |   1 +
plugins/check_podman.c                      |   1 +
plugins/check_readonlyfs.c                  |   1 +
plugins/check_swap.c                        |   1 +
plugins/check_tcpcount.c                    |   1 +
plugins/check_temperature.c                 |   1 +
plugins/check_uptime.c                      |   1 +
plugins/check_users.c                       |   1 +
tests/Makefile.am                           |   2 +-
tests/testutils.c                           |   1 +
tests/tsclock_thresholds.c                  |   1 +
tests/tscswch.c                             |   1 +
tests/tsintr.c                              |   1 +
tests/tslib_uname.c                         |   1 +
tests/tslibcontainer_docker_count.c         |   1 +
tests/tslibcontainer_docker_memory.c        |   1 +
tests/tslibkernelver.c                      |   1 +
tests/tslibmeminfo_conversions.c            |   1 +
tests/tslibmeminfo_interface.c              |   5 +-
tests/tslibmeminfo_procparser.c             |   1 +
tests/tslibmessages.c                       |   1 +
tests/tslibperfdata.c                       |   1 +
tests/tsliburlencode.c                      |   1 +
tests/tslibvminfo.c                         |   1 +
tests/tsload_normalize.c                    |   1 +
tests/tsload_thresholds.c                   |   1 +
tests/tspaging.c                            |   1 +
tests/tstemperature.c                       |   1 +
tests/{tststestutils.c => tstestutils.c}    |   1 +
tests/tsuptime.c                            |   1 +
125 files changed, 2447 insertions(+), 852 deletions(-)
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
