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
#include <opencv2/opencv.hpp>
#include "ilacLabeler.h"
#include "error.h"

using namespace cv;

class ILAC_Chessboard{
public:
  enum { ILACCB_MEDIAN, ILACCB_MAXLIKELIHOOD };

  ILAC_Chessboard ();
  ILAC_Chessboard ( const string&, const Size&, const Mat&, const Mat& );
  ILAC_Chessboard ( const Mat&, const Size&, const int );

  vector<unsigned short> get_image_id (); //Should go away.
  void process_image ( const string, const unsigned int = 80 ); //Should go away.
  vector<int> getAssociation ();

private:
  string image_file; //FIXME: Should go away.
  vector<Point2f> imageCBpoints; // FIXME: should go away.
  vector<unsigned short> id; //FIXME: should go away.
  Size dimension;
  Mat chessboard;
  vector<Point2f> cbPoints;
  vector<ILAC_Square> sampleSquares;
  vector<int> association;
  vector<ILAC_Square> squares; // Data squares.
  Mat camMat; // FIXME: should go away.
  Mat disMat; // FIXME: should go away.

  /* helper functions */
  void init_chessboard ();
  static void check_input ( const string&, Size& ); //FIXME: should go away.
  static double rad2deg ( const double );
  static vector<Point2f> get_image_points ( const Mat&, const Size ); //FIXME: should go away.
};

class ILAC_Image{
  public:
    ILAC_Image ();
    ILAC_Image ( const string&, const Size&, const Mat&, const Mat& );

    vector<unsigned short> getID (); // previously get_image_id
    void normalize ( const string, const unsigned int = 80 ); // previously process_image

    /* Calculate image intrinsics */
    static void calcIntr ( const vector<string>, //image
                           const unsigned int, //size1
                           const unsigned int, //size2
                           Mat&, Mat& );

  private:
    ILAC_Chessboard cb;
    Mat img; //Original image
    Mat normImg; //Normalized image
    Mat camMat; //Camera intrinsics
    Mat disMat; //Distortion intrinsics.
    vector<unsigned short> id;

    static void check_input ( const string&, Size& );
};
