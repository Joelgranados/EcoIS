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
import pyexiv2
import math
from markerCreator import plotMarker

def ilac_classify_file( from_file_name, size1, size2, to_dir,
        camMat=[[1,0,0],[0,1,0],[0,0,1]], disMat=[0,0,0,0,0],
        sqrSize = 10, sphSize = 40):
    """ Sorts files without processing them
    from_file_name = Full path of the image file
    size1 = # chessboard intersections (largest)
    size2 = # chessboard intersections (smalles)
    to_dir = Full path of the destination dir
    camMat = Camera intrinsics
    disMat = Distortion values.
    sqrSize = Size of one square in the chessboard
    sphSize = Size of the sphere

    Units of sqrSize and sphSize need to be the same.
    """
    # tell the user when we start.
    ilaclog.debug( "ilac_classify_file, from_file:%s, to_dir:%s" \
            % (from_file_name, to_dir) )

    # Let the exception go to the caller.
    cb = _ilac.IlacCB( from_file_name, size1, size2, camMat, disMat,
        sqrSize, sphSize )

    # Make sure the "new" to_file_dir exists.
    image_id_dir = str(cb.img_id())
    to_file_dir = os.path.join(to_dir, image_id_dir)
    if ( not os.path.isdir( to_file_dir ) ):
        os.mkdir( to_file_dir )

    # We need the full to_file_name path
    to_file_name = os.path.join(to_file_dir, \
            os.path.basename(from_file_name))

    # shutil uses rename on same fielsystem (doc), copy2 otherwise
    shutil.move( from_file_name, to_file_name )

    # tell the user about the move
    ilaclog.debug( "Moved %s to %s" % (from_file_name, to_file_name) )


def ilac_classify_dir( from_dir, to_dir, size1, size2,
        camMat=[[1,0,0],[0,1,0],[0,0,1]], disMat=[0,0,0,0,0] ):
    """  Sorts all files contained an a directory without processing them.
    from_dir = Full path of the source dir.
    to_dir = Full path of the destination dir.
    size1 = # chessboard intersections (largest)
    size2 = # chessboard intersections (smalles)
    camMat = Camera intrinsics
    disMat = Distortion values.
    """
    #Check that the two dirs exist.
    for dir in [from_dir, to_dir]:
        if not os.path.isdir(dir):
            raise ILACDirException(dir)

    # classify file for all the /root/files
    for root, dirs, files in os.walk(from_dir):
        for f in files:
            try:
                ilac_classify_file( os.path.join(root,f), \
                                    size1, size2, to_dir, \
                                    camMat, disMat )
            except Exception, err:
                ilaclog.error( "Error in File(%s):%s"%(f,err) )
                copyToError(os.path.join(root,f), to_dir)

def ilac_process_classify_dir ( from_dir, to_dir, size1, size2,
        camMat=[[1,0,0],[0,1,0],[0,0,1]], disMat=[0,0,0,0,0],
        sqrSize = 10, sphSize = 40):
    """ Classify all files in a directory and normalize the images
    from_dir = Source dir (full path)
    to_dir = Dest dir (full path)
    size1 = # chessboard intersections (largest)
    size2 = # chessboard intersections (smalles)
    camMat = Camera intrinsics.
    disMat = Distortion values.
    sqrSize = Size of one square in the chessboard
    sphSize = Size of the sphere

    Units of sqrSize and sphSize need to be the same.
    """
    #Check that the two dirs exist.
    for dir in [from_dir, to_dir]:
        if not os.path.isdir(dir):
            raise ILACDirException(dir)

    for root, dirs, files in os.walk(from_dir):
        for f in files:
            try:
                from_file_name = os.path.join(root, f)
                ilaclog.debug("Processing file(%s)."%from_file_name)
                cb = _ilac.IlacCB( from_file_name, size1, size2,
                    camMat, disMat, sqrSize, sphSize )
            except Exception, err:
                ilaclog.error( "File(%s): %s"%(from_file_name, err) )
                copyToError(from_file_name, to_dir)
                continue

            # Make sure the "new" to_file_dir exists.
            try:
                id_dir = str(cb.img_id())
            except Exception, err:
                ilaclog.error( "File(%s): %s"%(from_file_name, err) )
                copyToError(from_file_name, to_dir)
                continue

            to_file_dir = os.path.join(to_dir, id_dir)
            if ( not os.path.isdir( to_file_dir ) ):
                os.mkdir( to_file_dir )

            # We need the full to_file_name path
            to_file_name = os.path.join(to_file_dir, f)
            try:
                cb.process_image(to_file_name)
            except Exception, err:
                ilaclog.error( "File(%s): %s"%(from_file_name, err) )
                copyToError(from_file_name, to_dir)
                continue

            # tell the user about the move
            ilaclog.debug("Moved %s to %s"% \
                    (from_file_name, to_file_name))

