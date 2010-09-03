/*
 * image adjust.  Automatic image normalization.
 * Copyright (C) 2010 Joel Granados <joel.granados@gmail.com>
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
  /*
   * We default to calculating the intrinsics of the camera.  User must specify
   * an intrinsics file to change behavior.
   */
  input->iif = NULL;

  input->camMat = Mat::eye(3, 3, CV_64F);
  input->disMat = Mat::zeros(5, 1, CV_64F);
  input->corDist = false;

  input->camera_id = -1; //no camera id unless specified.
  input->vid_file = NULL;

  /*
   * Chessboard default size will be 0,0.  If the user does not specify the
   * sizes, the intrinsics cannot be calculated.
   */
  input->b_size.height = (unsigned int)0;
  input->b_size.width = (unsigned int)0;

  input->squareSize = 1;
  input->delay = 250;

  input->images = NULL;

  input->num_in_img = 20;

  /* default command objective will be to create a config file */
  input->objective = NONE;
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
      "BoardSize (w,h): (%u, %u)\n"
      "SquareSize: %f\n"
      "Delay: %d\n"
      "Video File: %s\n"
      "Camera id: %d\n"
      "Number of Images for Intrinsics: %d\n",

      //file name
      input->iif,
      //camera distortion
      dm.at<double>(0,0), dm.at<double>(0,1), dm.at<double>(0,2),
        dm.at<double>(0,3), dm.at<double>(0,4),
      //camera matrix
      cm.at<double>(0,0),cm.at<double>(0,1),cm.at<double>(0,2),
      cm.at<double>(1,0),cm.at<double>(1,1),cm.at<double>(1,2),
      cm.at<double>(2,0),cm.at<double>(2,1),cm.at<double>(2,2),
      //Undistort flag
      input->corDist,
      //chessboard sizes.
      input->b_size.width, input->b_size.height,
      //square size
      input->squareSize,
      input->delay,
      input->vid_file,
      input->camera_id,
      input->num_in_img);

  // We print the image list.
  fprintf(stdout, "Image List: ");
  for (int i = 0 ; input->images != NULL && input->images[i] != '\0' ; i++)
    fprintf(stdout, "%s, ", input->images[i]);
  fprintf(stdout, "\n");
}

void
ia_usage ( char *command )
{
  printf( "%s [OPTIONS] IMAGES\n\n"
          "OPTIONS:\n"
          "-h | --help    Print this help message.\n"
          "-i | --ininput Intrinsics file name.  Defaults to intrinsics.cfg\n"
          "-W | --cw      Chessboard width in inner squares\n"
          "-H | --ch      Chessboard height in inner squares\n"
          "-s | --squaresize\n"
          "               The size of the chessboard square.  1 by default\n"
          "               The resulting values will be given with respect to\n"
          "               this number.\n"
          "-d | --delay   The delay time in miliseconds between events when\n"
          "               capturing.\n"
          "-v | --video   Use a video file. Supporst whatever opencv supports.\n"
          "-C | --camera  The camera id.\n"
          "-I | --num_int The number of images to calculate intrinsic data.\n"
          "               Defaults to 20. -1 means use all images/frames.\n"
          "-c | --camera_id\n"
          "               Should specify the camera id. Default is 0.\n"
          "OBJECTIVES\n"
          "-D | --video_demo\n"
          "               Demostrates the ability of the command with video\n"
          "               input.  Works with -c or -v\n"
          "-k | --create_conf\n"
          "               This will create a configuration file\n"
          "-a | --image_adjust\n"
          "               This will only accept a list of images.\n\n"
          , command);
}

/** Get intrinsics from file.
 *
 * camMat and disMat will remain unchanged unless both values are successfully
 * found in the file.
 *
 * @param filename Name of the file where the intrinsics are
 * @param camMat Mat where the camera matrix will end up.
 * @param disMat Mat where the distortion matrix will end up.
 *
 * @return true if all the intrinsics were found in the filename. false
 * otherwise.
 */
