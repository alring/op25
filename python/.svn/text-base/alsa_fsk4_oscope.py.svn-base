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

# Based on usrp_fsk4_oscope.py - sound card coded added by KA1RBI ( ikj1234i at yahoo dot com )

# print "Loading revised usrp_oscope with additional options for scopesink..."

from gnuradio import gr, gru, optfir
from gnuradio import audio
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

	self.offset = 0.0
        
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=None,
                          help="select USRP Rx side A or B (default=first one with a daughterboard)")
        parser.add_option("-d", "--decim", type="int", default=1,
                          help="set fgpa decimation rate to DECIM [default=%default]")
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                          help="set frequency to FREQ", metavar="FREQ")
        parser.add_option("-p", "--protocol", type="int", default=0,
                          help="set protocol: 0 = RDLAP 19.2kbps (default); 1 = APCO25")
        parser.add_option("-g", "--gain", type="eng_float", default=1,
                          help="set gain multiplier")
        parser.add_option("-8", "--width-8", action="store_true", default=False,
                          help="Enable 8-bit samples across USB")
        parser.add_option( "--no-hb", action="store_true", default=False,
                          help="don't use halfband filter in usrp")
        parser.add_option("-C", "--basic-complex", action="store_true", default=False,
                          help="Use both inputs of a basicRX or LFRX as a single Complex input channel")
        parser.add_option("-D", "--basic-dualchan", action="store_true", default=False,
                          help="Use both inputs of a basicRX or LFRX as seperate Real input channels")
        parser.add_option("-n", "--frame-decim", type="int", default=1,
                          help="set oscope frame decimation factor to n [default=1]")
        parser.add_option("-I", "--audio-input", type="string", default="", help="pcm input device name.  E.g., hw:0,0 or /dev/dsp")
        parser.add_option("-s", "--sample-rate", type="int", default=48000, help="sound card input sample rate")
        parser.add_option("-v", "--v-scale", type="eng_float", default=1000,
                          help="set oscope initial V/div to SCALE [default=%default]")
        parser.add_option("-t", "--t-scale", type="eng_float", default=49e-6,
                          help="set oscope initial s/div to SCALE [default=50us]")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        self.show_debug_info = True
        
        # build the graph
        self.num_inputs=1

        self.msgq = gr.msg_queue(2)         # queue that holds a maximum of 2 messages

        if options.protocol == 0:

  	  # ---------- RD-LAP 19.2 kbps (9600 ksps), 25kHz channel, 
          self.symbol_rate = 9600				# symbol rate; at 2 bits/symbol this corresponds to 19.2kbps
 	  self.channel_decimation = 1 				# decimation (final rate should be at least several symbol rate)
          self.input_sample_rate = options.sample_rate / options.decim		# for USRP: 64MHz / FPGA decimation rate	
	  self.protocol_processing = fsk4.rdlap_f(self.msgq, 0)	# desired protocol processing block selected here

          self.channel_rate = self.input_sample_rate / self.channel_decimation

	  # symbol shaping filter characteristics
          symbol_coeffs = gr.firdes.root_raised_cosine (1.0,            	# gain
                                          self.channel_rate ,       	# sampling rate
                                          self.symbol_rate,             # symbol rate
                                          0.2,                		# width of trans. band
                                          500) 				# filter type 


        if options.protocol == 1:

	  # ---------- APCO-25 C4FM Test Data
          self.symbol_rate = 4800					# symbol rate; at 2 bits/symbol this corresponds to 19.2kbps
	  self.channel_decimation = 1 				# decimation
          self.input_sample_rate = options.sample_rate / options.decim		# for USRP: 64MHz / FPGA decimation rate	
	  self.protocol_processing = fsk4.apco25_f(self.msgq, 0)
          self.channel_rate = self.input_sample_rate / self.channel_decimation

	  # symbol shaping filter
          symbol_coeffs = gr.firdes.root_raised_cosine (1.0,            	# gain
                                          self.channel_rate ,       	# sampling rate
                                          self.symbol_rate,             # symbol rate
                                          0.2,                		# width of trans. band
                                          500) 				# filter type 


        # ----------------- End of setup block




        self.audio_input = audio.source(options.sample_rate, options.audio_input)
        self.audio_gain = gr.multiply_const_ff(options.gain)

        self.scope2 = scopesink2.scope_sink_f(panel, sample_rate=self.symbol_rate,
                                            frame_decim=1,
                                            v_scale=2,
                                            t_scale= 0.025,
                                            num_inputs=self.num_inputs)



	# also note: 
        # this specifies the nominal frequency deviation for the 4-level fsk signal
        symbol_decim = 1
        self.symbol_filter = gr.fir_filter_fff (symbol_decim, symbol_coeffs)

        # eventually specify: sample rate, symbol rate
        self.demod_fsk4 = fsk4.demod_ff(self.msgq, self.channel_rate ,  self.symbol_rate)

	#self.rdlap_processing = fsk4.rdlap_f(self.msgq, 0)

	self.connect(self.audio_input, self.audio_gain, self.symbol_filter, self.demod_fsk4, self.protocol_processing)

        self.connect(self.demod_fsk4, self.scope2)


	# --------------- End of most of the 4L-FSK hack & slash



        self._build_gui(vbox)

        # set initial values

        self.set_gain(options.gain)

        if self.show_debug_info:
