#!/bin/bash
# Multi-platform build system
# Copyright (C) 2016 Davide Madrisan <davide.madrisan@gmail.com>

PROGNAME="${0##*/}"
PROGPATH="${0%/*}"
REVISION=1

die () { echo -e "$PROGNAME: error: $1" 1>&2; exit 1; }
msg () { echo "*** info: $1"; }

docker_helpers="$PROGPATH/docker-shell-helpers/docker-shell-helpers.sh"

[ -r "$docker_helpers" ] || die "no such file: $docker_helpers"
. "$docker_helpers"

usage () {
   cat <<__EOF
Usage: $PROGNAME -o <os> -s <shared> [-d <distro>] [--spec <file>] \
[-t <folder>]
       $PROGNAME --help
       $PROGNAME --version

Where:
   -d | --distro : distribution name (default: no distribution set)
   -o | --os     : distribution (example: centos:centos6)"
   -s | --shared : shared folder that will be mounted on the docker instance
        --spec   : the specfile to be used for building the rpm packages
   -t | --target : the directory where to copy the rpm packages

Example:
       $0 -s $PROGPATH/../../nagios-plugins-linux:/shared:rw \\
          --spec specs/nagios-plugins-linux.spec \\
          -t pcks -d mamba -o centos:latest

__EOF
}

help () {
   cat <<__EOF
$PROGNAME v$REVISION - containerized software build checker
Copyright (C) 2016 Davide Madrisan <davide.madrisan@gmail.com>

__EOF

   usage
}

usr_os=
usr_disk=
usr_distro=
usr_specfile=
usr_targetdir=

while test -n "$1"; do
   case "$1" in
      --help|-h) help; exit 0 ;;
      --version|-V)
         echo "$PROGNAME v$REVISION"
         exit 0 ;;
      --distro|-d)
         usr_distro="$2"; shift ;;
      --os|-o)
         usr_os="$2"; shift ;;
      --shared|-s)
         usr_disk="$2"; shift ;;
      --spec)
         usr_specfile="$2"; shift ;;
      --target|-t)
         usr_targetdir="$2"; shift ;;
      --*|-*) die "unknown argument: $1" ;;
      *) die "unknown option: $1" ;;
    esac
    shift
done

[ "$usr_disk" ] || { usage; exit 1; }
[ "$usr_os" ] || { usage; exit 1; }
[ "$usr_specfile" ] &&
 { [ -r "$usr_specfile" ] || die "no such file: $usr_specfile"; }

# parse the shared disk string
IFS_save="$IFS"
IFS=":"; set -- $usr_disk
shared_disk_host="$(readlink -f $1)"
shared_disk_container="$2"
IFS="$IFS_save"

([ "$shared_disk_host" ] && [ "$shared_disk_container" ]) ||
   die "bad syntax for --shared"


if [ "$usr_specfile" ]; then
   specfile="$(readlink -f $usr_specfile)"
   case "$specfile" in
      ${shared_disk_host}*)
          specfile="./${specfile#$shared_disk_host}" ;;
      *) die "the specfile must be in $shared_disk_host" ;;
   esac
fi
if [ "$usr_targetdir" ]; then
   targetdir="$(readlink -f $usr_targetdir)"
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
   centos*) pckmgr="yum" ;;
   centos-7.*) rpm_dist=".el7" ;;
   centos-6.*) rpm_dist=".el6" ;;
   centos-5.*) rpm_dist=".el5" ;;
   fedora*) pckmgr="dnf" ;;
   fedora-24) rpm_dist=".fc24" ;;
   *) die "FIXME: unsupported os: $os" ;;
esac
[ "$usr_distro" ] && rpm_dist="${rpm_dist}.${usr_distro}"

pckname="nagios-plugins-linux"

echo "\
Container \"$container\"  status:running  ipaddr:$ipaddr  os:$os
"

msg "testing the build process inside $container ..."
container_exec_command "$container" "\
# install the build prereqs
'$pckmgr' install -y bzip2 make gcc xz rpm-build

# create a non-root user for building the software (developer) ...
useradd -c 'Developer' developer

# ... and switch to this user
su - developer -c '
msg () { echo \"*** info: \$1\"; }

cp -pr '"$shared_disk_container"/*' .
./configure
make dist

if [ \"'$specfile'\" ]; then
   mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
   cp -p '$specfile' ~/rpmbuild/SPECS/
   cp -p ${pckname}*.tar.* ~/rpmbuild/SOURCES/

   msg \"creating the rpm packages ...\"
   pushd ~/rpmbuild/SPECS/ &>/dev/null
   rpmbuild \
      --define=\"dist $rpm_dist\" \
      --define=\"_topdir \$HOME/rpmbuild\" \
      -ba ${pckname}.spec

   msg \"testing the rpm installation ...\"
   rpm --test -hiv ../RPMS/*/*.rpm

   if [ \"'$targetdir'\" ]; then
      msg \"copying the rpm packages to the target folder ...\"
      id
      ls -ld '$targetdir'
      ls -l '$targetdir'
      ls -ld /shared
      ls -l /shared
      ls -ld /shared/tests-build
      ls -l /shared/tests-build
      mkdir -p '$targetdir' &&
      cp -p ../SRPMS/*.src.rpm ../RPMS/*/*.rpm '$targetdir'
   fi
   popd &>/dev/null
fi
'"

msg "removing the temporary container ..."
container_remove "$container"
