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
#include <math.h>

using namespace cv;

/* Returns the angle between the two vectos in radians */
#define VECTORS_ANGLE(a, b, v) \
  acos( \
        ( ((a->x-v->x)*(b->x-v->x)) \
          + ((a->y-v->y)*(b->y-v->y)) ) \
        / \
        ( sqrt(pow(a->x-v->x,2)+pow(a->y-v->y,2)) \
          * sqrt(pow(b->x-v->x,2)+pow(b->y-v->y,2)) ) \
      )

#define X_MIN(p1, p2, p3, p4) \
  min ( min (p1.x, p2.x), min (p3.x, p4.x) )

#define X_MAX(p1, p2, p3, p4) \
 max ( max (p1.x, p2.x), max (p3.x, p4.x) )

#define Y_MIN(p1, p2, p3, p4) \
 min ( min (p1.y, p2.y), min (p3.y, p4.y) )

#define Y_MAX(p1, p2, p3, p4) \
 max ( max (p1.y, p2.y), max (p3.y, p4.y) )

#define O_S_WIDTH(p1, p2, p3, p4) \
  X_MAX ( p1, p2, p3, p4 ) - X_MIN ( p1, p2, p3, p4 )

#define O_S_HEIGHT(p1, p2, p3, p4) \
  Y_MAX( p1, p2, p3, p4 ) - Y_MIN ( p1, p2, p3, p4 )

class IA_Line;

struct ia_square_line
{
  IA_Line *lref;
  struct ia_square_point *lps[2];
  struct ia_square_line *ladjs[2];
};

struct ia_square_point
{
  Point2f pref;
  struct ia_square_point *padjs[2];
  struct ia_square_line *pls[2];
};

struct ia_square_square
{
  struct ia_square_point *ps[4];
  struct ia_square_line *ls[4];
};

class IA_Square{
public:
  IA_Square ( Point2f*[4], const Mat*, bool );
  IA_Square ( Point2f*[4], const Mat* );
  ~IA_Square ();
  int get_red_value ();
  int get_green_value ();
  int get_blue_value ();
  int* get_values();

private:
  Mat hsv_subimg; /* minimal subimage that contains the 4 points. */
  Mat *h_subimg; /* subimage for the hue dimension */
  Mat *s_subimg; /* subimage for the saturation dimension */
  Mat *v_subimg; /* subimage for the value dimension */
  int rgb[3]; // Representation of the values in the square.
  struct ia_square_square sqr;
  void gen_square_messy_points (Point2f**);
  void calculate_rgb ();
  inline bool row_between_lines ( const unsigned int,
      const struct ia_square_line*, const struct ia_square_line* );
};

class IA_Line{
public:
  IA_Line ( Point2f*, Point2f* );
  int resolve_width ( int );
  int resolve_height ( int );

private:
  Point2f *p1;
  Point2f *p2;
  float slope;
  bool vertical;
  bool horizontal;
};

class IA_ChessboardImage{
public:
  IA_ChessboardImage ( const char*, const Size );
  void debug_print ();
private:
  vector<IA_Square> squares;
  bool has_chessboard;
};
