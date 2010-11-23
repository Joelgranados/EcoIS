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
#include "IA_Square.h"
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>

IA_Square::IA_Square ( Point2f upper_left, Point2f upper_right,
                       Point2f lower_right, Point2f lower_left,
                       const Mat& img )
{
  /* Remember upper_left and its brothers refer to the possition with respect to
   * the chessboard, not the image. */
  Point2f s[4] = {upper_left, upper_right, lower_right, lower_left};

  /* Calculate the enclosing rectangle. referenced to the original image */
  Rect t_rect = Rect( /* helper rectangle (x, y, width, height) */
    X_MIN (s[0], s[1], s[2], s[3]), Y_MIN (s[0], s[1], s[2], s[3]),
    O_S_WIDTH(s[0],s[1],s[2],s[3]), O_S_HEIGHT(s[0],s[1],s[2],s[3]) );

  /* Calculate source and destination points for the perspective transform.*/
  for ( int i = 0 ; i < 4 ; i++ )
  {
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

/* FIXME: describe range here */
void
IA_Square::calc_rgb ( const unsigned int range[7] )
{
  uchar *data_ptr;
  /* Each accumulator offset will represent a color.
   * c_accum[0] -> red,  c_accum[1] -> yellow, c_accum[2] -> green,
   * c_accum[3] -> cyan, c_accum[4] -> Blue,   c_accum[5] -> magenta
   * These values are related to the range argument.
   * The color with more hits is the one that is chosen.*/
  unsigned long c_accum[6] = {0,0,0,0,0,0};
  for ( unsigned int row = 0 ; row <= h_subimg.rows ; row++ )
    for ( unsigned int col = 0 ; col <= h_subimg.cols ; col++ )
    {
      data_ptr = h_subimg.data + (row*h_subimg.cols) + col;
      for ( int j = 0 ; j < 8 ; j++ )
        if ( range[j] > *data_ptr )
        {
          c_accum[j-1]++;
          break;
        }
    }

  /* find where the maximum offset is*/
  int max_offset = 0;
  for ( int i = 0 ; i < 6 ; i++ )
    if ( c_accum[max_offset] < c_accum[i] )
      max_offset = i;

  if ( (max_offset+1)%6 >= 0 && (max_offset+1)%6 <=2 )
    rgb[0] = 1;
  if ( max_offset >= 1 && max_offset <= 3 )
    rgb[1] = 1;
  if ( max_offset >= 3 && max_offset <= 5 )
    rgb[2] = 1;
}

int
IA_Square::get_red_value ()
{
  return rgb[0];
}

int
IA_Square::get_green_value ()
{
  return rgb[1];
}

int
IA_Square::get_blue_value ()
{
  return rgb[2];
}

int&
IA_Square::get_values ()
{
  return *rgb;
}

IA_ChessboardImage::IA_ChessboardImage ( string &image, Size &boardSize )
{
  Mat a_image = Mat::zeros(1,1,CV_64F); //adjusted image
  vector<Point2f> pointbuf;
  has_chessboard = true;

  /* get next image*/
  a_image = imread ( image );

  try
  {
    /* Initialize gray image here so the scope takes care of it for us */
    Mat g_img; //temp gray image
    /* transform to grayscale */
    cvtColor ( a_image, g_img, CV_BGR2GRAY );

    /* find the chessboard points in the image and put them in pointbuf.*/
    if ( !findChessboardCorners(g_img, boardSize, (pointbuf),
                                CV_CALIB_CB_ADAPTIVE_THRESH) )
      has_chessboard = false;

    else
      /* improve the found corners' coordinate accuracy */
      cornerSubPix ( g_img, (pointbuf), Size(11,11), Size(-1,-1),
                     TermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1) );
  }catch (cv::Exception){has_chessboard = false;}

  if (has_chessboard)
  {
    bool isBlack = true;
    for ( int r = 0 ; r < boardSize.height-1 ; r++ )
      for ( int c = 0 ; c < boardSize.width-1 ; c++ )
      {
        if ( !isBlack )
          squares.push_back(
            IA_Square(
              pointbuf[ (r*boardSize.width)+c ], /* upper left */
              pointbuf[ (r*boardSize.width)+c+1 ], /* upper right */
              pointbuf[ (r*boardSize.width)+boardSize.width+c+1 ],/*lower right*/
              pointbuf[ (r*boardSize.width)+boardSize.width+c ], /*lower left*/
              a_image ) );
        isBlack = !isBlack;
      }
  }

  int unsigned color_range[8] = {0, 21, 64, 106, 149, 192, 234, 256};
  for ( vector<IA_Square>::iterator square = squares.begin() ;
        square != squares.end() ; ++square )
    square->calc_rgb(color_range);
}

void
IA_ChessboardImage::debug_print ()
{
  std::cout << endl << "Printing red\t";
  for ( int i = 0; i < squares.size() ; i++ )
    std::cout << squares[i].get_red_value();

  std::cout << endl << "Printing green\t";
  for ( int i = 0; i < squares.size() ; i++ )
    std::cout << squares[i].get_green_value();

  std::cout << endl << "Printing blue\t";
  for ( int i = 0; i < squares.size() ; i++ )
    std::cout << squares[i].get_blue_value();

  std::cout << endl;
}
