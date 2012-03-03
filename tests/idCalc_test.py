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

class IDCalc_SimpleCalc(unittest.TestCase):
    """ Designed to test simple id calculation from an image.

    chessboard1.jpg was taken with Lumix. camMat and disMat are the intrinsics
    and were calulated using opencv.
    """
    def setUp (self):
        # intrinsics for Nikion 5100 with sigma lens 10mm to 20mm
        self.camMatS10mm20mm = [[3868.352132323942, 0.0, 1793.818904445119],
                                [0.0, 3861.2653579525527, 1309.1546288312893],
                               [0.0, 0.0, 1.0]]
        self.disMatS10mm20mm = [-0.23074414076614339,
                                0.06082764182000765,
                                0.004686710353697188,
                                8.29981714263666e-05,
                                1.8496002163239513]
        self.ifS10mm20mm = "images/chessSpheres1.jpg"

        # intrinsics for Lumix 35 Equiv
        self.camMatLumix = [[3868.352132323942, 0.0, 1793.818904445119],
                            [0.0, 3861.2653579525527, 1309.1546288312893],
                            [0.0, 0.0, 1.0]]
        self.disMatLumix = [-0.23074414076614339,
                            0.06082764182000765,
                            0.004686710353697188,
                            8.29981714263666e-05,
                            1.8496002163239513]
        self.ifLumix = "images/chessboard1.jpg"

    def test_Sigma (self):
        import _ilac
        icb = _ilac.IlacCB(self.ifS10mm20mm, 5, 6,
                self.camMatS10mm20mm, self.disMatS10mm20mm, 10, 40)
        id = icb.getID()
        self.assertEqual ( id, 6 )

    def test_Lumix (self):
        import _ilac
        icb = _ilac.IlacCB(self.ifLumix, 5, 6,
                self.camMatLumix, self.disMatLumix, 10, 40)
        id = icb.getID()
        self.assertEqual ( id, 8 )

