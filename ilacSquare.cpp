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
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <sys/stat.h>

/* Notice ul:UpperLeft, ur:UpperRight, lr:LowerRight, ll:LowerLeft*/
ILAC_Square::ILAC_Square ( const Point2f ul, const Point2f ur,
                       const Point2f lr, const Point2f ll,
                       const Mat& img )
{
  /* Calculate the enclosing rectangle. referenced to the original image */
  Rect t_rect = Rect( /* helper rectangle (x, y, width, height) */
    FLOOR_MIN (ul.x, ur.x, lr.x, ll.x), FLOOR_MIN (ul.y, ur.y, lr.y, ll.y),
    CEIL_DIST (ul.x, ur.x, lr.x, ll.x), CEIL_DIST (ul.y, ur.y, lr.y, ll.y) );

  /* Calculate source and destination points for the perspective transform.*/
  Point2f s[4] = {ul, ur, lr, ll};
  for ( int i = 0 ; i < 4 ; i++ )
  {
    /* Remember that ul and its brothers refer to the position with respect to
     * the chessboard, not the image. */
    s[i].x = floor(s[i].x - t_rect.x);
    s[i].y = floor(s[i].y - t_rect.y);
  }
  Point2f d[4]={Point2f(0,0), Point2f(t_rect.width,0),
                Point2f(t_rect.width, t_rect.height), Point2f(0,t_rect.height)};

  /* Calculate the perspective transform and create a subimage. */
  Mat t_img = Mat( img, t_rect );
  warpPerspective( t_img, hsv_subimg,
                   getPerspectiveTransform(s,d), t_img.size() );

  /* Transform from BGR to HSV */
  cvtColor ( hsv_subimg, hsv_subimg, CV_BGR2HSV_FULL );

  /* We separate hsv into its different dimensions */
  //FIXME: try using the mixchannels so we can avoid s and v
  vector<Mat> tmp_dim;
  split( hsv_subimg, tmp_dim );
  h_subimg = tmp_dim[0];

  /* Initialize the array that will hold the bits. */
  rgb[0]=rgb[1]=rgb[2]=0;
}

int
ILAC_Square::calc_exact_median ()
{
  uchar *data_ptr;
  unsigned long c_accum[256] = {0};
  for ( unsigned int row = 0 ; row <= h_subimg.rows ; row++ )
    for ( unsigned int col = 0 ; col <= h_subimg.cols ; col++ )
    {
      data_ptr = h_subimg.data + (row*h_subimg.cols) + col;
      c_accum[ *data_ptr ]++;
    }

  /* The median is the offset in c_accum where we cross the middle of the data
   * set. */
  unsigned long hp_size = (h_subimg.rows * h_subimg.cols)/2; //half population size
  unsigned long accum_size = 0;
  int median = 0;
  for ( ; median <= 256 ; median++ )
  {
    if ( accum_size > hp_size )
      break;// We have found the median
    accum_size = accum_size + c_accum[median];
  }

  return median;
}

/* FIXME: describe range anywhere */
void
ILAC_Square::calc_rgb ( vector<color_hue> range )
{
  uchar *data_ptr;
  /* Each accumulator offset will represent a color.
   * c_accum[0] -> red,  c_accum[1] -> yellow, c_accum[2] -> green,
   * c_accum[3] -> cyan, c_accum[4] -> Blue,   c_accum[5] -> magenta
   * These values are related to the range argument.
   * The color with more hits is the one that is chosen.*/
  unsigned long c_accum[6] = {0};
  for ( unsigned int row = 0 ; row <= h_subimg.rows ; row++ )
    for ( unsigned int col = 0 ; col <= h_subimg.cols ; col++ )
    {
      data_ptr = h_subimg.data + (row*h_subimg.cols) + col;
      for ( int j = 0 ; j < 8 ; j++ )
        if ( range[j].hue > *data_ptr )
        {
          c_accum[(j-1)%6]++;
          break;
        }
    }

  /* find where the maximum offset is*/
  int max_offset = 0;
  for ( int i = 0 ; i < 6 ; i++ )
    if ( c_accum[max_offset] < c_accum[i] )
      max_offset = i;

  switch ( range[max_offset].color ){
    case RED:
      rgb[0]=1;
      break;
    case YELLOW:
      rgb[0]=rgb[1]=1;
      break;
    case GREEN:
      rgb[1]=1;
      break;
    case CYAN:
      rgb[0]=rgb[1]=rgb[2]=1;
      break;
    case BLUE:
      rgb[2]=1;
      break;
    case MAGENTA:
      rgb[0]=rgb[2]=1;
      break;
    default:
      // should not get here
      ;
  }
}

int
ILAC_Square::get_red_value ()
{
  return rgb[0];
}

int
ILAC_Square::get_green_value ()
{
  return rgb[1];
}

