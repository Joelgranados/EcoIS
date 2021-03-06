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

def ilac_classify_file( from_file_name, size1, size2, to_dir, camMat, disMat,
    sqrSize = 10, sphSize = 40):
    """ Sorts files without processing them
    from_file_name = Full path of the image file
    size1 = Largets chessboard size
    size2 = Smallest chessboard size
    to_dir = Full path of the destination dir
    """
    # tell the user when we start.
    ilaclog.debug( "ilac_classify_file, from_file:%s, to_dir:%s" \
            % (from_file_name, to_dir) )

    # Let the exception go to the caller.
    cb = _ilac.IlacCB( from_file_name, size1, size2, camMat, disMat,
        sqrSize, sphSize )

    # Create id string that will be the dir name.
    image_id_dir = ""
    for dirpart in cb.img_id():
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


def ilac_classify_dir( from_dir, to_dir, size1, size2, camMat, distMat ):
    """  Sorts all files contained an a directory without processing them.
    from_dir = Full path of the source dir.
    to_dir = Full path of the destination dir.
    size1 = Largest chessboard size.
    size2 = Smallest chessboard size.
    camMat = Camera intrinsics
    disMat = Distortion values.
    """
    #Check that the two dirs exist.
    for dir in [from_dir, to_dir]:
        if not os.path.isdir(dir):
            raise ILACDirException(dir)

    # classify file for all the /root/files
    for root, dirs, files in os.walk(from_dir):
        for file in files:
            try:
                ilac_classify_file( os.path.join(root,file), \
                                    size1, size2, to_dir, camMat, disMat )
            except Exception, err:
                ilaclog.error( err )

def ilac_process_classify_dir ( from_dir, to_dir, \
                                size1, size2, camMat, disMat, \
                                sqrSize = 10, sphSize = 40):
    """ Classify all files in a directory and normalize the images
    from_dir = Source dir (full path)
    to_dir = Dest dir (full path)
    size1 = Largest chessboard size.
    size2 = Smallest chessboard size.
    camMat = Camera intrinsics.
    disMat = Distortion values.
    """
    #Check that the two dirs exist.
    for dir in [from_dir, to_dir]:
        if not os.path.isdir(dir):
            raise ILACDirException(dir)

    for root, dirs, files in os.walk(from_dir):
        for f in files:
            try:
                from_file_name = os.path.join(root, f)
                cb = _ilac.IlacCB( from_file_name, size1, size2,
                    camMat, disMat, sqrSize, sphSize )
            except Exception, err:
                ilaclog.error( "File(%s): %s"%(from_file_name, err) )
                continue

            # Create id string that will be the dir name.
            id_dir = ""
            for dirpart in cb.img_id():
                id_dir = id_dir + str(dirpart)

            # Make sure the "new" to_file_dir exists.
            to_file_dir = os.path.join(to_dir, id_dir)
            if ( not os.path.isdir( to_file_dir ) ):
                os.mkdir( to_file_dir )

            # We need the full to_file_name path
            to_file_name = os.path.join(to_file_dir, f)
            cb.process_image(to_file_name)

            # tell the user about the move
            ilaclog.debug("Moved %s to %s"%(from_file_name, to_file_name))

def ilac_calc_intrinsics ( img_dir, size1, size2 ):
    filenames = [];
    for img_file in os.listdir(img_dir):
        if os.path.isfile( os.path.join(img_dir,img_file) ):
            filenames.append ( os.path.join(img_dir,img_file) )
    return _ilac.calc_intrinsics ( filenames, size1, size2 )

class ILACException(Exception):
    def __init__(self):
        pass
    def __str__(self):
        return ("ILAC_ERROR: %s" % self.message)
class ILACDirException(ILACException):
    def __init__(self, dir_name):
        self.message = "The directory %s was not found." % dir_name

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
