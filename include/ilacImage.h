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
#include "ilacChess.h"
#include <opencv2/opencv.hpp>

using namespace cv;

class ILAC_Image{
  public:
    ILAC_Image ();
    ILAC_Image ( const string&, const Size&,
                 const Mat&, const Mat&,
                 const int, const int,
                 const bool = true );
    ~ILAC_Image ();

    vector<unsigned short> getID ();
    void initChess ();
    void calcPixPerUU ();
    void calcID ();
    void calcRefPoints ();
    void normalize ();

    void saveNormalized ( const string& );
    /* Calculate image intrinsics */
    static void calcIntr ( const vector<string>, //image
                           const unsigned int, //size1
                           const unsigned int, //size2
                           Mat&, Mat& );

  private:
    ILAC_Chess_SSD *cb;
    string image_file;
    Mat img; //Original image
    Mat normImg; //Normalized image
    Mat camMat; //Camera intrinsics
    Mat disMat; //Distortion intrinsics.
    vector<unsigned short> id;
    vector<Point2f> plotCorners;
    Size dimension;

    /*
     * Pixels per millimeter. Has errors regarding perspective
     * and overall distortion. We will use it to hint at the
     * sphere detection.
     */
    double pixPerUU;
    int sphDiamUU; /* sphere diameter in whatever UNIT */
    int sqrSideUU; /* size of square in same UNIT as diamUU */

    /*
     * The normWidth variable is the width of the normalized image. It can
     * become non-static in the future. The number 5000 is selected in hope that
     * most images will have a smaller width. For large images, the normalization
     * process will not lose resolution. For smaller images, it will replace
     * non-existing pixels with extrapolations.
     */
    static const int normWidth = 5000;

    /*
     * The relation between width and height in the normalized image. This does
     * not have to do much with the camera and more to do with the shape of the
     * plot. Best case scenario: the plot has normRatio as well.
     */
    static const int normRatio = 1.5;

    static void check_input ( const string&, Size& );
    int calcAngle ( const Point2f&, const Point2f&, const Point2f& );
    Point2f calcChessCenter ( const vector<Point2f> points );
};
