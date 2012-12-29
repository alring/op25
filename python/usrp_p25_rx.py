#!/usr/bin/env python

# -*- mode: Python -*-

# Copyright 2008-2011 Steve Glass
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

import os
import sys
import threading
import wx
import wx.html
import wx.wizard

from gnuradio import audio, eng_notation, gr, gru
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, fftsink2, scopesink2
from math import pi
from optparse import OptionParser

import pickle

# Python is doing strange things to our packages
# So we try to handle it here
#
try:
    from gnuradio import op25
except Exception:
    import op25

# The P25 receiver
#
class p25_rx_block (stdgui2.std_top_block):

    # Initialize the P25 receiver
    #
    def __init__(self, frame, panel, vbox, argv):

        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        # do we have a USRP?
        try:
            self.usrp = None
            from gnuradio import usrp
            self.usrp = usrp.source_c()
        except Exception:
            ignore = True

        # setup (read-only) attributes
        self.channel_rate = 125000
        self.symbol_rate = 4800
        self.symbol_deviation = 600.0

        # keep track of flow graph connections
        self.cnxns = []

        # initialize the UI
        # 
        self.__init_gui(frame, panel, vbox)

        # command line argument parsing
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(1, 0), help="select USRP Rx side A or B")
        parser.add_option("-d", "--decim", type="int", default=256, help="source decimation factor")
        parser.add_option("-f", "--frequency", type="eng_float", default=None, help="USRP center frequency", metavar="Hz")
        parser.add_option("-g", "--gain", type="eng_float", default=None, help="set USRP gain in dB (default is midpoint)")
        parser.add_option("-i", "--input", default=None, help="input file name")
        parser.add_option("-w", "--wait", action="store_true", default=False, help="block on startup")
	parser.add_option("-t", "--transient", action="store_true", default=False, help="enable transient captures")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)
        self.options = options

        # wait for gdb
        if options.wait:
            print 'Ready for GDB to attach (pid = %d)' % (os.getpid(),)
            raw_input("Press 'Enter' to continue...")

        # configure specified data source
        if options.input:
            self.open_file(options.input)
        elif options.frequency:
            self._set_state("CAPTURING")
            self.open_usrp(options.rx_subdev_spec, options.decim, options.gain, options.frequency, not options.transient)
        else:
            self._set_state("STOPPED")

        # save cmd-line options
        self.options = options

    # setup common flow graph elements
    #
    def __build_graph(self, source, capture_rate):
        # tell the scope the source rate
        self.spectrum.set_sample_rate(capture_rate)
        # channel filter
        self.channel_offset = 0.0
        channel_decim = capture_rate // self.channel_rate
        channel_rate = capture_rate // channel_decim
        trans_width = 12.5e3 / 2;
        trans_centre = trans_width + (trans_width / 2)
        coeffs = gr.firdes.low_pass(1.0, capture_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
        self.channel_filter = gr.freq_xlating_fir_filter_ccf(channel_decim, coeffs, 0.0, capture_rate)
        self.set_channel_offset(0.0, 0, self.spectrum.win._units)
        # power squelch
        squelch_db = 0
        self.squelch = gr.pwr_squelch_cc(squelch_db, 1e-3, 0, True)
        self.set_squelch_threshold(squelch_db)
        # FM demodulator
        fm_demod_gain = channel_rate / (2.0 * pi * self.symbol_deviation)
        fm_demod = gr.quadrature_demod_cf(fm_demod_gain)
        # symbol filter        
        symbol_decim = 1
        # symbol_coeffs = gr.firdes.root_raised_cosine(1.0, channel_rate, self.symbol_rate, 0.2, 500)
        # boxcar coefficients for "integrate and dump" filter
        samples_per_symbol = channel_rate // self.symbol_rate
        symbol_coeffs = (1.0/samples_per_symbol,)*samples_per_symbol
        symbol_filter = gr.fir_filter_fff(symbol_decim, symbol_coeffs)
        # C4FM demodulator
        autotuneq = gr.msg_queue(2)
        self.demod_watcher = demod_watcher(autotuneq, self.adjust_channel_offset)
        demod_fsk4 = op25.fsk4_demod_ff(autotuneq, channel_rate, self.symbol_rate)
        # symbol slicer
        levels = [ -2.0, 0.0, 2.0, 4.0 ]
        slicer = op25.fsk4_slicer_fb(levels)
        # ALSA output device (if not locked)
        try:
            sink = audio.sink(8000, "plughw:0,0", True) # ToDo: get actual device from prefs
        except Exception:
            sink = gr.null_sink(gr.sizeof_float)

        # connect it all up
        self.__connect([[source, self.channel_filter, self.squelch, fm_demod, symbol_filter, demod_fsk4, slicer, self.p25_decoder, sink],
                        [source, self.spectrum],
                        [symbol_filter, self.signal_scope],
                        [demod_fsk4, self.symbol_scope]])

    # Connect up the flow graph
    #
    def __connect(self, cnxns):
        for l in cnxns:
            for b in l:
                if b == l[0]:
                    p = l[0]
                else:
                    self.connect(p, b)
                    p = b
        self.cnxns.extend(cnxns)

    # Disconnect the flow graph
    #
    def __disconnect(self):
        for l in self.cnxns:
            for b in l:
                if b == l[0]:
                    p = l[0]
                else:
                    self.disconnect(p, b)
                    p = b
        self.cnxns = []

    # initialize the UI
    # 
    def __init_gui(self, frame, panel, vbox):
        self.frame = frame
        self.frame.CreateStatusBar()
        self.panel = panel
        self.vbox = vbox
        
        # setup the menu bar
        menubar = self.frame.GetMenuBar()

        # setup the "File" menu
        file_menu = menubar.GetMenu(0)
        self.file_new = file_menu.Insert(0, wx.ID_NEW)
        self.frame.Bind(wx.EVT_MENU, self._on_file_new, self.file_new)
        self.file_open = file_menu.Insert(1, wx.ID_OPEN)
        self.frame.Bind(wx.EVT_MENU, self._on_file_open, self.file_open)
        file_menu.InsertSeparator(2)
        self.file_properties = file_menu.Insert(3, wx.ID_PROPERTIES)
        self.frame.Bind(wx.EVT_MENU, self._on_file_properties, self.file_properties)
        file_menu.InsertSeparator(4)
        self.file_close = file_menu.Insert(5, wx.ID_CLOSE)
        self.frame.Bind(wx.EVT_MENU, self._on_file_close, self.file_close)

        # setup the "Edit" menu
        if False:
            edit_menu = wx.Menu()
            self.edit_undo = edit_menu.Insert(0, wx.ID_UNDO)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_undo, self.edit_undo)
            self.edit_redo = edit_menu.Insert(1, wx.ID_REDO)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_redo, self.edit_redo)
            edit_menu.InsertSeparator(2)
            self.edit_cut = edit_menu.Insert(3, wx.ID_CUT)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_cut, self.edit_cut)
            self.edit_copy = edit_menu.Insert(4, wx.ID_COPY)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_copy, self.edit_copy)
            self.edit_paste = edit_menu.Insert(5, wx.ID_PASTE)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_paste, self.edit_paste)
            self.edit_delete = edit_menu.Insert(6, wx.ID_DELETE)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_delete, self.edit_delete)
            edit_menu.InsertSeparator(7)
            self.edit_select_all = edit_menu.Insert(8, wx.ID_SELECTALL)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_select_all, self.edit_select_all)
            edit_menu.InsertSeparator(9)
            self.edit_prefs = edit_menu.Insert(10, wx.ID_PREFERENCES)
            self.frame.Bind(wx.EVT_MENU, self._on_edit_prefs, self.edit_prefs)
            menubar.Append(edit_menu, "&Edit"); # ToDo use wx.ID_EDIT stuff

        # setup the toolbar
        if True:
            self.toolbar = wx.ToolBar(frame, -1, style = wx.TB_DOCKABLE | wx.TB_HORIZONTAL)
            frame.SetToolBar(self.toolbar)
            icon_size = wx.Size(24, 24)
            new_icon = wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, icon_size)
            toolbar_new = self.toolbar.AddSimpleTool(wx.ID_NEW, new_icon, "New Capture")
            open_icon = wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN, wx.ART_TOOLBAR, icon_size)
            toolbar_open = self.toolbar.AddSimpleTool(wx.ID_OPEN, open_icon, "Open")
            #
            # self.toolbar.AddSeparator()
            # self.gain_control = wx.Slider(self.toolbar, 100, 50, 1, 100, style=wx.SL_HORIZONTAL)
            # slider.SetTickFreq(5, 1)
            # self.toolbar.AddControl(self.gain_control)
            #
            self.toolbar.Realize()
        else:
            self.toolbar = None

        # setup the notebook
        self.notebook = wx.Notebook(self.panel)
        self.vbox.Add(self.notebook, 1, wx.EXPAND)     
        # add spectrum scope
        self.spectrum = fftsink2.fft_sink_c(self.notebook, fft_size=512, average=True, peak_hold=True)
        self.spectrum_plotter = self.spectrum.win.plot
