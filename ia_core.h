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

using namespace cv;

bool ia_calc_image_chess_points ( char**, const Size,
                                  vector<vector<Point2f> >*, Size* );

bool ia_calc_object_chess_points ( const Size, const int, const int,
                                   vector<vector<Point3f> >* );

bool ia_calculate_all ( char**, const Size, Mat*, Mat*,
                        vector<Mat>*, vector<Mat>*, const float = 1);

bool ia_calculate_intrinsics ( char**, const Size, Mat&, Mat&,
                               const float = 1 );

bool ia_calculate_extrinsics ( char**, const Mat*, const Mat*, const Size,
                               vector<Mat>*, vector<Mat>*, const float = 1 );

void ia_calculate_and_capture ( const Size, const int,  const char*,
                                const int = 0, const float = 1 );
