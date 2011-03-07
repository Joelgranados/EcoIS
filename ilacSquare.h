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
                         const Mat&, //camMat
                         const Mat& ); //disMat

  vector<unsigned short> get_image_id ();
  void process_image ( const string, //output filename
                       const unsigned int = 80 );

  static void calc_img_intrinsics ( const vector<string>, //image
                                    const unsigned int, //size1
                                    const unsigned int, //size2
                                    const unsigned int, //square size
                                    Mat&, Mat& );

private:
  vector<Point2f> imageCBpoints;
  vector<unsigned short> id;
  Size boardSize;
  Mat orig_img;
  Mat camMat;
  Mat disMat;

  /* helper functions */
  static void check_input ( const string&, Size& );
  void init_chessboard ( const string&, const Size& );
  static double rad2deg ( const double );
  static Size get_size ( unsigned int, unsigned int );
  static vector<Point2f> get_image_points ( const Mat&, const Size );
};
