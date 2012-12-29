#!/usr/bin/env python
import sys
import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir, repeater
from gnuradio.eng_option import eng_option
from optparse import OptionParser
# import cqpsk
from math import pi 

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)

        parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
        parser.add_option("-g", "--gain", type="eng_float", default=1.0)
        parser.add_option("-L", "--low-pass", type="eng_float", default=15e3, help="low pass cut-off", metavar="Hz")
        parser.add_option("-o", "--output-file", type="string", default="out.dat", help="specify the output file")
        parser.add_option("-s", "--sample-rate", type="int", default=48000, help="input sample rate")
        parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
        (options, args) = parser.parse_args()
 
        sample_rate = options.sample_rate
        sps = 10
        symbol_rate = 4800
        # output rate will be 48,000
        ntaps = 11 * sps

        channel_taps = gr.firdes.low_pass(1.0, sample_rate, options.low_pass, options.low_pass * 0.1, gr.firdes.WIN_HANN)

        IN = gr.file_source(gr.sizeof_short, options.input_file)
        OUT = gr.file_sink(gr.sizeof_char, options.output_file)

        CVT = gr.short_to_float()
        AMP = gr.multiply_const_ff(options.gain / 32767.0)

        symbol_decim = 1
        symbol_coeffs = gr.firdes.root_raised_cosine (1.0,        	# gain
                                          sample_rate ,  	# sampling rate
                                          symbol_rate,     # symbol rate
                                          0.20,     	# width of trans. band
                                          500) 		# filter type 
        SYMBOL_FILTER = gr.fir_filter_fff (symbol_decim, symbol_coeffs)
        self.msgq = gr.msg_queue(2)

        FSK4 = op25.fsk4_demod_ff(self.msgq, sample_rate, symbol_rate)

        levels = [ -2.0, 0.0, 2.0, 4.0 ]
        SLICER = repeater.fsk4_slicer_fb(levels)

        hostname = "127.0.0.1"
        port = 23456
        debug = 255
        do_imbe = False
        do_output = True
        do_msgq = False
        msgqd = gr.msg_queue(2)
        DECODER = repeater.p25_frame_assembler(hostname, port, debug, do_imbe, do_output, do_msgq, msgqd)

        self.connect(IN, CVT, AMP, SYMBOL_FILTER, FSK4, SLICER, DECODER, OUT)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()
