/*
 * image adjust.  Automatic image normalization.
 * Copyright (C) 2010 Joel Granados <joel.granados@gmail.com>
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
#include "ia_input.h"
#include "IA_Square.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <time.h>
#include <sstream>
#include <iostream>

using namespace cv;

/* Helper function: Puts object vector in objectPoints */
bool
ia_calc_object_chess_points ( const Size boardSize, const int squareSize,
                              const int numElems,
                              vector<vector<Point3f> > *objectPoints )
{
  /* create one element of the objectPoints vector. */
  vector<Point3f> corners;
  for ( int i = 0 ; i < boardSize.height ; i++ )
    for ( int j = 0; j < boardSize.width ; j++ )
      corners.push_back( Point3f(double(j*squareSize),double(i*squareSize),0) );

  /* replicate that element size times */
  for ( int i = 0 ; i < numElems ; i++ )
    objectPoints->push_back(corners);

  return true;
}

/* Helper function.*/
void
ia_put_text_on_image ( const char* message, Mat& image )
{
  string msg;
  int baseLine = 0;

  msg = format ( message );
  Point orig(20,20);

  putText ( image, msg, orig, 1, 1, Scalar(0,0,255) );
}

/* Helper function. */
double
ia_rad2deg (const double Angle)
{
  static double ratio = 180.0 / 3.141592653589793238;
  return Angle * ratio;
}

/* Helper function. returns -1 when unsuccessfull. this is repeated a lot*/
int
ia_find_chessboard_points( const Mat *image, const Size boardSize,
                           vector<Point2f> *pointbuf )
{
  Mat t_img; //temp image
  try
  {
    /* transform to grayscale */
    cvtColor((*image), t_img, CV_BGR2GRAY);

    /* find the chessboard points in the image and put them in pointbuf.*/
    if ( !findChessboardCorners(t_img, boardSize, (*pointbuf),
                                CV_CALIB_CB_ADAPTIVE_THRESH) )
      return -1;

    /* improve the found corners' coordinate accuracy */
    cornerSubPix ( t_img, (*pointbuf), Size(11,11), Size(-1,-1),
                   TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ) );
  }catch (cv::Exception){return -1;}
}

void
ia_calculate_and_capture ( const Size boardSize, const int delay,
                           const string vid_file, const int camera_id = 0,
                           const float squareSize = 1, const int num_in_imgs=20)
{
  /* Reflects the process state of the function. start accumulating. */
  enum proc_state { ACCUM, OUTPUT} p_state = ACCUM;
  VideoCapture capture;
  vector<vector<Point2f> > imagePoints;
  vector<vector<Point3f> > objectPoints;
  vector<Point2f> pointbuf;
  Mat camMat, disMat; // camera matrix, distorition matrix.
  Mat rvec, tvec; //translation and rotation vectors.
  Mat o_img = Mat::zeros(1,1,CV_64F); // original capture
  Mat a_img = Mat::zeros(1,1,CV_64F); // adjusted capture
  Mat frame_buffer, trans_mat; // temporary vars
  clock_t timestamp = 0;
  char image_message[30]; //output text to the image
  float r_dist = 0;

  /* We start the capture.  And bail out if we can't */
  if ( vid_file != ""
       && !capture.isOpened()
       && !capture.open( vid_file ) )
    std::cout << vid_file << " could not be played" << endl;
  if ( vid_file == ""
       && !capture.isOpened()
       && !capture.open(camera_id) )
    fprintf ( stderr, "Could not open camera input\n" );
  if ( !capture.isOpened() )
    return;

  /* Open two windows for comparison. */
  namedWindow ( "Original", 1 );
  namedWindow ( "Adjusted", 1 );

  for ( int i = 0 ;; i++ )
  {
    if ( !capture.grab() ) break;
    capture.retrieve ( frame_buffer );

    try{
      imshow("Original", o_img);
      imshow("Adjusted", a_img);
    }catch (cv::Exception){;}
    if( (waitKey(50) & 255) == 27 )
      break;

    frame_buffer.copyTo(o_img); //can't use frame_buffer.
    if ( ia_find_chessboard_points ( &o_img, boardSize, &pointbuf ) == -1 )
      continue;

    /* Draw chessboard on image.*/
    drawChessboardCorners( o_img, boardSize, Mat(pointbuf), true );

    if ( p_state == OUTPUT )
    {
      /* calc the rvec and tvec.  Note that we use the camMat and disMat*/
      solvePnP ( (Mat)objectPoints[0], (Mat)pointbuf, camMat, disMat,
                 rvec, tvec );

      /* Calc rotation transformation matrix. First arg is center */
      trans_mat = getRotationMatrix2D ( Point(o_img.size().width/2,
                                              o_img.size().height/2),
                                        ia_rad2deg(rvec.at<double>(0,2)),
                                        1 );

      /* Perform the rotation and put it in a_img */
      warpAffine ( o_img, frame_buffer, trans_mat, o_img.size() );

      /* calculate the scaling size. tvec(0.2)/r_dist = ratio */
      if ( 0 < r_dist && tvec.at<double>(0,2) < r_dist )
        resize ( frame_buffer, a_img, Size(0,0),
                 tvec.at<double>(0,2)/r_dist, tvec.at<double>(0,2)/r_dist );
      else
        frame_buffer.copyTo(a_img);
    }
    else if ( p_state == ACCUM )
    {
      /* We make sure we keep the image points. */
      if ( clock() - timestamp > delay*1e-3*CLOCKS_PER_SEC )
      {
        imagePoints.push_back(pointbuf);
        timestamp = clock();
      }

      /* Create and put message on image */
      sprintf ( image_message, "Cal Intrinsics: %d/%d.", imagePoints.size(),
          num_in_imgs );
      ia_put_text_on_image ( image_message, o_img );

      /* we change state when we have enough images */
      if ( imagePoints.size() >= num_in_imgs )
      {
        /* get the points for the object. */
        ia_calc_object_chess_points ( boardSize, squareSize,
                                      imagePoints.size(), &objectPoints);

        /*
         * calc camera matrix, dist matrix, rvector, tvector, no flags.
         * imagesize = o_img.size() current image.
         */
        vector<Mat> rvecs, tvecs; // will not be used in other places.
        calibrateCamera( objectPoints, imagePoints, o_img.size(),
                         camMat, disMat, rvecs, tvecs, 0 );

        /* we calculate the maximum height from the tvecs.*/
        for ( int i = 0 ; i < tvecs.size() ; i++ )
          if ( r_dist < tvecs[i].at<double>(0,2) )
            r_dist = tvecs[i].at<double>(0,2);

        p_state = OUTPUT;
      }
    }
  }
}

void
ia_information_extraction_debug ( vector<string>& images, Size& boardSize )
{
  for ( vector<string>::iterator image = images.begin() ;
        image != images.end() ; image++ )
  {
    IA_ChessboardImage cb = IA_ChessboardImage ( *image, boardSize );
    cb.debug_print();
  }
}

