#!/bin/bash
# Helper functions for using Docker in shell scripts
# Copyright (C) 2016,2017 Davide Madrisan <davide.madrisan@gmail.com>

# Here's is a simple example of how the library functions can be used!
#
#    container_create --name centos7 --os "centos:latest" \
#                     --disk ~/docker-datadisk:/shared:rw
#    container_start centos7
#    echo "The running OS is: $(container_property --os centos7)"
#    container_exec_command centos7 "echo 'Hello World!'"
#    container_remove centos7

docker_bash_helpers_revision="3"

# 'private' definitions and functions

__PROGNAME="${0##*/}"
__die() { echo "${__PROGNAME}: error: $1" 1>&2; exit 1; }
__isnotempty() { [ "$1" ] && return 0 || return 1; }
__validate_input() {
   [ "$2" ] || __die "$1 requires an argument (container name)"
}

# public helper functions

container_id() {
   # doc.desc: return the _Docker Id_ of a container
   # doc.args: container name
   __validate_input "${FUNCNAME[0]}" "$1"
   sudo docker inspect --format='{{.Id}}' "$1" 2>/dev/null
}

container_exists() {
   # doc.desc: return true if the container exists, and false otherwise
   # doc.args: container name
   __validate_input "${FUNCNAME[0]}" "$1"
   __isnotempty "$(container_id "$1")"
}

container_is_running() {
   # doc.desc: return true if the container is running, and false otherwise
   # doc.args: container name
   __validate_input "${FUNCNAME[0]}" "$1"
   __isnotempty "$(\
sudo docker ps -q --filter "name=$1" --filter 'status=running' 2>/dev/null)"
}

container_stop() {
   # doc.desc: stop a container, if it's running
   # doc.args: container name
   __validate_input "${FUNCNAME[0]}" "$1"
   container_is_running "$1" && sudo docker stop "$1"
}

container_remove() {
   # doc.desc: stop and remove a container from the host node
   # doc.args: container name
   __validate_input "${FUNCNAME[0]}" "$1"
   container_stop "$1" >/dev/null &&
      sudo docker rm -f "$1" >/dev/null
}

container_exec_command() {
   # doc.desc: run a command (or a sequence of commands) inside a container
   # doc.args: container name
   __validate_input "${FUNCNAME[0]}" "$1"
   sudo docker exec -it "$1" /bin/bash -c "$2"
}

container_property() {
   # doc.desc: get a container property
   # doc.args: container name and one of the options: --id, --ipaddr, --os
   local container_name
   local property

   while test -n "$1"; do
      case "$1" in
         --id) property="id" ;;
         --ipaddr) property="ipaddr" ;;
         --os) property="os" ;;
         --*|-*) __die "${FUNCNAME[0]}: unknown switch \"$1\"" ;;
         *) container_name="$1" ;;
      esac
      shift
   done

   __validate_input "${FUNCNAME[0]}" "$container_name"
   container_exists "$container_name" || {
      echo "unknown-no_such_container"; return; }
   container_is_running "$container_name" || {
      echo "unknown-not_running"; return; }

   case "$property" in
      "id")
          container_id "$container_name" ;;
      "ipaddr")
          sudo docker inspect \
             --format '{{ .NetworkSettings.IPAddress }}' \
             "$container_name" 2>/dev/null ;;
      "os")
          local os="unknown-os"
          # CentOS release 6.8 (Final)
          # CentOS Linux release 7.2.1511 (Core)
          # Fedora release 24 (Twenty Four)
          local -r container_os="$(\
container_exec_command "$container_name" "\
   if [ -r /etc/redhat-release ]; then
      cat /etc/redhat-release
   elif [ -r /etc/os-release ]; then
      . /etc/os-release
      echo \$ID \$VERSION_ID
   elif [ -r /etc/debian_version ]; then
      echo -n 'debian '
      cat /etc/debian_version
   fi")"
          set -- $container_os
          if [ "$1" = "CentOS" ]; then
             [ "$2" = "Linux" ] && os="centos-${4}" || os="centos-${3}"
          elif [ "$1" = "Fedora" ]; then
             os="fedora-${3}"
          elif [ "$1" = "debian" ]; then
             os="debian-${2}"
          fi
          echo "$os"
      ;;
      *) __die "${FUNCNAME[0]}: unknown property \"$property\"" ;;
   esac
}

container_create() {
   # doc.desc: create a container and return its name
   # doc.args: --name (or --random-name), --os and a host folder (--disk) to map
   # doc.example: container_create --random-name --os "centos:latest" --disk ~/docker-datadisk:/shared:rw
   local disk=
   local name=
   local os=

   local random_name=0

   while test -n "$1"; do
      case "$1" in
         --disk) disk="$2"; shift ;;
         --name) name="$2"; shift ;;
         --random-name) random_name=1 ;;
         --os) os="$2"; shift ;;
         --*|-*) __die "${FUNCNAME[0]}: unknown switch \"$1\"" ;;
         *) __die "${FUNCNAME[0]}: unknown option(s): $*" ;;
      esac
      shift
   done

   [ "$os" ] || __die "${FUNCNAME[0]}: --os has not been set"
   if [ "$random_name" = 1 ]; then
      name="${os/:/.}_$(</dev/urandom tr -dc _A-Z-a-z-0-9 | head -c8)"
   elif [ -z "$name" ]; then
      __die "${FUNCNAME[0]}: --name has not been set"
   fi

   if ! (container_exists "$name" ||
      sudo docker run -itd --name="$name" ${disk:+-v $disk} "$os" \
         "/bin/bash" >/dev/null); then
      __die "${FUNCNAME[0]}: cannot instantiate the container $name"
   fi
   echo "$name"
}

container_start() {
   # doc.desc: start a container if not running
   # doc.args: container name
   container_is_running "$1" || sudo docker start "$1"
}

container_status_table() {
   # doc.desc: return the status of a container
   # doc.args: container name of no args to list all the containers
   [ "$1" ] && { sudo docker ps --filter "name=$1"; return; }
   sudo docker ps -a
}

container_list() {
   # doc.desc: return the available container name
   # doc.args: none
   sudo docker ps -qa --format='{{.Names}}' 2>/dev/null
}
