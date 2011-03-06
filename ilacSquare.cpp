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
#include <iostream>
#include <math.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <sys/stat.h>

ILAC_ChessboardImage::ILAC_ChessboardImage (){}/*Used to initialize.*/

ILAC_ChessboardImage::ILAC_ChessboardImage ( const string &image,
                                             const unsigned int size1,
                                             const unsigned int size2,
                                             const unsigned int sqr_size )
{
  Size boardSize = ILAC_ChessboardImage::get_size ( size1, size2 );
  check_input ( image, boardSize, sqr_size );
  init_chessboard ( image, boardSize, sqr_size );
}

ILAC_ChessboardImage::ILAC_ChessboardImage ( const string &image,
                                             const Size &boardsize,
                                             const unsigned int sqr_size )
{
  Size boardSize = boardsize; /* we cant have a const in check_input*/
  check_input ( image, boardSize, sqr_size );
  init_chessboard ( image, boardSize, sqr_size );
}

Size
ILAC_ChessboardImage::get_size ( unsigned int size1, unsigned int size2 )
{
  Size boardSize;
  boardSize.width = max ( size1, size2 );
  boardSize.height = min ( size1, size2 );
  return boardSize;
}

void
ILAC_ChessboardImage::check_input ( const string &image, Size &boardSize,
                                    const unsigned int sqr_size )
{
  // Check that file exists.
  struct stat file_stat;
  if ( stat ( image.data(), &file_stat ) != 0 )
    throw ILACExFileError();

  // Check to see if sizes are possitive.
  if ( boardSize.height < 0 || boardSize.width < 0 || sqr_size < 0 )
      throw ILACExSizeFormatError();

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
 * This function has FOUR steps:
 * 1. CALCULATE PERFECT CHESSBOARD POINTS
 * 2. CALCULATE IMAGE CHESSBOARD POINTS.
 * 3. CALCULATE THE IMAGE ID.
 */
void
ILAC_ChessboardImage::init_chessboard ( const string &image,
                                        const Size &boardSize,
                                        const unsigned int sqr_size )
{
  /* 1. CALCULATE PERFECT CHESSBOARD POINTS */
  this->sqr_size = sqr_size; /* class sqr_size var */
  perfectCBpoints.clear();
  for ( int i = 0 ; i < boardSize.height ; i++ )
    for ( int j = 0; j < boardSize.width ; j++ )
      perfectCBpoints.push_back( Point3f( double(j*sqr_size),
                                          double(i*sqr_size),
                                          0 ) );

  /* 2. CALCULATE CHESSBOARD POINTS.*/
  orig_img = imread ( image );

  try
  {
    /* Initialize gray image here so the scope takes care of it for us */
    Mat g_img; //temp gray image
    /* transform to grayscale */
    cvtColor ( orig_img, g_img, CV_BGR2GRAY );

    /* find the chessboard points in the image and put them in imageCBpoints.*/
    if ( !findChessboardCorners(g_img, boardSize, (imageCBpoints),
                                CV_CALIB_CB_ADAPTIVE_THRESH) )
      throw ILACExNoChessboardFound();

    else
      /* The 3rd argument is of interest.  It defines the size of the subpix
       * window.  window_size = NUM*2+1.  This means that with 5,5 we have a
       * window of 11x11 pixels.  If the window is too big it will mess up the
       * original corner calculations for small chessboards. */
      cornerSubPix ( g_img, (imageCBpoints), Size(5,5), Size(-1,-1),
                     TermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1) );
  }catch (cv::Exception){throw ILACExNoChessboardFound();}

  /* 3. CALCULATE IMAGE ID */
  ILAC_Labeler labeler ( orig_img, imageCBpoints, boardSize );
  id = labeler.calculate_label();
}

/* Helper function for process_image. */
double
ILAC_ChessboardImage::rad2deg ( const double Angle )
{
    static double ratio = 180.0 / 3.141592653589793238;
      return Angle * ratio;
}

/*
 * There are the possible actions.
 * 1. CALCULATE TVEC AND RVEC
 * 2. CORRECT DISTORTION
 * 3. NORMALIZE DISTANCE
 * 4. NORMALIZE ROTATION
 */
void
ILAC_ChessboardImage::process_image ( const int action,
                                      const Mat &camMat, const Mat &disMat,
                                      const int distNorm,
                                      const string filename_output )
{
  Mat trans_mat; /*temporal Mats*/
  Mat rvec, tvec;
  Mat final_img;
  Mat mid_img = Mat::zeros( 1, 1, CV_32F );

  /*1. CALCULATE TVEC AND RVEC */
  solvePnP ( (Mat)perfectCBpoints, (Mat)imageCBpoints, camMat, disMat,
             rvec, tvec );

  /* 2. CORRECT DISTORTION */
  if ( action & ILAC_DO_UNDISTORT )
    undistort ( orig_img, final_img, camMat, disMat );

  /*3. NORMALIZE DISTANCE */
  if ( action & ILAC_DO_DISTNORM )
    if ( 0 < distNorm && tvec.at<double>(0,2) > distNorm )
      try{
        resize ( final_img, mid_img, Size(0,0),
                 tvec.at<double>(0,2)/distNorm,
                 tvec.at<double>(0,2)/distNorm );
        mid_img.copyTo ( final_img );
      }catch (cv::Exception cve){
        /* if the scale factor tvec.at<double>(0,2)/distNorm is too big, the
         * resized image will be too big and there will not be enough memory.*/
        if ( cve.func == "OutOfMemoryError" )
          throw ILACExInvalidResizeScale();
        else
          throw ILACExUnknownError();
      }

  /* 4. NORMALIZE ROTATION */
  if ( action & ILAC_DO_ANGLENORM )
  {
    /* pad image.  Enough for the rotation to fit. */
    int pad = ( sqrt ( pow ( final_img.size().width, 2 )
                       + pow ( final_img.size().height, 2 ) )
                - max(final_img.size().width, final_img.size().height) ) / 2;
    copyMakeBorder ( final_img, mid_img, pad, pad, pad, pad, BORDER_CONSTANT );

    /* Calc rotation transformation matrix. First arg is center */
    trans_mat = getRotationMatrix2D ( Point( mid_img.size().width/2,
                                             mid_img.size().height/2),
                                      rad2deg(rvec.at<double>(0,2)),
                                      1 );

    /* Perform the rotation and put it in a_img */
    warpAffine ( mid_img, final_img, trans_mat, mid_img.size() );
  }

  //FIXME: this shouldn't really be here. fix it someday :)
  /* 5. We write the image to a file */
  imwrite ( filename_output, final_img );
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
  boardSize = ILAC_ChessboardImage::get_size ( size1, size2 );
  for ( vector<string>::const_iterator img = images.begin() ;
        img != images.end() ; ++img )
    try {
      check_input ( (*img), boardSize, sqr_size );/*validate args*/
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

vector<unsigned short>
ILAC_ChessboardImage::get_image_id ()
{
  return id;
}
