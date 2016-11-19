## Process this file with automake to produce Makefile.in

## Copyright (c) 2016 Davide Madrisan <davide.madrisan@gmail.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

multibuilddir = $(shell pwd)

DOCKER_BINARY = /usr/bin/docker
SPECFILE = $(PACKAGE).spec

# GID and UID of the user 'developer'
GID ?= "$(shell id -g)"
UID ?= "$(shell id -u)"

MULTIBUILD_SCRIPT = $(multibuilddir)/multibuild.sh

MULTIBUILD_OPTS = -s $(top_srcdir):/shared:rw \
	--gid $(GID) --uid $(UID) --target pcks
MULTIBUILD_OPTS_REDHAT = $(MULTIBUILD_OPTS) \
	--spec $(multibuilddir)/specs/$(SPECFILE)

TARGETS_REDHAT = \
	centos-centos5 centos-centos6 centos-latest \
	fedora-latest fedora-rawhide

centos-centos7: centos-latest
fedora-fedora24: fedora-latest

TARGETS_DEBIAN = debian-jessie

$(TARGETS_REDHAT): $(DOCKER_BINARY) $(MULTIBUILD_SCRIPT) specs/$(SPECFILE)
	@ostarget=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS_REDHAT) -o $$ostarget

$(TARGETS_DEBIAN): $(DOCKER_BINARY) $(MULTIBUILD_SCRIPT)
	@ostarget=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) -o $$ostarget

EXTRA_DIST = docker-shell-helpers multibuild.sh
SUBDIRS = specs
