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
#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <time.h>

using namespace cv;

bool
ia_calc_image_chess_points ( char **images, const Size boardSize,
                             vector<vector<Point2f> > *imagePoints,
                             Size *generalSize)
{
  for ( int i = 0 ; images[i] != '\0' ; i++ )
  {
    vector<Point2f> pointbuf; //temp buffer for object points
    Mat t_image; //temp image holder

    /* Read the image in gray scale */
    t_image = imread(images[i], 0);
    if ( t_image.data == NULL )
    {
      fprintf(stderr, "The image %s could not be read\n", images[i]);
      continue;
    }

    /* Keep the image size. tell the use if something fishy is going on. */
    if ( i > 0  //so we don't output unnecessary messages
         && ( t_image.size().height != generalSize->height
              || t_image.size().width != generalSize->width) )
      fprintf(stderr, "Found an image in the list that is of different"
                      " size. height: %u and width: %u.  Note that the"
                      " used size is height: %u and width: %u.",
                      t_image.size().height, t_image.size().width,
                      generalSize->height, generalSize->width);

    else if ( i == 0 ) //We use the initial image size as general size.
      *generalSize = t_image.size();

    /* Put chessboard points in pointbuff */
    if ( !findChessboardCorners(t_image, boardSize, pointbuf,
                                CV_CALIB_CB_ADAPTIVE_THRESH) )
    {
      /* Corners not found or not organized. */
      fprintf(stderr, "Could not find chess corners in image %s\n", images[i]);
      continue;
    }

    /* improve the found corners' coordinate accuracy */
    cornerSubPix( t_image, pointbuf, Size(11,11),
                  Size(-1,-1),
                  TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));

    /* Keep the found points */
    imagePoints->push_back(pointbuf);
  }

  /* We need to had read at least 1 image to continue. */
  if ( imagePoints->size() < 1 )
  {
    fprintf(stderr, "There are not enough recognized images to work with\n");
    return false;
  }

  return true;
}

/* Puts object vector in objectPoints */
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

/*
 * Calculates camera matrix, camera distortion, rotational vector and
 * translational vector
 */
bool
ia_calculate_all ( char **images, const Size boardSize,
                   Mat *camMat, Mat *disMat,
                   vector<Mat> *rvecs, vector<Mat> *tvecs,
                   const float squareSize = 1 )
{
  vector<vector<Point3f> > objectPoints; //chessboards points in the object.
  vector<vector<Point2f> > imagePoints; //chessboards points in the image.
  Size generalSize;

  /* get the points and the generalSize for all the images */
  if ( !ia_calc_image_chess_points( images, boardSize, &imagePoints,
                                    &generalSize) )
    return false;

  /* get the points for the object. */
  if ( !ia_calc_object_chess_points ( boardSize, squareSize,
                                      imagePoints.size(), &objectPoints) )
    return false;

  /* calc camera matrix, distorition matrix rvector and tvector (per image)*/
  calibrateCamera(objectPoints, imagePoints, generalSize, *camMat, *disMat,
      *rvecs, *tvecs, 0);

  return true;
}

bool
ia_calculate_intrinsics ( char **images, const Size boardSize,
                          Mat& camMat, Mat& disMat, const float squareSize )
{
  vector<Mat> rvecs, tvecs;
  return ia_calculate_all ( images, boardSize, &camMat, &disMat,
                            &rvecs, &tvecs, squareSize );
}

bool
ia_calculate_extrinsics ( char **images, const Mat *camMat, const Mat *disMat,
                          const Size boardSize, vector<Mat> *rvecs,
                          vector<Mat> *tvecs, const float squareSize = 1 )
{
  vector<vector<Point3f> > objectPoints; //chessboard points in the object.
  vector<vector<Point2f> > imagePoints; //chessboard points in the image.
  Size generalSize;

  /* get the points and the generalSize for all the images */
  if ( !ia_calc_image_chess_points(images, boardSize, &imagePoints, &generalSize) )
    return false;

  /* get one set of points for the images. */
  if ( !ia_calc_object_chess_points (boardSize, squareSize, 1, &objectPoints) )
    return false;

  /* calculates rvecs and tvecs for each imagePoint */
  for ( vector<vector<Point2f> >::iterator image_iter = imagePoints.begin() ;
        image_iter != imagePoints.end() ; image_iter++ )
  {
    Mat rvec, tvec;
    solvePnP ( (Mat)objectPoints[0], (Mat)*image_iter, *camMat, *disMat,
               rvec, tvec, false ); //false -> don't use extrinsic guess.
    rvecs->push_back(rvec);
    tvecs->push_back(tvec);
  }

  return true;
}

void
ia_print_matrix ( const Mat mat, const char* end_char = "\n" )
{
  Size m_size = mat.size();
  for ( int i = 0 ; i < m_size.height ; i++ )
    for ( int j = 0 ; j < m_size.width ; j++ )
      fprintf ( stdout, "%-15e ", mat.at<double>(i,j) );

  if ( end_char == "\n" || end_char == "\r" )
    fprintf ( stdout, end_char );

  fflush( stdout );
}

