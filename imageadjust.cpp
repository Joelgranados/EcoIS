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
#include <stdio.h>
#include <cv.h>
#include <highgui.h>
#include "ia_core.h"
#include "ia_input.h"

int
main ( int argc, char** argv ) {
  struct ia_input *input;
  Mat camMat, disMat;
  vector<Mat> rvecs, tvecs;

  /* parse intput */
  if ( (input = ia_init_input(argc, argv)) == NULL )
    exit(0); //an error message has already been printed

  if ( input->objective == CREATE_CONF )
  {
    if ( input->images != NULL )
      ia_image_calc_intr ( (const char**)input->images, input->b_size,
                           input->squareSize, input->num_in_img, true,
                           &camMat, &disMat, &rvecs, &tvecs );

    else if ( input->vid_file != NULL )
      ia_video_calc_intr ( input->vid_file, input->b_size, input->squareSize,
                           input->num_in_img, true, input->delay,
                           &camMat, &disMat );

    else if ( input->camera_id >= 0 )
      ia_video_calc_intr ( NULL, input->b_size, input->squareSize,
                           input->num_in_img, true, input->delay,
                           &camMat, &disMat );

    else
      fprintf ( stderr, "Could not create the configuration file.\n" );

  }else if (input->objective == IMAGE_ADJUST )
  {
    ia_information_extraction_debug ( (const char**)input->images, input->b_size );

  }else if (input->objective == VIDEO_DEMO )
    ia_calculate_and_capture ( input->b_size, input->delay, input->vid_file,
                               input->camera_id, input->squareSize,
                               input->num_in_img );
}
