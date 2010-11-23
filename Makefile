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

CXXFLAGS += `pkg-config --cflags opencv`
LIBS += `pkg-config --libs opencv`
SRCS = imagelabel.cpp ia_input.cpp IA_Square.cpp
DEBUGFLAGS += -pg -fprofile-arcs -ftest-coverage

all: ctag
	$(CXX) $(LIBS) $(CXXFLAGS) $(SRCS) -o imagelabel

debug: ctag
	$(CXX) $(LIBS) $(CXXFLAGS) $(DEBUGFLAGS) -g $(SRCS) -o imagelabel

ctag:
	ctags -R *

clean:
	rm -rf imagelabel imagelabel.tar.gz *.gcda *.gcno
