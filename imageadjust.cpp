#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <iostream>
//#include <fstream>
#include "imageadjust.h"
#include "ia_input.h"


using namespace cv;
using namespace std;

/*
example command line when 3 cameras are connected.
   tri_calibration  -w 4 -h 5 -s 0.025 -o camera_left.yml -op -oe

 example command line for a list of stored images(for copy-n-paste):
   tri_calibration -w 4 -h 5 -s 0.025 -o camera.yml -op -oe image_list.xml
 where image_list.xml is the standard OpenCV XML/YAML
 file consisting of the list of strings, e.g.:

<?xml version="1.0"?>
<opencv_storage>
<images>
"view000.png"
"view001.png"
<!-- view002.png -->
"view003.png"
"view010.png"
"one_extra_view.jpg"
</images>
</opencv_storage>

 you can also use a video file or live camera input to calibrate the camera

 */

enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };

static double computeReprojectionErrors(
        const vector<vector<Point3f> >& objectPoints,
        const vector<vector<Point2f> >& imagePoints,
        const vector<Mat>& rvecs, const vector<Mat>& tvecs,
        const Mat& cameraMatrix, const Mat& distCoeffs,
        vector<float>& perViewErrors )
{
    vector<Point2f> imagePoints2;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for( i = 0; i < (int)objectPoints.size(); i++ )
    {
        projectPoints(Mat(objectPoints[i]), rvecs[i], tvecs[i],
                      cameraMatrix, distCoeffs, imagePoints2);
        err = norm(Mat(imagePoints[i]), Mat(imagePoints2), CV_L1 );
        int n = (int)objectPoints[i].size();
        perViewErrors[i] = err/n;
        totalErr += err;
        totalPoints += n;
    }

    return totalErr/totalPoints;
}

static void calcChessboardCorners(Size boardSize, float squareSize, vector<Point3f>& corners)
{
    corners.resize(0);

    for( int i = 0; i < boardSize.height; i++ )
        for( int j = 0; j < boardSize.width; j++ )
            corners.push_back(Point3f(float(j*squareSize),
                                      float(i*squareSize), 0));
}

static bool runCalibration( vector<vector<Point2f> > imagePoints,
                    Size imageSize, Size boardSize,
                    float squareSize, float aspectRatio,
                    int flags, Mat& cameraMatrix, Mat& distCoeffs,
                    vector<Mat>& rvecs, vector<Mat>& tvecs,
                    vector<float>& reprojErrs,
                    double& totalAvgErr)
{
    cameraMatrix = Mat::eye(3, 3, CV_64F);
    if( flags & CV_CALIB_FIX_ASPECT_RATIO )
        cameraMatrix.at<double>(0,0) = aspectRatio;

    distCoeffs = Mat::zeros(5, 1, CV_64F);

    vector<vector<Point3f> > objectPoints(1);
    calcChessboardCorners(boardSize, squareSize, objectPoints[0]);
    for( size_t i = 1; i < imagePoints.size(); i++ )
        objectPoints.push_back(objectPoints[0]);

    calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
                    distCoeffs, rvecs, tvecs, flags);

    bool ok = checkRange( cameraMatrix, CV_CHECK_QUIET ) &&
            checkRange( distCoeffs, CV_CHECK_QUIET );

    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints,
                rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs);

    return ok;
}


static bool readStringList( const string& filename, vector<string>& l )
{
    l.resize(0);
    FileStorage fs(filename, FileStorage::READ);
    if( !fs.isOpened() )
        return false;
    FileNode n = fs.getFirstTopLevelNode();
    if( n.type() != FileNode::SEQ )
        return false;
    FileNodeIterator it = n.begin(), it_end = n.end();
    for( ; it != it_end; ++it )
        l.push_back((string)*it);
    return true;
}


bool runAndSave(const string& outputFilename,
                const vector<vector<Point2f> >& imagePoints,
                Size imageSize, Size boardSize, float squareSize,
                float aspectRatio, int flags, Mat& cameraMatrix,
                Mat& distCoeffs, bool writeExtrinsics, bool writePoints )
{
    vector<Mat> rvecs, tvecs;
    vector<float> reprojErrs;
    double totalAvgErr = 0;

    bool ok = runCalibration(imagePoints, imageSize, boardSize, squareSize,
                   aspectRatio, flags, cameraMatrix, distCoeffs,
                   rvecs, tvecs, reprojErrs, totalAvgErr);
    printf("%s. avg reprojection error = %.2f\n",
           ok ? "Calibration succeeded" : "Calibration failed",
           totalAvgErr);

    return ok;
}

/*
 * This function calculates the intrinsics of a camera using a series of
 * pictures that contain a chessboard images.  Preferably the chessboard image
 * should be non-square and should have an even side and an uneven side.  A
 * 5x8 matrix is ok, but a 5x5 or a 4x8 is not.  Refer to
 * http://opencv.willowgarage.com/documentation/cpp/camera_calibration_and_3d_reconstruction.html
 * or similar.
 */
void
ia_calculate_image_intrinsics ( char **images, Mat *camMat, Mat *disMat,
                                Size boardSize )
{
  bool found;
  float squareSize = 1.f;
  float aspectRatio = 1.f;
  Mat image;
  vector<Point2f> pointbuf;

  for ( int i = 0 ; image[i] != '\0' ; i++ )
  {
    // 0 -> lets try with a grayscale of the image.
    image = imread(imags[i], 0);

    try
    {
      found = findChessboardCorners( image, boardSize, pointbuf,
          CV_CALIB_CB_ADAPTIVE_THRESH );
    }
    catch(cv::Exception)
    {
      //FIXME: Do something when the function fails.
      continue;
    }

    if ( found )
    {
        // improve the found corners' coordinate accuracy
        cornerSubPix( image, pointbuf, Size(11,11), Size(-1,-1),
                TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));
    }

    vector<Mat> rvecs, tvecs;
    vector<float> reprojErrs;
    double totalAvgErr = 0;

    bool ok = runCalibration(imagePoints, imageSize, boardSize, squareSize,
                   aspectRatio, flags, cameraMatrix, distCoeffs,
                   rvecs, tvecs, reprojErrs, totalAvgErr);
    printf("%s. avg reprojection error = %.2f\n",
           ok ? "Calibration succeeded" : "Calibration failed",
           totalAvgErr);

}

int
main (int argc, char** argv )
{
  struct ia_input *input;
  Mat *camMat;
  Mat *disMat;

  // Analyze the input
  if ( (input = ia_init_input(argc, argv)) == NULL )
    exit(0); //an error message has already been printed

  // We used the intrinsics from the file if the calculation is avoided
  if ( input->calInt )
  {
    Size boardSize;
    boardSize.width = input->bsize_width;
    boardSize.height = input->bsize_height;
    ia_calculate_image_intrinsics ( input->images, camMat, disMat, boardSize );
  }
  else
  {
    camMat = &(input->camMat);
    disMat = &(input->disMat);
  }
}
