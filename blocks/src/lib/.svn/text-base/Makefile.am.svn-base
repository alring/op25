#
# Copyright 2004,2005,2006,2008 Free Software Foundation, Inc.
# 
# This file is part of OP25
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
# 

include $(top_srcdir)/Makefile.common

# Install this stuff so that it ends up as the op25.decoder_ff
# module. This usually ends up at:
# ${prefix}/lib/python${python_version}/site-packages/gnuradio

ourpythondir = $(grpythondir)
ourlibdir    = $(grpyexecdir)

AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES) $(PYTHON_CPPFLAGS) $(WITH_INCLUDES)

SWIGPYTHONARGS = $(SWIGPYTHONFLAGS) $(SWIGGRFLAGS) $(WITH_SWIG_INCLUDES) \
	$(WITH_INCLUDES)

ALL_IFILES = \
	$(LOCAL_IFILES) \
	$(NON_LOCAL_IFILES)		

NON_LOCAL_IFILES = \
	$(GNURADIO_CORE_INCLUDEDIR)/swig/gnuradio.i

LOCAL_IFILES = \
	$(top_srcdir)/src/lib/op25.i				

# These files are built by SWIG.  The first is the C++ glue.
# The second is the python wrapper that loads the _op25 shared library
# and knows how to call our extensions.

BUILT_SOURCES = \
	op25.cc \
	op25.py				

# This gets op25.py installed in the right place
ourpython_PYTHON = \
	op25.py

ourlib_LTLIBRARIES = _op25.la

# These are the source files that go into the shared library
_op25_la_SOURCES = \
	abstract_data_unit.cc \
	data_unit.cc \
	hdu.cc \
	ldu1.cc \
	ldu2.cc \
	pdu.cc \
	tdu.cc \
	tsbk.cc \
	data_unit_handler.cc \
	logfile_du_handler.cc \
	p25cai_du_handler.cc \
	snapshot_du_handler.cc \
	imbe_decoder.cc \
	imbe_decoder_factory.cc \
	dummy_imbe_decoder.cc \
	offline_imbe_decoder.cc \
	voice_data_unit.cc \
	voice_du_handler.cc \
	op25.cc \
	op25_decoder_bf.cc \
	op25_fsk4_demod_ff.cc \
	op25_fsk4_slicer_fb.cc \
	op25_pcap_source_b.cc \
	software_imbe_decoder.cc \
	vc55_imbe_decoder.cc \
	value_string.cc \
	pickle.cc

# magic flags
_op25_la_LDFLAGS = $(NO_UNDEFINED) -module -avoid-version

# link the library against some comon swig runtime code and the 
# c++ standard library
_op25_la_LIBADD = \
	$(PYTHON_LDFLAGS) \
	-lstdc++ \
	-litpp

op25.cc op25.py: $(LOCAL_IFILES) $(ALL_IFILES)
	$(SWIG) $(SWIGPYTHONARGS) -module op25 -o op25.cc $(LOCAL_IFILES)

# These headers get installed in ${prefix}/include/gnuradio
grinclude_HEADERS = \
	op25_decoder_ff.h		

# These swig headers get installed in ${prefix}/include/gnuradio/swig
swiginclude_HEADERS = \
	$(LOCAL_IFILES)

MOSTLYCLEANFILES = $(BUILT_SOURCES) *.pyc

# Don't distribute output of swig
dist-hook:
	@for file in $(BUILT_SOURCES); do echo $(RM) $(distdir)/$$file; done
	@for file in $(BUILT_SOURCES); do $(RM) $(distdir)/$$file; done
