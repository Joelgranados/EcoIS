# ILAC: Image labeling and Classifying
# Copyright (C) 2011 Joel Granados <joel.granados@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
import unittest

class Intr_SimpleCalc(unittest.TestCase):
    """ Designed to test simple id calculation intrinsic parameters."""
    def setUp (self):
        self.files = []
        self.size = []
        self.expected = []

    def test_Intrinsics (self):
        import _ilac

        for i in range(6):
            self.files.append("images/intr%d.jpg"%(i+1))
        self.size = [7, 10]
        self.expectedResult = \
            [[[439.7124571450218, 0.0, 508.09121466942526],
              [0.0, 439.52587749737506, 338.8996339931403],
              [0.0, 0.0, 1.0]],
             [-0.05265642376912142,
               0.13362192333442172,
               0.0011102772334400505,
               -0.00033490409900301766,
               -0.1263608469044541]]

        intrinsics = \
            _ilac.calc_intrinsics(self.files, self.size[0], self.size[1])
        self.assertEqual ( self.expectedResult, intrinsics )

    def test_kodakIntr (self):
        import _ilac

        for i in range(6):
            self.files.append("images/kodakIntr%d.jpg"%(i+1))
        self.size = [7,10]
        self.expectedResult = \
                [[[676.5781926854147, 0.0, 395.52926192378436],
                  [0.0, 677.2952780926865, 218.51201310572563],
                  [0.0, 0.0, 1.0]],
                 [-0.11604572360793801,
                  0.6220373333073841,
                  -0.006066776328165834,
                  -0.002219970761699905,
                  -3.0430144976827744]]

        intrinsics = \
            _ilac.calc_intrinsics(self.files, self.size[0], self.size[1])
        self.assertEqual ( self.expectedResult, intrinsics )
