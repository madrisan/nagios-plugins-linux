#!/bin/bash
# Multi-platform build system
# Copyright (C) 2016 Davide Madrisan <davide.madrisan@gmail.com>

PROGNAME="${0##*/}"
PROGPATH="${0%/*}"
REVISION=1

die () { echo "$PROGNAME: error: $1" 1>&2; exit 1; }
msg () { echo "*** info: $1"; }

docker_helpers="$PROGPATH/docker-shell-helpers/docker-shell-helpers.sh"

[ -r "$docker_helpers" ] || die "no such file: $docker_helpers"
. "$docker_helpers"

usage () {
   cat <<__EOF
Usage: $PROGNAME -o <os> -s <disk> [-d <distro>]
       $PROGNAME --help
       $PROGNAME --version

Where:
   -d | --distro : distribution name (default: no distribution set)
   -o | --os     : distribution (example: centos:centos6)"
   -s | --shared : shared folder that will be mounted on the docker instance

Example:
       $0 -s $PROGPATH/../../nagios-plugins-linux:/shared:rw -o centos:latest

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
      --*|-*) die "unknown argument: $1" ;;
      *) die "unknown option: $1" ;;
    esac
    shift
done

[ "$usr_disk" ] || { usage; exit 1; }
[ "$usr_os" ] || { usage; exit 1; }

# parse the shared disk string
IFS_save="$IFS"
IFS=":"; set -- $usr_disk
shared_disk_host="$(readlink -f $1)"
shared_disk_container="$2"
IFS="$IFS_save"

([ "$shared_disk_host" ] && [ "$shared_disk_container" ]) ||
   die "bad syntax for --shared"

msg "instantiating a new container based on $usr_os ..."

container="$(container_create --random-name --os "$usr_os" \
                --disk "$shared_disk_host:$shared_disk_container")" ||
   die "failed to create a new container with os $usr_os"

container_start "$container"

ipaddr="$(container_property --ipaddr "$container")"
os="$(container_property --os "$container")"

case "$os" in
   centos-7.*) rpm_dist=".el7${usr_distro:+.$usr_distro}" ;;
   centos-6.*) rpm_dist=".el6${usr_distro:+.$usr_distro}" ;;
   centos-5.*) rpm_dist=".el5${usr_distro:+.$usr_distro}" ;;
   *) die "FIXME: unsupported os: $os" ;;
esac

pckname="nagios-plugins-linux"

echo "\
Container \"$container\"  status:running  ipaddr:$ipaddr  os:$os
"

msg "buiding the packages inside the container $container ..."
container_exec_command "$container" "\

# installing packages require administrative permissions
yum install -y bzip2 make gcc xz rpm-build

# create a non-root user (developer)
useradd -c 'Developer' developer

# switch to the user developer
su - developer -c '
cp -pr '"$shared_disk_container"/*' .
./configure
make dist

mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cp -p tests-build/${pckname}.spec ~/rpmbuild/SPECS/
cp -p *.tar.* ~/rpmbuild/SOURCES/

pushd ~/rpmbuild/SPECS/ &>/dev/null
rpmbuild \
   --define=\"dist $rpm_dist\" \
   --define=\"_topdir \$HOME/rpmbuild\" \
   -ba ${pckname}.spec
popd &>/dev/null
'"

msg "removing the temporary container ..."
##container_remove "$container"
