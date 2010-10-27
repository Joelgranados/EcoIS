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
#include <cv.h>
#include <highgui.h>
#include <stdio.h>

//using namespace cv;

IA_Square::IA_Square ( Point2f *p[4], const Mat *img )
{
  IA_Square::IA_Square ( p, img, false );
}

IA_Square::IA_Square ( Point2f *p[4], const Mat *img, bool messy_points )
{
  /* Initialize the square struct */
  for ( int i = 0 ; i <= 3 ; i++ )
  {
    sqr.ps[i] = new ia_square_point;
    sqr.ls[i] = new ia_square_line;
  }

  /* Interconnect lines and points.  None point to anything */
  for ( int i = 0 ; i <= 4 ; i++ )
  {
    /* Create a double linked list of points */
    sqr.ps[i]->padjs[0] = sqr.ps[ (i+1)%4 ];
    sqr.ps[ (i+1)%4 ]->padjs[1] = sqr.ps[i];

    /* Create a double linked list of lines */
    sqr.ls[i]->ladjs[0] = sqr.ls[ (i+1)%4 ];
    sqr.ls[ (i+1)%4 ]->ladjs[1] = sqr.ls[i];

    /* link each line with it's respective point */
    sqr.ls[i]->lps[0] = sqr.ps[i];
    sqr.ls[i]->lps[1] = sqr.ps[ (i+1)%4 ];

    /* link each point with it's respective line */
    sqr.ps[i]->pls[0] = sqr.ls[i];
    sqr.ps[ (i+1)%4 ]->pls[1] = sqr.ls[i];
  }

  /* We fill in the point pointers */
  if ( messy_points )
    gen_square_messy_points ( p );
  else
    for ( int i = 0 ; i <= 3 ; i++ )
      sqr.ps[i]->pref = p[i];

  /* We fill in the square lines */
  for ( int i = 0 ; i <= 3 ; i++ )
    sqr.ls[i]->lref =  new IA_Line ( sqr.ls[i]->lps[0]->pref,
                                     sqr.ls[i]->lps[1]->pref );

  /* We need a reference to the original image */
  hsv_img = img;

  /* We create a helper rectangle (x, y, width, height)*/
  Rect t_rect = Rect(
    X_MIN (sqr.ps[0]->pref, sqr.ps[1]->pref, sqr.ps[2]->pref, sqr.ps[3]->pref),
    Y_MIN (sqr.ps[0]->pref, sqr.ps[1]->pref, sqr.ps[2]->pref, sqr.ps[3]->pref),
    O_S_WIDTH(sqr.ps[0]->pref,sqr.ps[1]->pref,sqr.ps[2]->pref,sqr.ps[3]->pref),
    O_S_HEIGHT(sqr.ps[0]->pref,sqr.ps[1]->pref,sqr.ps[2]->pref,sqr.ps[3]->pref)
  );

  /* The square is contained in this image.  They are probably not the same */
  *hsv_subimg = Mat( *hsv_img, t_rect );

  /* We separate hsv into its different dimensions */
  vector<Mat> tmp_dim;
  split( *hsv_img, tmp_dim );
  h_subimg = &tmp_dim[0];
  s_subimg = &tmp_dim[1];
  v_subimg = &tmp_dim[2];

  calculate_color_average();
}

IA_Square::~IA_Square ()
{
  for ( int i = 0 ; i <=3 ; i++ )
  {
    delete sqr.ls[i]->lref;
    delete sqr.ps[i];
    delete sqr.ls[i];
  }
  delete hsv_subimg;
}

void
IA_Square::calculate_color_average ()
{
  struct ia_square_point *v_order[4], *temp;
  LineIterator *row_iter;
  float ca_angle = 0;

  /* Calculate the "vertical" order of the points */
  for ( int i=0 ; i<=3 ; i++ )
    v_order[i] = sqr.ps[i];
  for ( int i=0, j=0 ; i<=3 ; i++ )
  {
    temp = v_order[i];
    j = i;
    for ( ; j>0 && v_order[j-1]->pref->y > v_order[j]->pref->y ; j-- )
      v_order[j-1] = v_order[j];
    v_order[j] = temp;
  }

  /* We analyze all the rows in the image */
  for ( unsigned int row ; row <= hsv_subimage->size().height ; row++ )
  {
    initRowIter ( v_order, row_iter, row );
    //FIXME: Lets cast to float for now.  We mus make sure we are dealing with
    //the HUE!!!!!!!
    for ( int i = 0 ; i < row_iter->count ; i++, row_iter++ )
      ca_angle = (*(float*)row_iter + i*ca_angle) / (i + 1);
  }
}

