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
#include "ilacLabeler.h"
#include "error.h"
#include <opencv2/opencv.hpp>

/*{{{ ILAC_Square*/
/* Notice ul:UpperLeft, ur:UpperRight, lr:LowerRight, ll:LowerLeft*/
ILAC_Square::ILAC_Square ( const Point2f ul, const Point2f ur,
                       const Point2f lr, const Point2f ll,
                       const Mat& img )
{
  /* Calculate the enclosing rectangle. referenced to the original image */
  Rect t_rect = Rect( /* helper rectangle (x, y, width, height) */
    FLOOR_MIN (ul.x, ur.x, lr.x, ll.x), FLOOR_MIN (ul.y, ur.y, lr.y, ll.y),
    CEIL_DIST (ul.x, ur.x, lr.x, ll.x), CEIL_DIST (ul.y, ur.y, lr.y, ll.y) );

  /* Calculate source and destination points for the perspective transform.*/
  Point2f s[4] = {ul, ur, lr, ll};
  for ( int i = 0 ; i < 4 ; i++ )
  {
    /* Remember that ul and its brothers refer to the position with respect to
     * the chessboard, not the image. */
    s[i].x = floor(s[i].x - t_rect.x);
    s[i].y = floor(s[i].y - t_rect.y);
  }
  Point2f d[4]={Point2f(0,0), Point2f(t_rect.width,0),
                Point2f(t_rect.width, t_rect.height), Point2f(0,t_rect.height)};

  /* Calculate the perspective transform and create a subimage. */
  Mat t_img = Mat( img, t_rect );
  warpPerspective( t_img, this->img,
                   getPerspectiveTransform(s,d), t_img.size() );
}

Mat
ILAC_Square::getImg ()
{
  return this->img;
}
/*}}} ILAC_Square*/

/*{{{ ILAC_ColorClassifiers*/
ILAC_ColorClassifier::ILAC_ColorClassifier
  ( const vector<ILAC_Square>& samples, const vector<ILAC_Square>& data )
{
  this->samples = samples;
  this->data = data;
  this->classes.resize(this->data.size());
}

vector<int>
ILAC_ColorClassifier::getClasses ()
{
  return this->classes;
}

/*
 * Sample colors should be ordered: red, yellow, green, cyan, blue, magenta.
 * This cannot be checked.The caller must make sure.
 */
ILAC_Median_CC::ILAC_Median_CC
  ( const vector<ILAC_Square>& samples, const vector<ILAC_Square>& data ):
    ILAC_ColorClassifier ( samples, data )
{
  /* Detection only works with 6 colors :)*/
  if ( samples.size() != 6 )
    throw ILACExTooManyColors();
}

void
ILAC_Median_CC::classify ()
{
  /*
   * 1 CREATE RANGE ARRAY
   * Color mapping: hRange[0] < RED < hRange[1] |
   *                hRange[1] < YELLOW < hRange[2] |
   *                ... |
   *                hRange[6] < RED < hRange[7]
   * Ranges are sorted in increasing order.
   */
  vector<int> hRange(8,0);

  /*
   * Fill the range with colors and means. Order based on hue. Make sure
   * smallest and largets values in the endpoints.
   */
  for ( int i = 0 ; i < 6 ; i++ )
  {
    hRange[i] = this->calcHueMedian(samples[i]);
    for ( int j = i ; j > 0 && hRange[j] < hRange[j-1] ; j-- )
      std::swap( hRange[j], hRange[j-1] );
  }

  /*
   * Calculate the max ranges. The midpoints between two medians are calculated
   * by adding half (/2) the distance (subtraction) -between to medians- to the
   * smallest median; and making sure it does not exceed 256 (%256)
   */
  hRange[6] = 256+hRange[0];
  for ( int i = 6 ; i > 0 ; i-- )
    hRange[i] = ( hRange[i-1]
                  + ((hRange[i] - hRange[i-1]) / 2) )
                % 256;

  /*
   * There is a possibility that things are not in order. We do this because the
   * hue 257 is equal to 0 (and so on)
   */
  for ( int i = 1 ; i < 7 ; i++ )
    for ( int j = i ; j > 1 && hRange[j] < hRange[j-1] ; j-- )
      std::swap( hRange[j], hRange[j-1] );

  /* We polish the ends. */
  hRange[0] = 0;
  hRange[7] = 256;

  /*
   * 2.CALCULATE CLASS
   * Each accumulator offset will represent a color.
   * c_accum[0] -> red,  c_accum[1] -> yellow, c_accum[2] -> green,
   * c_accum[3] -> cyan, c_accum[4] -> Blue,   c_accum[5] -> magenta
   * These values are related to the range argument.
   * The color with more hits is the one that is chosen.
   */
  Mat hImg;
  uchar *data_ptr;

  int coffset = 0;
  for ( vector<ILAC_Square>::iterator _data = data.begin();
        _data != data.end() ; ++_data, coffset++ )
  {
    /* Transform from BGR to HSV */
    {
      Mat hsvImg;
      cvtColor ( (*_data).getImg(), hsvImg, CV_BGR2HSV_FULL );
      vector<Mat> tmp_dim;
      split( hsvImg, tmp_dim );
      hImg = tmp_dim[0];
    }

    unsigned long c_accum[6] = {0};
    for ( unsigned int row = 0 ; row <= hImg.rows ; row++ )
      for ( unsigned int col = 0 ; col <= hImg.cols ; col++ )
      {
        data_ptr = hImg.data + (row*hImg.cols) + col;
        for ( int j = 0 ; j < 8 ; j++ )
          if ( hRange[j] > *data_ptr )
          {
            c_accum[(j-1)%6]++;
            break;
          }
      }

    /* find where the maximum offset is*/
    int max_offset = 0;
    for ( int i = 0 ; i < 6 ; i++ )
      if ( c_accum[max_offset] < c_accum[i] )
        max_offset = i;

    this->classes[coffset] = max_offset;
  }
}