int
ILAC_Square::get_blue_value ()
{
  return rgb[2];
}

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
 * 3. INITIALIZE THE SQUARES VECTOR BASED ON POINTS.
 * 4. CALCULATE THE RANGE VECTOR.
 * 5. CALCULATE THE IMAGE ID.
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

  /* 3. INITIALIZE THE SQUARES VECTOR BASED ON POINTS. */
  //FIXME: Can we do the same without the isBlack flag?
  bool isBlack = true;
  for ( int r = 0 ; r < boardSize.height-1 ; r++ )
    for ( int c = 0 ; c < boardSize.width-1 ; c++ )
    {
      if ( !isBlack )
        squares.push_back(
          ILAC_Square(
            imageCBpoints[ (r*boardSize.width)+c ], /* upper left */
            imageCBpoints[ (r*boardSize.width)+c+1 ], /* upper right */
            imageCBpoints[ (r*boardSize.width)+boardSize.width+c+1 ],/*lower right*/
            imageCBpoints[ (r*boardSize.width)+boardSize.width+c ], /*lower left*/
            orig_img ) );
      isBlack = !isBlack;
    }

  /* 4. CALCULATE THE RANGE VECTOR.*/
  vector<color_hue> range;
  for ( int i = 0 ; i < 8 ; i++ )
  {
    color_hue temp;
    temp.hue = 0;
    temp.color = NO_COLOR;
    range.push_back ( temp );
  }

  /* Known Colors. */
  alcolor_t kc[8] = {RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA, RED, NO_COLOR};

  /* Fill the range with colors and medians. Inserted in order */
  for ( int i = 0 ; i < 6 ; i++ )
  {
    range[i].hue = squares[i].calc_exact_median();
    range[i].color = kc[i];
    for ( int j = i ; j > 0 && range[j].hue < range[j-1].hue ; j-- )
      swap( range[j], range[j-1] );
  }

  /* Calculate the max ranges */
  range[6].hue = 256+range[0].hue;
  range[6].color = range[0].color;
  for ( int i = 6 ; i > 0 ; i-- )
    range[i].hue = (((range[i].hue-range[i-1].hue)/2)+range[i-1].hue)%256;

  /* There is a possibility that things are not in order. */
  for ( int i = 1 ; i < 7 ; i++ )
    for ( int j = i ; j > 1 && range[j].hue < range[j-1].hue ; j-- )
      swap( range[j], range[j-1] );

  /* We polish the ends. */
  range[0].hue = 0;
  range[0].color = range[6].color;
  range[7].hue = 256;
  range[7].color = NO_COLOR;

  /* There is no information in the calibration squares. */
  squares.erase(squares.begin(), squares.begin()+6);

  /* Assign "binary" values to the rgb variable */
  for ( vector<ILAC_Square>::iterator square = squares.begin() ;
        square != squares.end() ; ++square )
    square->calc_rgb(range);

  /* 5. CALCULATE THE IMAGE ID. */
  int short_size = 8*sizeof(unsigned short); //assume short is a factor of 8
  int id_offset;
  id.clear(); // make sure we don't have any info.

  /* We create the id vector as we go along.*/
  for ( int i = 0 ; i < squares.size() ; i++ )
  {
    if ( i % short_size == 0 )
    {
      /* Move to the next position in id when i > multiple of short_size */
      id_offset = (int)(i/short_size);

      /* Make sure value = 0 */
      id.push_back ( (unsigned short)0 );
    }

    /* All the colored squares should have red bit on.*/
    if ( squares[i].get_red_value() != 1 )
      throw ILACExNoneRedSquare();

    id[id_offset] = id[id_offset]<<2;/* bit shift for green and blue */

    if ( squares[i].get_blue_value() )/* modify the blue bit */
      id[id_offset] = id[id_offset] | (unsigned short)1;

    if ( squares[i].get_green_value() )/* modify the green bit */
      id[id_offset] = id[id_offset] | (unsigned short)2;
  }
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
 * 2. NORMALIZE DISTANCE
 * 3. NORMALIZE ROTATION
 * 4. CORRECT DISTORTION
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

  /*2. NORMALIZE DISTANCE */
  if ( action & ILAC_DO_DISTNORM )
  {
    if ( 0 < distNorm && tvec.at<double>(0,2) > distNorm )
      try{
        resize ( orig_img, final_img, Size(0,0),
                 tvec.at<double>(0,2)/distNorm,
                 tvec.at<double>(0,2)/distNorm );
      }catch (cv::Exception cve){
        /* if the scale factor tvec.at<double>(0,2)/distNorm is too big, the
         * resized image will be too big and there will not be enough memory.*/
        if ( cve.func == "OutOfMemoryError" )
          throw ILACExInvalidResizeScale();
        else
          throw ILACExUnknownError();
      }
    else
      orig_img.copyTo ( final_img );
  }

  /* 3. NORMALIZE ROTATION */
  if ( action & ILAC_DO_ANGLENORM )
  {
    final_img.copyTo ( mid_img );
    /* Calc rotation transformation matrix. First arg is center */
    trans_mat = getRotationMatrix2D ( Point( mid_img.size().width/2,
                                             mid_img.size().height/2),
                                      rad2deg(rvec.at<double>(0,2)),
                                      1 );

    /* Perform the rotation and put it in a_img */
    warpAffine ( mid_img, final_img, trans_mat, mid_img.size() );
  }

  /* 4. CORRECT DISTORTION */
  if ( action & ILAC_DO_UNDISTORT )
  {
    final_img.copyTo ( mid_img );
    undistort ( mid_img, final_img, camMat, disMat );
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
