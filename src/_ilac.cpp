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
#include <opencv2/opencv.hpp>
#include "ilacConfig.h"
#include "ilacImage.h"

#define ILAC_RETERR( message ) \
  { \
    PyErr_SetString ( PyExc_StandardError, message ); \
    return NULL; \
  }

/*{{{ IlacCB Object*/
typedef struct{
  PyObject_HEAD /* ";" provided by macro*/
  ILAC_Image *ii;
} IlacCB;

static void
IlacCB_dealloc ( IlacCB *self )
{
  delete self->ii;
  self->ob_type->tp_free((PyObject*)self);
}

/* Creats (not instantiates) new object */
static PyObject*
IlacCB_new ( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
  IlacCB *self;
  self = (IlacCB *)type->tp_alloc(type, 0);
  if ( self != NULL )
    self->ii = NULL;
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

  /* We do nothing if ii has already been created */
  if ( self->ii != NULL )
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

  /* Instantiate ILAC_Chessboard into an object */
  try{
    self->ii = new ILAC_Image ( image_file, Size(size1,size2),
                                camMat_cvmat, disMat_cvmat, false );
    self->ii->initChess();
    self->ii->calcID();
    self->ii->calcRefPoints();
  }catch(ILACExNoChessboardFound){
    PyErr_SetString ( PyExc_StandardError, "Chessboard not found." );
    return -1;
  }catch(ILACExNoneRedSquare){
    PyErr_SetString ( PyExc_StandardError, "None red square found." );
    return -1;
  }catch(ILACExFileError){
    PyErr_SetString ( PyExc_StandardError, "Unable to read image file" );
    return -1;
  }

  return 0;
}

static PyObject*
IlacCB_process_image ( IlacCB *self, PyObject *args )
{
  int squareSize; /*squareSize is in pixels.*/
  char *outfile;

  if ( !PyArg_ParseTuple ( args, "is", &squareSize, &outfile ) )
    ILAC_RETERR("Invalid parameters for ilac_calc_process_image.");

  self->ii->normalize ();

  Py_RETURN_TRUE;
}

static PyObject*
IlacCB_img_id ( IlacCB *self )
{
  PyObject *list_image_id;
  vector<unsigned short> image_id;

  image_id = self->ii->getID();

  /*Construct python list that will hold the image id*/
  list_image_id = PyList_New ( image_id.size() );
  if ( list_image_id == NULL ){ILAC_RETERR("Error creating a new list.");}

  for ( int i = 0 ; i < image_id.size() ; i++ )
    if ( PyList_SetItem ( list_image_id, i, Py_BuildValue("H", image_id[i]) )
         == -1 )
      ILAC_RETERR("Error creating id list elem.");

  return list_image_id;
}

static PyMemberDef IlacCB_members[] = { {NULL} };

static PyMethodDef IlacCB_methods[] = {
  {"process_image", (PyCFunction)IlacCB_process_image, METH_VARARGS,
    "Saves an processed image to a FILENAME with a given SQUARE SIZE"},
  {"img_id", (PyCFunction)IlacCB_img_id, METH_NOARGS,
    "Return the chessboard id of the image"},
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
  ILAC_Image::calcIntr ( images, size1, size2, camMat, disMat );

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

static struct PyMethodDef ilac_methods [] =
{
  { "calc_intrinsics",
    (PyCFunction)ilac_calc_intrinsics,
    METH_VARARGS, "Returns the camera matrix and distortion vector."
    " [[x,x,x],[x,x,x],[x,x,x]],[x,x,...x] <- (list filenames, int "
    " sizeofchessboard1, int sizeofchessboard2)"},

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
