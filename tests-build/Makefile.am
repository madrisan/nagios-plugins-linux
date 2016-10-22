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

MULTIBUILD_SCRIPT = $(multibuilddir)/multibuild.sh
MULTIBUILD_OPTS = -s $(top_srcdir):/shared:rw \
	--spec $(multibuilddir)/specs/$(SPECFILE)

check-build-centos5: $(DOCKER_BINARY) $(BUILD_SCRIPT)
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) -o centos:centos5

check-build-centos6: $(DOCKER_BINARY) $(BUILD_SCRIPT)
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) -o centos:centos6

check-build-centos7: check-build-latest

check-build-latest: $(DOCKER_BINARY) $(BUILD_SCRIPT)
	$(SHELL) $(MULTIBUILD_SCRIPT) $(MULTIBUILD_OPTS) -o centos:latest
