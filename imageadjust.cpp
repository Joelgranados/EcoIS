#include <cv.h>
#include <highgui.h>
#include "ia_core.h"
#include "ia_input.h"

int
main ( int argc, char** argv ) {
  struct ia_input *input;
  Mat camMat;
  Mat disMat;
  vector<Mat> rvecs, tvecs;

  /* parse intput */
  if ( (input = ia_init_input(argc, argv)) == NULL )
    exit(0); //an error message has already been printed

  if ( input->capture )
    ia_calculate_and_capture ( input->b_size );
  else if ( input->calInt )
    ia_calculate_all ( input->images, input->b_size, &camMat, &disMat,
                       &rvecs, &tvecs );

  else  // We used the intrinsics from the file if the calculation is avoided
    ia_calculate_extrinsics ( input->images, &(input->camMat), &(input->disMat),
                              input->b_size, &rvecs, &tvecs );

}