void
IA_Square::initRowIter ( struct ia_square_point **v_order,
                         LineIterator *row_iter, const unsigned int row )
{
  struct ia_square_line *line1, *line2;
  struct ia_square_line **l1ad; // To shorten the lines further down.
  Point2f p1, p2;

  /* It's 2 because we traverse 3 intervals and not 4 points */
  for ( int order_pos = 0 ; order_pos <= 2; order_pos++)
  {
    if ( v_order[order_pos]->pref->y <= row
         && v_order[order_pos+1]->pref->y > row )
    {
      /* line1 is the common line between the two points */
      line1=(v_order[order_pos]->pls[0] != v_order[order_pos+1]->pls[0]
             && v_order[order_pos]->pls[0] != v_order[order_pos+1]->pls[1])
            ? v_order[order_pos]->pls[1] : v_order[order_pos]->pls[0];
      l1ad = line1->ladjs;

      if ( min(l1ad[0]->lps[0]->pref->y, l1ad[0]->lps[1]->pref->y) < row
           && max(l1ad[0]->lps[0]->pref->y, l1ad[0]->lps[1]->pref->y) > row )
        line2 = l1ad[0];

      else if ( min(l1ad[1]->lps[0]->pref->y, l1ad[1]->lps[1]->pref->y) < row
            && max(l1ad[1]->lps[0]->pref->y, l1ad[1]->lps[1]->pref->y) > row )
        line2 = l1ad[1];

      else /*We choose the opposite line */
        line2 = l1ad[1]->ladjs[1];

      break;
    }
  }

  p1 = Point ( min (line1->lref->resolve_width(row), line2->lref->resolve_width(row)),
               row );
  p2 = Point ( max (line1->lref->resolve_width(row), line2->lref->resolve_width(row)),
               row );
  LineIterator ri( *hsv_subimg, p1, p2, 8 );
  row_iter = &ri;
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

int*
IA_Square::get_values ()
{
  return rgb;
}

void
IA_Square::gen_square_messy_points ( Point2f *p[] )
{
  /* P0 will be the vertex */
  Point2f *combinations[5] = { p[1], p[2], p[3], p[1], NULL };
  int result;
  float angle_temp, angle_result = 0;
  for ( int i = 0 ; combinations[i+1] != NULL ; i ++ )
  {
    angle_temp = VECTORS_ANGLE(combinations[i], combinations[i+1], p[0]);
    if ( angle_temp > angle_result )
    {
      angle_result = angle_temp;
      result = i; //It's i and i+1
    }
  }

  /* We fill in the point pointers */
  sqr.ps[0]->pref = p[0];
  sqr.ps[1]->pref = combinations[result];
  sqr.ps[3]->pref = combinations[result+1];
  sqr.ps[2]->pref = combinations[result+2] != NULL
                    ? combinations[result+2]
                    : combinations[result-1];

  /* Fill in the lines */
  for ( int i = 0 ; i <= 3 ; i++ )
    sqr.ls[i]->lref = new IA_Line::IA_Line ( sqr.ls[i]->lps[0]->pref,
                                             sqr.ls[i]->lps[1]->pref );
}

IA_Line::IA_Line ( Point2f *p1, Point2f *p2 )
{
  this->p1 = p1;
  this->p2 = p2;

  horizontal = vertical = false;
  if ( p2->y - p1->y == 0 ) /* We have a horizontal line */
    horizontal = true;

  else if ( p2->x - p1->x == 0 ) /* We have a vertical line */
    vertical = true;

  else /* We have a "normal" liine */
    slope = ( p2->y - p1->y)/(p2->x - p1->x);
}

int
IA_Line::resolve_width ( int height )
{
  /* it's an error to ask for width of a horizontal line */
  if ( horizontal )
    return -1;

  else if ( vertical )
    return p1->x; /* == to p2->x */

  else
    return floor ( (height - p1->y)/slope + p1->x );
}

int
IA_Line::resolve_height ( int width )
{
  if ( horizontal )
    return p1->y; /*== to p2->y*/

  /* it's an error to ask for height of a vertical line */
  else if ( vertical )
    return -1;

  else
    return floor ( slope * (width - p1->x) + p1->y );
}


IA_ChessboardImage::IA_ChessboardImage ( const char *image,
                                         const Size boardSize )
{
  Mat a_image = Mat::zeros(1,1,CV_64F); //adjusted image
  Mat hsv_img; //temp image
  vector<Point2f> pointbuf;

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
    cvtColor ( a_image, hsv_img, CV_BGR2HSV );

    for ( int r = 0 ; r <= boardSize.height ; r++ )
      for ( int c = 0 ; r <= boardSize.width ; c++ )
      {
        Point2f *ordered_points[4];
        ordered_points[0] = &pointbuf[ (r*boardSize.width)+c ];
        ordered_points[1] = &pointbuf[ (r*boardSize.width)+c+1 ];
        ordered_points[2] = &pointbuf[ (r*boardSize.width)+boardSize.width+c ];
        ordered_points[3] = &pointbuf[ (r*boardSize.width)+boardSize.width+c+1 ];

        squares.push_back( IA_Square( ordered_points, &hsv_img ) );
      }
  }
}

void
IA_ChessboardImage::debug_print ()
{
  fprintf ( stderr, "Printing red\n" );
  for ( int i = 0; i < squares.size() ; i++ )
    fprintf ( stderr, " %d ", squares[i].get_red_value() );

  fprintf ( stderr, "Printing green\n" );
  for ( int i = 0; i < squares.size() ; i++ )
    fprintf ( stderr, " %d ", squares[i].get_green_value() );

  fprintf ( stderr, "Printing blue\n" );
  for ( int i = 0; i < squares.size() ; i++ )
    fprintf ( stderr, " %d ", squares[i].get_blue_value() );
}