static bool
ia_get_intrinsics_from_file ( const char *filename, Mat *camMat, Mat *disMat)
{
  char *line = NULL;
  FILE *fp;
  size_t len;
  double t_dist[5];
  double t_cam[3][3];
  bool dist_found = false, cammat_found = false;

  /* we try to access the file*/
  fp = fopen(filename, "r");
  if ( fp == NULL )
  {
    fprintf(stderr, "Could not open file: %s\n", filename);
    return false;
  }

  /* we parse the file */
  while ( getline(&line, &len, fp) != -1 )
  {
    /* We parse the 5 distortion values */
    if ( !dist_found
         && (sscanf( line, "distortion %le %le %le %le %le",
                     &t_dist[0], &t_dist[1], &t_dist[2], &t_dist[3], &t_dist[4] )
             == 5) )
      dist_found = true;

    /* We parse the 9 cameramatrix values */
    if ( !cammat_found
         && (sscanf( line, "cameramatrix %le %le %le %le %le %le %le %le %le",
                     &t_cam[0][0], &t_cam[0][1], &t_cam[0][2],
                     &t_cam[1][0], &t_cam[1][1], &t_cam[1][2],
                     &t_cam[2][0], &t_cam[2][1], &t_cam[2][2] )
             == 9) )
      cammat_found = true;
  }
  fclose(fp);

  /* we make sure we actually read everything */
  if ( !dist_found || !cammat_found )
  {
    fprintf(stderr, "File contained bad format: %s\n", filename);
    return false;
  }

  for ( int i = 0 ; i < 5 ; i++ )
    (*disMat).at<double>(0,i) = t_dist[i];

  for ( int i = 0 ; i < 3 ; i++ )
    for ( int j = 0 ; j < 3 ; j++ )
      (*camMat).at<double>(i,j) = t_cam[i][j];

  /* At this point we are confident that we have correctly read the values*/
  return true;
}

