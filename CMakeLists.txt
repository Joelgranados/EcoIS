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

cmake_minimum_required (VERSION 2.8)

project (ImageLabel)

# Set the version number
set (ILAC_VER_MAJOR 1)
set (ILAC_VER_MINOR 1)
set (ILAC_NAME "\"ILAC Image Labeling and Classifying\"")

# Create the ia_config.h file.  Copy <from> <to> and make the modifications
configure_file (
    "${PROJECT_SOURCE_DIR}/include/ilacConfig.h.in"
    "${PROJECT_BINARY_DIR}/ilacConfig.h"
    )

# Add the binary tree so we can find the include files.
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/include")

# Try to find the opencv stuff
find_package (OpenCV REQUIRED)
find_package (PythonLibs REQUIRED)

# for including python
include_directories(${PYTHON_INCLUDE_PATH})

# add an option for debug.
option(DEFINE_DEBUG "Build using debugging flags." OFF)
if(DEFINE_DEBUG)
    message("Adding Debug flags...")
    set(CMAKE_BUILD_TYPE Debug)
    message("Build type is " ${CMAKE_BUILD_TYPE})
endif(DEFINE_DEBUG)

add_library (_ilac SHARED src/ilacLabeler.cpp src/ilacChess.cpp src/_ilac.cpp)
set_target_properties (_ilac PROPERTIES PREFIX "") #get rid of the lib*

# Make sure we link to the found opencv stuff.
target_link_libraries (_ilac ${OpenCV_LIBS})
