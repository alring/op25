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

import math
import os
import sys
import threading

from gnuradio import audio, gr, gru, op25, usrp2
from gnuradio.eng_option import eng_option
from math import pi

# The P25 receiver
#
class usrp2_c4fm_rx (gr.top_block):

    # Initialize the P25 receiver
    #
    def __init__(self, interface, address, center_freq, offset_freq, decim, squelch, gain):

        gr.top_block.__init__(self)

        # setup USRP2
        u = usrp2.source_32fc(interface, address)
        u.set_decim(decim)
        capture_rate = u.adc_rate() / decim
        u.set_center_freq(center_freq)
        if gain is None:
            g = u.gain_range()
            gain = float(g[0] + g[1]) / 2
        u.set_gain(gain)

        # Setup receiver attributes
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
        self.connect(u, self.channel_filter)

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

        # try to connect audio output device
        try:
            audio_sink = audio.sink(8000, "plughw:0,0", True)
            self.connect(decoder, audio_sink)
        except Exception:
            sink = gr.null_sink(gr.sizeof_float)
            self.connect(decoder, null);


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
    parser.add_option("-i", "--interface", type="string", default="eth0",
                      help="select Ethernet interface, default is eth0")
    parser.add_option("-a", "--address", type="string", default="",
                      help="select USRP by MAC address, default is auto-select")
    parser.add_option("-f", "--freq", type="eng_float", default=434.075e6,
                      help="rx frequency [=%default]", metavar="Hz")
    parser.add_option("-c", "--calibration", type="eng_float", default=0.0,
                      help="frequency offset [=%default]", metavar="Hz")
    parser.add_option("-d", "--decim", type="int", default=400,
                      help="decimation factor [=%default]")
    parser.add_option("-s", "--squelch", type="eng_float", default=15.0,
                      help="squelch threshold [=%default]", metavar="dB")
    parser.add_option("-g", "--gain", type="eng_float", default=None, help="gain", metavar="dB")
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        sys.exit(1)
        self.options = options
        
    try:
        rx = usrp2_c4fm_rx(options.interface, options.address, options.freq, options.calibration, options.decim, options.squelch, options.gain)
        rx.run()
    except KeyboardInterrupt:
        pass