void
ia_print_matrix_vector ( vector<Mat>* vec, const char* message )
{
  fprintf ( stdout, "%s\n", message );

  for ( vector<Mat>::iterator iter = vec->begin() ; iter != vec->end() ;
      iter++ )
    ia_print_matrix ( *iter );

  fprintf ( stdout, "\n" );
  fflush ( stdout );
}

void
ia_calculate_and_capture ( const Size boardSize, const int delay,
                           const char* vid_file, const float squareSize = 1 )
{

  /* Reflects the process state of the function. start accumulating. */
  enum proc_state { ACCUM, OUTPUT, CALC } p_state = ACCUM;

  VideoCapture capture;
  vector<vector<Point2f> > imagePoints;
  vector<vector<Point3f> > objectPoints;
  Mat camMat, disMat; // camera matrix, distorition matrix.
  Mat rvec, tvec; //translation and rotation vectors.
  Size generalSize;
  string msg;
  clock_t timestamp = 0;
  int num_int_images = 20; //number of intrinsic images needed

  //open a window...
  namedWindow( "Image View", 1 );

  // We start the capture.  And bail out if we cant
  if ( vid_file != NULL
       && !capture.isOpened()
       && !capture.open( (string)vid_file ) )
    fprintf ( stderr, "File %s could not be played\n", vid_file );

  if ( vid_file == NULL
       && !capture.isOpened()
       && !capture.open(0) )
    fprintf ( stderr, "Could not open camera input\n" );

  if ( !capture.isOpened() )
    return;

  for ( int i = 0 ;; i++ )
  {
    Mat frame_buffer, t_image;
    vector<Point2f> pointbuf;

    if ( !capture.grab() ) break;
    capture.retrieve ( frame_buffer );

    try
    {
      /* transform to grayscale */
      cvtColor(frame_buffer, t_image, CV_BGR2GRAY);

    /* find the chessboard points in the image and put them in pointbuf.*/
      if ( !findChessboardCorners(t_image, boardSize, pointbuf,
                                  CV_CALIB_CB_ADAPTIVE_THRESH) )
      {
        imshow("Image View", t_image);
        if( (waitKey(50) & 255) == 27 )
          break;
        continue; //We will get another change in the next image
      }
    }catch (cv::Exception){continue;}

    /* improve the found corners' coordinate accuracy */
    cornerSubPix ( t_image, pointbuf, Size(11,11),
                   Size(-1,-1),
                   TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ) );

    switch ( p_state )
    {
      case OUTPUT:
        /* calc the rvec and tvec.  Note that we use the camMat and disMat*/
        solvePnP ( (Mat)objectPoints[0], (Mat)pointbuf, camMat, disMat,
                   rvec, tvec );
        if ( clock() - timestamp > delay*1e-3*CLOCKS_PER_SEC )
        {
          /* output rvec and tvec to stdout */
          fprintf ( stdout, "tvecs: (  " );
          ia_print_matrix ( tvec, (char*)"\0" );
          fprintf ( stdout, ") | rvecs: (  " );
          ia_print_matrix ( rvec, (char*)"\0" );
          fprintf ( stdout, ")\n" );
        }
      break;

      case ACCUM: //accumulate info to calculate intrinsics.
        /*
         * We make sure we keep the image points.
         */
        if ( clock() - timestamp > delay*1e-3*CLOCKS_PER_SEC )
        {
          imagePoints.push_back(pointbuf);
          timestamp = clock();
        }

        /* Create and put message on image */
        msg = format ( "Cal Intrinsics: %d/%d.", imagePoints.size(),
            num_int_images );
        int baseLine = 0;
        Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(t_image.cols - 2*textSize.width - 10,
            t_image.rows - 2*baseLine - 10);
        putText ( t_image, msg, textOrigin, 1, 1, Scalar(0,0,255) );

        /* we change state when we have enough images */
        if ( imagePoints.size() >= num_int_images )
        {
          /* We use the last image size as generalSize.*/
          generalSize = t_image.size();

          /* get the points for the object. */
          ia_calc_object_chess_points ( boardSize, squareSize,
                                        imagePoints.size(), &objectPoints);

          /* calc camera matrix, dist matrix, rvector, tvector, no flags */
          vector<Mat> rvecs, tvecs; // will not be used in other places.
          calibrateCamera( objectPoints, imagePoints, generalSize,
                           camMat, disMat, rvecs, tvecs, 0 );

          /* print the calibration info */
          fprintf ( stdout, "cammat : (  ");
          ia_print_matrix ( camMat, (char*)"\0" );
          fprintf ( stdout, "\n" );
          fprintf ( stdout, "dismat : (  ");
          ia_print_matrix ( disMat, (char*)"\0" );
          fprintf ( stdout, ")\n" );
          p_state = OUTPUT;
        }
      break;
    }

    /* Draw chessboard on image.*/
    drawChessboardCorners( t_image, boardSize, Mat(pointbuf), true );

    /* finally, show image :) */
    imshow("Image View", t_image);

    /* we wait for user interaction */
    int key = waitKey(50);
    if( (key & 255) == 27 )
        break;
  }
}
