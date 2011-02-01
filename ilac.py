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

import _ilac
import os
import os.path
import shutil
import sys
import logging

def ilac_classify_file( from_file_name, size1, size2, to_dir ):
    """
    from_file_name = Full path of the image file
    size1 = The largets size
    size2 = The smallest size
    to_dir = Full path of the destination dir
    """
    # Let the exception go to the caller.
    image_id = _ilac.get_image_id( from_file_name, size1, size2 )

    # Create id string that will be the dir name.
    image_id_dir = ""
    for dirpart in image_id:
        image_id_dir = image_id_dir + str(dirpart)

    # Make sure the "new" to_file_dir exists.
    to_file_dir = os.path.join(to_dir, image_id_dir)
    if ( not os.path.isdir( to_file_dir ) ):
        os.mkdir( to_file_dir )

    # We need the full to_file_name path
    to_file_name = os.path.join(to_file_dir, os.path.basename(from_file_name))

    # shutil uses rename if on the same fielsystem (python doc), copy2 otherwise
    shutil.move( from_file_name, to_file_name )

    # tell the user about the move
    ilaclog.debug( "Moved %s to %s" % (from_file_name, to_file_name) )


def ilac_classify_dir( from_dir, to_dir, size1, size2 ):
    """
    from_dir = Full path of the source dir.
    to_dir = Full path of the destination dir.
    size1 = The largest size.
    size2 = The smallest size.
    """
    #FIXME: Check that the two dirs exist.
    # classify file for all the /root/files
    for root, dirs, files in os.walk(from_dir):
        for file in files:
            try:
                ilac_classify_file(os.path.join(root,file), size1, size2, to_dir)
            except Exception, err:
                ilaclog.err( err )

def initLogger():
    Logger = logging.getLogger("ilac")
    Logger.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)

    #FIXME: Add a file log when we add the config file.
    #handler = logging.FileHandler(config.log.filename)

    formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
    handler.setFormatter(formatter)
    Logger.addHandler(handler)

initLogger()
ilaclog = logging.getLogger("ilac")
