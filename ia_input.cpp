#include "ia_input.h"
#include <cv.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;

/*
 * Input specific functions.
 */
static void
ia_init_input_struct ( struct ia_input *input )
{
  input->iif = (char*)"intrinsics.cfg"; //this is the default value
  input->camMat = Mat::zeros(3, 3, CV_64F);
  input->disMat = Mat::zeros(5, 1, CV_64F);
  input->corDist = false;
  input->calInt = true;

  /*
   * Chessboard default size will be 0,0.  If the user does not specify the
   * sizes, the intrinsics cannot be calculated.
   */
  input->bsize_height = (unsigned int)0;
  input->bsize_width = (unsigned int)0;
  return;
}

static void
ia_free_input_struct ( struct ia_input *input )
{
  input->camMat.release();
  input->disMat.release();
  free(input);
}

void
ia_print_input_struct ( struct ia_input *input )
{
  if ( input == NULL )
    return;

  Mat cm = input->camMat;
  Mat dm = input->disMat;
  fprintf(stdout, "Arguments used for input:\n"

      "Filename: %s\n"
      "Camera distortion: %f, %f, %f, %f, %f\n"
      "Camera Matrix: [ %f, %f, %f;\n"
      "                 %f, %f, %f;\n"
      "                 %f, %f, %f]\n"
      "Undistort flat: %d\n"
      "Calculate Intrinsics: %d\n",

      //file name
      input->iif,
      //camera distortion
      dm.at<float>(0,1), dm.at<float>(0,1), dm.at<float>(0,2),
        dm.at<float>(0,3), dm.at<float>(0,4),
      //camera matrix
      cm.at<float>(0,0),cm.at<float>(0,1),cm.at<float>(0,2),
      cm.at<float>(1,0),cm.at<float>(1,1),cm.at<float>(1,2),
      cm.at<float>(2,0),cm.at<float>(2,1),cm.at<float>(2,2),
      //Undistort flag
      input->corDist,
      //Calculate intrinsics flag
      input->calInt);

  // We print the image list.
  fprintf(stdout, "Image List: ");
  for (int i = 0 ; input->images[i] != '\0' ; i++)
    fprintf(stdout, "%s, ", input->images[i]);
  fprintf(stdout, "\n");
}

void
ia_usage ( char *command )
{
  printf( "%s [OPTIONS] IMAGES\n\n"
          "OPTIONS:\n"
          "-h | --help    Print this help message.\n"
          "-c | --noincalc Avoid intrinsics calculations  from the images.\n"
          "-i | --ininput Intrinsics file name.  Defaults to intrinsics.cfg\n"
          "--cw           Chessboard width in inner squares\n"
          "--ch           Chessboard height in inner squares\n\n"
          "The intrinsics file should have two lines specifying the\n"
          "distortion values and the camera matrix values.  The distortion\n"
          "values are given by a line that begins with 'distortion ' followed\n"
          "by 5 space separated values that specify K1, K2, P1, P2 and K3\n"
          "respectively.  The camera matrix values are given by a line that\n"
          "begins with 'cameramatrix ' and is followed by 9 space separated\n"
          "values that represent a 3x3 matrix.  The first 3 numbers are the\n"
          "first row, the second three are the second row ant the last three\n"
          "are the last row. Example:\n"
          "distortion K1 K2 P1 P2 K3\n"
          "cameramatrix 00(FX) 01 02(CX) 10 11(FY) 12(CY) 20 21 22\n"
          , command);
}

/*
 * Return true if all the intrinsics were found in the filename. false
 * otherwise.
 */
