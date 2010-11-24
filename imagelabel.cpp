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
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "IA_Square.h"
#include "ia_input.h"

int
main ( int argc, char** argv ) {

  /* parse intput */
  ia_input input = ia_init_input ( argc, argv );
  if ( !input.checked )
    exit(0); //an error message has already been printed

  if (input.objective == IMAGE_ADJUST )
    for ( vector<string>::iterator image = input.images.begin() ;
          image != input.images.end() ; image++ )
      try{
        IA_ChessboardImage cb = IA_ChessboardImage ( *image, input.b_size );
      }catch(exception& iacie){
        std::cerr << "ERROR: " << iacie.what() << endl;
      }

  else
    std::cerr << "You did not specify any valid objective " << endl;
}