#            self.myform['decim'].set_value(self.u.decim_rate())
            self.myform['dbname'].set_value("alsa")
            self.myform['baseband'].set_value(0)
            self.myform['ddc'].set_value(0)
            if self.num_inputs==2:
              self.myform['baseband2'].set_value(0)
              self.myform['ddc2'].set_value(0)              

    def _set_status_msg(self, msg):
        self.frame.GetStatusBar().SetStatusText(msg, 0)

    def _build_gui(self, vbox):

        def _form_set_freq(kv):
            return self.set_freq(kv['freq'])

        def _form_set_freq2(kv):
            return self.set_freq2(kv['freq2'])            
        vbox.Add(self.scope2.win, 10, wx.EXPAND)
        
        # add control area at the bottom
        self.myform = myform = form.form()
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add((5,0), 0, 0)
        myform['gain'] = form.slider_field(parent=self.panel, sizer=hbox, label="Gain",
                                           weight=3,
                                           min=1, max=150,
                                           callback=self.set_gain)

        hbox.Add((5,0), 0, 0)
        vbox.Add(hbox, 0, wx.EXPAND)

        self._build_subpanel(vbox)

    def _build_subpanel(self, vbox_arg):
        # build a secondary information panel (sometimes hidden)

        # FIXME figure out how to have this be a subpanel that is always
        # created, but has its visibility controlled by foo.Show(True/False)
        
#        def _form_set_decim(kv):
#            return self.set_decim(kv['decim'])

        if not(self.show_debug_info):
            return

        panel = self.panel
        vbox = vbox_arg
        myform = self.myform

        #panel = wx.Panel(self.panel, -1)
        #vbox = wx.BoxSizer(wx.VERTICAL)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add((5,0), 0)

#        myform['decim'] = form.int_field(
#            parent=panel, sizer=hbox, label="Decim",
#            callback=myform.check_input_and_call(_form_set_decim, self._set_status_msg))

        hbox.Add((5,0), 1)
        myform['fs@usb'] = form.static_float_field(
            parent=panel, sizer=hbox, label="Fs@USB")

        hbox.Add((5,0), 1)
        myform['dbname'] = form.static_text_field(
            parent=panel, sizer=hbox)

        hbox.Add((5,0), 1)
        myform['baseband'] = form.static_float_field(
            parent=panel, sizer=hbox, label="Analog BB")

        hbox.Add((5,0), 1)
        myform['ddc'] = form.static_float_field(
            parent=panel, sizer=hbox, label="DDC")
        if self.num_inputs==2:
          hbox.Add((1,0), 1)
          myform['baseband2'] = form.static_float_field(
              parent=panel, sizer=hbox, label="BB2")
          hbox.Add((1,0), 1)
          myform['ddc2'] = form.static_float_field(
            parent=panel, sizer=hbox, label="DDC2")          

        hbox.Add((5,0), 0)
        vbox.Add(hbox, 0, wx.EXPAND)

        
    def set_gain(self, gain):
        self.myform['gain'].set_value(gain)     # update displayed value
        self.audio_gain.set_k(gain)

#    def set_decim(self, decim):
#        ok = self.u.set_decim_rate(decim)
#        if not ok:
#            print "set_decim failed"
#        input_rate = self.u.adc_freq() / self.u.decim_rate()
#        self.scope.set_sample_rate(input_rate)
#        if self.show_debug_info:  # update displayed values
#            self.myform['decim'].set_value(self.u.decim_rate())
#            self.myform['fs@usb'].set_value(self.u.adc_freq() / self.u.decim_rate())
#        return ok


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

    app = stdgui2.stdapp( app_top_block, "USRP O'scope + 4LFSK", nstatus=1 )
    app.MainLoop()

if __name__ == '__main__':
    main ()
