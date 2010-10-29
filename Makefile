#
# Makefile - The main makefile for mod_gfx
# Copyright (C) 2010 Michael Spiegle (mike@nauticaltech.com)
# 
# This file is part of mod_gfx.
# 
# mod_mcd is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# mod_mcd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# user configurables
APXS=/usr/sbin/apxs
LIBS=-lgd
MODULE=mod_gfx
SRCDIR=src
GCCFLAGS="-g3 -O0 -Iinclude -Wall"

# don't touch below here
OUTPUT=$(SRCDIR)/$(MODULE).so
SOURCE=$(SRCDIR)/$(MODULE).c

all: $(MODULE).so

mod_gfx.so:
	$(APXS) -c $(LIBS) -o $(OUTPUT) -Wc,$(GCCFLAGS) $(SOURCE)

install:
	sudo $(APXS) -i -n gfx_module $(SRCDIR)/.libs/mod_gfx.so

clean:
	rm -f $(SRCDIR)/$(MODULE).la
	rm -f $(SRCDIR)/$(MODULE).lo
	rm -f $(SRCDIR)/$(MODULE).o
	rm -f $(SRCDIR)/$(MODULE).slo
	rm -rf $(SRCDIR)/.libs

run:
	sudo gdb -d /home/mspiegle/tmp/httpd-source --args /usr/sbin/apache2 -X -D DEFAULT_VHOST -D PHP5 -D PROXY -d /usr/lib64/apache2 -f /etc/apache2/httpd.conf -k start
	sudo killall apache2 2>&1 >/dev/null

debug:
	$(MAKE) clean && $(MAKE) && $(MAKE) install && $(MAKE) run

# vim: ts=2
