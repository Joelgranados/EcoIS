#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <time.h>
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
  vector<vector<Point3f> > objectPoints; //chessboard points in the object.
  vector<vector<Point2f> > imagePoints; //chessboard points in the image.
  Size generalSize;

  /* get the points and the generalSize for all the images */
  if ( !ia_calc_image_chess_points(images, boardSize, &imagePoints, &generalSize) )
    return false;

  /* get one set of points for the images. 1->unitless squareSize */
  if ( !ia_calc_object_chess_points (boardSize, 1, 1, &objectPoints) )
    return false;

  /* calculates rvecs and tvecs for each imagePoint */
  for ( vector<vector<Point2f> >::iterator image_iter = imagePoints.begin() ;
      image_iter != imagePoints.end() ; image_iter++ )
  {
    Mat rvec, tvec;
    solvePnP ( (Mat)objectPoints[0], (Mat)*image_iter, camMat, disMat,
               rvec, tvec, useExtrinsicGuess );
    rvecs.push_back(rvec);
    tvecs.push_back(tvec);
  }

  return true;
}

void
ia_print_matrix ( Mat mat )
{
  Size m_size = mat.size();
  for ( int i = 0 ; i < m_size.height ; i++ )
    for ( int j = 0 ; j < m_size.width ; j++ )
      fprintf( stdout, "%f ", mat.at<float>(i,j) );
  fprintf ( stdout, "\n" );
}
/**
 * @param matrix is  vector of matrices
 * @param message is the message that should appear before printing the matrix.
 *
 */
void
ia_print_matrix_vector ( vector<Mat>* vec, char* message )
{
  fprintf ( stdout, "%s\n", message );

  for ( vector<Mat>::iterator iter = vec->begin() ; iter != vec->end() ;
      iter++ )
    ia_print_matrix ( *iter );
}

void
ia_calculate_and_capture ( Size boardSize )
{

  /* Reflects the process state of the function. start accumulating. */
  enum proc_state { ACCUM, OUTPUT, CALC } p_state = ACCUM;

  VideoCapture capture;
  vector<vector<Point2f> > imagePoints;
  vector<vector<Point3f> > objectPoints;
  Mat camMat, disMat; //matrices that will hold the camera and distorition info
  Mat rvec, tvec; //translation and rotation vectors.
  Size generalSize;
  string msg;
  clock_t timestamp = 0;
  int delay = 1000; //one second
  int num_int_images = 20; //number of intrinsic images needed

  //open a window...
  namedWindow( "Image View", 1 );

  // We start the capture.
  // FIXME: make the user define the camera id.
  capture.open(0);

  for ( int i = 0 ;; i++ )
  {
    Mat view0, t_image;
    vector<Point2f> pointbuf;

    // loop until capture is opened
    if ( !capture.isOpened() )
      continue;

    capture >> view0;

    try
    {
      /* transform to grayscale */
      cvtColor(view0, t_image, CV_BGR2GRAY);

    /*
     * We try to find the chessboard points in the image and put them in
     * pointbuf.
     */
      if ( !findChessboardCorners(t_image, boardSize, pointbuf,
              CV_CALIB_CB_ADAPTIVE_THRESH) )
      {
        imshow("Image View", t_image);
        waitKey(50);
        continue; //We will get another change in the next image
      }
    }catch (cv::Exception){continue;}

    /* improve the found corners' coordinate accuracy */
    cornerSubPix ( t_image, pointbuf, Size(11,11),
       Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ) );

    switch ( p_state )
    {
      case OUTPUT:
        /* calculate the rvec and tvec.  Note that we use the camMat and disMat*/
        solvePnP ( (Mat)objectPoints[0], (Mat)pointbuf, camMat, disMat,
                   rvec, tvec );

        /* output rvec and tvec to stdout */
        fprintf ( stdout, "--------------------------\n");
        fprintf ( stdout, "These are the tvecs\n" );
        ia_print_matrix ( tvec );
        fprintf ( stdout, "These are the rvecs\n" );
        ia_print_matrix ( rvec );
      break;

      case ACCUM: //accumulate info to calculate intrinsics.
        /*
         * We make sure we keep the image points.
         */
        if ( clock() - timestamp > delay*1e-3*CLOCKS_PER_SEC )
        {
          imagePoints.push_back(pointbuf);
          timestamp = clock();
        }

        /* Create and put message on image */
        msg = format ( "Cal Intrinsics: %d/%d.", imagePoints.size(), num_int_images );
        int baseLine = 0;
        Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(t_image.cols - 2*textSize.width - 10, t_image.rows - 2*baseLine - 10);
        putText ( t_image, msg, textOrigin, 1, 1, Scalar(0,0,255) );

        /* we change state when we have enough images */
        if ( imagePoints.size() >= 20 )
        {
          /* We use the last image size as generalSize.*/
          generalSize = t_image.size();

          /* get the points for the object. 1->unitless squareSize */
          ia_calc_object_chess_points (boardSize, 1, imagePoints.size(), &objectPoints);

          /*
           * Run the calibrateCamera function.  This will give us the distortion matrix
           * the camera matrix the rotation vectors (per image) and the traslation
           * vectors (per image).  No flags are used (the 0 at the end).
           */
          vector<Mat> rvecs, tvecs; // will not be used in other places.
          calibrateCamera(objectPoints, imagePoints, generalSize, camMat, disMat,
              rvecs, tvecs, 0);

          p_state = OUTPUT;
        }
      break;
    }

    /* Draw chessboard on image.*/
    drawChessboardCorners( t_image, boardSize, Mat(pointbuf), true );

    /* finally, show image :) */
    imshow("Image View", t_image);

    /* we wait for user interaction */
    int key = waitKey(50);
    if( (key & 255) == 27 )
        break;
  }
}

int
main ( int argc, char** argv ) {
  struct ia_input *input;
  Mat camMat;
  Mat disMat;
  vector<Mat> rvecs, tvecs;

  // Analyze the input
  if ( (input = ia_init_input(argc, argv)) == NULL )
    exit(0); //an error message has already been printed

  if ( input->capture )
    ia_calculate_and_capture ( input->b_size );
  else if ( input->calInt )
    ia_calculate_all ( input->images, input->b_size, camMat, disMat,
                       rvecs, tvecs );

  else  // We used the intrinsics from the file if the calculation is avoided
    ia_calculate_extrinsics ( input->images, input->camMat, input->disMat,
                              input->b_size, rvecs, tvecs );

  ia_print_matrix_vector ( &rvecs,
      (char*)"These are the rvecs for the images" );
  ia_print_matrix_vector ( &tvecs,
      (char*)"These are the tvecs for the images" );

}
