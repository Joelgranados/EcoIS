/*
 * ILAC: Image labeling and Classifying
 * Copyright (C) 2011 Joel Granados <joel.granados@gmail.com>
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

#include <Python.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "ilacSquare.h"
#include "ilacConfig.h"

static PyObject*
ilac_get_version ( PyObject *self, PyObject *args )
{
    PyObject *ver_mes;
    ver_mes = PyString_FromFormat (
            "%s, Version: %d.%d.", ILAC_NAME, ILAC_VER_MAJOR, ILAC_VER_MINOR );
    return ver_mes;
}

static PyObject*
ilac_get_image_id ( PyObject *self, PyObject *args )
{
  char *image_file;
  int size1, size2;
  PyObject *list_image_id;
  vector<unsigned short> image_id;

  /* parse incoming arguments. */
  //FIXME: set the python error.
  if ( !PyArg_ParseTuple ( args, "sII", &image_file, &size1, &size2 ) )
    return NULL;

  /* Calculate the image id vector */
  try
  {
    ILAC_ChessboardImage cb = ILAC_ChessboardImage ( image_file, size1, size2 );
    image_id = cb.get_image_id ();
  }
  catch (std::exception& ilace)
  {
    PyErr_SetString ( PyExc_StandardError, ilace.what() );
    return NULL;
  }

  /*Construct python list that will hold the image id*/
  list_image_id = PyList_New ( image_id.size() );
  if ( list_image_id == NULL )
  {
    PyErr_SetString ( PyExc_StandardError, "Error creating a new list." );
    return NULL;
  }

  for ( int i = 0 ; i < image_id.size() ; i++ )
    if ( PyList_SetItem ( list_image_id, i, Py_BuildValue("H", image_id[i]) )
         == -1 )
    {
      PyErr_SetString ( PyExc_StandardError, "Error creating id list elem." );
      return NULL;
    }

  return list_image_id;
}

/*
 * 1. PARSE ARGS
 * 2. CALL CALC_IMG_INTRINSICS
 * 3. CREATE RETURN LIST
 */
static PyObject*
ilac_calc_intrinsics ( PyObject *self, PyObject *args )
{
  PyObject *py_file_list, *ret_list, *tmp_list, *camMat_list, *disMat_list;
  vector<string> images;
  int size1, size2, sqr_size;
  Mat camMat, disMat;

  /* 1. PARSE ARGS */
  if ( !PyArg_ParseTuple ( args, "Oiii", &py_file_list, &size1, &size2, &sqr_size ) )
  {
    PyErr_SetString ( PyExc_TypeError,
                      "ArgError: should be (list, int, int, int)." );
    return NULL;
  }

  for ( int i = 0 ; i < PyList_Size( py_file_list ) ; i++ )
    images.push_back (
        (string)PyString_AsString ( PyList_GetItem(py_file_list, i) ) );

  /* 2. CALL CALC_IMG_INTRINSICS */
  ILAC_ChessboardImage::calc_img_intrinsics ( images, size1, size2, sqr_size,
                                              camMat, disMat );

  /*
   * 3. CREATE RETURN LIST
   * ret_list[camMat[[x,x,x],[x,x,x],[x,x,x]], disMat[x,x ... x,x]]
   */
  ret_list = PyList_New(0);
  camMat_list = PyList_New (0);
  disMat_list = PyList_New(0);
  PyList_Append ( ret_list, camMat_list );
  PyList_Append ( ret_list, disMat_list );

  for ( int row = 0 ; row < 3 ; row++ )/* create the camMat rows */
  {
    tmp_list = PyList_New (0);
    for ( int col = 0 ; col < 3 ; col++ )
      PyList_Append ( tmp_list,
                      Py_BuildValue ( "d", camMat.at<double>(row,col) ) );
    PyList_Append ( camMat_list, tmp_list );
  }

  for ( int col = 0 ; col < disMat.size().width ; col++ )/* create disMat */
    PyList_Append ( disMat_list,
        Py_BuildValue ( "d", disMat.at<double>(0,col) ) );

  return ret_list;
}

