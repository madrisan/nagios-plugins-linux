#!/bin/bash
# Multi-platform build system
# Copyright (C) 2016-2023 Davide Madrisan <davide.madrisan@gmail.com>

PROGNAME="${0##*/}"
PROGPATH="${0%/*}"
REVISION=3

die () { echo -e "$PROGNAME: error: $1" 1>&2; exit 1; }
msg () { echo "*** info: $1"; }

docker_helpers="$PROGPATH/docker-shell-helpers/docker-shell-helpers.sh"

[ -r "$docker_helpers" ] || die "no such file: $docker_helpers"
# shellcheck source=/dev/null
. "$docker_helpers"

usage () {
   cat <<__EOF
Usage: $PROGNAME -o <os> -s <shared> [-d <distro>] [--spec <file>] \
[-t <folder>] -v <version>
       $PROGNAME --help
       $PROGNAME --version

Where:
   -d|--distro : distribution name (default: no distribution set)
   -o|--os     : distribution (example: centos:centos6)
   -s|--shared : shared folder that will be mounted on the docker instance
      --spec   : the specfile to be used for building the rpm packages
   -t|--target : the directory where to copy the rpm packages
   -v|--pckver : the package version
   -g|--gid    : group ID of the user 'developer' used for building the software
   -u|--uid    : user ID of the user 'developer' used for building the software

Supported distributions:
   CentOS 5-8
   CentOS Stream 8, 9
   Debian 9-12
   Fedora 33-38/rawhide
   Rocky Linux 8, 9

Example:
       $0 -s $PROGPATH/../../nagios-plugins-linux:/shared:rw \\
          --spec specs/nagios-plugins-linux.spec \\
          -t pcks -d mamba -g 100 -u 1000 -o centos:latest
       $0 -s $PROGPATH/../../nagios-plugins-linux:/shared:rw \\
          -t pcks -d mamba -v 20 -o debian:jessie

__EOF
}

help () {
   cat <<__EOF
$PROGNAME v$REVISION - containerized software build checker
Copyright (C) 2016-2023 Davide Madrisan <davide.madrisan@gmail.com>

__EOF

   usage
}

while test -n "$1"; do
   case "$1" in
      --help|-h) help; exit 0 ;;
      --version|-V)
         echo "$PROGNAME v$REVISION"
         exit 0 ;;
      --distro|-d)
         usr_distro="$2"; shift ;;
      --gid|-g)
         usr_gid="$2"; shift ;;
      --os|-o)
         usr_os="$2"; shift ;;
      --shared|-s)
         usr_disk="$2"; shift ;;
      --spec)
         usr_specfile="$2"; shift ;;
      --target|-t)
         usr_targetdir="$2"; shift ;;
      --pckver|-v)
         usr_pckver="$2"; shift ;;
      --uid|-u)
         usr_uid="$2"; shift ;;
      --*|-*) die "unknown argument: $1" ;;
      *) die "unknown option: $1" ;;
    esac
    shift
done

[ "$usr_disk" ] || { usage; exit 1; }
[ "$usr_os" ] || { usage; exit 1; }
[ "$usr_specfile" ] &&
 { [ -r "$usr_specfile" ] || die "no such file: $usr_specfile"; }
[ "$usr_pckver" ] || { usage; exit 1; }

# parse the shared disk string
IFS_save="$IFS"
IFS=":"; set -- $usr_disk
shared_disk_host="$(readlink -f "$1")"
shared_disk_container="$2"
IFS="$IFS_save"

([ "$shared_disk_host" ] && [ "$shared_disk_container" ]) ||
   die "bad syntax for --shared"

if [ "$usr_specfile" ]; then
   specfile="$(readlink -f "$usr_specfile")"
   case "$specfile" in
      ${shared_disk_host}*)
          specfile="./${specfile#$shared_disk_host}" ;;
      *) die "the specfile must be in $shared_disk_host" ;;
   esac
fi
if [ "$usr_targetdir" ]; then
   targetdir="$(readlink -m "$usr_targetdir")"
   case "$targetdir" in
      ${shared_disk_host}*)
         targetdir="$shared_disk_container/${targetdir#$shared_disk_host}" ;;
      *) die "the target dir must be in $shared_disk_host" ;;
   esac
fi

msg "instantiating a new container based on $usr_os ..."
container="$(container_create --random-name --os "$usr_os" \
                --disk "$shared_disk_host:$shared_disk_container")" ||
   die "failed to create a new container with os $usr_os"

container_start "$container"

ipaddr="$(container_property --ipaddr "$container")"
os="$(container_property --os "$container")"

