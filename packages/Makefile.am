## Process this file with automake to produce Makefile.in

## Copyright (c) 2016-2024 Davide Madrisan <davide.madrisan@gmail.com>
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
	alpine-3.18 \
	alpine-3.19 \
	alpine-3.20
alpine-latest: alpine-3.20

TARGETS_CENTOS_STREAM = \
	centos-stream-8 centos-stream-9
centos-stream-latest: centos-stream-9

TARGETS_DEBIAN = \
	debian-buster debian-bullseye debian-bookworm
debian-10: debian-buster
debian-11: debian-bullseye
debian-12: debian-bookworm
debian-latest: debian-bookworm

TARGETS_FEDORA = \
	fedora-38 fedora-39 fedora-40 \
	fedora-rawhide
fedora-latest: fedora-40

TARGETS_ROCKY = \
	rockylinux-8 \
	rockylinux-9
rockylinux-latest: rockylinux-9

.PHONY: specfile
specfile:
	@cd $(builddir)/specs && $(MAKE) $(AM_MAKEFLAGS) all

.PHONY: source-archive
source-archive:
	@cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) dist

$(TARGETS_ALPINE): specfile source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --spec $(multibuilddir)/specs/APKBUILD \
	   --os $$distr --target pcks/$@

$(TARGETS_CENTOS_STREAM): specfile source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/centos-stream-//\;s~^~docker.io/tgagor/centos:stream~`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --spec $(multibuilddir)/specs/$(SPECFILE) \
	   --os $$distr --target pcks/$@

$(TARGETS_DEBIAN): source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`-slim; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --os $$distr --target pcks/$@

$(TARGETS_FEDORA): specfile source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --spec $(multibuilddir)/specs/$(SPECFILE) \
	   --os $$distr --target pcks/$@

$(TARGETS_ROCKY): specfile source-archive $(DOCKER) $(MULTIBUILD_SCRIPT)
	@distr=`echo $@ | sed s/-/:/`; \
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) \
	   --spec $(multibuilddir)/specs/$(SPECFILE) \
	   --os $$distr --target pcks/$@

EXTRA_DIST = docker-shell-helpers multibuild.sh
SUBDIRS = specs
