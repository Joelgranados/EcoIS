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
#include <structmember.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "ilacSquare.h"
#include "ilacConfig.h"

#define ILAC_RETERR( message ) \
  { \
    PyErr_SetString ( PyExc_StandardError, message ); \
    return NULL; \
  }

/*{{{ IlacCB Object*/
typedef struct{
  PyObject_HEAD /* ";" provided by macro*/
  ILAC_ChessboardImage *cb;
} IlacCB;

static void
IlacCB_dealloc ( IlacCB *self )
{
  delete self->cb;
  self->ob_type->tp_free((PyObject*)self);
}

/* Creats (not instantiates) new object */
static PyObject*
IlacCB_new ( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
  IlacCB *self;
  self = (IlacCB *)type->tp_alloc(type, 0);
  if ( self != NULL )
    self->cb = NULL;
  return (PyObject *)self;
}

/* When new object is instantiated, can be called multiple times. */
static int
IlacCB_init(IlacCB *self, PyObject *args, PyObject *kwds)
{
  char *image_file;
  int size1, size2;
  PyObject *camMat_pylist, *disMat_pylist;
  Mat camMat_cvmat, disMat_cvmat;

  /* We do nothing if cb has already been created */
  if ( self->cb != NULL )
    return 0;

  /* parse incoming arguments. */
  if ( !PyArg_ParseTuple ( args, "sIIOO", &image_file, &size1, &size2,
                                        &camMat_pylist, &disMat_pylist ) )
  {
    PyErr_SetString ( PyExc_StandardError,
        "Invalid parameters for IlacCB_init.");
    return -1;
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

  /* Instantiate ILAC_ChessboardImage into an object */
  try{
    self->cb = new ILAC_ChessboardImage ( image_file, Size(size1,size2),
                                          camMat_cvmat, disMat_cvmat);
  }catch (std::exception& ilace)
  {
    PyErr_SetString ( PyExc_StandardError, ilace.what() );
    return -1;
  }

  return 0;
}

static PyObject*
IlacCB_version ( IlacCB *self )
{
  Py_RETURN_TRUE;
}

static PyMemberDef IlacCB_members[] = { {NULL} };

static PyMethodDef IlacCB_methods[] = {
  {"ver", (PyCFunction)IlacCB_version, METH_NOARGS,
    "Return the version of IlacCB"},
  {NULL}
};

static PyTypeObject IlacCBType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "_ilac.IlacCB",            /*tp_name*/
  sizeof(IlacCB),            /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)IlacCB_dealloc,/*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "IlacCB objects",          /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  IlacCB_methods,            /* tp_methods */
  IlacCB_members,            /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)IlacCB_init,     /* tp_init */
  0,                         /* tp_alloc */
  IlacCB_new,                /* tp_new */
};

/*}}} IlacCB Object*/

/*{{{ ilac Module Methods*/
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
  PyObject *camMat_pylist, *disMat_pylist;
  Mat camMat_cvmat, disMat_cvmat;

  /* parse incoming arguments. */
  if ( !PyArg_ParseTuple ( args, "sIIOO", &image_file, &size1, &size2,
                                        &camMat_pylist, &disMat_pylist ) )
    ILAC_RETERR("Invalid parameters for ilac_get_image_id");

  /* Lets create the disMat_cvmat var from the disMat_pylist */
  disMat_cvmat = Mat::zeros( 1, 8, CV_64F );
  for ( int i = 0 ; i < (int)PyList_Size(disMat_pylist) ; i++ )
    disMat_cvmat.at<double>(0,i) = PyFloat_AsDouble (
      PyList_GetItem (disMat_pylist, i) );

  camMat_cvmat = Mat::zeros( 3, 3, CV_64F );
  for ( int i = 0 ; i < 9 ; i++ )
    camMat_cvmat.at<double>(floor(i/3), i%3) = PyFloat_AsDouble (
        PyList_GetItem ( PyList_GetItem ( camMat_pylist, floor(i/3) ), i%3 ) );

  /* Calculate the image id vector */
  try
  {
    ILAC_ChessboardImage cb =
      ILAC_ChessboardImage ( image_file, Size(size1,size2),
                             camMat_cvmat, disMat_cvmat);
    image_id = cb.get_image_id ();
  }
  catch (std::exception& ilace){ILAC_RETERR(ilace.what());}

  /*Construct python list that will hold the image id*/
  list_image_id = PyList_New ( image_id.size() );
  if ( list_image_id == NULL ){ILAC_RETERR("Error creating a new list.");}

  for ( int i = 0 ; i < image_id.size() ; i++ )
    if ( PyList_SetItem ( list_image_id, i, Py_BuildValue("H", image_id[i]) )
         == -1 )
      ILAC_RETERR("Error creating id list elem.");

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
  int size1, size2;
  Mat camMat, disMat;

  /* 1. PARSE ARGS */
  if ( !PyArg_ParseTuple ( args, "Oii", &py_file_list, &size1, &size2 ) )
    ILAC_RETERR("Invalid parameters for ilac_calc_intrinsics.");

  for ( int i = 0 ; i < PyList_Size( py_file_list ) ; i++ )
    images.push_back (
        (string)PyString_AsString ( PyList_GetItem(py_file_list, i) ) );

  /* 2. CALL CALC_IMG_INTRINSICS */
  ILAC_ChessboardImage::calc_img_intrinsics ( images, size1, size2,
                                              camMat, disMat );

  /*
   * 3. CREATE RETURN LIST
   * ret_list[camMat[[x,x,x],[x,x,x],[x,x,x]], disMat[x,x ... x,x]]
   */
  ret_list = PyList_New(0);
  camMat_list = PyList_New (0);
  disMat_list = PyList_New(0);
  if ( ret_list == NULL || camMat_list == NULL || disMat_list == NULL )
    ILAC_RETERR("Error initializing python objects in ilac_calc_intrinsics.");

  if ( PyList_Append ( ret_list, camMat_list ) == -1
       || PyList_Append ( ret_list, disMat_list ) == -1 )
    ILAC_RETERR("Error initializing python objects in ilac_calc_intrinsics.");

  //FIXME: add error detection
  for ( int row = 0 ; row < 3 ; row++ )/* create the camMat rows */
  {
    tmp_list = PyList_New (0);
    for ( int col = 0 ; col < 3 ; col++ )
      PyList_Append ( tmp_list, Py_BuildValue ( "d", camMat.at<double>(row,col) ) );
    PyList_Append ( camMat_list, tmp_list );
  }

  for ( int col = 0 ; col < disMat.size().width ; col++ )/* create disMat */
    PyList_Append ( disMat_list,
        Py_BuildValue ( "d", disMat.at<double>(0,col) ) );

  return ret_list;
}