void
ia_create_config ( const Mat *dist = NULL, const Mat *cam = NULL )
{
  char *filename = (char*)"intrinsics.cfg";
  FILE *fp;

  /* we try to access the file*/
  fp = fopen(filename, "w");
  if ( fp == NULL )
  {
    fprintf(stderr, "Could not open file: %s\n", filename);
    return;
  }

  /* We put the documentation in the file */
  fprintf ( fp, "# The intrinsics file should have two lines specifying the\n"
          "# distortion values and the camera matrix values.  The distortion\n"
          "# values are given by a line that begins with 'distortion ' followed\n"
          "# by 5 space separated values that specify K1, K2, P1, P2 and K3\n"
          "# respectively.  The camera matrix values are given by a line that\n"
          "# begins with 'cameramatrix ' and is followed by 9 space separated\n"
          "# values that represent a 3x3 matrix.  The first 3 numbers are the\n"
          "# first row, the second three are the second row ant the last three\n"
          "# are the last row. Example:\n"
          "# distortion K1 K2 P1 P2 K3\n"
          "# cameramatrix 00(FX) 01 02(CX) 10 11(FY) 12(CY) 20 21 22\n\n" );

  if ( dist == NULL || cam == NULL )
    fprintf ( fp, "distortion 0 0 0 0 0\ncameramatrix 0 0 0 0 0 0 0 0 0\n" );
  else
  {
    /* We put the distortion*/
    fprintf ( fp, "distortion" );
    for ( int i = 0 ; i < 5 ; i++ )
      fprintf ( fp, " %e", (*dist).at<double>(0,i) );
    fprintf ( fp, "\n" );

    /* We put the camera matrix*/
    fprintf ( fp, "cameramatrix" );
    for ( int i = 0 ; i < 3 ; i++ )
      for ( int j = 0 ; j < 3 ; j++ )
        fprintf ( fp, " %e", (*cam).at<double>(i,j) );
    fprintf ( fp, "\n" );
  }

  /* close the file*/
  fclose(fp);
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
  int undistort=0;
  int c;
  static struct option long_options[] =
    {
      /* These options set a flag. */
      {"undistort",     no_argument,          &undistort, 1},
      /* These options don't set a flag.
      *                   We distinguish them by their indices. */
      {"help",          no_argument,          0, 'h'},
      {"create_conf",   no_argument,          0, 'k'},
      {"image_adjust",  no_argument,          0, 'a'},
      {"video_demo",    no_argument,          0, 'D'},
      {"camera_id",     required_argument,    0, 'c'},
      {"num_int",       required_argument,    0, 'I'},
      {"video",         required_argument,    0, 'v'},
      {"ininput",       required_argument,    0, 'i'},
      {"ch",            required_argument,    0, 'H'},
      {"cw",            required_argument,    0, 'W'},
      {"squaresize",    required_argument,    0, 's'},
      {"delay",         required_argument,    0, 'd'},
      {0, 0, 0, 0}
    };

  input = (ia_input*)malloc(sizeof(ia_input));
  ia_init_input_struct(input);

  while (1)
  {
    /* getopt_long stores the option index here. */
    int option_index = 0;
    c = getopt_long ( argc, argv, "hkDai:b:s:d:v:c:I:", long_options,
                      &option_index );

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
          /* Notihing changes if ia_get_intrinsics_from_file returns false. */
          if (optarg
              && ia_get_intrinsics_from_file(optarg, &(input->camMat),
                    &(input->disMat)) )
          {
            input->iif = optarg;
          }
          break;

        case 'H':
          if ( sscanf(optarg, "%u", &(input->b_size.height)) != 1 )
          {
            fprintf(stderr, "Remember to give --ch an argument");
            ia_usage(argv[0]);
            ia_free_input_struct(input);
            return NULL;
          }
          break;

        case 'W':
          if ( sscanf(optarg, "%u", &(input->b_size.width)) != 1 )
          {
            fprintf(stderr, "Remember to give --cw an argument");
            ia_usage(argv[0]);
            ia_free_input_struct(input);
            return NULL;
          }
          break;

        case 'c':
          if ( sscanf(optarg, "%d", &(input->camera_id)) != 1 )
          {
            fprintf ( stderr, "Bad value for camera id.  Using 0\n" );
            input->camera_id = 0;
          }
          break;

        case 's':
          if ( sscanf(optarg, "%f", &(input->squareSize)) != 1 )
          {
            fprintf( stderr, "Could not use specified squareSize: %s"
                             "Using default: 1.\n", optarg );
            input->squareSize = (float)1.0;
          }
          break;

        case 'd':
          if ( sscanf(optarg, "%d", &(input->delay)) != 1 )
          {
            fprintf( stderr, "Could not use specified delay: %s"
                             "Using default: 250.\n", optarg );
            input->delay = 250;
          }
          break;

        case 'v':
          input->vid_file = (char*)optarg;

          break;

        case 'I':
          if ( sscanf(optarg, "%d", &(input->num_in_img)) != 1 )
          {
            fprintf ( stderr, "Could not use the specified value for "
                              " the -I argument.  Using the devfault\n" );
            input->num_in_img = 20;
          }
          break;

        case 'k':
          input->objective = CREATE_CONF;
          break;

        case 'a':
          input->objective = IMAGE_ADJUST;
          break;

        case 'D':
          input->objective = VIDEO_DEMO;
          break;

        default:
          ia_usage(argv[0]);
          ia_free_input_struct(input);
          return NULL;
      }
  }

  /* We error out if there are no additional images and no capture method. */
  if ( optind >= argc && input->camera_id == -1 && input->vid_file == NULL )
  {
    // The option indicator is at the last argument and there are no images...
    fprintf(stderr, "You must provide a list of images\n");
    ia_usage(argv[0]);
    return NULL;
  }

  /*
   * Irespective of the state of input->capture we point the input->images to
   * the rest of the arguments (if possible).
   */
  if ( optind < argc ) // We consider everything as an image name.
    input->images = &argv[optind];

  // Check the flags.
  if ( undistort )
    input->corDist = true;

  // Check to see if the relation between arguments is ok.
  /*
   * Minimum chessboard check.  FIXME: we could do better by checking the
   * relation between sides.  I'll leave it for later.
   */
  if (  input->b_size.height <= 0 || input->b_size.width <= 0 )
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
