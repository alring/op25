/* -*- c++ -*- */
#define OP25_API

%include "gnuradio.i"                   // the common stuff

//load generated python docstrings
%include "op25_swig_doc.i"

%{
#include "op25_fsk4_demod_ff.h"
#include "op25_fsk4_slicer_fb.h"
#include "op25_decoder_bf.h"
%}


%include "op25_fsk4_demod_ff.h"
GR_SWIG_BLOCK_MAGIC(op25, fsk4_demod_ff);
%include "op25_fsk4_slicer_fb.h"
GR_SWIG_BLOCK_MAGIC(op25, fsk4_slicer_fb);
%include "op25_decoder_bf.h"
GR_SWIG_BLOCK_MAGIC(op25, decoder_bf);
