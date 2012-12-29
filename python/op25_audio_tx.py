#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
# 
# GNU Radio Multichannel APCO P25 Tx (c) Copyright 2009, KA1RBI
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

"""
This program is used in conjunction with op25_tx.py.

When op25_tx.py is used with its -p option (selecting UDP input),
you should start this program beforehand, to start the data flow.

"""

import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir
from gnuradio.eng_option import eng_option
from optparse import OptionParser

from gnuradio import repeater
import op25_c4fm_mod

class app_top_block(gr.top_block):
    def __init__(self, options, queue):
        gr.top_block.__init__(self, "mhp")

        self.audio_amps = []
        self.converters = []
        self.vocoders = []
        self.filters = []
        self.c4fm = []
        self.output_gain = []

        input_audio_rate = 8000
        output_audio_rate = 48000
        c4fm_rate = 96000
        if not options.input_files:
            self.audio_input  = audio.source(input_audio_rate,  options.audio_input)
        self.audio_output = audio.sink  (output_audio_rate, options.audio_output)

        for i in range (options.nchannels):
          if options.input_files:
            t = gr.file_source(gr.sizeof_char,  "baseband-%d.dat" % i, options.repeat)
            self.vocoders.append(t)
          else:
            t = gr.multiply_const_ff(32767 * options.audio_gain)
            self.audio_amps.append(t)
            t = gr.float_to_short()
            self.converters.append(t)
            t = repeater.vocoder(True,                 # 0=Decode,True=Encode
                                  options.verbose,      # Verbose flag
                                  options.stretch,      # flex amount
                                  "",                   # udp ip address
                                  0,                    # udp port
                                  False)                # dump raw u vectors
            self.vocoders.append(t)
          t = op25_c4fm_mod.p25_mod(output_sample_rate=c4fm_rate,
                                 log=False,
                                 verbose=True)
          self.c4fm.append(t)

          # FIXME: it would seem as if direct output at 48000 would be the
          # obvious way to go, but for unknown reasons, it produces hideous
          # distortion in the output waveform.  For the same unknown
          # reasons, it's clean if we output at 96000 and then decimate 
          # back down to 48k...
          t = blks2.rational_resampler_fff(1, 2) # 96000 -> 48000
          self.filters.append(t)
          t = gr.multiply_const_ff(options.output_gain)
          self.output_gain.append(t)

        for i in range (options.nchannels):
          if not options.input_files:
            self.connect(self.audio_amps[i], self.converters[i], self.vocoders[i])
          self.connect(self.vocoders[i], self.c4fm[i], self.filters[i], self.output_gain[i])
        if options.nchannels == 1:
          if not options.input_files:
            self.connect(self.audio_input, self.audio_amps[0])
          self.connect(self.output_gain[0], self.audio_output)
        else:
            for i in range (options.nchannels):
              if not options.input_files:
                self.connect((self.audio_input, i), self.audio_amps[i])
              self.connect(self.output_gain[i], (self.audio_output, i))

def main():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-g", "--audio-gain", type="eng_float", default=1.0, help="input gain factor")
    parser.add_option("-G", "--output-gain", type="eng_float", default=0.6, help="output gain factor")
    parser.add_option("-i", "--input-files", action="store_true", default=False, help="take input from file(s)")
    parser.add_option("-n", "--nchannels", type="int", default=2, help="number of audio channels")
    parser.add_option("-r", "--repeat", action="store_true", default=False, help="replay input")
    parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
    parser.add_option("-I", "--audio-input", type="string", default="", help="pcm input device name.  E.g., hw:0,0 or /dev/dsp")
    parser.add_option("-O", "--audio-output", type="string", default="", help="pcm output device name.  E.g., hw:0,0 or /dev/dsp")
    parser.add_option("-S", "--stretch", type="int", default=0)
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