##        self.spectrum_plotter = self.spectrum.win.plotter # opengl
        self.spectrum_plotter.Bind(wx.EVT_LEFT_DOWN, self._on_spectrum_left_click)
        self.notebook.AddPage(self.spectrum.win, "RF Spectrum")
        # add C4FM scope
        self.signal_scope = scopesink2.scope_sink_f(self.notebook, sample_rate = self.channel_rate, v_scale=5, t_scale=0.001)
        self.signal_plotter = self.signal_scope.win.graph
##        self.signal_plotter = self.signal_scope.win
        self.notebook.AddPage(self.signal_scope.win, "C4FM Signal")
        # add symbol scope
        self.symbol_scope = scopesink2.scope_sink_f(self.notebook, frame_decim=1, sample_rate=self.symbol_rate, v_scale=1, t_scale=0.05)
        self.symbol_plotter = self.symbol_scope.win.graph
##        self.symbol_plotter = self.symbol_scope.win
        self.symbol_scope.win.set_format_plus()
        self.notebook.AddPage(self.symbol_scope.win, "Demodulated Symbols")
        # Traffic snapshot
        self.traffic = TrafficPane(self.notebook)
        self.notebook.AddPage(self.traffic, "Traffic")
        # Setup the decoder and report the TUN/TAP device name
        self.p25_decoder = op25.decoder_bf()
        self.decode_watcher = decode_watcher(msgq, self.traffic)
        self.p25_decoder.set_msgq(gr.msg_queue(2))
        self.frame.SetStatusText("Destination: " + self.p25_decoder.destination())

    # read capture file properties (decimation etc.)
    #
    def __read_file_properties(self, filename):
        f = open(filename, "r")
        self.info = pickle.load(f)
        f.close()

    # Setup to rx from file
    #
    def __set_rx_from_file(self, filename, capture_rate):
        file = gr.file_source(gr.sizeof_gr_complex, filename, True)
        throttle = gr.throttle(gr.sizeof_gr_complex, capture_rate)
        self.__connect([[file, throttle]])
        self.__build_graph(throttle, capture_rate)

    # setup to rx from USRP
    #
    def __set_rx_from_usrp(self, subdev_spec, decimation_rate, gain, frequency, preserve):
        from gnuradio import usrp
        # setup USRP
        self.usrp.set_decim_rate(decimation_rate)
        if subdev_spec is None:
            subdev_spec = usrp.pick_rx_subdevice(self.usrp)
        self.usrp.set_mux(usrp.determine_rx_mux_value(self.usrp, subdev_spec))
        subdev = usrp.selected_subdev(self.usrp, subdev_spec)
        capture_rate = self.usrp.adc_freq() / self.usrp.decim_rate()
        self.info["capture-rate"] = capture_rate
        if gain is None:
            g = subdev.gain_range()
            gain = float(g[0]+g[1])/2
        subdev.set_gain(gain)
        r = self.usrp.tune(0, subdev, frequency)
        if not r:
            raise RuntimeError("failed to set USRP frequency")
        # capture file
        if preserve:
            try:
                self.capture_filename = os.tmpnam()
            except RuntimeWarning:
                ignore = True
            capture_file = gr.file_sink(gr.sizeof_gr_complex, self.capture_filename)
            self.__connect([[self.usrp, capture_file]])
        else:
            self.capture_filename = None
        # everything else
        self.__build_graph(self.usrp, capture_rate)

    # Change the UI state
    #
    def _set_state(self, new_state):
        self.state = new_state
        if "STOPPED" == self.state:
            # menu items
            can_capture = self.usrp is not None
            self.file_new.Enable(can_capture)
            self.file_open.Enable(True)
            self.file_properties.Enable(False)
            self.file_close.Enable(False)
            # toolbar
            if self.toolbar:
                self.toolbar.EnableTool(wx.ID_NEW, can_capture)
                self.toolbar.EnableTool(wx.ID_OPEN, True)
            # Visually reflect "no file"
            self.frame.SetStatusText("", 1)
            self.frame.SetStatusText("", 2)
            self.spectrum_plotter.Clear()
            self.signal_plotter.Clear()
            self.symbol_plotter.Clear()
            self.traffic.clear()
        elif "RUNNING" == self.state:
            # menu items
            self.file_new.Enable(False)
            self.file_open.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)
            # toolbar
            if self.toolbar:
                self.toolbar.EnableTool(wx.ID_NEW, False)
                self.toolbar.EnableTool(wx.ID_OPEN, False)
        elif "CAPTURING" == self.state:
            # menu items
            self.file_new.Enable(False)
            self.file_open.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)
            # toolbar
            if self.toolbar:
                self.toolbar.EnableTool(wx.ID_NEW, False)
                self.toolbar.EnableTool(wx.ID_OPEN, False)


    # Append filename to standard title bar
    #
    def _set_titlebar(self, filename):
        ToDo = True

    # Write capture file properties
    #
    def __write_file_properties(self, filename):
        f = open(filename, "w")
        pickle.dump(self.info, f)
        f.close()

    # Adjust the channel offset
    #
    def adjust_channel_offset(self, delta_hz):
        max_delta_hz = 12000.0
        delta_hz *= self.symbol_deviation      
        delta_hz = max(delta_hz, -max_delta_hz)
        delta_hz = min(delta_hz, max_delta_hz)
        self.channel_filter.set_center_freq(self.channel_offset - delta_hz)

    # Close an open file
    #
    def _on_file_close(self, event):
        self.stop()
        self.wait()
        self.__disconnect()
        if "CAPTURING" == self.state and self.capture_filename:
            dialog = wx.MessageDialog(self.frame, "Save capture file before closing?", style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if wx.ID_YES == dialog.ShowModal():
                save_dialog = wx.FileDialog(self.frame, "Save capture file as:", wildcard="*.dat", style=wx.SAVE|wx.OVERWRITE_PROMPT)
                if save_dialog.ShowModal() == wx.ID_OK:
                    path = str(save_dialog.GetPath())
                    save_dialog.Destroy()
                    os.rename(self.capture_filename, path)
                    self.__write_file_properties(path + ".info")
            else:
                os.remove(self.capture_filename)
        self.capture_filename = None
        self._set_state("STOPPED")

    # New capture from USRP 
    #
    def _on_file_new(self, event):
#         wizard = wx.wizard.Wizard(self.frame, -1, "New Capture from USRP")
#         page1 = wizard_intro_page(wizard)
#         page2 = wizard_details_page(wizard)
#         page3 = wizard_preserve_page(wizard)
#         page4 = wizard_finish_page(wizard)
#         wx.wizard.WizardPageSimple_Chain(page1, page2)
#         wx.wizard.WizardPageSimple_Chain(page2, page3)
#         wx.wizard.WizardPageSimple_Chain(page3, page4)
#         wizard.FitToPage(page1)
#         if wizard.RunWizard(page1):
        self.stop()
        self.wait()
        # ToDo: get open_usrp() arguments from wizard
        self.open_usrp(self.options.rx_subdev_spec, self.options.decim, self.options.gain, self.options.frequency, not self.options.transient)
        self.start()

    # Open an existing capture
    #
    def _on_file_open(self, event):
        dialog = wx.FileDialog(self.frame, "Choose a capture file:", wildcard="*.dat", style=wx.OPEN)
        if dialog.ShowModal() == wx.ID_OK:
            file = str(dialog.GetPath())
            dialog.Destroy()
            self.stop()
            self.wait()
            self.open_file(file)
            self.start()

    # Present file properties dialog
    #
    def _on_file_properties(self, event):
        # ToDo: show what info we have about the capture file (name,
        # capture source, capture rate, date(?), size(?),)
        todo = True

    # Undo the last edit
    #
    def _on_edit_undo(self, event):
        todo = True

    # Redo the edit
    #
    def _on_edit_redo(self, event):
        todo = True

    # Cut the current selection
    #
    def _on_edit_cut(self, event):
        todo = True

    # Copy the current selection
    #
    def _on_edit_copy(self, event):
        todo = True

    # Paste into the current sample
    #
    def _on_edit_paste(self, event):
        todo = True

    # Delete the current selection
    #
    def _on_edit_delete(self, event):
        todo = True

    # Select all
    #
    def _on_edit_select_all(self, event):
        todo = True

    # Open the preferences dialog
    #
    def _on_edit_prefs(self, event):
        todo = True

    # Set channel offset and RF squelch threshold
    #
    def _on_spectrum_left_click(self, event):
        if "STOPPED" != self.state:
            # set frequency
            x,y = self.spectrum_plotter.GetXY(event)
            xmin, xmax = self.spectrum_plotter.GetXCurrentRange()
            x = min(x, xmax)
            x = max(x, xmin)
            scale_factor = self.spectrum.win._scale_factor
            chan_width = 6.25e3
            x /= scale_factor
            x += chan_width / 2
            x  = (x // chan_width) * chan_width
            self.set_channel_offset(x, scale_factor, self.spectrum.win._units)
            # set squelch threshold
            ymin, ymax = self.spectrum_plotter.GetYCurrentRange()
            y = min(y, ymax)
            y = max(y, ymin)
            squelch_increment = 5
            y += squelch_increment / 2
            y = (y // squelch_increment) * squelch_increment
            self.set_squelch_threshold(int(y))

    # Open an existing capture file
    #
    def open_file(self, capture_file):
        try:
            self.__read_file_properties(capture_file + ".info")
            capture_rate = self.info["capture-rate"]
            self.__set_rx_from_file(capture_file, capture_rate)
            self._set_titlebar(capture_file)
            self._set_state("RUNNING")
        except Exception, x:
            wx.MessageBox("Cannot open capture file: " + str(x), "File Open Error", wx.CANCEL | wx.ICON_EXCLAMATION)

    # Open the USRP
    #
    def open_usrp(self, subdev_spec, decim, gain, frequency, preserve):
        try:
            self.info = {
                "capture-rate": "unknown",
                "center-freq": frequency,
                "source-dev": "USRP",
                "source-decim": decim }
            self.__set_rx_from_usrp(subdev_spec, decim, gain, frequency, preserve)
            self._set_titlebar("Capturing")
            self._set_state("CAPTURING")
        except Exception, x:
            wx.MessageBox("Cannot open USRP: " + x.message, "USRP Error", wx.CANCEL | wx.ICON_EXCLAMATION)

    # Set the channel offset
    #
    def set_channel_offset(self, offset_hz, scale, units):
        self.channel_offset = -offset_hz
        self.channel_filter.set_center_freq(self.channel_offset)
        self.frame.SetStatusText("Channel offset: " + str(offset_hz * scale) + units, 1)

    # Set the RF squelch threshold level
    #
    def set_squelch_threshold(self, squelch_db):
        self.squelch.set_threshold(squelch_db)
        self.frame.SetStatusText("Squelch: " + str(squelch_db) + "dB", 2)


# A snapshot of important fields in current traffic
#
class TrafficPane(wx.Panel):

    # Initializer
    #
    def __init__(self, parent):

        wx.Panel.__init__(self, parent)
        sizer = wx.GridBagSizer(hgap=10, vgap=10)
        self.fields = {}

        label = wx.StaticText(self, -1, "DUID:")
        sizer.Add(label, pos=(1,1))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(1,2))
        self.fields["duid"] = field;

        label = wx.StaticText(self, -1, "NAC:")
        sizer.Add(label, pos=(2,1))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(2,2))
        self.fields["nac"] = field;

        label = wx.StaticText(self, -1, "Source:")
        sizer.Add(label, pos=(3,1))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(3,2))
        self.fields["source"] = field;

        label = wx.StaticText(self, -1, "Destination:")
        sizer.Add(label, pos=(4,1))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(4,2))
        self.fields["dest"] = field;

