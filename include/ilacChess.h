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
    static size_t getSamplesSize ();
    size_t getDatasSize ();

    ILAC_Square getSquare ( const size_t );
    ILAC_Square getSampleSquare ( const size_t );
    ILAC_Square getDataSquare ( const size_t );

    vector<int> getAssociation ();

    /*
     * This method always runs through all the points.
     */
    Rect getEnclosingRect ( Size );

    static const size_t numSamples = 6;

  protected:
    vector<ILAC_Square> squares; // Data squares.
    vector<int> association;

  private:
    Size dimension;
    vector<Point2f> cbPoints;
};

/* ILAC Chessboard Sampels and Data (SD) */
class ILAC_Chess_SD:public ILAC_Chessboard{
  public:
    ILAC_Chess_SD ();
    ILAC_Chess_SD ( const Mat&, const Size&, const int );
};

/* ILAC Chessboard Sample, Shpere, Data (SSD) */
class ILAC_Chess_SSD:public ILAC_Chessboard{
  public:
    ILAC_Chess_SSD ();
    ILAC_Chess_SSD ( const Mat&, const Size&, const int );

    size_t getDatasSize ();
    ILAC_Square getDataSquare ( const size_t );
    ILAC_Square& getSphereSquare ();
};
