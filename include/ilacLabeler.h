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
#include <math.h>

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
  Mat get_h_subimg ();

private:
  Mat hsv_subimg; /* minimal subimage that contains the 4 points. */
  Mat h_subimg; /* subimage for the hue dimension */
  int rgb[3]; // Representation of the values in the square.
};

class ILAC_Labeler{
public:
  ILAC_Labeler ();
  ILAC_Labeler ( const Mat&, const vector<Point2f>, const Size );

  vector<unsigned short> calculate_label ();
  vector<unsigned short> get_label ();

private:
  Mat image;
  Size boardSize;
  vector<Point2f> imageCBpoints;
  vector<unsigned short> id;
  vector<ILAC_Square> squares;
};

class ILAC_ColorClassifier{
  public:
    ILAC_ColorClassifier ( const vector<ILAC_Square>&,
                           const vector<ILAC_Square>& );
    vector<int> getClasses ();
    virtual void classify () = 0;

  protected:
    vector<ILAC_Square> samples; //Sample squares
    vector<ILAC_Square> data; //Data squares

    /*
     * Possition in classes refers to data squares.
     * Value in clsses refers to the offset in samples.
     * classes[I] = J -> data square I is of class J.
     */
    vector <int> classes;
};

class ILAC_Median_CC : public ILAC_ColorClassifier{
  public:
    ILAC_Median_CC ( const vector<ILAC_Square>&, const vector<ILAC_Square>& );
    virtual void classify ();

  private:
    int calcHueMedian ( ILAC_Square& );
};
