#!/usr/bin/env python

# Copyright 2010,2011 KA1RBI
# 
# This file is part of OP25.
# 
# OP25 is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# OP25 is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with OP25; see the file COPYING. If not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Boston, MA
# 02110-1301, USA.

import sys
import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir, fsk4, repeater
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
This program reads the soundcard port and listens for two types of activity
 1) P25 voice
 2) conventional analog (optionally, with CTCSS decoding [-c])

The two types of activity are separated and sent over two separate channels
to app_rpt (chan_usrp).  Of course, only one of these would be "active" at
any given time.

No FM demod block is used - the FM demodulation function has already been
performed by hardware inside the host receiver.

Note: proper input gain [-g] is critical for the fsk4 demodulator.  Use the 
procedure shown on the HardwarePage to obtain the proper value for the gain
setting.
"""

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-a", "--audio-input", type="string", default="")
        parser.add_option("-A", "--analog-gain", type="float", default=1.0, help="output gain for analog channel")
        parser.add_option("-c", "--ctcss-freq", type="float", default=0.0, help="CTCSS tone frequency")
        parser.add_option("-d", "--debug", type="int", default=0, help="debug level")
        parser.add_option("-g", "--gain", type="eng_float", default=1, help="adjusts input level for standard data levels")
        parser.add_option("-H", "--hostname", type="string", default="127.0.0.1", help="asterisk host IP")
        parser.add_option("-p", "--port", type="int", default=32001, help="chan_usrp UDP port")
        parser.add_option("-s", "--sample-rate", type="int", default=48000, help="input sample rate")
        parser.add_option("-S", "--stretch", type="int", default=0)
        (options, args) = parser.parse_args()
 
        sample_rate = options.sample_rate
        symbol_rate = 4800
        symbol_decim = 1

        IN = audio.source(sample_rate, options.audio_input)

        symbol_coeffs = gr.firdes.root_raised_cosine(1.0,	# gain
                                          sample_rate ,	# sampling rate
                                          symbol_rate,  # symbol rate
                                          0.2,     	# width of trans. band
                                          500) 		# filter type 
        SYMBOL_FILTER = gr.fir_filter_fff (symbol_decim, symbol_coeffs)
        AMP = gr.multiply_const_ff(options.gain)
        msgq = gr.msg_queue(2)
        FSK4 = fsk4.demod_ff(msgq, sample_rate, symbol_rate)
        levels = levels = [-2.0, 0.0, 2.0, 4.0]
        SLICER = repeater.fsk4_slicer_fb(levels)
        framer_msgq = gr.msg_queue(2)
        DECODE = repeater.p25_frame_assembler('',	# udp hostname
                                              0,	# udp port no.
                                              options.debug,	#debug
                                              True,	# do_imbe
                                              True,	# do_output
                                              False,	# do_msgq
                                              framer_msgq)
        IMBE = repeater.vocoder(False,                 # 0=Decode,True=Encode
                                  options.debug,      # Verbose flag
                                  options.stretch,      # flex amount
                                  "",                   # udp ip address
                                  0,                    # udp port
                                  False)                # dump raw u vectors

        CHAN_RPT  = repeater.chan_usrp_rx(options.hostname, options.port,   options.debug)

        self.connect(IN, AMP, SYMBOL_FILTER, FSK4, SLICER, DECODE, IMBE, CHAN_RPT)

        # blocks for second channel (fm rx)
        output_sample_rate = 8000
        decim_amt = sample_rate / output_sample_rate
        RESAMP = blks2.rational_resampler_fff(1, decim_amt)

        if options.ctcss_freq > 0:
            level = 5.0
            len = 0
            ramp = 0
            gate = True
            CTCSS = repeater.ctcss_squelch_ff(output_sample_rate, options.ctcss_freq, level, len, ramp, gate)

        AMP2 = gr.multiply_const_ff(32767.0 * options.analog_gain)
        CVT = gr.float_to_short()
        CHAN_RPT2 = repeater.chan_usrp_rx(options.hostname, options.port+1, options.debug)

        if options.ctcss_freq > 0:
            self.connect(IN, RESAMP, CTCSS, AMP2, CVT, CHAN_RPT2)
        else:
            self.connect(IN, RESAMP, AMP2, CVT, CHAN_RPT2)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()
