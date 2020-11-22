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

INFILES := $(wildcard APKBUILD.in *.spec.in)
OUTFILES := $(INFILES:.in=)

PACKAGE_RELEASE = 1

SUFFIXES = .in
.in:; @echo "Generating $@...";\
	sed "s,@package@,$(PACKAGE_NAME),g;\
	     s,@version@,$(PACKAGE_VERSION),g;\
	     s,@release@,$(PACKAGE_RELEASE),g;\
	     s,@plugindir@,$(PLUGINDIR),g;\
	     s,@date@,`LC_ALL="C" date "+%a %b %d %Y"`," $< > $@

EXTRA_DIST = $(INFILES)

all: $(OUTFILES)

clean-local:
	rm -f $(OUTFILES)
