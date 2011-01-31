#
# ILAC: Image labeling and Classifying
# Copyright (C) 2011 Joel Granados <joel.granados@gmail.com>
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
#
PYVER  := $(shell python -c 'import sys; print sys.version[0:3]')
PYTHON = python$(PYVER)
PYTHONINCLUDE = /usr/include/$(PYTHON)

CXXFLAGS += `pkg-config --cflags opencv`
CXXFLAGS += -I$(PYTHONINCLUDE)

LIBS += `pkg-config --libs opencv`
LIBS += -shared -fPIC
SRCS = ilacSquare.cpp _ilac.cpp
DEBUGFLAGS += -pg -fprofile-arcs -ftest-coverage

all: ctag
	$(CXX) $(LIBS) $(CXXFLAGS) $(SRCS) -o _ilac.so

debug: ctag
	$(CXX) $(LIBS) $(CXXFLAGS) $(DEBUGFLAGS) -g $(SRCS) -o _ilac.so

ctag:
	ctags -R *

clean:
	rm -rf *.gcda *.gcno *.out *.so *.pyc
