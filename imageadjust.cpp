#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include "imageadjust.h"
#include "ia_input.h"


using namespace cv;

bool
ia_calc_image_chess_points ( char **images, const Size boardSize,
                             vector<vector<Point2f> > *imagePoints,
                             Size *generalSize)
{
  for ( int i = 0 ; images[i] != '\0' ; i++ )
  {
    vector<Point2f> pointbuf; //temp buffer for object points
    Mat t_image; //temp image holder

    /*
     * We see if we can actually read the image,  the 0 means we read the image
     * in gray scale.
     */
    t_image = imread(images[i], 0);
    if ( t_image.data == NULL )
    {
      fprintf(stderr, "The image %s could not be read\n", images[i]);
      continue;
    }

    /*
     * We make note of the image size and tell the user if we found something
     * fishy.
     */
    if ( i > 0  //so we don't output unnecessary messages
         && ( t_image.size().height != generalSize->height
              || t_image.size().width != generalSize->width) )
      fprintf(stderr, "Found an image in the list that is of different"
                      " size. height: %u and width: %u.  Note that the"
                      " used size is height: %u and width: %u.",
                      t_image.size().height, t_image.size().width,
                      generalSize->height, generalSize->width);

    else if ( i == 0 ) //We use the initial image size as general size.
      *generalSize = t_image.size();

    /*
     * We try to find the chessboard points in the image and put them in
     * pointbuf.
     */
    if ( !findChessboardCorners(t_image, boardSize, pointbuf,
            CV_CALIB_CB_ADAPTIVE_THRESH) )
    {
      //findChessboardCorners will return 0 if the corners where not found or
      //they could not be organized.
      fprintf(stderr, "Could not find chess corners in image %s\n", images[i]);
      continue;
    }

    // improve the found corners' coordinate accuracy
    cornerSubPix( t_image, pointbuf, Size(11,11),
       Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));

    /*
     * We make sure we keep the image points and count this image as 'found'
     */
    imagePoints->push_back(pointbuf);
  }

  /* We need to had read at least 1 image to continue. */
  if ( imagePoints->size() < 1 )
  {
    fprintf(stderr, "There are not enough recognized images to work with\n");
    return false;
  }

  return true;
}

/**
 * Create the objectPoints vector.  It needs to be of the same size as the
 * imagePoints vector.  All the elements are the same because it is the same
 * object (the chessboard).
 *
 * @param boardSize the size of the board in number of corners
 * @param squareSize  the size of the squares in the board.  should be 1.
 * @param numElems The number of images that were found with
 * ia_calc_image_chess_points.
 */
bool
ia_calc_object_chess_points ( const Size boardSize, const int squareSize,
                              const int numElems,
                              vector<vector<Point3f> > *objectPoints )
{
  // create one element of the objectPoints vector.
  vector<Point3f> corners;
  for ( int i = 0 ; i < boardSize.height ; i++ )
    for ( int j = 0; j < boardSize.width ; j++ )
      corners.push_back( Point3f(float(j*squareSize),float(i*squareSize),0) );

  // replicate that element size times
  for ( int i = 0 ; i < numElems ; i++ )
    objectPoints->push_back(corners);

  return true;
}

/**
 * Calculate all the intrinsics for the image list.
 *
 * @param images A pointer to the image file names.
 * @param boardSize Contain the height and width of the object board.
 * @param camMat Holds the Camera matrix
 * @param disMat Holds the distortion coefficients
 * @param rvecs The list of rotational vectors.
 * @param tvecs The list of translational vectors.
 *
 * @return Returns true on success, false in any other case.
 */
bool
ia_calculate_all ( char **images, const Size boardSize, Mat& camMat,
                   Mat& disMat, vector<Mat>& rvecs, vector<Mat>& tvecs)
{
  vector<vector<Point3f> > objectPoints; //points of the chessboards in the object.
  vector<vector<Point2f> > imagePoints; //points of the chessboards in the image.
  Size generalSize;

  /* get the points and the generalSize for all the images */
  if ( !ia_calc_image_chess_points(images, boardSize, &imagePoints, &generalSize) )
    return false;

  /* get the points for the object. 1->unitless squareSize */
  if ( !ia_calc_object_chess_points (boardSize, 1, imagePoints.size(),
                                     &objectPoints) )
    return false;

  /*
   * Run the calibrateCamera function.  This will give us the distortion matrix
   * the camera matrix the rotation vectors (per image) and the traslation
   * vectors (per image).  No flags are used (the 0 at the end).
   */
  calibrateCamera(objectPoints, imagePoints, generalSize, camMat, disMat,
      rvecs, tvecs, 0);

  return true;
}

bool
ia_calculate_intrinsics ( char **images, const Size boardSize, Mat& camMat,
                          Mat& disMat)
{
  vector<Mat> rvecs, tvecs;
  return ia_calculate_all ( images, boardSize, camMat, disMat, rvecs, tvecs );
}

bool
ia_calculate_extrinsics ( char **images, const Mat& camMat, const Mat& disMat,
                          const Size boardSize, vector<Mat>& rvecs,
                          vector<Mat>& tvecs, bool useExtrinsicGuess = false )
{
  vector<vector<Point3f> > objectPoints; //points of the chessboards in the object.
  vector<vector<Point2f> > imagePoints; //points of the chessboards in the image.
  Size generalSize;

  /* get the points and the generalSize for all the images */
  if ( !ia_calc_image_chess_points(images, boardSize, &imagePoints, &generalSize) )
    return false;

  /* get one set of points for the images. 1->unitless squareSize */
  if ( !ia_calc_object_chess_points (boardSize, 1, 1, &objectPoints) )
    return false;

  /* calculates rvecs and tvecs for each imagePoint */
  vector<vector<Point2f> >::iterator image_iter;
  image_iter = imagePoints.begin();
  while ( image_iter != imagePoints.end() )
  {
    Mat rvec, tvec;
    solvePnP ( (Mat)objectPoints[0], (Mat)*image_iter, camMat, disMat, rvec, tvec,
               useExtrinsicGuess );
    rvecs.push_back(rvec);
    tvecs.push_back(tvec);
    image_iter++;
  }

  return true;
}

int
main (int argc, char** argv )
{
  struct ia_input *input;
  Mat camMat;
  Mat disMat;
  vector<Mat> rvecs, tvecs;

  // Analyze the input
  if ( (input = ia_init_input(argc, argv)) == NULL )
    exit(0); //an error message has already been printed

  // We used the intrinsics from the file if the calculation is avoided
  if ( input->calInt )
    ia_calculate_all ( input->images, input->b_size, camMat, disMat,
                       rvecs, tvecs );
  else
    ia_calculate_extrinsics ( input->images, input->camMat, input->disMat,
                              input->b_size, rvecs, tvecs );
}
