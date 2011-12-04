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
  enum { CB_MEDIAN, CB_MAXLIKELIHOOD };

  ILAC_Chessboard ();
  ILAC_Chessboard ( const Mat&, const Size& );

  vector<Point2f> getPoints ();
  size_t getSquaresSize ();
  ILAC_Square getSquare ( const size_t );

protected:
  Size dimension;
  vector<Point2f> cbPoints;
  vector<ILAC_Square> squares; // Data squares.
};

//FIXME: We should be able to put ILAC_Chess_SD and ILAC_Chess_SSD together.
/* ILAC Chessboard Sampels and Data (SD) */
class ILAC_Chess_SD:public ILAC_Chessboard{
  public:
    ILAC_Chess_SD ();
    ILAC_Chess_SD ( const Mat&, const Size&, const int );

    ILAC_Square getSampleSquare ( const size_t );
    size_t getDatasSize ();
    ILAC_Square getDataSquare ( const size_t );
    vector<int> getAssociation ();

    static size_t getSamplesSize ();
    static const size_t numSamples = 6;

  private:
    vector<int> association;
};

/* ILAC Chessboard Sample, Shpere, Data (SSD) */
class ILAC_Chess_SSD:public ILAC_Chessboard{
  public:
    ILAC_Chess_SSD ();
    ILAC_Chess_SSD ( const Mat&, const Size&, const int );

    ILAC_Square getSampleSquare ( const size_t );
    size_t getDatasSize ();
    ILAC_Square getDataSquare ( const size_t );
    vector<int> getAssociation ();
    ILAC_Square& getSphereSquare ();

    static size_t getSamplesSize ();
    static const size_t numSamples = 6;

  private:
    vector<int> association;
};

class ILAC_Image{
  public:
    ILAC_Image ();
    ILAC_Image ( const string&, const Size&,
                 const Mat&, const Mat&, const bool = true );
    ~ILAC_Image ();

    vector<unsigned short> getID ();
    void initChess ();
    void calcID ();
    void calcRefPoints ();
    void normalize ( const unsigned int = 80 );

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

    static void check_input ( const string&, Size& );
    int calcAngle ( const Point2f&, const Point2f&, const Point2f& );
    Point2f calcChessCenter ( const vector<Point2f> points );
};
