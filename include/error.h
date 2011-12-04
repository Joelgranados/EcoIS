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

#include <exception>

class ILACExInvalidResizeScale:public std::exception{
  virtual const char* what() const throw()
  {
    return "Resulting resize scale factor is non-manageable";
  }
};

class ILACExUnknownError:public std::exception{
  virtual const char* what() const throw(){return "Unknown error encountered";}
};

class ILACExNoChessboardFound:public std::exception{
  virtual const char* what() const throw(){return "No Chessboard found";}
};

class ILACExNoneRedSquare:public std::exception{
  virtual const char* what() const throw(){return "None red square found";}
};

class ILACExSymmetricalChessboard:public std::exception{
  virtual const char* what() const throw()
  {
    return "Chessboard parity sizes are equal.  The dimensions of the "
      "chessboard must be odd (5.6 for example). Refer to Learning Opencv "
      "page 382.";
  }
};

//FIXME: put the filename in the exception some how.
class ILACExFileError:public std::exception{
  virtual const char* what() const throw(){return "Image file error.";}
};

class ILACExSizeFormatError:public std::exception{
  virtual const char* what() const throw(){return "Invalid size format";}
};

class ILACExTooManyColors:public std::exception{
  virtual const char* what() const throw(){return "Too many colors to classify";}
};

class ILACExChessboardTooSmall:public std::exception{
  virtual const char* what() const throw(){return "Too few squares in chessboard.";}
};

class ILACExNotImplemented:public std::exception{
  virtual const char* what() const throw(){return "Not implemented yet.";}
};

class ILACExInvalidClassifierType:public std::exception{
  virtual const char* what() const throw(){return "Invalid Classifier type";}
};

class ILACExLessThanThreeSpheres:public std::exception{
  virtual const char* what() const throw()
    {return "Not enough spheres in image.";}
};

class ILACExCouldNotCreateQuadType:public std::exception{
  virtual const char* what() const throw(){return "Quadrilateral strangeness";}
};

class ILACExOutOfBounds:public std::exception{
  virtual const char* what() const throw(){return "Out of bounds exception.";}
};
