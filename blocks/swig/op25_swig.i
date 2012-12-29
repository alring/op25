/* -*- c++ -*- */
#define OP25_API

%include "gnuradio.i"                   // the common stuff

//load generated python docstrings
%include "op25_swig_doc.i"

%{
#include "op25_decoder_ff.h"
%}


%include "op25_decoder_ff.h"
GR_SWIG_BLOCK_MAGIC(op25, decoder_ff);
