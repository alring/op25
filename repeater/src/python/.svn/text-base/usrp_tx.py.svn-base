#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
# 
# GNU Radio Multichannel APCO P25 Tx (c) Copyright 2009,2010 KA1RBI
# 
# This file is part of GNU Radio and part of OP25
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# It is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

"""
Transmit N simultaneous narrow band P25 signals.

"""

from gnuradio import gr, eng_notation, repeater
from gnuradio import usrp
from gnuradio import audio
from gnuradio import blks2
from gnuradio.eng_option import eng_option
from optparse import OptionParser
from usrpm import usrp_dbid
import math
import sys
import os

from gnuradio.wxgui import stdgui2, fftsink2
import wx

MAX_GAIN = 16000.0
MAX_COMPLEX_RATE = 960e3	# per complex mod tables in waveforms.h
				# this is lcm(48k, 320k)

class tx_channel_usrp(gr.hier_block2):
    def __init__(self, port, gain, usrp_rate, lo_freq):
        gr.hier_block2.__init__(self, "tx_channel_usrp",
                                gr.io_signature(0, 0, 0),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))

        msg_queue = gr.msg_queue(2)
        do_imbe = 1
        do_float = 0
        do_complex = 1
        decim = MAX_COMPLEX_RATE / usrp_rate
        # per-channel GR source block including these steps:
        # - receive audio chunks from asterisk via UDP
        # - imbe encode
        # - generate phase-modulated complex output stream (table lookup method)
        # - generates no power while no input received
        self.chan = repeater.chan_usrp(port, do_imbe, do_complex, do_float, gain, int(decim), msg_queue)

        # Local oscillator
        lo = gr.sig_source_c (usrp_rate,      # sample rate
                              gr.GR_SIN_WAVE, # waveform type
                              lo_freq,        #frequency
                              1.0,            # amplitude
                              0)              # DC Offset
        self.mixer = gr.multiply_cc ()
        self.connect (self.chan, (self.mixer, 0))
        self.connect (lo, (self.mixer, 1))
        self.connect (self.mixer, self)

class fm_tx_block(stdgui2.std_top_block):
    def __init__(self, frame, panel, vbox, argv):
        MAX_CHANNELS = 7
        stdgui2.std_top_block.__init__ (self, frame, panel, vbox, argv)

        parser = OptionParser (option_class=eng_option)
        parser.add_option("-T", "--tx-subdev-spec", type="subdev", default=None,
                          help="select USRP Tx side A or B")
        parser.add_option("-d","--debug", action="store_true", default=False)
        parser.add_option("-e","--enable-fft", action="store_true", default=False,
                          help="enable spectrum plot (and use more CPU)")
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                           help="set Tx frequency to FREQ [required]", metavar="FREQ")
        parser.add_option("-n", "--nchannels", type="int", default=2,
                           help="number of Tx channels [1,4]")
        parser.add_option("-p", "--udp-port", type="int", default=32001,
                           help="initial UDP port number")
        (options, args) = parser.parse_args ()

        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        if options.nchannels < 1 or options.nchannels > MAX_CHANNELS:
            sys.stderr.write ("op25_tx: nchannels out of range.  Must be in [1,%d]\n" % MAX_CHANNELS)
            sys.exit(1)
        
        if options.freq is None:
            sys.stderr.write("op25_tx: must specify frequency with -f FREQ\n")
            parser.print_help()
            sys.exit(1)

        # ----------------------------------------------------------------
        # Set up constants and parameters

        self.u = usrp.sink_c ()       # the USRP sink (consumes samples)

        self.dac_rate = self.u.dac_rate()                    # 128 MS/s
        self.usrp_interp = 400
        self.u.set_interp_rate(self.usrp_interp)
        self.usrp_rate = self.dac_rate / self.usrp_interp    # 320 kS/s

        # determine the daughterboard subdevice we're using
        if options.tx_subdev_spec is None:
            options.tx_subdev_spec = usrp.pick_tx_subdevice(self.u)

        m = usrp.determine_tx_mux_value(self.u, options.tx_subdev_spec)
        #print "mux = %#04x" % (m,)
        self.u.set_mux(m)
        self.subdev = usrp.selected_subdev(self.u, options.tx_subdev_spec)
        print "Using TX d'board %s" % (self.subdev.side_and_name(),)

        self.subdev.set_gain(self.subdev.gain_range()[0])    # set min Tx gain
        if not self.set_freq(options.freq):
            freq_range = self.subdev.freq_range()
            print "Failed to set frequency to %s.  Daughterboard supports %s to %s" % (
                eng_notation.num_to_str(options.freq),
                eng_notation.num_to_str(freq_range[0]),
                eng_notation.num_to_str(freq_range[1]))
            raise SystemExit
        self.subdev.set_enable(True)                         # enable transmitter

        self.sum = gr.add_cc()

        step = 25e3
        offset = (0 * step, 1 * step, -1 * step, 2 * step, -2 * step, 3 * step, -3 * step)

        # Instantiate N TX channels
	for i in range(options.nchannels):
            t = tx_channel_usrp(options.udp_port + i, MAX_GAIN / options.nchannels, self.usrp_rate, offset[i])
            self.connect (t, (self.sum, i))
        self.connect (self.sum, self.u)

        # plot an FFT to verify we are sending what we want
        if options.enable_fft:
            post_mod = fftsink2.fft_sink_c(panel, title="Post Modulation",
                                           fft_size=512, sample_rate=self.usrp_rate,
                                           y_per_div=20, ref_level=40)
            self.connect (self.sum, post_mod)
            vbox.Add (post_mod.win, 1, wx.EXPAND)
            

        if options.debug:
        #    self.debugger = tx_debug_gui.tx_debug_gui(self.subdev)
        #    self.debugger.Show(True)
          print "attach pid %d" % os.getpid()
          raw_input("press enter")


    def set_freq(self, target_freq):
        """
        Set the center frequency we're interested in.

        @param target_freq: frequency in Hz
        @rypte: bool

        Tuning is a two step process.  First we ask the front-end to
        tune as close to the desired frequency as it can.  Then we use
        the result of that operation and our target_frequency to
        determine the value for the digital up converter.  Finally, we feed
        any residual_freq to the s/w freq translater.
        """

        r = self.u.tune(self.subdev.which(), self.subdev, target_freq)
        if r:
            print "r.baseband_freq =", eng_notation.num_to_str(r.baseband_freq)
            print "r.dxc_freq      =", eng_notation.num_to_str(r.dxc_freq)
            print "r.residual_freq =", eng_notation.num_to_str(r.residual_freq)
            print "r.inverted      =", r.inverted
            
            # Could use residual_freq in s/w freq translator
            return True

        return False

def main ():
    sys.stderr.write("GNU Radio Multichannel APCO P25 Tx (c) Copyright 2009,2010 KA1RBI\n")
    app = stdgui2.stdapp(fm_tx_block, "Multichannel APCO P25 Tx", nstatus=1)
    app.MainLoop ()

if __name__ == '__main__':
    main ()