#        label = wx.StaticText(self, -1, "ToDo:")
#        sizer.Add(label, pos=(5,1))
#        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
#        sizer.Add(field, pos=(5,2))
#        self.fields["nid"] = field;

        label = wx.StaticText(self, -1, "MFID:")
        sizer.Add(label, pos=(1,4))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(1,5))
        self.fields["mfid"] = field;

        label = wx.StaticText(self, -1, "ALGID:")
        sizer.Add(label, pos=(2,4))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(2,5))
        self.fields["algid"] = field;

        label = wx.StaticText(self, -1, "KID:")
        sizer.Add(label, pos=(3,4))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(3,5))
        self.fields["kid"] = field;

        label = wx.StaticText(self, -1, "MI:")
        sizer.Add(label, pos=(4,4))
        field = wx.TextCtrl(self, -1, "", size=(216, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(4,5))
        self.fields["mi"] = field;

        label = wx.StaticText(self, -1, "TGID:")
        sizer.Add(label, pos=(5,4))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(5,5))
        self.fields["tgid"] = field;

        self.SetSizer(sizer)
        self.Fit()

    # Clear the field values
    #
    def clear(self):
        for v in self.fields.values():
            v.Clear()

    # Update the field values
    #
    def update(self, field_values):
        if field_values['duid'] == 'hdu':
            self.clear()
        for k,v in self.fields.items():
            f = field_values.get(k, None)
            if f:
                v.SetValue(f)


