#!/usr/bin/env python
import sys
import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir, op25, repeater
from gnuradio.eng_option import eng_option
from optparse import OptionParser

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-a", "--audio-input", type="string", default="")
        parser.add_option("-A", "--audio-output", type="string", default="")
        parser.add_option("-f", "--factor", type="eng_float", default=1)
        parser.add_option("-i", "--do-interp", action="store_true", default=False, help="enable output interpolator")
        parser.add_option("-s", "--sample-rate", type="int", default=48000, help="input sample rate")
        parser.add_option("-S", "--stretch", type="int", default=0, help="flex amt")
        parser.add_option("-y", "--symbol-rate", type="int", default=4800, help="input symbol rate")
        parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
        (options, args) = parser.parse_args()
 
        sample_rate = options.sample_rate
        symbol_rate = options.symbol_rate

        IN = audio.source(sample_rate, options.audio_input)
        audio_output_rate = 8000
        if options.do_interp:
            audio_output_rate = 48000
        OUT = audio.sink(audio_output_rate, options.audio_output)

        symbol_decim = 1
        symbol_coeffs = gr.firdes.root_raised_cosine(1.0,	# gain
                                          sample_rate ,	# sampling rate
                                          symbol_rate,  # symbol rate
                                          0.2,     	# width of trans. band
                                          500) 		# filter type 
        SYMBOL_FILTER = gr.fir_filter_fff (symbol_decim, symbol_coeffs)
        AMP = gr.multiply_const_ff(options.factor)
        msgq = gr.msg_queue(2)
        FSK4 = op25.fsk4_demod_ff(msgq, sample_rate, symbol_rate)
        levels = levels = [-2.0, 0.0, 2.0, 4.0]
        SLICER = repeater.fsk4_slicer_fb(levels)
        framer_msgq = gr.msg_queue(2)
        DECODE = repeater.p25_frame_assembler('',	# udp hostname
                                              0,	# udp port no.
                                              options.verbose,	#debug
                                              True,	# do_imbe
                                              True,	# do_output
                                              False,	# do_msgq
                                              framer_msgq)
        IMBE = repeater.vocoder(False,                 # 0=Decode,True=Encode
                                  options.verbose,      # Verbose flag
                                  options.stretch,      # flex amount
                                  "",                   # udp ip address
                                  0,                    # udp port
                                  False)                # dump raw u vectors

        CVT = gr.short_to_float()
        if options.do_interp:
            interp_taps = gr.firdes.low_pass(1.0, 48000, 4000, 4000 * 0.1, gr.firdes.WIN_HANN)
            INTERP = gr.interp_fir_filter_fff(48000 // 8000, interp_taps)
        AMP2 = gr.multiply_const_ff(1.0 / 32767.0)

        self.connect(IN, AMP, SYMBOL_FILTER, FSK4, SLICER, DECODE, IMBE, CVT, AMP2)
        if options.do_interp:
            self.connect(AMP2, INTERP, OUT)
        else:
            self.connect(AMP2, OUT)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()