int
ILAC_Median_CC::calcHueMedian ( ILAC_Square &square )
{
  Mat hImg;
  /* Transform from BGR to HSV */
  {
    Mat hsvImg;
    cvtColor ( square.getImg(), hsvImg, CV_BGR2HSV_FULL );
    vector<Mat> tmp_dim;
    //FIXME: try using mixchannels to avoid s and v
    split( hsvImg, tmp_dim );
    hImg = tmp_dim[0];
  }

  uchar *data_ptr;
  unsigned long c_accum[256] = {0};
  for ( unsigned int row = 0 ; row <= hImg.rows ; row++ )
    for ( unsigned int col = 0 ; col <= hImg.cols ; col++ )
    {
      data_ptr = hImg.data + (row*hImg.cols) + col;
      c_accum[ *data_ptr ]++;
    }

  /* The median is the offset in c_accum where we cross the middle of the data
   * set. */
  unsigned long hp_size = (hImg.rows * hImg.cols)/2; //half population size
  unsigned long accum_size = 0;
  int median = 0;
  for ( ; median <= 256 ; median++ )
  {
    if ( accum_size > hp_size )
      break;// We have found the median
    accum_size = accum_size + c_accum[median];
  }

  return median;
}
/*}}} ILAC_ColorClassifiers*/

/*{{{ ILAC_Sphere and related*/
ILAC_Sphere::ILAC_Sphere ()
{
  this->img = NULL;
  this->center = Point(0,0);
  this->radius = 0;
}


ILAC_Sphere::ILAC_Sphere
  ( const Mat *img, const Point center, const int radius)
{
  this->img = (Mat*)img;
  this->center = (Point)center;
  this->radius = (int)radius;
}

Mat*
ILAC_Sphere::getImg()
{
  return this->img;
}

Point
ILAC_Sphere::getCenter()
{
  return this->center;
}

int
ILAC_Sphere::getRadius()
{
  return this->radius;
}

ILAC_SphereFinder::ILAC_SphereFinder () {}

/*
 * 1. CALCULATE RANGE FROM MEAN AND STANDARD DEVIATION
 * 2. CREATE A MASK FROM THE RANGE
 * 3. SMOOTH STUFF USING MORPHOLOGY
 * 4. DETECT THE CIRCLES
 */
vector<ILAC_Sphere>
ILAC_SphereFinder::findSpheres ( ILAC_Square &square, Mat &img )
{
  /* 1. CALCULATE RANGE FROM MEAN AND STANDARD DEVIATION */
  Mat mean, stddev;
  {/* Isolate the Hue */
    Mat tmpImg;
    cvtColor ( square.getImg(), tmpImg, CV_BGR2HSV_FULL );
    vector<Mat> tmp_dim;
    split( tmpImg, tmp_dim );
    tmpImg = tmp_dim[0];
    meanStdDev ( tmpImg, mean, stddev );
  }

  /*
   * Range will be -+ 1 standard deviation. This has aprox 68% of the data
   * (http://en.wikipedia.org/wiki/Standard_deviation)
   */
  Mat lowerb = mean - stddev;
  Mat upperb = mean + stddev;

  /* 2. CREATE A MASK FROM THE RANGE */
  Mat mask = Mat::ones(img.rows, img.cols, CV_8UC1);
  inRange(img, lowerb, upperb, mask);

  /* 3. SMOOTH STUFF USING MORPHOLOGY */
  /* Playing with the size of the structuring elements might be good */
  {
    Mat se1 = getStructuringElement ( MORPH_RECT, Size(21,10) );
    Mat se2 = getStructuringElement ( MORPH_RECT, Size(11,5) );
    morphologyEx ( mask, mask, MORPH_CLOSE, se1 );
    morphologyEx ( mask, mask, MORPH_OPEN, se2 );
  }

  /* 4. DETECT THE CIRCLES */
  /* Play with the arguments for HoughCircles. */
  vector<Vec3f> circles;
  vector<ILAC_Sphere> spheres;

  GaussianBlur ( mask, mask, Size(15, 15), 2, 2 );
  HoughCircles ( mask, circles, CV_HOUGH_GRADIENT, 2, mask.rows/10, 100, 40);

  for( size_t i = 0; i < circles.size(); i++ )
  {
    Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
    int radius = cvRound(circles[i][2]);
    ILAC_Sphere temp ( &img, center, radius );

    /* Add spheres in descending order */
    if ( spheres.size() == 0 )
      spheres.push_back ( temp );
    else
      for ( size_t j = 0 ; j < spheres.size() ; j++ )
        if ( temp.getRadius() > spheres[j].getRadius() )
        {
          spheres.insert ( spheres.begin()+j, temp );
          break;
        }
  }

  if ( spheres.size() < 3 )
    throw ILACExLessThanThreeSpheres ();

  return spheres;
}
/*}}} ILAC_Sphere and related*/

