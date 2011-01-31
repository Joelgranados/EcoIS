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
#include <opencv/cv.h>
#include <math.h>
#include <exception>

using namespace cv;

/* Returns the angle between the two vectos in radians */
#define MIN_4(v1, v2, v3, v4) \
  min ( min (v1,v2), min(v3,v4) )

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

enum alcolor_t { NO_COLOR, RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA };
struct color_hue {
  alcolor_t color;
  int hue;
};

class ILAC_Square{
public:
  ILAC_Square ( Point2f, Point2f, Point2f, Point2f, const Mat& );
  void calc_rgb ( const vector<color_hue> );
  int calc_exact_median ();
  int get_red_value ();
  int get_green_value ();
  int get_blue_value ();

private:
  Mat hsv_subimg; /* minimal subimage that contains the 4 points. */
  Mat h_subimg; /* subimage for the hue dimension */
  int rgb[3]; // Representation of the values in the square.
};

class ILAC_ChessboardImage{
public:
  ILAC_ChessboardImage ( const string&, Size& );
  ILAC_ChessboardImage ( const string&, const unsigned int,
                         const unsigned int );
  vector<unsigned short> get_image_id ();
private:
  vector<ILAC_Square> squares;
  vector<unsigned short> id;
  void calculate_image_id ();
  void check_input ( const string&, Size& );
  void init_chessboard ( const string&, const Size& );
};

class ILACExNoChessboardFound:public std::exception{
  virtual const char* what() const throw(){return "No Chessboard found";}
};

class ILACExNoneRedSquare:public std::exception{
  virtual const char* what() const throw(){return "None red square found";}
};

class ILACExSymmetricalChessboard:public std::exception{
  virtual const char* what() const throw()
  {
    return "Chessboard parity sizes are equal.  The dimenstions of the "
      "chessboard must be odd (5.6 for example). Refer to Learning Opencv "
      "page 382.";
  }
};

class ILACExFileError:public std::exception{
  virtual const char* what() const throw(){return "Image file error.";}
};

class ILACExSizeFormatError:public std::exception{
  virtual const char* what() const throw(){return "Invalid size format";}
};
