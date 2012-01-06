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
#include "ilacImage.h"
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <exiv2/exiv2.hpp>
#include <time.h>

/*{{{ ILAC_Image*/
ILAC_Image::ILAC_Image (){}

/*
 * 1. INITIALIZE VARIABLES
 * 2. INITIALIZE CHESSBOARD
 * 3. CALCULATE PIXELS PER MILIMITER
 * 4. CALCULATE IMAGE ID
 * 5. CALCULATE PLOT CORNERS
 */
ILAC_Image::ILAC_Image ( const string &image, const Size &boardSize,
                         const Mat &camMat, const Mat &disMat,
                         const int sqrSideUU, const int sphDiamUU,
                         const bool full )
  :camMat(camMat), disMat(disMat), image_file(image),
   sphDiamUU(sphDiamUU), sqrSideUU(sqrSideUU),
   cb(NULL), pixPerUU(-1), id((unsigned long)-1), plotCorners(), normImg()
{
  /* 1. INITIALIZE VARIABLES*/
  this->dimension.width = max ( boardSize.width, boardSize.height );
  this->dimension.height = min ( boardSize.width, boardSize.height );
  check_input ( image, this->dimension );
  this->img = imread(this->image_file);
  //undistort ( imread(this->image_file), this->img, camMat, disMat );

  if ( full )
  {
    /* 2. INITIALIZE CHESSBOARD*/
    this->initChess ();

    /* 3. CALCULATE PIXELS PER MILIMITER */
    this->calcPixPerUU ();

    /* 4. CALCULATE IMAGE ID */
    this->calcID();

    /* 5. CALCULATE PLOT CORNERS */
    this->calcRefPoints();
  }
}

ILAC_Image::~ILAC_Image () { delete this->cb; }

void
ILAC_Image::calcPixPerUU ()
{
  if ( this->cb->getSquaresSize() < 1 )
    throw ILACExNoChessboardFound ();

  double wAccum = 0, hAccum = 0;
  for ( int i = 0 ; i < this->cb->getSquaresSize() ; i++ )
  {
    wAccum += this->cb->getSquare(i).getImg().size().width;
    hAccum += this->cb->getSquare(i).getImg().size().height;
  }

  /* Average square side size in pixels:
   * averageSide = (averageWidth + averageHeight) / 2 */
  double avePixSide = (wAccum + hAccum)/(2*this->cb->getSquaresSize());
  this->pixPerUU = avePixSide / (double)this->sqrSideUU;
}

/*
 * This function sets the plotCorners up.
 * 1. EXTRACT THE FOUR MARKED POINTS: SPHERES AND CHESSBOARD.
 * 2. ORDER THE POINTS ACCORDINGLY
 */
void
ILAC_Image::calcRefPoints ()
{
  /* 1. EXTRACT THE FOUR MARKED POINTS: SPHERES AND CHESSBOARD. */
  ILAC_SphereFinder sf;
  vector<Point2f> tmpCor; /* Temp Corners */

  vector<ILAC_Sphere> spheres =
    sf.findSpheres ( this->cb->getSphereSquare(), this->img,
                     this->sphDiamUU*this->pixPerUU );
  if ( spheres.size() < 3 )
    throw ILACExLessThanThreeSpheres();
  else if ( spheres.size() > 3 )
    spheres.erase ( spheres.begin()+3, spheres.end() );

  tmpCor.push_back ( this->calcChessCenter(this->cb->getPoints()) );
  for ( vector<ILAC_Sphere>::iterator sphere = spheres.begin() ;
      sphere != spheres.end() ; ++sphere )
    tmpCor.push_back ( (*sphere).getCenter() );

  /* 2. ORDER THE POINTS ACCORDINGLY */
  convexHull ( tmpCor, this->plotCorners ); /*counter clockwise by default*/
  while ( this->plotCorners[0] == tmpCor[0] ) /* put chessboard at [0]*/
    rotate(this->plotCorners.begin(),
           this->plotCorners.end(),
           this->plotCorners.end() );
}

