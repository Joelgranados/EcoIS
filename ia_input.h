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
#include <cv.h>
#include <string.h>

/* command objective description */
enum ia_obj { NONE, CREATE_CONF };

//Structure holding all the user input and some initial calculations.
struct ia_input
{
  char* iif; //The intrinsic input file.
  bool calInt; //Whether to calculate intrinsic values or not.

  cv::Mat camMat; //The camera matrix.
  cv::Mat disMat; //The distortion matrix.

  bool corDist; //whether to correct distortion or not.

  int camera_id;
  char* vid_file; //Video file to use instead of the camera.
  char** images; // a list of image file names.

  cv::Size b_size; //chessboard size. height x width

  float r_dist; //distance from where to do the rescaling.
  float squareSize; //Chessboard square size

  int delay; //Delay between actions in the capture state.

  int num_in_img; // Number of images to calculate intrinsics.

  /* These will be the command objectives */
  ia_obj objective;
};

void ia_usage (char*);
ia_input* ia_init_input (int, char**);
void ia_print_input_struct (struct ia_input*);

void ia_create_config (const cv::Mat*, const cv::Mat*);
