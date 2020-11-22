## Process this file with automake to produce Makefile.in

## Copyright (c) 2016-2019 Davide Madrisan <davide.madrisan@gmail.com>
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

DOCKER = /usr/bin/docker
SPECFILE = $(PACKAGE).spec

# GID and UID of the user 'developer'
GID ?= "$(shell id -g)"
UID ?= "$(shell id -u)"

MULTIBUILD_SCRIPT = $(multibuilddir)/multibuild.sh

MULTIBUILD_OPTS = \
	--shared $(top_srcdir):/shared:rw \
	--gid $(GID) \
	--uid $(UID) \
	--pckver $(PACKAGE_VERSION)

TARGETS_ALPINE = \
	alpine-3.12
alpine-latest: alpine-3.12

TARGETS_DEBIAN = \
	debian-jessie debian-stretch debian-buster
debian-8: debian-jessie
debian-9: debian-stretch
debian-10: debian-buster
debian-latest: debian-buster

TARGETS_REDHAT = \
	centos-5 centos-6 centos-7 centos-8 \
	fedora-31 fedora-32 fedora-33 \
	fedora-rawhide
centos-latest: centos-8
fedora-latest: fedora-33

.PHONY: specfile
specfile:
	@cd $(builddir)/specs && $(MAKE) $(AM_MAKEFLAGS) all

.PHONY: source-archive
source-archive:
	@cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) dist

$(TARGETS_ALPINE): source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --spec $(multibuilddir)/specs/APKBUILD \
	   --os $$distr --target pcks/$@

$(TARGETS_DEBIAN): source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`-slim; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --os $$distr --target pcks/$@

$(TARGETS_REDHAT): specfile source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --spec $(multibuilddir)/specs/$(SPECFILE) \
	   --os $$distr --target pcks/$@

EXTRA_DIST = docker-shell-helpers multibuild.sh
SUBDIRS = specs
