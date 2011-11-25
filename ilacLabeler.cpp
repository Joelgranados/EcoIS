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
#include "ilacLabeler.h"
#include "error.h"
#include <opencv2/opencv.hpp>

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

ILAC_Labeler::ILAC_Labeler () {}
ILAC_Labeler::ILAC_Labeler ( const Mat& image, const vector<Point2f> cbpoints,
                             const Size boardSize )
{
  this->image = image;
  this->imageCBpoints = cbpoints;
  this->boardSize = boardSize;
}

/*
 * 1. INITIALIZE THE SQUARES VECTOR BASED ON POINTS.
 * 2. CALCULATE THE RANGE VECTOR.
 * 3. CALCULATE THE IMAGE ID.
 */
vector<unsigned short>
ILAC_Labeler::calculate_label ()
{
  /* 1. INITIALIZE THE SQUARES VECTOR BASED ON POINTS. */
  bool isBlack = true;
  squares.clear();
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
            image ) );
      isBlack = !isBlack;
    }

  /* 2. CALCULATE THE RANGE VECTOR.
   * Range vector:  Mapping from hue->color (one of 6 colors).
   * Range of range[i].color -> (range[i].hue, range[i+1].hue)
   * Ranges are sorted in increasing order.
   */
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

  /* Fill the range with colors and means. Order based on hue */
  for ( int i = 0 ; i < 6 ; i++ )
  {
    range[i].hue = squares[i].calc_exact_median();
    range[i].color = kc[i];
    for ( int j = i ; j > 0 && range[j].hue < range[j-1].hue ; j-- )
      std::swap( range[j], range[j-1] );
  }

  /* Calculate the max ranges */
  range[6].hue = 256+range[0].hue;
  range[6].color = range[0].color;
  for ( int i = 6 ; i > 0 ; i-- )
    range[i].hue = (((range[i].hue-range[i-1].hue)/2)+range[i-1].hue)%256;

  /* There is a possibility that things are not in order. */
  for ( int i = 1 ; i < 7 ; i++ )
    for ( int j = i ; j > 1 && range[j].hue < range[j-1].hue ; j-- )
      std::swap( range[j], range[j-1] );

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

  /* 3. CALCULATE THE IMAGE ID. */
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
  return id;
}

vector<unsigned short>
ILAC_Labeler::get_label ()
{
  return id;
}
