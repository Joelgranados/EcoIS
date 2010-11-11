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
#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace cv;

bool ia_calc_object_chess_points ( const Size, const int, const int,
                                   vector<vector<Point3f> >* );

void ia_calculate_and_capture ( const Size, const int,  const char*,
                                const int = 0, const float = 1,
                                const int = 20 );

int ia_video_calc_intr ( const char*, const Size, const float, const int,
                         const bool, const int,  Mat*, Mat* );
int ia_image_calc_intr ( const char**, const Size, const float, const int,
                         const bool, Mat*, Mat*, vector<Mat>*, vector<Mat>* );

void ia_imageadjust ( const char**, const Size, const float,
                      const Mat* = NULL, const Mat* = NULL );
void ia_information_extraction_debug ( const char**, const Size );