//FIXME: check for error in all the python transform calls.
static PyObject*
ilac_calc_process_image ( PyObject *self, PyObject *args )
{
  PyObject *camMat_pylist, *disMat_pylist;
  int action, normdist, size1, size2;
  Mat camMat_cvmat, disMat_cvmat;
  char *outfile, *infile;
  vector<unsigned short> image_id;
  ILAC_ChessboardImage cb;
  PyObject *list_image_id;

  //FIXME: comment on how the pyobjects should look like.
  if ( !PyArg_ParseTuple ( args, "iiiiOOss", &action, &size1, &size2,
                           &normdist, &camMat_pylist, &disMat_pylist,
                           &infile, &outfile ) )
  {
    //FIXME: change the error message.
    PyErr_SetString ( PyExc_TypeError, "FIXME: Change the error message" );
    return NULL;
  }

  //FIXME we should export the class to a python type!!!!!!
  /* create the ILAC_ChessboardImage object */
  /* Calculate the image id vector */
  try
  {
    cb = ILAC_ChessboardImage ( infile, size1, size2 );
    image_id = cb.get_image_id ();
  }catch(ILACExNoChessboardFound){
    PyErr_SetString ( PyExc_StandardError, "Chessboard not found." );
    return NULL;
  }

  /*Construct python list that will hold the image id*/
  list_image_id = PyList_New ( image_id.size() );
  if ( list_image_id == NULL )
  {
    PyErr_SetString ( PyExc_StandardError, "Error creating a new list." );
    return NULL;
  }

  for ( int i = 0 ; i < image_id.size() ; i++ )
    if ( PyList_SetItem ( list_image_id, i, Py_BuildValue("H", image_id[i]) )
         == -1 )
    {
      PyErr_SetString ( PyExc_StandardError, "Error creating id list elem." );
      return NULL;
    }

  /* Lets create the disMat_cvmat var from the disMat_pylist */
  disMat_cvmat = Mat::zeros( 1, 8, CV_64F );
  for ( int i = 0 ; i < (int)PyList_Size(disMat_pylist) ; i++ )
    disMat_cvmat.at<double>(0,i) = PyFloat_AsDouble (
      PyList_GetItem (disMat_pylist, i) );

  camMat_cvmat = Mat::zeros( 3, 3, CV_64F );
  for ( int i = 0 ; i < 9 ; i++ )
    camMat_cvmat.at<double>(floor(i/3), i%3) = PyFloat_AsDouble (
        PyList_GetItem ( PyList_GetItem ( camMat_pylist, floor(i/3) ), i%3 ) );

  //FIXME check for error.
  /* call the image process method. */
  cb.process_image ( action, camMat_cvmat, disMat_cvmat,
                     normdist, outfile );

  return list_image_id;
}

static struct PyMethodDef ilac_methods [] =
{
  { "get_image_id",
    (PyCFunction)ilac_get_image_id,
    METH_VARARGS, "Analyzes the image file and returns an id if a valid"
      "chessboard was found." },

  { "calc_intrinsics",
    (PyCFunction)ilac_calc_intrinsics,
    METH_VARARGS, "Returns the camera matrix and distortion vector."
    " [[x,x,x],[x,x,x],[x,x,x]],[x,x,...x] <- (list filenames, int "
    " sizeofchessboard1, int sizeofchessboard2, int squaresize)"},

  { "process_image",
    (PyCFunction)ilac_calc_process_image,
    METH_VARARGS, "Returns image id and the file where the processed image"
      " is located. Arguments: (action, size1, size2, distnorm,"
      " camMat, disMat, inputfile, outputfile." },

  { "version",
    (PyCFunction)ilac_get_version,
    METH_NOARGS, "Return the version of the library." },
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_ilac (void)
{
  (void) Py_InitModule ( "_ilac", ilac_methods );
}
