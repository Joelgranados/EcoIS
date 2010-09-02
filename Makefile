# image adjust.  Automatic image normalization.
# Copyright (C) 2010 Joel Granados <joel.granados@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

CFLAGS=`pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`

all: ctag
	g++ ${CFLAGS} ${LIBS} -g imageadjust.cpp ia_input.cpp ia_core.cpp -o imageadjust

ctag:
	ctags -R *

clean:
	rm -rf imageadjust imageadjust.tar.gz

# Make sure you change the cw and ch to your environemtn.
test:
	@if [ -f test.avi ] ; then \
		./imageadjust -I 10 --cw 5 --ch 8 --video test.avi; \
	fi

