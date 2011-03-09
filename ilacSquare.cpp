/*
 * ILAC: Image labeling and Classifying
 * Copyright (C) 2011 Joel Granados <joel.granados@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "ilacSquare.h"
#include "ilacLabeler.h"
#include <math.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <sys/stat.h>

ILAC_ChessboardImage::ILAC_ChessboardImage (){}/*Used to initialize.*/

ILAC_ChessboardImage::ILAC_ChessboardImage ( const string &image,
                                             const Size &boardsize,
                                             const Mat &camMat,
                                             const Mat &disMat )
{
  this->camMat = camMat;
  this->disMat = disMat;
  this->image_file = image;
  Size tmpsize = boardsize; /* we cant have a const in check_input*/
  check_input ( image, tmpsize );
  this->boardSize = tmpsize;
  init_chessboard ();
}

void //static method
ILAC_ChessboardImage::check_input ( const string &image, Size &boardSize )
{
  // Check that file exists.
  struct stat file_stat;
  if ( stat ( image.data(), &file_stat ) != 0 )
    throw ILACExFileError();

  // Check for width > height
  if ( boardSize.height > boardSize.width )
  {
    unsigned int temp = boardSize.height;
    boardSize.height = boardSize.width;
    boardSize.width = temp;
  }

  /* We need a chessboard of odd dimensions (6,5 for example).  This gives us
   * a chessboard with only one symmetry axis.  We use this in order to identify
   * a unique origin. <ISBN 978-0-596-51613-0 Learning Opencv page 382> */
  if ( boardSize.height % 2 == boardSize.width % 2 )
    throw ILACExSymmetricalChessboard();
}

/*
 * Function steps:
 * 1. TRY TO NORMALIZE IMAGE.
 * 2. CALCULATE IMAGE CHESSBOARD POINTS.
 * 3. CALCULATE THE IMAGE ID.
 */
void
ILAC_ChessboardImage::init_chessboard ()
{
  /* 1. TRY TO NORMALIZE IMAGE. */
  Mat temp;
  orig_img = imread ( this->image_file );
  undistort ( orig_img , temp, camMat, disMat ); //always undistort

  /* 2. CALCULATE CHESSBOARD POINTS.*/
  imageCBpoints = ILAC_ChessboardImage::get_image_points (orig_img, boardSize);

  /* 3. CALCULATE IMAGE ID */
  ILAC_Labeler labeler ( orig_img, imageCBpoints, boardSize );
  id = labeler.calculate_label();
}

void
ILAC_ChessboardImage::process_image ( const string filename_output,
                                      const unsigned int sizeInPixels )
{
  Mat final_img = Mat::zeros( 1, 1, CV_32F );
  Mat aftr;

  /* create triangle vertices */
  Point2f tvsrc[3] = { imageCBpoints[ boardSize.width*(boardSize.height-1) ],
                       imageCBpoints[ 0 ],
                       imageCBpoints[ boardSize.width - 1 ] };
  Point2f tvdst[3] = {
    Point2f ( 0, orig_img.size().height ),
    Point2f ( 0, orig_img.size().height - boardSize.height*sizeInPixels ),
    Point2f ( boardSize.width*sizeInPixels,
              orig_img.size().height - boardSize.height*sizeInPixels ) };

  aftr = getAffineTransform ( tvsrc, tvdst );
  warpAffine ( orig_img, final_img, aftr, orig_img.size() );

  imwrite ( filename_output, final_img );
}

vector<unsigned short>
ILAC_ChessboardImage::get_image_id ()
{
  return id;
}

/* Helper function for process_image. */
double //static function
ILAC_ChessboardImage::rad2deg ( const double Angle )
{
    static double ratio = 180.0 / 3.141592653589793238;
      return Angle * ratio;
}

vector<Point2f>// static method
ILAC_ChessboardImage::get_image_points ( const Mat& image,
                                         const Size boardSize )
{
  Mat g_img; //temp gray image
  vector<Point2f> retvec;

  try
  {
    cvtColor ( image, g_img, CV_BGR2GRAY );/* transform to grayscale */

    /* find the chessboard points in the image and put them in retvec.*/
    if ( !findChessboardCorners(g_img, boardSize, (retvec),
                                CV_CALIB_CB_ADAPTIVE_THRESH) )
      throw ILACExNoChessboardFound();

    else
      /* The 3rd argument is of interest.  It defines the size of the subpix
       * window.  window_size = NUM*2+1.  This means that with 5,5 we have a
       * window of 11x11 pixels.  If the window is too big it will mess up the
       * original corner calculations for small chessboards. */
      cornerSubPix ( g_img, (retvec), Size(5,5), Size(-1,-1),
                     TermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1) );
  }catch (cv::Exception){throw ILACExNoChessboardFound();}

  return retvec;
}


/*
 * 1. CREATE IMAGEPOINTS.
 * 2. CREATE OBJECTPOINTS.
 * 3. CALL CALIBRATE CAMERA.
 */
void //static method.
ILAC_ChessboardImage::calc_img_intrinsics ( const vector<string> images,
                                            const unsigned int size1,
                                            const unsigned int size2,
                                            const unsigned int sqr_size,
                                            Mat &camMat, Mat &disMat )
{
  Mat tmp_img;
  vector<Point2f> pointbuf;
  vector<Point3f> corners;
  vector< vector<Point2f> > imagePoints;
  vector< vector<Point3f> > objectPoints;
  vector<Mat> rvecs, tvecs;
  Size boardSize;

  /* 1. CREATE IMAGEPOINTS.*/
  boardSize.width = max ( size1, size2 );
  boardSize.height = min ( size1, size2 );
  for ( vector<string>::const_iterator img = images.begin() ;
        img != images.end() ; ++img )
    try {
      check_input ( (*img), boardSize );/*validate args*/
      cvtColor ( imread ( (*img) ), tmp_img, CV_BGR2GRAY );/*to grayscale*/
      if ( !findChessboardCorners ( tmp_img, boardSize, pointbuf,
                                    CV_CALIB_CB_ADAPTIVE_THRESH ) )
        continue;
      else
        cornerSubPix ( tmp_img, pointbuf, Size(5,5), Size(-1,-1),
                    TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ) );

      imagePoints.push_back(pointbuf); /*keep image points */
    }catch(ILACExFileError){continue;}
     catch(cv::Exception){continue;}

  if ( imagePoints.size() <= 0 )/* Need at least one element */
    throw ILACExNoChessboardFound();

  /* 2. CREATE OBJECTPOINTS.*/
  for ( int i = 0 ; i < boardSize.height ; i++ )
    for ( int j = 0; j < boardSize.width ; j++ )
      corners.push_back( Point3f( double(j*sqr_size),
                                  double(i*sqr_size),
                                  0 ) );

  /* replicate that element imagePoints.size() times */
  for ( int i = 0 ; i < imagePoints.size() ; i++ )
    objectPoints.push_back(corners);

  /* 3. CALL CALIBRATE CAMERA. find camMat, disMat */
  calibrateCamera( objectPoints, imagePoints, tmp_img.size(),
                   camMat, disMat, rvecs, tvecs, 0 );
}

