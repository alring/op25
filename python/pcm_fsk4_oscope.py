#!/usr/bin/env python
#
# Copyright 2004,2005,2006,2007 Free Software Foundation, Inc.
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

# Original 'usrp_fsk4_oscope.py' modified to work with a normal sound card, WAV files and complex data files.
# By balint256 on http://tech.groups.yahoo.com/group/op25-dev/
#
# Channel filter is only block that decimates
# Therefore same decimation value must be used as when channel filter was used in previous run so filters are identical
# Sensitive to gain when using discriminator tap from N-FM demod

from gnuradio import gr, gru, optfir
from gnuradio import audio	# Soundcard input
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, scopesink2, form, slider
from optparse import OptionParser
import wx
import sys
from gnuradio import fsk4
from math import pi

import threading

class app_top_block(stdgui2.std_top_block):
    def __init__( self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__( self, frame, panel, vbox, argv)

        self.frame = frame
        self.panel = panel

        self.offset = 0.0   # Channel frequency offset

        parser = OptionParser(option_class=eng_option)

        parser.add_option("-p", "--protocol", type="int", default=1,
                          help="set protocol: 0 = RDLAP 19.2kbps; 1 = APCO25 (default)")
        parser.add_option("-g", "--gain", type="eng_float", default=1.0,
                          help="set linear input gain (default: %default)")
        parser.add_option("-x", "--freq-translation", type="eng_float", default=0.0,
                          help="initial channel frequency translation")

        parser.add_option("-n", "--frame-decim", type="int", default=1,
                          help="set oscope frame decimation factor to n [default=1]")
        parser.add_option("-v", "--v-scale", type="eng_float", default=5000,
                          help="set oscope initial V/div to SCALE [default=%default]")
        parser.add_option("-t", "--t-scale", type="eng_float", default=49e-6,
                          help="set oscope initial s/div to SCALE [default=50us]")
        
        parser.add_option("-I", "--audio-input", type="string", default="",
                          help="pcm input device name.  E.g., hw:0,0 or /dev/dsp")
        parser.add_option("-r", "--sample-rate", type="eng_float", default=48000,
                          help="set sample rate to RATE (default: %default)")
        
        parser.add_option("-d", "--channel-decim", type="int", default=None,
                          help="set channel decimation factor to n [default depends on protocol]")
        
        parser.add_option("-w", "--wav-file", type="string", default=None,
                          help="WAV input path")
        parser.add_option("-f", "--data-file", type="string", default=None,
                          help="Data input path")
        parser.add_option("-B", "--base-band", action="store_true", default=False)
        parser.add_option("-R", "--repeat", action="store_true", default=False)
        
        parser.add_option("-o", "--wav-out", type="string", default=None,
                          help="WAV output path")
        parser.add_option("-G", "--wav-out-gain", type="eng_float", default=0.05,
                          help="set WAV output gain (default: %default)")
        
        parser.add_option("-F", "--data-out", type="string", default=None,
                          help="Data output path")
        
        parser.add_option("-C", "--carrier-freq", type="eng_float", default=None,
                          help="set data output carrier frequency to FREQ", metavar="FREQ")

        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)
        
        self.options = options
        
        if options.wav_file is not None:
            #try:
            self.input_file = gr.wavfile_source(options.wav_file, options.repeat)
            #except:
            #    print "WAV file not found or not a WAV file"
            #    sys.exit(1)
            
            print "WAV input: %i Hz, %i bits, %i channels" % (self.input_file.sample_rate(), self.input_file.bits_per_sample(), self.input_file.channels())
            
            self.sample_rate = self.input_file.sample_rate()
            self.input_stream = gr.throttle(gr.sizeof_float, self.sample_rate)
            self.connect(self.input_file, self.input_stream)
            self.src = gr.multiply_const_ff(options.gain)
        elif options.data_file is not None:
            if options.base_band:
                sample_size = gr.sizeof_float
                print "Data file is baseband (float)"
                self.src = gr.multiply_const_ff(options.gain)
            else:
                sample_size = gr.sizeof_gr_complex
                print "Data file is IF (complex)"
                self.src = gr.multiply_const_cc(options.gain)
            
            self.input_file = gr.file_source(sample_size, options.data_file, options.repeat)
            self.sample_rate = options.sample_rate  # E.g. 250000
            
            print "Data file sampling rate = " + str(self.sample_rate)
            
            self.input_stream = gr.throttle(sample_size, self.sample_rate)
            self.connect(self.input_file, self.input_stream)
        else:
            self.sample_rate = options.sample_rate
            print "Soundcard sampling rate = " + str(self.sample_rate)
            self.input_stream = audio.source(self.sample_rate, options.audio_input)  # float samples
            self.src = gr.multiply_const_ff(options.gain)
        
        print "Fixed input gain = " + str(options.gain)
        self.connect(self.input_stream, self.src)
        
        if options.wav_out is not None:
            output_rate = int(self.sample_rate)
            if options.channel_decim is not None:
                output_rate /= options.channel_decim
            self.wav_out = gr.wavfile_sink(options.wav_out, 1, output_rate, 16)
            print "Opened WAV output file: " + options.wav_out + " at rate: " + str(output_rate)
        else:
            self.wav_out = None
        
        if options.data_out is not None:
            if options.carrier_freq is None:
                self.data_out = gr.file_sink(gr.sizeof_float, options.data_out)
                print "Opened float data output file: " + options.data_out
            else:
                self.data_out = gr.file_sink(gr.sizeof_gr_complex, options.data_out)
                print "Opened complex data output file: " + options.data_out
        else:
            self.data_out = None
        
        self.num_inputs = 1
        
        input_rate = self.sample_rate
        #title='IF',
        #size=(1024,800),
        if (options.data_file is not None) and (options.base_band == False):
            self.scope = scopesink2.scope_sink_c(panel,
                                                sample_rate=input_rate,
                                                frame_decim=options.frame_decim,
                                                v_scale=options.v_scale,
                                                t_scale=options.t_scale,
                                                num_inputs=self.num_inputs)
        else:
            self.scope = scopesink2.scope_sink_f(panel,
                                                sample_rate=input_rate,
                                                frame_decim=options.frame_decim,
                                                v_scale=options.v_scale,
                                                t_scale=options.t_scale,
                                                num_inputs=self.num_inputs)
        
        #self.di = gr.deinterleave(gr.sizeof_float)
        #self.di = gr.complex_to_float(1)
        #self.di = gr.complex_to_imag()
        #self.dr = gr.complex_to_real()
        #self.connect(self.src,self.dr)
        #self.connect(self.src,self.di)
        #self.null = gr.null_sink(gr.sizeof_double)
        #self.connect(self.src,self.di)
        #self.connect((self.di,0),(self.scope,0))
        #self.connect((self.di,1),(self.scope,1))
        #self.connect(self.dr,(self.scope,0))
        #self.connect(self.di,(self.scope,1))

        self.msgq = gr.msg_queue(2)         # queue that holds a maximum of 2 messages
        self.queue_watcher = queue_watcher(self.msgq, self.adjust_freq_norm)

        #-------------------------------------------------------------------------------

        if options.protocol == 0:
          # ---------- RD-LAP 19.2 kbps (9600 ksps), 25kHz channel,
          print "RD-LAP selected"
          self.symbol_rate = 9600				# symbol rate; at 2 bits/symbol this corresponds to 19.2kbps

          if options.channel_decim is None:
            self.channel_decimation = 10				# decimation (final rate should be at least several symbol rate)
          else:
            self.channel_decimation = options.channel_decim

          self.max_frequency_offset = 12000.0			# coarse carrier tracker leash, ~ half a channel either way
          self.symbol_deviation = 1200.0			# this is frequency offset from center of channel to +1 / -1 symbols

          self.input_sample_rate = self.sample_rate

          self.protocol_processing = fsk4.rdlap_f(self.msgq, 0)	# desired protocol processing block selected here

          self.channel_rate = self.input_sample_rate / self.channel_decimation

          # channel selection filter characteristics
          channel_taps = optfir.low_pass(1.0,   # Filter gain
                               self.input_sample_rate, # Sample rate
                               10000, # One-sided modulation bandwidth
                               12000, # One-sided channel bandwidth
                               0.1,   # Passband ripple
                               60)    # Stopband attenuation

          # symbol shaping filter characteristics
          symbol_coeffs = gr.firdes.root_raised_cosine (1.0,            	# gain
                                          self.channel_rate ,       	# sampling rate
                                          self.symbol_rate,             # symbol rate
                                          0.2,                		# alpha
                                          500) 				# taps

        if options.protocol == 1:
          # ---------- APCO-25 C4FM Test Data
          print "APCO selected"
          self.symbol_rate = 4800					# symbol rate
          
          if options.channel_decim is None:
            self.channel_decimation = 20				# decimation
          else:
            self.channel_decimation = options.channel_decim
            
          self.max_frequency_offset = 6000.0			# coarse carrier tracker leash 
          self.symbol_deviation = 600.0				# this is frequency offset from center of channel to +1 / -1 symbols
          
          self.input_sample_rate = self.sample_rate
          
          self.protocol_processing = fsk4.apco25_f(self.msgq, 0)
          self.channel_rate = self.input_sample_rate / self.channel_decimation

          # channel selection filter
          if (options.data_file is not None) and (options.base_band == False):
                channel_taps = optfir.low_pass(1.0,   # Filter gain
                                     self.input_sample_rate, # Sample rate
                                     5000, # One-sided modulation bandwidth
                                     6500, # One-sided channel bandwidth
                                     0.2,   # Passband ripple (was 0.1)
                                     60)    # Stopband attenuation
          else:
            channel_taps = None

          # symbol shaping filter
          symbol_coeffs = gr.firdes.root_raised_cosine (1.0,            # gain
                                          self.channel_rate,       	# sampling rate
                                          self.symbol_rate,             # symbol rate
                                          0.2,                		# alpha
                                          500) 				# taps

        # ----------------- End of setup block
        
        print "Input rate = " + str(self.input_sample_rate)
        print "Channel decimation = " + str(self.channel_decimation)
        print "Channel rate = " + str(self.channel_rate)

        if channel_taps is not None:
            self.chan = gr.freq_xlating_fir_filter_ccf(self.channel_decimation,    # Decimation rate
                                                  channel_taps,  # Filter taps
                                                  0.0,   # Offset frequency
                                                  self.input_sample_rate) # Sample rate
            if (options.freq_translation != 0):
                print "Channel center frequency = " + str(options.freq_translation)
                self.chan.set_center_freq(options.freq_translation)
        else:
            self.chan = None
        
        if options.carrier_freq is not None:
            print "Carrier frequency = " + str(options.carrier_freq)
            self.sig_carrier = gr.sig_source_c(self.channel_rate, gr.GR_COS_WAVE, options.carrier_freq, 1, 0)
            self.carrier_mul = gr.multiply_vcc(1)   # cc(gr.sizeof_gr_complex)
            
            self.connect(self.sig_carrier, (self.carrier_mul, 0))
            self.connect(self.chan, (self.carrier_mul, 1))
            
            self.sig_i_carrier = gr.sig_source_f(self.channel_rate, gr.GR_COS_WAVE, options.carrier_freq, 1, 0)
            self.sig_q_carrier = gr.sig_source_f(self.channel_rate, gr.GR_COS_WAVE, options.carrier_freq, 1, (pi / 2.0))
            self.carrier_i_mul = gr.multiply_ff(1)
            self.carrier_q_mul = gr.multiply_ff(1)
            self.iq_to_float = gr.complex_to_float(1)
            self.carrier_iq_add = gr.add_ff(1)
            
            self.connect(self.carrier_mul, self.iq_to_float)
            self.connect((self.iq_to_float, 0), (self.carrier_i_mul, 0))
            self.connect((self.iq_to_float, 1), (self.carrier_q_mul, 0))
            self.connect(self.sig_i_carrier, (self.carrier_i_mul, 1))
            self.connect(self.sig_q_carrier, (self.carrier_q_mul, 1))
            self.connect(self.carrier_i_mul, (self.carrier_iq_add, 0))
            self.connect(self.carrier_q_mul, (self.carrier_iq_add, 1))
        else:
            self.sig_carrier = None
            self.carrier_mul = None
        
        #lf_channel_taps = optfir.low_pass(1.0,   # Filter gain
        #            self.input_sample_rate, # Sample rate
        #            2500, # One-sided modulation bandwidth
        #            3250, # One-sided channel bandwidth
        #            0.1,   # Passband ripple (0.1)
        #            60,    # Stopband attenuation
        #            9)     # Extra taps (default 2, which doesn't work at 48kHz)
        #self.lf = gr.fir_filter_fff(self.channel_decimation, lf_channel_taps)

        self.scope2 = scopesink2.scope_sink_f(panel, sample_rate=self.symbol_rate,
                                            frame_decim=1,
                                            v_scale=2,
                                            t_scale=0.025,
                                            num_inputs=self.num_inputs)

        # also note: this specifies the nominal frequency deviation for the 4-level fsk signal
        self.fm_demod_gain = self.channel_rate / (2.0 * pi * self.symbol_deviation)
        self.fm_demod = gr.quadrature_demod_cf(self.fm_demod_gain)
        
        symbol_decim = 1
        self.symbol_filter = gr.fir_filter_fff(symbol_decim, symbol_coeffs)

        # eventually specify: sample rate, symbol rate
        self.demod_fsk4 = fsk4.demod_ff(self.msgq, self.channel_rate, self.symbol_rate)

        if (self.chan is not None):
            self.connect(self.src, self.chan, self.fm_demod, self.symbol_filter, self.demod_fsk4, self.protocol_processing)
            
            if options.wav_out is not None:
                print "WAV output gain = " + str(options.wav_out_gain)
                self.scaled_wav_data = gr.multiply_const_ff(float(options.wav_out_gain))
                self.connect(self.scaled_wav_data, self.wav_out)
                
                if self.carrier_mul is None:
                    self.connect(self.fm_demod, self.scaled_wav_data)
                else:
                    self.connect(self.carrier_iq_add, self.scaled_wav_data)
            
            if self.data_out is not None:
                if self.carrier_mul is None:
                    self.connect(self.fm_demod, self.data_out)
                else:
                    self.connect(self.carrier_mul, self.data_out)
            
            # During signal, -4..4
            #self.connect(self.fm_demod, self.scope2)
        else:
            self.connect(self.src, self.symbol_filter, self.demod_fsk4, self.protocol_processing)
        
        self.connect(self.src, self.scope)
        #self.connect(self.lf, self.scope)
        
        self.connect(self.demod_fsk4, self.scope2)
        #self.connect(self.symbol_filter, self.scope2)

	# --------------- End of most of the 4L-FSK hack & slash

        self._build_gui(vbox)

        # set initial values

        if options.gain is None:
            options.gain = 0

        self.set_gain(options.gain)


    def adjust_freq_norm(self, frequency_offset):
        self.offset -=  frequency_offset * self.symbol_deviation
        if self.offset > self.max_frequency_offset:
          self.offset = self.max_frequency_offset
        if self.offset < -self.max_frequency_offset:
          self.offset = -self.max_frequency_offset
        print "Channel frequency offset message processed, now at (Hz):", int(self.offset)
        if (self.chan is not None):
            self.chan.set_center_freq(self.offset)


    def _set_status_msg(self, msg):
        self.frame.GetStatusBar().SetStatusText(msg, 0)

    def _build_gui(self, vbox):
        vbox.Add(self.scope.win, 10, wx.EXPAND)
        vbox.Add(self.scope2.win, 10, wx.EXPAND)
        
        # add control area at the bottom
        self.myform = myform = form.form()
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add((5,0), 0, 0)
        g = [0,1000]   # Faux gain range
        myform['gain'] = form.slider_field(parent=self.panel, sizer=hbox, label="Gain",
                                           weight=3,
                                           min=int(g[0]), max=int(g[1]),
                                           callback=self.gui_set_gain)
        self.myform['gain'].set_value(int(self.options.gain * 10.0))

    def gui_set_gain(self, gain):
        self.set_gain(float(gain) / 10.0)
        
    def set_gain(self, gain):
        #self.myform['gain'].set_value(int(gain * 10.0))     # update displayed value
        #self.subdev.set_gain(gain)
        print "Gain now = " + str(gain)
        self.src.set_k(gain)


#-----------------------------------------------------------------------
# Queue watcher.  Dispatches measured delay to callback.
#-----------------------------------------------------------------------
class queue_watcher (threading.Thread):
    def __init__ (self, msgq,  callback, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon (1)
        self.msgq = msgq
        self.callback = callback
        self.keep_running = True
        self.start ()

    def run (self):
        while (self.keep_running):
            msg = self.msgq.delete_head()  # blocking read of message queue
            frequency_correction = msg.arg1() 
            self.callback(frequency_correction)


def main ():
    app = stdgui2.stdapp( app_top_block, "PCM O'scope + 4LFSK", nstatus=1 )
    wav_out = app.TopWindow.top_block().wav_out
    app.MainLoop()
    if wav_out is not None:
        wav_out.close()

if __name__ == '__main__':
    main ()
