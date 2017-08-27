#!/bin/bash
# Copyright (C) 2016 Davide Madrisan <davide.madrisan@gmail.com>

# Simple script that help generating the README.md documentation

helpers_lib="docker-shell-helpers.sh"

[ -r "$helpers_lib" ] || exit 1

while read -r line; do
   set -- $line
   case "$1" in
      "function")
         fncname="$2"
         [[ "$fncname" =~ __ ]] && continue
         echo "* __${fncname/__/\\_\\_}__" ;;
      "#")
         [[ "$2" =~ doc. ]] || continue
         shift
         label="${1/doc./}"
         shift
         echo "  * _${label}_ $*" ;;
      "}")
         unset fncname ;;
   esac
done < "$helpers_lib"