# Introduction page for USRP capture wizard
#
class wizard_intro_page(wx.wizard.WizardPageSimple):

    # Initializer
    #
    def __init__(self, parent):
        wx.wizard.WizardPageSimple.__init__(self, parent)
        html = wx.html.HtmlWindow(self)
        html.SetPage('''
	<html>
	 <body>
         <h1>Capture from USRP</h1>
	 <p>
	  We will guide you through the process of capturing a sample from the USRP.
	  Please ensure that the USRP is switched on and connected to this computer.
	 </p>
	 </body>
	</html>
	''')
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(sizer)
        sizer.Add(html, 1, wx.ALIGN_CENTER | wx.EXPAND | wx.FIXED_MINSIZE)


# USRP wizard details page
#
class wizard_details_page(wx.wizard.WizardPageSimple):

    # Initializer
    #
    def __init__(self, parent):
        wx.wizard.WizardPageSimple.__init__(self, parent)
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(sizer)

    # Return a tuple containing the subdev_spec, gain, frequency, decimation factor
    #
    def get_details(self):
        ToDo = True


# Frequency tracker
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


# Decoder watcher
#
class decode_watcher(threading.Thread):

    def __init__(self, msgq, traffic_pane, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon(1)
        self.msgq = msgq
        self.traffic_pane = traffic_pane
        self.keep_running = True
        self.start()

    def run(self):
        while(self.keep_running):
            msg = self.msgq.delete_head()
            pickled_dict = msg.to_string()
            attrs = pickle.loads(pickled_dict)
            self.traffic_pane.update(attrs)


# Start the receiver
#
if '__main__' == __name__:
    app = stdgui2.stdapp(p25_rx_block, "APCO P25 Receiver", 3)
    app.MainLoop()
