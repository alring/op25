#!/usr/bin/env python
#
# Copyright 2009 KA1RBI
# Copyright 2011 Steve Glass.
# 
# This file is part of GNU Radio and part of OP25
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# It is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import math
import sys
import wx

from gnuradio import audio
from gnuradio import blks2
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import gru
from gnuradio import usrp
from gnuradio.eng_option import eng_option

# Python is putting the packages in some strange places
# This is a workaround until we figure out WTF is going on
try:
    from gnuradio import op25
except Exception:
    import op25


"""
Hierarchical block for RRC-filtered P25 FM modulation.

The input is a dibit (P25 symbol) stream (char, not packed) and the
output is the float "C4FM" signal at baseband, suitable for
application to an FM modulator stage

Input is at the base symbol rate (4800), output sample rate is
typically either 32000 (USRP TX chain) or 48000 (sound card)

@param output_sample_rate: output sample rate
@type output_sample_rate: integer
"""
class p25_mod_bf(gr.hier_block2):

    def __init__(self, output_rate):

	gr.hier_block2.__init__(self, "p25_c4fm_mod_bf",
				gr.io_signature(1, 1, gr.sizeof_char),  # Input signature
				gr.io_signature(1, 1, gr.sizeof_float)) # Output signature

        symbol_rate = 4800   # P25 baseband symbol rate
        lcm = gru.lcm(symbol_rate, output_rate)
        self._interp_factor = int(lcm // symbol_rate)
        self._decimation = int(lcm // output_rate)
        self._excess_bw =0.2

        mod_map = [1.0/3.0, 1.0, -(1.0/3.0), -1.0]
        self.C2S = gr.chunks_to_symbols_bf(mod_map)

        ntaps = 11 * self._interp_factor
        rrc_taps = gr.firdes.root_raised_cosine(
            self._interp_factor, # gain (since we're interpolating by sps)
            lcm,                 # sampling rate
            symbol_rate,
            self._excess_bw,     # excess bandwidth (roll-off factor)
            ntaps)

        self.rrc_filter = gr.interp_fir_filter_fff(self._interp_factor, rrc_taps)
        
        # FM pre-emphasis filter
        shaping_coeffs = [-0.018, 0.0347, 0.0164, -0.0064, -0.0344, -0.0522, -0.0398, 0.0099, 0.0798, 0.1311, 0.121, 0.0322, -0.113, -0.2499, -0.3007, -0.2137, -0.0043, 0.2825, 0.514, 0.604, 0.514, 0.2825, -0.0043, -0.2137, -0.3007, -0.2499, -0.113, 0.0322, 0.121, 0.1311, 0.0798, 0.0099, -0.0398, -0.0522, -0.0344, -0.0064, 0.0164, 0.0347, -0.018]
        self.shaping_filter = gr.fir_filter_fff(1, shaping_coeffs)

        # generate output at appropriate rate
        self.decimator = blks2.rational_resampler_fff(1, self._decimation)

        self.connect(self, self.C2S, self.rrc_filter, self.shaping_filter, self.decimator, self)

    def forecast(noutput, ninput_reqd):
        ninput_reqd[0] = noutput * 26.7

"""
Simple tx-from-pcap-file utility.
"""
class usrp_c4fm_tx_block(gr.top_block):

    def __init__(self, subdev_spec, freq, subdev_gain, filename, delay):

        gr.top_block.__init__ (self)

        # data sink and sink rate
        u = usrp.sink_c()

        # important vars (to be calculated from USRP when available
        dac_rate = u.dac_rate()
        usrp_rate = 320e3
        usrp_interp = int(dac_rate // usrp_rate)
        channel_rate = 32e3
        interp_factor = int(usrp_rate // channel_rate)

        # open the pcap source
        pcap = op25.pcap_source_b(filename, delay)
#        pcap = gr.glfsr_source_b(16)

        # convert octets into dibits
        bits_per_symbol = 2
        unpack = gr.packed_to_unpacked_bb(bits_per_symbol, gr.GR_MSB_FIRST)

        # modulator
        c4fm = p25_mod_bf(output_rate=channel_rate)

        # setup low pass filter + interpolator
        low_pass = 2.88e3
        interp_taps = gr.firdes.low_pass(1.0, channel_rate, low_pass, low_pass * 0.1, gr.firdes.WIN_HANN)
        interpolator = gr.interp_fir_filter_fff (int(interp_factor), interp_taps)

        # frequency modulator
        max_dev = 12.5e3
        k = 2 * math.pi * max_dev / usrp_rate
        adjustment = 1.5   # adjust for proper c4fm deviation level
        fm = gr.frequency_modulator_fc(k * adjustment)

        # signal gain
        gain = gr.multiply_const_cc(4000)

        # configure USRP
        if subdev_spec is None:
            subdev_spec = usrp.pick_tx_subdevice(u)
        u.set_mux(usrp.determine_tx_mux_value(u, subdev_spec))
        self.db = usrp.selected_subdev(u, subdev_spec)
        print "Using TX d'board %s" % (self.db.side_and_name(),)
        u.set_interp_rate(usrp_interp)

        if gain is None:
            g = self.db.gain_range()
            gain = float(g[0] + g[1]) / 2
        self.db.set_gain(self.db.gain_range()[0])
        u.tune(self.db.which(), self.db, freq)
        self.db.set_enable(True)

        self.connect(pcap, unpack, c4fm, interpolator, fm, gain, u)

# inject frames
#
if __name__ == '__main__':

    from optparse import OptionParser
    parser = OptionParser (option_class=eng_option)
    parser.add_option("-T", "--subdev-spec", type="subdev", default=None,
                      help="select USRP Tx side A or B")
    parser.add_option("-f", "--freq", type="eng_float", default=None,
                      help="set Tx frequency to FREQ [required]")
    parser.add_option("-g", "--gain", type="eng_float", default=None, 
                      help="set device gain")
    parser.add_option("-p", "--pcap-file", default="",
                      help="packet capture file [required]")
    parser.add_option("-d", "--delay", type="int", default=0,
                      help="delay before TX of first symbol [default=%default]")
    (options, args) = parser.parse_args ()
    if options.freq is None:
        parser.print_help()
        sys.exit(1)
    
    try:
        rx = usrp_c4fm_tx_block(options.subdev_spec, options.freq, options.gain, options.pcap_file, options.delay)
        rx.run()
    except KeyboardInterrupt:
        rx.db.set_enable(False)
