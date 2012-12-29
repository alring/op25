#!/usr/bin/env python
#
# Copyright 2004 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
# 

from gnuradio import gr, gr_unittest
import repeater

class qa_repeater (gr_unittest.TestCase):

    def setUp (self):
        self.fg = gr.top_block ()

    def tearDown (self):
        self.fg = None

    def test_001_slice (self):
        src_data = (-3, -1, 1, 3)
        expected_result = (3, 2, 0, 1)
        src = gr.vector_source_f (src_data)
        levels = [-2.0, 0.0, 2.0, 4.0]
        slicer = repeater.fsk4_slicer_fb (levels)
        dst = gr.vector_sink_b ()
        self.fg.connect (src, slicer)
        self.fg.connect (slicer, dst)
        self.fg.run ()
        result_data = dst.data ()
        self.assertFloatTuplesAlmostEqual (expected_result, result_data, 4)
        
if __name__ == '__main__':
    gr_unittest.main ()
