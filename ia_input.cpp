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
#include <iostream>
#include <fstream>
#include "ia_input.h"
#include <opencv/cv.h>
#include <getopt.h>
#include <stdio.h>

using namespace cv;

/*
 * Input specific functions.
 */
static void
ia_init_input_struct ( ia_input& input )
{
  input.camMat = Mat::eye(3, 3, CV_64F);
  input.disMat = Mat::zeros(5, 1, CV_64F);

  input.vid_file = "";

  /*
   * Chessboard default size will be 0,0.  If the user does not specify the
   * sizes, the intrinsics cannot be calculated.
   */
  input.b_size.height = (unsigned int)0;
  input.b_size.width = (unsigned int)0;

  input.squareSize = 1;

  input.images = vector<string>(0);

  /* default command objective will be to create a config file */
  input.objective = NONE;

  input.checked = false;
  return;
}

void
ia_print_input_struct ( ia_input& input )
{
  Mat& cm = input.camMat;
  Mat& dm = input.disMat;
  std::cout << "Arguments used for input:" << endl
    << "Camera distortion :" 
  <<dm.at<double>(0,0)<<" "<<dm.at<double>(0,1)<<" "<<dm.at<double>(0,2)<<" "
  <<dm.at<double>(0,3)<<" "<< dm.at<double>(0,4)<<endl

    << "Camera Matrix:" << endl
  <<cm.at<double>(0,0)<<", "<<cm.at<double>(0,1)<<", "<<cm.at<double>(0,2)<<endl
  <<cm.at<double>(1,0)<<", "<<cm.at<double>(1,1)<<", "<<cm.at<double>(1,2)<<endl
  <<cm.at<double>(2,0)<<", "<<cm.at<double>(2,1)<<", "<<cm.at<double>(2,2)<<endl

    <<"BoardSize (w,h): "<<input.b_size.width<<", "<<input.b_size.height<<endl
    << "SquareSize: " << input.squareSize << endl
    << "Video File: " << input.vid_file << endl;

  std::cout << "Image List: ";
  for ( vector<string>::iterator image = input.images.begin() ;
        image != input.images.end() ; ++image )
    std::cout << *image;
  std::cout << "\n";
}

void
ia_usage ( const string command )
{
  std::cout << command << " [OPTIONS] IMAGES" << endl;
    "OPTIONS:\n"
    "-h | --help    Print this help message.\n"
    "-W | --cw      Chessboard width in inner squares\n"
    "-H | --ch      Chessboard height in inner squares\n"
    "-s | --squaresize\n"
    "               The size of the chessboard square.  1 by default\n"
    "               The resulting values will be given with respect to\n"
    "               this number.\n"
    "-v | --video   Use a video file. Supporst whatever opencv supports.\n"
    "OBJECTIVES\n"
    "-D | --video_demo\n"
    "               Demostrates the ability of the command with video\n"
    "               input.  Works with -c or -v\n"
    "-a | --image_adjust\n"
    "               This will only accept a list of images.\n\n";
}

/*
 * Will prepare all the input parameters.
 * Returns a pointer to the input struct if all went well.  Pointer to NULL
 * otherwise.
 */
ia_input
ia_init_input ( int argc, char **argv)
{
  //Don't use distortion by default
  int c;
  char** temp_images;

  static struct option long_options[] =
    {
      /* These options don't set a flag.
      *                   We distinguish them by their indices. */
      {"help",          no_argument,          0, 'h'},
      {"image_adjust",  no_argument,          0, 'a'},
      {"video_demo",    no_argument,          0, 'D'},
      {"video",         required_argument,    0, 'v'},
      {"ch",            required_argument,    0, 'H'},
      {"cw",            required_argument,    0, 'W'},
      {"squaresize",    required_argument,    0, 's'},
      {0, 0, 0, 0}
    };

  ia_input input;
  ia_init_input_struct(input);

  while (1)
  {
    /* getopt_long stores the option index here. */
    int option_index = 0;
    c = getopt_long ( argc, argv, "hDab:s:v:", long_options,
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
          std::cout << "option " << long_options[option_index].name;
          if (optarg)
            std::cout << "with args " << optarg;
          std::cout << endl;
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        case 'h':
          ia_usage(argv[0]);
          return input;

        case 'H':
          if ( sscanf(optarg, "%u", &(input.b_size.height)) != 1 )
          {
            std::cerr << "Remember to give --ch an argument" << endl;
            ia_usage(argv[0]);
            return input;
          }
          break;

        case 'W':
          if ( sscanf(optarg, "%u", &(input.b_size.width)) != 1 )
          {
            std::cerr << "Remember to give --cw an argument" << endl;
            ia_usage(argv[0]);
            return input;
          }
          break;

        case 's':
          if ( sscanf(optarg, "%f", &(input.squareSize)) != 1 )
          {
            std::cerr << "Could not use specified squareSize: " << optarg
                      << ". Using default: 1." << endl;
            input.squareSize = (float)1.0;
          }
          break;

        case 'v':
          input.vid_file = (char*)optarg;

          break;

        case 'a':
          input.objective = IMAGE_ADJUST;
          break;

        case 'D':
          input.objective = VIDEO_DEMO;
          break;

        default:
          ia_usage(argv[0]);
          return input;
      }
  }
  /* We consider everything else as an image */
  if ( optind < argc )
  {
    temp_images = &argv[optind];
    for ( int i = 0 ; temp_images[i] != '\0' ; i++ )
      input.images.push_back( temp_images[i] );
  }

  //FIXME: remember that we don't need images for video state.
  if ( input.images.size() < 1 )
  {
    std::cerr << "You must provide a list of images" << endl;
    return input;
  }

  // Check to see if the relation between arguments is ok.
  /*
   * Minimum chessboard check.  FIXME: we could do better by checking the
   * relation between sides.  I'll leave it for later.
   */
  if (  input.b_size.height <= 0 || input.b_size.width <= 0 )
  {
    std::cerr << "To calculate the camera intrinsics you need to provide "
              << "positive chessboard height and width values." << endl;
    ia_usage(argv[0]);
    return input;
  }

  // We print the values so the user can see them
  ia_print_input_struct(input);

  input.checked = true;
  return input;
}
