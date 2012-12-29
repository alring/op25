/* -*- c++ -*- */

%feature("autodoc", "1");		// generate python docstrings

%{
#include <stddef.h>
%}

%include "exception.i"
%import "gnuradio.i"			// the common stuff

%{
#include "gnuradio_swig_bug_workaround.h"	// mandatory bug fix
#include "repeater_squelch_base_ff.h"
#include "repeater_fsk4_slicer_fb.h"
#include "repeater_s2v.h"
#include "repeater_p25_frame_assembler.h"
#include "repeater_pipe.h"
#include "repeater_ctcss_squelch_ff.h"
#include "repeater_gardner_costas_cc.h"
#include "repeater_tdetect_cc.h"
#include "cic_filter.h"
#include "repeater_vocoder.h"
#include "repeater_chan_usrp.h"
#include "repeater_chan_usrp_rx.h"
#include "rs.h"
#include <stdexcept>

%}

// ----------------------------------------------------------------

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can
 * access repeater_fsk4_slicer_fb from python as repeater.fsk4_slicer_fb
 */

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,fsk4_slicer_fb);

repeater_fsk4_slicer_fb_sptr repeater_make_fsk4_slicer_fb (const std::vector<float> &slice_levels);

class repeater_fsk4_slicer_fb : public gr_sync_block
{
private:
  repeater_fsk4_slicer_fb (const std::vector<float> &slice_levels);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,p25_frame_assembler);

repeater_p25_frame_assembler_sptr repeater_make_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue);

class repeater_p25_frame_assembler : public gr_sync_block
{
private:
  repeater_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,pipe);

repeater_pipe_sptr repeater_make_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);

class repeater_pipe : public gr_sync_block
{
private:
  repeater_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,squelch_base_ff);

class repeater_squelch_base_ff : public gr_block
{
private:
  enum { ST_MUTED, ST_ATTACK, ST_UNMUTED, ST_DECAY } d_state;

public:
  repeater_squelch_base_ff(const char *name, int ramp, bool gate);

  int ramp() const { return d_ramp; }
  void set_ramp(int ramp) { d_ramp = ramp; }
  bool gate() const { return d_gate; }
  void set_gate(bool gate) { d_gate = gate; }
  bool unmuted() const { return (d_state == ST_UNMUTED || d_state == ST_ATTACK); }

  virtual std::vector<float> squelch_range() const = 0;
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,ctcss_squelch_ff);

repeater_ctcss_squelch_ff_sptr repeater_make_ctcss_squelch_ff (int rate, float freq, float level, int len, int ramp, bool gate);

class repeater_ctcss_squelch_ff : public repeater_squelch_base_ff
{
private:
  repeater_ctcss_squelch_ff (int rate, float freq, float level, int len, int ramp, bool gate);

public:
  std::vector<float> squelch_range() const;
  float level() const { return d_level; }
  void set_level(float level) { d_level = level; }
  int len() const { return d_len; }

};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,gardner_costas_cc);

repeater_gardner_costas_cc_sptr repeater_make_gardner_costas_cc (float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq);

class repeater_gardner_costas_cc : public gr_sync_block
{
 private:
  repeater_gardner_costas_cc (float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,tdetect_cc);

repeater_tdetect_cc_sptr repeater_make_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length);

class repeater_tdetect_cc : public gr_sync_block
{
 private:
  repeater_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,vocoder);

repeater_vocoder_sptr repeater_make_vocoder (bool encode_flag, bool verbose_flag, int stretch_amt, char* udp_host, int udp_port, bool raw_vectors_flag);

class repeater_vocoder : public gr_block
{
private:
  repeater_vocoder (bool encode_flag, bool verbose_flag, int stretch_amt, char* udp_host, int udp_port, bool raw_vectors_flag);
};

// ----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(repeater,chan_usrp);
repeater_chan_usrp_sptr repeater_make_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue);

class repeater_chan_usrp : public gr_block
{
private:

  repeater_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue);  	// private constructor
};

// ----------------------------------------------------------------
GR_SWIG_BLOCK_MAGIC(repeater,chan_usrp_rx);
repeater_chan_usrp_rx_sptr repeater_make_chan_usrp_rx (const char* udp_host, int port, int debug);

class repeater_chan_usrp_rx : public gr_block
{
private:

  repeater_chan_usrp_rx (const char* udp_host, int port, int debug);  	// private constructor

public:
  void unkey(void);
};

GR_SWIG_BLOCK_MAGIC(repeater,s2v);

repeater_s2v_sptr repeater_make_s2v (size_t item_size, size_t nitems_per_block);

class repeater_s2v : public gr_block
{
private:
  repeater_s2v (size_t item_size, size_t nitems_per_block);
};

