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

#define FLOOR_MIN(p1, p2, p3, p4) \
  floor ( min ( min(p1,p2), min(p3,p4) ) )

#define CEIL_DIST(p1, p2, p3, p4) \
  ceil ( max ( max(p1,p2), max(p3,p4) ) ) \
  - floor ( min ( min(p1,p2), min(p3,p4) ) )

enum alcolor_t { NO_COLOR, RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA };
struct color_hue {
  alcolor_t color;
  int hue;
};

class ILAC_Square{
public:
  ILAC_Square ( const Point2f, const Point2f, const Point2f, const Point2f,
              const Mat& );
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
  ILAC_ChessboardImage ( const string&, Size&, const unsigned int = 1 );
  ILAC_ChessboardImage ( const string&,
                         const unsigned int, const unsigned int,
                         const unsigned int = 1 );
  vector<unsigned short> get_image_id ();
  void process_image ( const int, const Mat&, const Mat&, const int );
private:
  vector<Point3f> perfectCBpoints;
  vector<Point2f> imageCBpoints;
  vector<unsigned short> id;
  vector<ILAC_Square> squares;
  Mat orig_img;
  unsigned int sqr_size;

  void check_input ( const string&, Size&, const unsigned int = 1 );
  void init_chessboard ( const string&, const Size&, const unsigned int = 1 );
  void calc_img_intrinsics ( vector<string>, const Size&, Mat&, Mat& );
  double rad2deg ( const double );

  /* define the argument for the process_image function */
  enum {ILAC_DO_DISTNORM=1, ILAC_DO_ANGLENORM=2, ILAC_DO_UNDISTORT=4};
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
