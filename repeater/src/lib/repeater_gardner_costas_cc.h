/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 *
 * Gardner symbol recovery block for GR - Copyright 2010, KA1RBI
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_REPEATER_GARDNER_SYMBOL_RECOVERY_CC_H
#define	INCLUDED_REPEATER_GARDNER_SYMBOL_RECOVERY_CC_H

#include <gr_block.h>
#include <gr_complex.h>
#include <gr_math.h>

class gri_mmse_fir_interpolator_cc;

class repeater_gardner_costas_cc;
typedef boost::shared_ptr<repeater_gardner_costas_cc> repeater_gardner_costas_cc_sptr;

// public constructor
repeater_gardner_costas_cc_sptr 
repeater_make_gardner_costas_cc (float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq);

/*!
 * \brief Gardner based repeater gardner_costas block with complex input, complex output.
 * \ingroup sync_blk
 *
 * This implements a Gardner discrete-time error-tracking synchronizer.
 *
 * input samples should be within normalized range of -1.0 thru +1.0
 *
 * includes some simplifying approximations KA1RBI
 */
class repeater_gardner_costas_cc : public gr_block
{
 public:
  ~repeater_gardner_costas_cc ();
  void forecast(int noutput_items, gr_vector_int &ninput_items_required);
  int general_work (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
  void set_verbose (bool verbose) { d_verbose = verbose; }

  //! Sets value of omega and its min and max values 
  void set_omega (float omega);

protected:
  bool input_sample0(gr_complex, gr_complex& outp);
  bool input_sample(gr_complex, gr_complex& outp);
  repeater_gardner_costas_cc (float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq);

 private:

  float				d_mu;
  float d_omega, d_gain_omega, d_omega_rel, d_max_omega, d_min_omega, d_omega_mid;
  float				d_gain_mu;

  gr_complex                    d_last_sample;
  gri_mmse_fir_interpolator_cc 	*d_interp;
  bool			        d_verbose;

  gr_complex			*d_dl;
  int				d_dl_index;

  int				d_twice_sps;

  float				d_timing_error;

  float				d_alpha;
  float				d_beta;
  uint32_t			d_interp_counter;

  float				d_theta;
  float				d_phase;
  float				d_freq;
  float				d_max_freq;

  friend repeater_gardner_costas_cc_sptr
  repeater_make_gardner_costas_cc (float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq);

  float phase_error_detector_qpsk(gr_complex sample);
  void phase_error_tracking(gr_complex sample);

};

#endif
