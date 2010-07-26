#include <cv.h>

//Structure holding all the user input and some initial calculations.
struct ia_input
{
  char* iif; //The intrinsic input file.
  bool calInt; //Whether to calculate intrinsic values or not.

  cv::Mat camMat; //The camera matrix.
  cv::Mat disMat; //The distortion matrix.

  bool corDist; //whether to correct distortion or not.


  char** images; // a list of image file names.

  cv::Size b_size; //size of the board.
};

void ia_usage (char*);
ia_input* ia_init_input (int, char**);
void ia_print_input_struct (struct ia_input*);
