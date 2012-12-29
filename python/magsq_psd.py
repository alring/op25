#!/usr/bin/env python

"""plot power spectral density of magnitude squared, from samples in a file"""

from gnuradio import gr, gru, blks2, optfir
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, fftsink2
from optparse import OptionParser
import wx
import sys

class app_top_block(stdgui2.std_top_block):

    def __init__(self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        self.frame = frame
        self.panel = panel
        
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-c", "--calibration", type="eng_float", default=0, help="frequency offset", metavar="Hz")
        parser.add_option("-i", "--input", default="baseband.dat", help="specify input file")
        parser.add_option("-r", "--repeat", action="store_true", default=False, help="repeat in a loop")
        parser.add_option("-s", "--sample-rate", type="eng_float", default=0, help="sample rate")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        sample_rate = options.sample_rate
        symbol_rate = 4800
        resample_rate = symbol_rate * 4

        FILE = gr.file_source(gr.sizeof_gr_complex, options.input, options.repeat)

        THROTTLE = gr.throttle(gr.sizeof_gr_complex, sample_rate)

        # resample to new rate = resample_rate
        nphases = 64
        frac_bw = 0.25
        rs_taps = gr.firdes.low_pass(nphases, nphases, frac_bw, 0.5-frac_bw)
        #rs_rate = sample_rate / float(resample_rate)
        rs_rate = resample_rate / float(sample_rate)
        RESAMP = blks2.pfb_arb_resampler_ccf(rs_rate, (rs_taps), nphases,)

        channel_taps = optfir.low_pass(1.0, resample_rate, 6500, 8000, 0.1, 60)
        TUNE = gr.freq_xlating_fir_filter_ccf(1, channel_taps, options.calibration, resample_rate)

        # get magnitude squared
        P = gr.complex_to_mag_squared()

        # plot FFT
        SCOPE = fftsink2.fft_sink_f (panel, fft_size=2048, sample_rate=resample_rate)
        self.connect(FILE, THROTTLE, RESAMP, TUNE, P, SCOPE)

        vbox.Add(SCOPE.win, 10, wx.EXPAND)

def main ():
    app = stdgui2.stdapp(app_top_block, "Signal Scope", nstatus=1)
    app.MainLoop()

if __name__ == '__main__':
    main ()
