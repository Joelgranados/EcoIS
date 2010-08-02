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

  if ( input->capture )
    ia_calculate_and_capture ( input->b_size, input->squareSize );

  else if ( input->calInt )
    ia_calculate_all ( input->images, input->b_size, &camMat, &disMat,
                       &rvecs, &tvecs, input->squareSize );

  else  // We used the intrinsics from the file if the calculation is avoided
    ia_calculate_extrinsics ( input->images, &(input->camMat), &(input->disMat),
                              input->b_size, &rvecs, &tvecs,
                              input->squareSize );

}