case "$os" in
   alpine-*)
      pck_format="apk"
      pck_install="apk add"
      pcks_dev="alpine-sdk curl-dev"
      have_libcurl="1"
      have_libvarlink="0"
   ;;
   centos-*)
      pck_format="rpm"
      pck_install="yum install -y"
      pck_dist=".elr"
      pcks_dev="bzip2 make gcc libcurl-devel rpm-build util-linux xz"
      have_libcurl="1"
      have_libvarlink="0"
   ;;
   debian-*)
      pck_format="deb"
      pck_install="\
export DEBIAN_FRONTEND=noninteractive;
apt-get update && apt-get -y --no-install-recommends install"
      pcks_dev="\
build-essential bzip2 debhelper devscripts fakeroot gcc make pkg-config xz-utils"
      pcks_dev="$pcks_dev libcurl4-gnutls-dev" ;;
   fedora-*)
      pck_format="rpm"
      pck_install="dnf install -y"
      pck_dist=".fc${os:7:2}"
      pcks_dev="bzip2 make gcc libcurl-devel libvarlink-devel xz rpm-build util-linux"
      have_libcurl="1"
      have_libvarlink="1" ;;
   rockylinux-*)
      pck_format="rpm"
      pck_install="dnf install -y"
      pck_dist=".el${os:11:1}"
      pcks_dev="bzip2 make gcc libcurl-devel xz rpm-build util-linux"
      have_libcurl="1"
      have_libvarlink="0" ;;
   *) die "unsupported os: $os" ;;
esac
pck_dist="${pck_dist}${usr_distro:+.$usr_distro}"

pckname="nagios-plugins-linux"

echo "\
Container \"$container\"  status:running  ipaddr:$ipaddr  os:$os
"

if [ "$have_libcurl" = "0" ]; then
   msg "the docker plugin will not be built because libcurl is too old."
else
   msg "the available curl library allows the docker plugin to be built."
fi

msg "testing the build process inside $container ..."
container_exec_command "$container" "\
# fixes for debian 6 (squeeze)
[ -r /etc/debian_version ] &&
case \"\$(cat /etc/debian_version 2>/dev/null)\" in
   6.*) sed -i 's,httpredir,archive,g;
                /squeeze-updates/d;' /etc/apt/sources.list ;;
esac

# install the build prereqs
$pck_install $pcks_dev

# create a non-root user for building the software (developer) ...
case $os in
   alpine-*)
      addgroup -g 1000 developers
      adduser -D -G developers -u 1000 -s /bin/sh developer
      addgroup developer abuild
      #abuild-keygen -a -i
   ;;
   *) groupadd -g $usr_gid developers
      useradd -m -g $usr_gid -u $usr_uid -s /bin/bash developer
   ;;
esac

# ... and switch to this user
su - developer -c '
msg () { echo \"*** info: \$1\"; }

mkdir -p $targetdir

if [ \"'$pck_format'\" = apk ]; then
   mkdir -p ~/${pckname}
   cp -p $shared_disk_container/${pckname}*.tar.* ~/${pckname}
   cp -p $shared_disk_container/$specfile ~/${pckname}

   msg \"creating the required abuild certificates ...\"
   abuild-keygen -n -a
   cat .abuild/abuild.conf
   echo
   find /home/developer/.abuild/ -name \"*rsa*\" -exec cat {} \\;
   echo

   msg \"creating the apk packages ...\"
   cd ~/${pckname}
   abuild checksum
   abuild -r

   if [ \"'$targetdir'\" ]; then
      msg \"copying the apk packages to the target folder ...\"
      cp -p ../packages/developer/$(arch)/*.apk $targetdir
   fi
elif [ \"'$pck_format'\" = rpm ] && [ \"'$specfile'\" ]; then
   mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
   cp -p $shared_disk_container/$specfile ~/rpmbuild/SPECS/
   cp -p $shared_disk_container/${pckname}*.tar.* ~/rpmbuild/SOURCES/

   msg \"creating the rpm packages ...\"
   pushd ~/rpmbuild/SPECS/ &>/dev/null
   rpmbuild \
      --define=\"dist $pck_dist\" \
      --define=\"_topdir \$HOME/rpmbuild\" \
      --define=\"have_libcurl $have_libcurl\" \
      --define=\"have_libvarlink $have_libvarlink\" \
      -ba ${pckname}.spec

   msg \"testing the installation of the rpm packages ...\"
   rpm --test -hiv ../RPMS/*/*.rpm || exit 1

   if [ \"'$targetdir'\" ]; then
      msg \"copying the rpm packages to the target folder ...\"
      cp -p ../SRPMS/*.src.rpm ../RPMS/*/*.rpm $targetdir
   fi
   popd &>/dev/null
elif [ \"'$pck_format'\" = deb ]; then
   msg \"creating the origin package ${pckname}_${usr_pckver}.orig.tar.xz...\"
   mkdir -p ~/debian-build
   cp $shared_disk_container/${pckname}-${usr_pckver}.tar.xz \
      ~/debian-build/${pckname}_${usr_pckver}.orig.tar.xz

   export PATH=\$PATH:/usr/sbin:/sbin

   msg \"creating the deb package and build files for version ${usr_pckver}...\"
   cd ~/debian-build
   tar xf ${pckname}_${usr_pckver}.orig.tar.xz
   cd ~/debian-build/${pckname}-${usr_pckver}
   debuild -us -uc || exit 1
   msg \"testing the installation of the deb package(s) ...\"
   dpkg --dry-run -i ../*.deb || exit 1

   if [ \"'$targetdir'\" ]; then
      msg \"copying the debian packages to the target folder ...\"
      cp -p ../*.{changes,deb,dsc,orig.tar.*} $targetdir
   fi
fi
'"

msg "removing the temporary container ..."
container_remove "$container"
