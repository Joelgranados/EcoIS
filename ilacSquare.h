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
#include "error.h"

using namespace cv;

class ILAC_ChessboardImage{
public:
  ILAC_ChessboardImage ();
  ILAC_ChessboardImage ( const string&,
                         const Size&,
                         const unsigned int = 1 );
  ILAC_ChessboardImage ( const string&,
                         const unsigned int, const unsigned int,
                         const unsigned int = 1 );

  vector<unsigned short> get_image_id ();
  void process_image ( const int, //action int
                       const Mat&, //camMat
                       const Mat&, //disMat
                       const int, // Normalization distance.
                       const string ); //output filename

  static void calc_img_intrinsics ( const vector<string>, //image
                                    const unsigned int, //size1
                                    const unsigned int, //size2
                                    const unsigned int, //square size
                                    Mat&, Mat& );

private:
  vector<Point3f> perfectCBpoints;
  vector<Point2f> imageCBpoints;
  vector<unsigned short> id;
  Mat orig_img;
  unsigned int sqr_size;

  /* helper functions */
  static void check_input ( const string&, Size&, const unsigned int = 1 );
  void init_chessboard ( const string&, const Size&, const unsigned int = 1 );
  static double rad2deg ( const double );
  static Size get_size ( unsigned int, unsigned int );

  /* define the argument for the process_image function */
  enum {ILAC_DO_DISTNORM=1, ILAC_DO_ANGLENORM=2, ILAC_DO_UNDISTORT=4};
};
