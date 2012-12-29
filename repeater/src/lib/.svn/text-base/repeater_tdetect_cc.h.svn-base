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

#ifndef INCLUDED_REPEATER_TDETECT_CC_H
#define	INCLUDED_REPEATER_TDETECT_CC_H

#include <gr_block.h>
#include <gr_complex.h>
#include <gr_math.h>
#include <cic_filter.h>

class gri_mmse_fir_interpolator_cc;

class repeater_tdetect_cc;
typedef boost::shared_ptr<repeater_tdetect_cc> repeater_tdetect_cc_sptr;

// public constructor
repeater_tdetect_cc_sptr 
repeater_make_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length);

/*!
 * \brief symbol timing loop using tone detection - complex input, complex output.
 * \ingroup sync_blk
 *
 * input samples should be within normalized amplitude range of 0 thru +1.0
 *
 * KA1RBI
 */
class repeater_tdetect_cc : public gr_block
{
 public:
  ~repeater_tdetect_cc ();
  void forecast(int noutput_items, gr_vector_int &ninput_items_required);
  int general_work (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);

protected:
  repeater_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length);

private:

  int				d_samples_per_symbol;
  int				d_half_sps;
  float				d_step_size;
  int				d_theta;
  int				d_cic_length;

  integrator			d_integrator;
  comb				d_comb;
  delay				input_delay;
  int				d_l2ctr;
  int				d_d2ctr;

  int				d_delta;
  float				d_delta_c;
  int				d_previous_phase_offset;

  friend repeater_tdetect_cc_sptr
  repeater_make_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length);

  float phase_error_detector_qpsk(gr_complex sample);
  void phase_error_tracking(gr_complex sample);

};

#endif
