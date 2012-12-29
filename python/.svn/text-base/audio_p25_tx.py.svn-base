#!/usr/bin/env python
#
# accepts input file of P25 symbols consisting of packed characters
# soundcard output signal is suitable for direct connection to an FM transmitter
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
#
# audio_p25_tx.py original portions (C) Copyright 2009, KA1RBI
# coeffs for shaping and cosine filter from Eric Ramsey thesis 
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
from gnuradio import gr, gru, audio, eng_notation, optfir
from gnuradio.eng_option import eng_option
from optparse import OptionParser

class app_top_block(gr.top_block):
    def __init__(self, options, queue):
        gr.top_block.__init__(self, "mhp")

        sample_rate = options.sample_rate

        arity = 2
        IN = gr.file_source(gr.sizeof_char, options.input_file, options.repeat)
        B2C = gr.packed_to_unpacked_bb(arity, gr.GR_MSB_FIRST)
        mod_map = [1.0, 3.0, -1.0, -3.0]
        C2S = gr.chunks_to_symbols_bf(mod_map)
        if options.reverse:
            polarity = gr.multiply_const_ff(-1)
        else:
            polarity = gr.multiply_const_ff( 1)

        symbol_rate = 4800
        samples_per_symbol = sample_rate // symbol_rate
        excess_bw = 0.1
        ntaps = 11 * samples_per_symbol
        rrc_taps = gr.firdes.root_raised_cosine(
            samples_per_symbol, # gain  (sps since we're interpolating by sps
            samples_per_symbol, # sampling rate
            1.0,                # symbol rate
            excess_bw,          # excess bandwidth (roll-off factor)
            ntaps)
        rrc_filter = gr.interp_fir_filter_fff(samples_per_symbol, rrc_taps)

        rrc_coeffs = [0, -0.003, -0.006, -0.009, -0.012, -0.014, -0.014, -0.013, -0.01, -0.006, 0, 0.007, 0.014, 0.02, 0.026, 0.029, 0.029, 0.027, 0.021, 0.012, 0, -0.013, -0.027, -0.039, -0.049, -0.054, -0.055, -0.049, -0.038, -0.021, 0, 0.024, 0.048, 0.071, 0.088, 0.098, 0.099, 0.09, 0.07, 0.039, 0, -0.045, -0.091, -0.134, -0.17, -0.193, -0.199, -0.184, -0.147, -0.085, 0, 0.105, 0.227, 0.36, 0.496, 0.629, 0.751, 0.854, 0.933, 0.983, 1, 0.983, 0.933, 0.854, 0.751, 0.629, 0.496, 0.36, 0.227, 0.105, 0, -0.085, -0.147, -0.184, -0.199, -0.193, -0.17, -0.134, -0.091, -0.045, 0, 0.039, 0.07, 0.09, 0.099, 0.098, 0.088, 0.071, 0.048, 0.024, 0, -0.021, -0.038, -0.049, -0.055, -0.054, -0.049, -0.039, -0.027, -0.013, 0, 0.012, 0.021, 0.027, 0.029, 0.029, 0.026, 0.02, 0.014, 0.007, 0, -0.006, -0.01, -0.013, -0.014, -0.014, -0.012, -0.009, -0.006, -0.003, 0]

        # rrc_coeffs work slightly differently: each input sample
        # (from mod_map above) at 4800 rate, then 9 zeros are inserted
        # to bring to a 48000 rate, then this filter is applied:
        # rrc_filter = gr.fir_filter_fff(1, rrc_coeffs)
        # FIXME: how to insert the 9 zero samples using gr ?

        # FM pre-emphasis filter
        shaping_coeffs = [-0.018, 0.0347, 0.0164, -0.0064, -0.0344, -0.0522, -0.0398, 0.0099, 0.0798, 0.1311, 0.121, 0.0322, -0.113, -0.2499, -0.3007, -0.2137, -0.0043, 0.2825, 0.514, 0.604, 0.514, 0.2825, -0.0043, -0.2137, -0.3007, -0.2499, -0.113, 0.0322, 0.121, 0.1311, 0.0798, 0.0099, -0.0398, -0.0522, -0.0344, -0.0064, 0.0164, 0.0347, -0.018]
        shaping_filter = gr.fir_filter_fff(1, shaping_coeffs)

        OUT = audio.sink(sample_rate, options.audio_output)
        amp = gr.multiply_const_ff(options.factor)

        self.connect(IN, B2C, C2S, polarity, rrc_filter, shaping_filter, amp)
        # output to both L and R channels
        self.connect(amp, (OUT,0) )
        self.connect(amp, (OUT,1) )

def main():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-f", "--factor", type="eng_float", default=1.0, help="multiply factor")
    parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
    parser.add_option("-r", "--repeat", action="store_true", default=False, help="repeat input in a loop")
    parser.add_option("-v", "--reverse", action="store_true", default=False, help="reverse polarity")
    parser.add_option("-s", "--sample-rate", type="int", default=48000)
    parser.add_option("-O", "--audio-output", type="string", default="", help="pcm input device name.  E.g., hw:0,0 or /dev/dsp")
    (options, args) = parser.parse_args()

 
    queue = gr.msg_queue()
    tb = app_top_block(options, queue)
    try:
        tb.start()
        while 1:
            if not queue.empty_p():
                sys.stderr.write("main: q.delete_head()\n")
                msg = queue.delete_head()
    except KeyboardInterrupt:
        tb.stop()

if __name__ == "__main__":
    main()
