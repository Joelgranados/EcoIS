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

#include "ia_config.h"
#include "ia_input.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <opencv/cv.h>
#include <getopt.h>
#include <stdio.h>


using namespace cv;

void
ia_print_input_struct ( ia_input& input )
{
  std::cout << "Arguments used for input:" << endl
    <<"BoardSize (w,h): "<<input.b_size.width<<", "<<input.b_size.height<<endl;

  std::cout << "Image List: ";
  for ( vector<string>::iterator image = input.images.begin() ;
        image != input.images.end() ; ++image )
    std::cout << *image;
  std::cout << "\n";
}

void
ia_usage ( const string& command )
{
  std::cout << command << " OBJECTIVE OPTIONS IMAGES \n"
    << "Current Version: " << IL_VER_MAJOR << "." << IL_VER_MINOR <<
    "\nOPTIONS:\n"
    "-H | --help    Print this help message.\n"
    "-s | --cs      The minimum of the two sizes.\n"
    "-S | --cS      The maximum of the two sizes.\n\n"
    "OBJECTIVE\n"
    "-a | --image_adjust\n"
    "               This will only accept a list of images.\n"
    "IMAGES         List of images\n" << endl;
}

static void
ia_init_input_struct ( ia_input& input )
{
  /*
   * Chessboard default size will be 0,0.  If the user does not specify the
   * sizes, the intrinsics cannot be calculated.
   */
  input.b_size.height = (unsigned int)0;
  input.b_size.width = (unsigned int)0;

  input.images = vector<string>(0);

  /* default command objective will be to create a config file */
  input.objective = NONE;

  input.checked = false;
  return;
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
      {"help",          no_argument,          0, 'H'},
      {"image_adjust",  no_argument,          0, 'a'},
      {"cs",            required_argument,    0, 's'},
      {"cS",            required_argument,    0, 'S'},
      {0, 0, 0, 0}
    };

  ia_input input;
  ia_init_input_struct(input);

  while (1)
  {
    /* getopt_long stores the option index here. */
    int option_index = 0;
    c = getopt_long ( argc, argv, "Hab:s:S:", long_options,
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

        case 'H':
          ia_usage(argv[0]);
          return input;

        case 's':
          if ( sscanf(optarg, "%u", &(input.b_size.height)) != 1 )
          {
            std::cerr << "Remember to give --cs an argument" << endl;
            ia_usage(argv[0]);
            return input;
          }
          break;

        case 'S':
          if ( sscanf(optarg, "%u", &(input.b_size.width)) != 1 )
          {
            std::cerr << "Remember to give --cS an argument" << endl;
            ia_usage(argv[0]);
            return input;
          }
          break;

        case 'a':
          input.objective = IMAGE_ADJUST;
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
    struct stat stat_temp;
    for ( int i = 0 ; temp_images[i] != '\0' ; i++ )
      if ( stat ( temp_images[i], &stat_temp ) == 0 )
        input.images.push_back( temp_images[i] );
      else
        std::cout << "Could not read filename: " << temp_images[i] << endl;
  }

  if ( input.images.size() < 1 )
  {
    std::cerr << "You must provide a list of images" << endl;
    return input;
  }

  /* Minimum chessboard check. */
  if ( input.b_size.height <= 0 || input.b_size.width <= 0 )
  {
    std::cerr << "Positive chessboard needed to detect corners." << endl;
    ia_usage(argv[0]);
    return input;
  }

  if ( input.b_size.height > input.b_size.width )
  {
    std::cerr << "WARNING: For consistency purposes the width always " << endl
              << "\tneeds to be greater than the height.  I'm going to " << endl
              << "\tinvert the values you gave me: height=" << input.b_size.width
              << " and width=" << input.b_size.height << endl << endl;
    unsigned int temp = input.b_size.width;
    input.b_size.width = input.b_size.height;
    input.b_size.height = temp;
  }

  /* We need a chessboard of odd dimensions (5,6 for example).  This gives us
   * a chessboard with only one symmetry axis.  We use this in order to identify
   * a unique origin. <ISBN 978-0-596-51613-0 Learning Opencv page 382> */
  if ( input.b_size.height % 2 == input.b_size.width % 2 )
  {
    std::cerr << "ERROR: The dimensions must be odd. (5,6 for example). "<< endl
              << "\tPlease change your chessboard and try again. Refer " << endl
              << "\tto \"Learning Opencv page\" page 382." << endl << endl;
    ia_usage(argv[0]);
    return input;
  }

  /* We default to IMAGE_ADJUST */
  if ( input.objective == NONE )
    input.objective = IMAGE_ADJUST;


  /* We print the values so the user can see them */
  ia_print_input_struct(input);

  input.checked = true;
  return input;
}
