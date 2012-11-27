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
            [[[439, 0, 508],
              [0.0, 439, 338],
              [0, 0, 1]],
             [-0,0,0,-0,-0]]

        intrinsics = \
            _ilac.calc_intrinsics(self.files, self.size[0], self.size[1])
        intrinsics[0][0] = map(int, intrinsics[0][0])
        intrinsics[0][1] = map(int, intrinsics[0][1])
        intrinsics[0][2] = map(int, intrinsics[0][2])
        intrinsics[1] = map(int, intrinsics[1])
        self.assertEqual ( self.expectedResult, intrinsics )

    def test_kodakIntr (self):
        import _ilac

        for i in range(6):
            self.files.append("images/kodakIntr%d.jpg"%(i+1))
        self.size = [7,10]
        self.expectedResult = \
                [[[676, 0, 395],
                  [0, 677, 218],
                  [0, 0, 1]],
                 [-0,0,-0,-0,-3]]

        intrinsics = \
            _ilac.calc_intrinsics(self.files, self.size[0], self.size[1])
        intrinsics[0][0] = map(int, intrinsics[0][0])
        intrinsics[0][1] = map(int, intrinsics[0][1])
        intrinsics[0][2] = map(int, intrinsics[0][2])
        intrinsics[1] = map(int, intrinsics[1])
        self.assertEqual ( self.expectedResult, intrinsics )
