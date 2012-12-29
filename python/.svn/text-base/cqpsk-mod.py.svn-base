#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
#
# cqpsk.py (C) Copyright 2009, KA1RBI
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir
from gnuradio.eng_option import eng_option
from optparse import OptionParser

# produces an output file in complex format at sampling rate=48000
# the input file (a character stream) is used to produce the modulated
# complex output file.

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)

        parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
        parser.add_option("-l", "--log", action="store_true", default=False, help="dump debug .dat files")
        parser.add_option("-o", "--output-file", type="string", default="out.dat", help="specify the output file")
        parser.add_option("-s", "--sample-rate", type="int", default=48000, help="input sample rate")
        parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
        (options, args) = parser.parse_args()
 
        IN = gr.file_source(gr.sizeof_char, options.input_file)

        symbol_rate = 4800
	sample_rate = options.sample_rate
        sps = sample_rate // symbol_rate

        ENC = blks2.cqpsk_mod(samples_per_symbol=sps,
                                 log=options.log,
                                 verbose=options.verbose)

        OUT = gr.file_sink(gr.sizeof_gr_complex, options.output_file)

        self.connect(IN, ENC, OUT)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()