void
ILAC_Image::calcID ()
{
  /* Reset the class id value */
  this->id = (unsigned long)0;

  for ( int i = this->cb->getAssociation().size() - 1 ; i >= 0 ; i-- )
  {
    /* Don't consider case 2,3,4 because red is on*/
    int r, g, b;
    switch ( this->cb->getAssociation()[i])
    {
      case 0:
        r=1;g=0;b=0;
        break;
      case 1:
        r=1;g=1;b=0;
        break;
      case 5:
        r=1;g=0;b=1;
        break;
      default:
        r=0;g=0;b=0;
       break;
    }

    /* All the colored squares should have red bit on.*/
    if ( r != 1 ) throw ILACExNoneRedSquare();

    /* modify the blue bit */
    if ( b ) this->id = this->id | (unsigned long)1;

    /* modify the green bit */
    if ( g ) this->id = this->id | (unsigned long)2;

    /* bit shift for green and blue */
    this->id = this->id<<2;
  }
}

void
ILAC_Image::initChess ()
{
  this->cb = new ILAC_Chess_SSD( this->img,
                                 this->dimension,
                                 ILAC_Chessboard::CB_MEDIAN );
}

unsigned long
ILAC_Image::getID () {
  /* This depends on initChess & calcID */
  if ( this->cb == NULL )
    this->initChess ();
  if ( this->id == -1 )
    this->calcID ();

  return this->id;
}

void
ILAC_Image::normalize ()
{
  /* This depends on plotPoints, chessboard & pixPerUU */
  if ( this->cb == NULL )
    this->initChess ();
  if ( this->pixPerUU == -1 )
    this->calcPixPerUU ();
  if ( this->plotCorners.size() == 0 )
    this->calcRefPoints();

  Mat persTrans;
  int width = ILAC_Image::normWidth;
  int height = width/ILAC_Image::normRatio;

  Point2f tvsrc[4] = { this->plotCorners[0], this->plotCorners[1],
                       this->plotCorners[2], this->plotCorners[3] };
  Point2f tvdst[4] = { Point2f(height,0), Point2f(height,width),
                       Point2f(0,width), Point2f(0,0) };

  Size endSize(height,width); /* Size(rows,cols)*/
  persTrans = getPerspectiveTransform ( tvsrc, tvdst );
  warpPerspective ( this->img, this->normImg, persTrans, endSize );
}

/*
 * 1. SAVE NORMALIZED IMAGE
 * 2. ADD EXIF DATA TO THE NEWLY CREATED IMAGE
 */
void
ILAC_Image::saveNormalized ( const string &fileName, const bool overwrite )
{
  /* 1. SAVE NORMALIZED IMAGE */
  /* Depends on normalize being executed */
  if ( this->normImg.size() == Size(0,0) )
    this->normalize();

  if ( !overwrite )
  {
    struct stat file_stat;
    if ( stat ( fileName.data(), &file_stat ) == 0 )
      throw ILACExFileError(); /* Do not overwrite */
  }
  imwrite ( fileName, this->normImg );

  /* 2. ADD EXIF DATA TO THE NEWLY CREATED IMAGE */
  //FIXME: catch the warnings from the output.
  time_t rawtime;
  time (&rawtime);
  std::string date = ctime(&rawtime); /*We need to remove trailing \n*/

  std::stringstream userComment;
  userComment << "charset=Ascii normalized="
              << date.substr(0,date.find_last_of('\n'))
              << ",plotid="
              << this->id
              << ",";

  Exiv2::Image::AutoPtr srcImg = Exiv2::ImageFactory::open(this->image_file);
  Exiv2::Image::AutoPtr dstImg = Exiv2::ImageFactory::open(fileName);

  srcImg->readMetadata ();
  Exiv2::ExifData &srcExifData = srcImg->exifData();
  srcExifData ["Exif.Photo.UserComment"] = userComment.str();
  dstImg->setExifData ( srcExifData );
  dstImg->writeMetadata ();
}

