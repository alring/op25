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

import sys
import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir, fsk4
from gnuradio.eng_option import eng_option
from optparse import OptionParser

# accepts an input file in complex format 
# applies frequency translation, resampling (interpolation/decimation)
# resampled result is CQPSK demodulated then decoded via fsk4.apco25_f()

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)

        parser.add_option("-c", "--calibration", type="int", default=0, help="freq offset")
        parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
        parser.add_option("-l", "--log", action="store_true", default=False, help="dump debug .dat files")
        parser.add_option("-L", "--low-pass", type="eng_float", default=15e3, help="low pass cut-off", metavar="Hz")
        parser.add_option("-o", "--output-file", type="string", default="out.dat", help="specify the output file")
        parser.add_option("-s", "--sample-rate", type="int", default=250000, help="input sample rate")
        parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
        (options, args) = parser.parse_args()
 
        sample_rate = options.sample_rate
        symbol_rate = 4800
        sps = 10
        # output rate will be 48,000
        ntaps = 11 * sps
        new_sample_rate = symbol_rate * sps
        lcm = gru.lcm(sample_rate, new_sample_rate)
        interp = lcm // sample_rate
        decim  = lcm // new_sample_rate

        channel_taps = gr.firdes.low_pass(1.0, sample_rate, options.low_pass, options.low_pass * 0.1, gr.firdes.WIN_HANN)

        FILTER = gr.freq_xlating_fir_filter_ccf(1, channel_taps, options.calibration, sample_rate)

        sys.stderr.write("interp: %d decim: %d\n" %(interp, decim))

        bpf_taps = gr.firdes.low_pass(1.0, sample_rate * interp, options.low_pass, options.low_pass * 0.1, gr.firdes.WIN_HANN)
        INTERPOLATOR = gr.interp_fir_filter_ccf (int(interp), bpf_taps)
        DECIMATOR = blks2.rational_resampler_ccf(1, int(decim))

        IN = gr.file_source(gr.sizeof_gr_complex, options.input_file)

        DEMOD = blks2.cqpsk_demod( samples_per_symbol = sps,
                                 excess_bw=0.35,
                                 costas_alpha=0.03,
                                 gain_mu=0.05,
                                 mu=0.05,
                                 omega_relative_limit=0.05,
                                 log=options.log,
                                 verbose=options.verbose)

        msgq = gr.msg_queue()
        DECODER = fsk4.apco25_f(msgq,0)

        self.connect(IN, FILTER, INTERPOLATOR, DECIMATOR, DEMOD, DECODER)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()
