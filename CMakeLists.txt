# image adjust.  Automatic image normalization.
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

cmake_minimum_required (VERSION 2.8)

project (ImageLabel)

# Set the version number
set (IL_VER_MAJOR 1)
set (IL_VER_MINOR 0)

# Create the ia_config.h file.  Copy <from> <to> and make the modifications
configure_file (
    "${PROJECT_SOURCE_DIR}/ia_config.h.in"
    "${PROJECT_BINARY_DIR}/ia_config.h"
    )

# Add the binary tree so we can find the include files.
include_directories("${PROJECT_BINARY_DIR}")

# Try to find the opencv stuff
find_package (OpenCV REQUIRED)

add_executable (imagelabel imagelabel.cpp ia_input.cpp IA_Square.cpp)

# Make sure we link to the found opencv stuff.
target_link_libraries (imagelabel ${OpenCV_LIBS})