static PyObject*
ilac_calc_process_image ( PyObject *self, PyObject *args )
{
  PyObject *camMat_pylist, *disMat_pylist;
  int size1, size2, squareSize; /*squareSize is in pixels.*/
  Mat camMat_cvmat, disMat_cvmat;
  char *outfile, *infile;
  vector<unsigned short> image_id;
  ILAC_ChessboardImage cb;
  PyObject *list_image_id;

  /* First pyobjcect is a list of lists (camMat)
   * second pyobject is a list of ints */
  if ( !PyArg_ParseTuple ( args, "iiiOOss", &size1, &size2, &squareSize,
                           &camMat_pylist, &disMat_pylist,
                           &infile, &outfile ) )
    ILAC_RETERR("Invalid parameters for ilac_calc_process_image.");

  /* Lets create the disMat_cvmat var from the disMat_pylist */
  disMat_cvmat = Mat::zeros( 1, 8, CV_64F );
  for ( int i = 0 ; i < (int)PyList_Size(disMat_pylist) ; i++ )
    disMat_cvmat.at<double>(0,i)
      = PyFloat_AsDouble ( PyList_GetItem (disMat_pylist, i) );

  camMat_cvmat = Mat::zeros( 3, 3, CV_64F );
  for ( int i = 0 ; i < 9 ; i++ )
    camMat_cvmat.at<double>(floor(i/3), i%3)
      = PyFloat_AsDouble (
          PyList_GetItem ( PyList_GetItem ( camMat_pylist, floor(i/3) ),
                           i%3 ) );

  //FIXME we should export the class to a python type!!!!!!
  /* create the ILAC_ChessboardImage object */
  /* Calculate the image id vector */
  try
  {
    cb = ILAC_ChessboardImage ( infile, Size(size1,size2),
                                camMat_cvmat, disMat_cvmat );
    image_id = cb.get_image_id ();
  }catch(ILACExNoChessboardFound){
    PyErr_SetString ( PyExc_StandardError, "Chessboard not found." );
    return NULL;
  }catch(ILACExNoneRedSquare){
    PyErr_SetString ( PyExc_StandardError, "None red square found." );
    return NULL;
  }catch(ILACExFileError){
    PyErr_SetString ( PyExc_StandardError, "Unknown Image format" );
    return NULL;
  }

  /*Construct python list that will hold the image id*/
  list_image_id = PyList_New ( image_id.size() );
  if ( list_image_id == NULL )
    ILAC_RETERR("Error creating a new list.");

  for ( int i = 0 ; i < image_id.size() ; i++ )
    if ( PyList_SetItem ( list_image_id, i, Py_BuildValue("H", image_id[i]) )
         == -1 )
      ILAC_RETERR("Error creating id list elem.");

  /* call the image process method. */
  cb.process_image ( outfile, squareSize);

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
    " sizeofchessboard1, int sizeofchessboard2)"},

  { "process_image",
    (PyCFunction)ilac_calc_process_image,
    METH_VARARGS, "Returns image id and the file where the processed image"
      " is located. Arguments: (size1, size2, camMat, disMat, inputfile, "
      " outputfile." },

  { "version",
    (PyCFunction)ilac_get_version,
    METH_NOARGS, "Return the version of the library." },
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_ilac (void)
{
  //(void) Py_InitModule ( "_ilac", ilac_methods );
  PyObject *m;

  if ( PyType_Ready(&IlacCBType) < 0 )
    return;

  m = Py_InitModule3 ( "_ilac", ilac_methods,
      "Module that Normalizes images based on a marker" );

  if ( m == NULL )
    return;

  Py_INCREF ( &IlacCBType );
  PyModule_AddObject ( m, "IlacCB", (PyObject *)&IlacCBType );
}
/*}}} ilac Module Methods*/
