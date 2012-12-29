#!/usr/bin/env python

# -*- mode: Python -*-

# Copyright 2011 Steve Glass
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

import cPickle
import math
import os
import sys
import threading

from gnuradio import audio, gr, gru, op25, usrp
from gnuradio.eng_option import eng_option
from math import pi

# The P25 receiver
#
class file_c4fm_rx (gr.top_block):

    # Initialize the P25 receiver
    #
    def __init__(self, filename, offset_freq, squelch):

        gr.top_block.__init__(self)

        # open file and info
        f = open(filename + ".info", "r")
        info = cPickle.load(f)
        capture_rate = info["capture-rate"]
        f.close()
        file = gr.file_source(gr.sizeof_gr_complex, filename, True)
        throttle = gr.throttle(gr.sizeof_gr_complex, capture_rate)
        self.connect(file, throttle)
                
        # setup receiver attributes
        channel_rate = 125000
        symbol_rate = 4800
        
        # channel filter
        self.channel_offset = offset_freq
        channel_decim = capture_rate // channel_rate
        channel_rate = capture_rate // channel_decim
        trans_width = 12.5e3 / 2;
        trans_centre = trans_width + (trans_width / 2)
        coeffs = gr.firdes.low_pass(1.0, capture_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
        self.channel_filter = gr.freq_xlating_fir_filter_ccf(channel_decim, coeffs, self.channel_offset, capture_rate)
        self.connect(throttle, self.channel_filter)

        # power squelch
        power_squelch = gr.pwr_squelch_cc(squelch, 1e-3, 0, True)
        self.connect(self.channel_filter, power_squelch)

        # FM demodulator
        self.symbol_deviation = 600.0
        fm_demod_gain = channel_rate / (2.0 * pi * self.symbol_deviation)
        fm_demod = gr.quadrature_demod_cf(fm_demod_gain)
        self.connect(power_squelch, fm_demod)

        # symbol filter        
        symbol_decim = 1
        samples_per_symbol = channel_rate // symbol_rate
        symbol_coeffs = (1.0/samples_per_symbol,) * samples_per_symbol
        symbol_filter = gr.fir_filter_fff(symbol_decim, symbol_coeffs)
        self.connect(fm_demod, symbol_filter)

        # C4FM demodulator
        autotuneq = gr.msg_queue(2)
        self.demod_watcher = demod_watcher(autotuneq, self.adjust_channel_offset)
        demod_fsk4 = op25.fsk4_demod_ff(autotuneq, channel_rate, symbol_rate)
        self.connect(symbol_filter, demod_fsk4)

        # symbol slicer
        levels = [ -2.0, 0.0, 2.0, 4.0 ]
        slicer = op25.fsk4_slicer_fb(levels)
        self.connect(demod_fsk4, slicer)

        # frame decoder
        decoder = op25.decoder_bf()
        self.connect(slicer, decoder)

        # try to connect default output device
        try:
            audio_sink = audio.sink(8000, "plughw:0,0", True)
            self.connect(decoder, audio_sink)
        except Exception:
            sink = gr.null_sink(gr.sizeof_float)
            self.connect(decoder, sink)


    # Adjust the channel offset
    #
    def adjust_channel_offset(self, delta_hz):
        max_delta_hz = 12000.0
        delta_hz *= self.symbol_deviation      
        delta_hz = max(delta_hz, -max_delta_hz)
        delta_hz = min(delta_hz, max_delta_hz)
        self.channel_filter.set_center_freq(self.channel_offset - delta_hz)


# Demodulator frequency tracker
#
class demod_watcher(threading.Thread):

    def __init__(self, msgq,  callback, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon(1)
        self.msgq = msgq
        self.callback = callback
        self.keep_running = True
        self.start()

    def run(self):
        while(self.keep_running):
            msg = self.msgq.delete_head()
            frequency_correction = msg.arg1()
            self.callback(frequency_correction)


# Run the receiver
#
if '__main__' == __name__:

    from optparse import OptionParser
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-i", "--input-file", type="string", default=None, help="path to input file [=%default]")
    parser.add_option("-c", "--calibration", type="eng_float", default=0.0, help="channel frequency offset [=%default]", metavar="Hz")
    parser.add_option("-s", "--squelch", type="eng_float", default=15.0, help="squelch threshold [=%default]", metavar="dB")
    (options, args) = parser.parse_args()
    if len(args) != 0 or options.input_file is None:
        parser.print_help()
        sys.exit(1)
        self.options = options
        
    try:
        rx = file_c4fm_rx(options.input_file, options.calibration, options.squelch)
        rx.run()
    except KeyboardInterrupt:
        pass
