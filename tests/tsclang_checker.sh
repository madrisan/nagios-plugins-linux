#!/bin/bash 
# Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com> 
 
__PROGNAME="${0##*/}" 
 
TEST_SKIP=77 
OUTPUT_DIR="/tmp/clang-checker-analysis" 
 
SCAN_BUILD_COMMAND="make clean all" 
SCAN_BUILD_COMMAND_PREPEND="autoreconf -v -Wall -i -Im4 && ./configure" 
 
function __die() { 
   echo "${__PROGNAME}: error: $1" 1>&2 
   exit ${2:-0} 
} 
 
[ -x /usr/bin/scan-build ] || 
   die "program \"scan-build\" not found in path" ${TEST_SKIP} 
 
echo "Running Clang Static Analyzer..." 
 
pushd .. >/dev/null 
scan-build -analyze-headers -o ${OUTPUT_DIR} ${SCAN_BUILD_COMMAND} 
popd >/dev/null 
 
exit 0