void
ILAC_Image::calcIntr ( const vector<string> images,
                       const unsigned int size1,
                       const unsigned int size2,
                       Mat &camMat, Mat &disMat )
{
  Mat tmp_img;
  vector<Point2f> pointbuf;
  vector<Point3f> corners;
  vector< vector<Point2f> > imagePoints;
  vector< vector<Point3f> > objectPoints;
  vector<Mat> rvecs, tvecs;
  Size boardSize;
  int sqr_size = 1;

  /* 1. CREATE IMAGEPOINTS.*/
  boardSize.width = max ( size1, size2 );
  boardSize.height = min ( size1, size2 );
  for ( vector<string>::const_iterator img = images.begin() ;
        img != images.end() ; ++img )
    try {
      check_input ( (*img), boardSize );/*validate args*/
      cvtColor ( imread ( (*img) ), tmp_img, CV_BGR2GRAY );/*to grayscale*/
      if ( !findChessboardCorners ( tmp_img, boardSize, pointbuf,
                                    CV_CALIB_CB_ADAPTIVE_THRESH ) )
        continue;
      else
        cornerSubPix ( tmp_img, pointbuf, Size(5,5), Size(-1,-1),
                    TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ) );

      imagePoints.push_back(pointbuf); /*keep image points */
    }catch(ILACExFileError){continue;}
     catch(cv::Exception){continue;}

  if ( imagePoints.size() <= 0 )/* Need at least one element */
    throw ILACExNoChessboardFound();

  /* 2. CREATE OBJECTPOINTS.*/
  for ( int i = 0 ; i < boardSize.height ; i++ )
    for ( int j = 0; j < boardSize.width ; j++ )
      corners.push_back( Point3f( double(j*sqr_size),
                                  double(i*sqr_size),
                                  0 ) );

  /* replicate that element imagePoints.size() times */
  for ( int i = 0 ; i < imagePoints.size() ; i++ )
    objectPoints.push_back(corners);

  /* 3. CALL CALIBRATE CAMERA. find camMat, disMat */
  calibrateCamera( objectPoints, imagePoints, tmp_img.size(),
                   camMat, disMat, rvecs, tvecs, 0 );
}

void //static method
ILAC_Image::check_input ( const string &image, Size &boardSize )
{
  // Check that file exists.
  struct stat file_stat;
  if ( stat ( image.data(), &file_stat ) != 0 )
    throw ILACExFileError();

  /* We need a chessboard of odd dimensions (6,5 for example).  This gives us
   * a chessboard with only one symmetry axis.  We use this in order to identify
   * a unique origin. <ISBN 978-0-596-51613-0 Learning Opencv page 382> */
  if ( boardSize.height % 2 == boardSize.width % 2 )
    throw ILACExSymmetricalChessboard();
}

/* Helper function. Calculates the angle made up by A-V-B */
int
ILAC_Image::calcAngle ( const Point2f &V, const Point2f &A, const Point2f &B )
{
  /* 1. Calculate the lengths of the oposite lines */
  double a_opp, b_opp, v_opp, retAng;
  a_opp = sqrt ( pow( (V.x-B.x),2 ) - pow( (V.y-B.y),2 ) );
  b_opp = sqrt ( pow( (V.x-A.x),2 ) - pow( (V.y-A.y),2 ) );
  v_opp = sqrt ( pow( (A.x-B.x),2 ) - pow( (A.y-B.y),2 ) );
  retAng = (int) acos ( (pow(a_opp,2) + pow(b_opp,2) - pow(v_opp,2))
                        / ( 2 * a_opp * b_opp ) );
  return retAng;
}

Point2f
ILAC_Image::calcChessCenter ( vector<Point2f> points )
{
  Point2f retVal;
  double accumWidth=0, accumHeight=0;

  for ( vector<Point2f>::iterator point = points.begin() ;
        point != points.end() ; point++ )
  {
    accumWidth = accumWidth + (double)((*point).x);
    accumHeight = accumHeight + (double)((*point).y);
  }

  retVal.x = ceil((double)accumWidth/points.size());
  retVal.y = ceil((double)accumHeight/points.size());

  return retVal;
}
/*}}} ILAC_Image*/