static bool
ia_get_intrinsics_from_file ( char *filename, Mat *camMat, Mat *disMat)
{
  char *line = NULL;
  FILE *fp;
  size_t len;
  ssize_t read;
  bool dist_found = false, cammat_found = false;
  int dist[5], cammat[4];

  /* we try to access the file*/
  fp = fopen(filename, "r");
  if ( fp == NULL )
  {
    fprintf(stderr, "Could not open file: %s\n", filename);
    return false;
  }

  /* we parse the file */
  while ( (read = getline(&line, &len, fp)) != -1 )
  {
    /* We parse the 5 distorition values */
    if ( !dist_found
         && (sscanf( line, "distortion %f %f %f %f %f",
                     &(*disMat).at<float>(0,0), &(*disMat).at<float>(0,1),
                     &(*disMat).at<float>(0,2), &(*disMat).at<float>(0,3),
                     &(*disMat).at<float>(0,4) )
             == 5) )
      dist_found = true;

    /* We parse the 9 cameramatrix values */
    if ( !cammat_found
         && (sscanf( line, "cameramatrix %f %f %f %f %f %f %f %f %f",
                     &(*camMat).at<float>(0,0), &(*camMat).at<float>(0,1),
                     &(*camMat).at<float>(0,2), &(*camMat).at<float>(1,0),
                     &(*camMat).at<float>(1,1), &(*camMat).at<float>(1,2),
                     &(*camMat).at<float>(2,0), &(*camMat).at<float>(2,1),
                     &(*camMat).at<float>(2,2) )
             == 9) )
      cammat_found = true;
  }
  fclose(fp);

  /* we make sure we actually read something */
  if ( !dist_found || !cammat_found )
  {
    fprintf(stderr, "File contained bad format: %s\n", filename);
    return false;
  }

  /* At this point we are confident that we have correctly read the values*/
  return true;
}

/*
 * Will prepare all the input parameters.
 * Returns a pointer to the input struct if all went well.  Pointer to NULL
 * otherwise.
 */
ia_input*
ia_init_input ( int argc, char **argv)
{

  struct ia_input *input;
  //Don't use distortion by default
  int undistort=0, noincalc=0;
  int c;
  static struct option long_options[] =
    {
      /* These options set a flag. */
      {"undistort",     no_argument,          &undistort, 1},
      {"noincalc",      no_argument,          &noincalc, 1},
      /* These options don't set a flag.
      *                   We distinguish them by their indices. */
      {"help",          no_argument,          0, 'h'},
      {"ininput",       required_argument,    0, 'i'},
      {"--ch",          required_argument,    0, 'a'},
      {"--cw",          required_argument,    0, 'b'},
      {0, 0, 0, 0}
    };

  input = (ia_input*)malloc(sizeof(ia_input));
  ia_init_input_struct(input);

  while (1)
  {
    /* getopt_long stores the option index here. */
    int option_index = 0;
    c = getopt_long (argc, argv, "hci:a:b:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c)
      {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        case 'h':
          ia_usage(argv[0]);
          ia_free_input_struct(input);
          return NULL;

        case 'i':
          /* wait until the end of the option parse to try to open */
          if (optarg)
            input->iif = optarg;
          break;

        case 'a':
          if ( sscanf(optarg, "%d", &(input->bsize_height) ) != 1 )
          {
            fprintf(stderr, "Remember to give --ch an argument");
            ia_usage(argv[0]);
            ia_free_input_struct(input);
            return NULL;
          }
          break;

        case 'b':
          if ( sscanf(optarg, "%d", &(input->bsize_width) ) != 1 )
          {
            fprintf(stderr, "Remember to give --cw an argument");
            ia_usage(argv[0]);
            ia_free_input_struct(input);
            return NULL;
          }
          break;



        default:
          ia_usage(argv[0]);
          ia_free_input_struct(input);
          break;
      }
  }

  /*
   * The rest of the arguments are to be considered image file names.  If there
   * are none, we error out.
   */
  if ( optind >= argc )
  {
    // The option indicator is at the last argument and there are not images...
    fprintf(stderr, "You must provide a list of images\n");
    ia_usage(argv[0]);
    return NULL;
  }
  else if ( optind < argc ) // We consider everything as an image name.
    input->images = &argv[optind];

  /*
   * Fill up the intrinsics for the camera
   */
  if ( !ia_get_intrinsics_from_file(input->iif, &(input->camMat), &(input->disMat)) )
  {
    ia_usage(argv[0]);
    ia_free_input_struct(input);
    return NULL;
  }

  // Check the flags.
  if ( undistort )
    input->corDist = true;
  if ( noincalc )
    input->calInt = false;

  // Check to see if the relation between arguments is ok.
  /*
   * Minimum chessboard check.  FIXME: we could do better by checking the
   * relation between sides.  I'll leave it for later.
   */
  if ( input->calInt 
       && ( input->bsize_height <= 0 || input->bsize_width <= 0 ) )
  {
    fprintf(stderr, "To calculate the camera intrinsics you need to provide"
        " positive chessboard height and width values.\n");
    ia_usage(argv[0]);
    ia_free_input_struct(input);
    return NULL;
  }

  // We print the values so the user can see them
  ia_print_input_struct(input);

  return input;
}
