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
#include <math.h>
#include <opencv2/opencv.hpp>

/*{{{ ILAC_Chessboard*/
ILAC_Chessboard::ILAC_Chessboard (){}/*Used to initialize.*/

/*
 * 1. GET CHESSBOARD POINTS IN IMAGE
 * 2. INITIALIZE THE SQUARES VECTOR BASED ON POINTS
 */
ILAC_Chessboard::ILAC_Chessboard ( const Mat &image, const Size &dimension )
{
  /* 1. GET CHESSBOARD POINTS IN IMAGE */
  try
  {
    Mat g_img; //temp gray image

    cvtColor ( image, g_img, CV_BGR2GRAY );/* transform to grayscale */

    /* find the chessboard points in the image and put them in points.*/
    if ( !findChessboardCorners(g_img, dimension, (cbPoints),
                                CV_CALIB_CB_ADAPTIVE_THRESH) )
      throw ILACExNoChessboardFound();

    else
      /* The 3rd argument is of interest.  It defines the size of the subpix
       * window.  window_size = NUM*2+1.  This means that with 5,5 we have a
       * window of 11x11 pixels.  If the window is too big it will mess up the
       * original corner calculations for small chessboards. */
      cornerSubPix ( g_img, (cbPoints), Size(5,5), Size(-1,-1),
                     TermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1) );
  }catch (cv::Exception){throw ILACExNoChessboardFound();}

  /* 2. INITIALIZE THE SQUARES VECTOR BASED ON POINTS. */
  bool isBlack = true;
  //int numSamples = 6; //FIXME: Generalize this better.

  for ( int r = 0 ; r < dimension.height-1 ; r++ )
    for ( int c = 0 ; c < dimension.width-1 ; c++ )
    {
      if ( !isBlack )
      {
        ILAC_Square tmpSqr = ILAC_Square(
            cbPoints[ (r*dimension.width)+c ], /* upper left */
            cbPoints[ (r*dimension.width)+c+1 ], /* upper right */
            cbPoints[ (r*dimension.width)+dimension.width+c+1 ],/*lower right*/
            cbPoints[ (r*dimension.width)+dimension.width+c ], /*lower left*/
            image );
        this->squares.push_back ( tmpSqr );
      }
      isBlack = !isBlack;
    }

  if ( this->squares.size() <= 0 )
    throw ILACExChessboardTooSmall ();
}

size_t
ILAC_Chessboard::getSquaresSize ()
{
  return this->squares.size();
}

ILAC_Square
ILAC_Chessboard::getSquare ( const size_t offset )
{
  if ( offset < this->squares.size() )
    return this->squares[offset];
  else
    throw ILACExOutOfBounds();
}

vector<Point2f>
ILAC_Chessboard::getPoints ()
{
  return this->cbPoints;
}

ILAC_Chess_SD::ILAC_Chess_SD():ILAC_Chessboard(){}

/*
 * 1. SPLIT INTO SAMPLES AND DATA.
 * 2. CLASSIFY DATA SQUARES
 */
ILAC_Chess_SD::ILAC_Chess_SD ( const Mat &image,
                               const Size &dimension,
                               const int methodology )
  :ILAC_Chessboard ( image, dimension )
{
  ILAC_ColorClassifier *cc;
  vector<ILAC_Square> samples ( this->squares.begin(),
      this->squares.begin() + ILAC_Chess_SD::numSamples );
  vector<ILAC_Square> datas ( this->squares.begin() + ILAC_Chess_SD::numSamples,
      this->squares.end() );

  switch (methodology){
    case (CB_MEDIAN):
      cc = new ILAC_Median_CC ( samples, datas );
      break;
    case (CB_MAXLIKELIHOOD):
      throw ILACExNotImplemented();
      break;
    default:
      throw ILACExInvalidClassifierType();
  }
  cc->classify();
  this->association = cc->getClasses();
  delete cc;
}

size_t //static method
ILAC_Chess_SD::getSamplesSize ()
{
  return ILAC_Chess_SD::numSamples;
}

ILAC_Square
ILAC_Chess_SD::getSampleSquare ( const size_t offset )
{
  if ( offset < ILAC_Chess_SD::numSamples )
    return this->squares[offset];
  else
    throw ILACExOutOfBounds();
}

size_t
ILAC_Chess_SD::getDatasSize ()
{
  return this->squares.size() - ILAC_Chess_SD::numSamples;
}

ILAC_Square
ILAC_Chess_SD::getDataSquare ( const size_t offset )
{
  if ( offset < ( this->squares.size() - ILAC_Chess_SD::numSamples ) )
    return this->squares[ILAC_Chess_SD::numSamples + offset];
  else
    throw ILACExOutOfBounds();
}

vector<int>
ILAC_Chess_SD::getAssociation ()
{
  return this->association;
}

ILAC_Chess_SSD::ILAC_Chess_SSD():ILAC_Chessboard(){}

/*
 * 1. SPLIT INTO SAMPLES AND DATA.
 * 2. CLASSIFY DATA SQUARES
 */
ILAC_Chess_SSD::ILAC_Chess_SSD ( const Mat &image,
                                 const Size &dimension,
                                 const int methodology )
  :ILAC_Chessboard ( image, dimension )
{
  ILAC_ColorClassifier *cc;
  vector<ILAC_Square> samples ( this->squares.begin(),
      this->squares.begin() + ILAC_Chess_SSD::numSamples );
  vector<ILAC_Square> datas (
      this->squares.begin() + ILAC_Chess_SSD::numSamples + 1,
      this->squares.end() );

  switch (methodology){
    case (CB_MEDIAN):
      cc = new ILAC_Median_CC ( samples, datas );
      break;
    case (CB_MAXLIKELIHOOD):
      throw ILACExNotImplemented();
      break;
    default:
      throw ILACExInvalidClassifierType();
  }
  cc->classify();
  this->association = cc->getClasses();
  delete cc;
}

size_t //static method
ILAC_Chess_SSD::getSamplesSize ()
{
  return ILAC_Chess_SSD::numSamples;
}

ILAC_Square
ILAC_Chess_SSD::getSampleSquare ( const size_t offset )
{
  if ( offset < ILAC_Chess_SSD::numSamples )
    return this->squares[offset];
  else
    throw ILACExOutOfBounds();
}

size_t
ILAC_Chess_SSD::getDatasSize ()
{
  return this->squares.size() - ILAC_Chess_SSD::numSamples;
}

ILAC_Square
ILAC_Chess_SSD::getDataSquare ( const size_t offset )
{
  if ( offset < ( this->squares.size() - ILAC_Chess_SSD::numSamples ) )
    return this->squares[ILAC_Chess_SSD::numSamples + offset];
  else
    throw ILACExOutOfBounds();
}

ILAC_Square&
ILAC_Chess_SSD::getSphereSquare ()
{
  return this->squares[ILAC_Chess_SSD::numSamples];
}

vector<int>
ILAC_Chess_SSD::getAssociation ()
{
  return this->association;
}

/*}}} ILAC_Chessboard*/