def ilac_calc_intrinsics ( img_dir, size1, size2 ):
    """
    img_dir = Source dir (full path)
    size1 = # chessboard intersections (largest)
    size2 = # chessboard intersections (smalles)
    """
    filenames = [];
    for img_file in os.listdir(img_dir):
        if os.path.isfile( os.path.join(img_dir,img_file) ):
            filenames.append ( os.path.join(img_dir,img_file) )
    return _ilac.calc_intrinsics ( filenames, size1, size2 )

def ilac_rename_images ( img_dir ):
    """Prefix date and time to image filename if present.
    Rename to the same img_dir."""

    # Check if directory exists.
    if not os.path.isdir(img_dir):
        raise ILACDirException(img_dir)

    for img_file in os.listdir(img_dir):
        img_pth = os.path.join(img_dir,img_file)
        im = pyexiv2.ImageMetadata(img_pth)
        try:
            im.read()
        except Exception, e:
            ilaclog.error( "File %s not found: %s"%(img_pth,e) )
            continue

        if not "Exif.Photo.DateTimeOriginal" in im.exif_keys:
            ilaclog.error("Exiv of %s is missing " \
                            "Exif.Photo.DateTimeOriginal" % (img_file,))
            continue

        d = im["Exif.Photo.DateTimeOriginal"].value
        prestr = d.strftime("%Y-%m-%d_%H-%M-%S_")
        newimg_file = prestr+img_file

        try:
            ilaclog.debug ("Renaming: %s -> %s"% \
                    (img_file, newimg_file))
            os.rename( img_pth, os.path.join(img_dir, newimg_file) )
        except Exception, e:
            ilaclog.error ( "Rename error: %s"%(e,))
            continue


def ilac_create_marker ( id, width, height, rectSize=36, dir="./" ):
    def get_filename ( width, height, id ):
        return ( "marker_width:" + str(width)
                 + "_height:" + str(height)
                 + "_id:" + str(id)
                 + "_.svg" )

    ilaclog.debug ( "Creating plot number %d." % id )
    pmfn = os.path.join ( dir, get_filename ( width, height, id ) )
    pm = plotMarker ( width, height, plid=id, svgname=pmfn )

    pmfd = open ( pmfn, "w" )
    pmfd.write ( str(pm) )
    pmfd.close()

def ilac_create_markers ( numMar, rectSize=36, dir="./" ):
    tbn = math.floor(math.log(numMar, 2))+1 # total bits needed
    # total colored squares. 7 additional for the samples.
    tsq = int(math.ceil(tbn/float(2)) + 7)
    # Try to make it a square.
    height = int(math.floor(float(tsq)/2))
    # Remember: numColoredSq = floor( (h-1)(w-1)/2 )
    width = int(math.ceil(2*tsq/float(height-1)) + 1)

    for i in range(numMar):
        ilac_create_marker ( i, width, height, rectSize=rectSize, dir=dir )

def copyToError ( file_path, to_dir ):
    # Make sure dir exists
    if not os.path.isdir(to_dir):
        raise ILACDirException(to_dir)
    if not os.path.isfile(file_path):
        raise ILACFileException(file_path)

    error_dir = os.path.join(to_dir, "error")
    if not os.path.isdir(error_dir):
        os.mkdir( error_dir )

    # shutil uses rename on same fielsystem (doc), copy2 otherwise
    shutil.copy2( file_path, error_dir )
    ilaclog.debug ( "Copied %s to error dir %s." % (file_path, error_dir) )

class ILACException(Exception):
    def __init__(self):
        pass
    def __str__(self):
        return ("ILAC_ERROR: %s" % self.message)
class ILACDirException(ILACException):
    def __init__(self, dir_name):
        self.message = "The directory %s was not found." % dir_name
class ILACFileException(ILACException):
    def __init__(self, file_name):
        self.message = "The file %s was not found." % file_name

def initLogger():
    Logger = logging.getLogger("ilac")
    if ( len(Logger.handlers) > 0 ):
        return Logger

    Logger.setLevel(logging.DEBUG)
    fstr = "%(asctime)s - %(levelname)s - %(message)s"
    formatter = logging.Formatter(fstr)

    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(formatter)
    Logger.addHandler(handler)

    # FIXME: User should be able to modify this.
    handler = logging.FileHandler("ilac.log")
    handler.setFormatter(formatter)
    Logger.addHandler(handler)

    return Logger

ilaclog = initLogger()
