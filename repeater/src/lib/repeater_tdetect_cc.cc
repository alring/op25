/* -*- c++ -*- */
/*
 * Copyright 2005,2006 Free Software Foundation, Inc.
 *
 * Tone detect symbol recovery block for GR - Copyright 2012, KA1RBI
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <gr_prefs.h>
#include <gr_math.h>
#include <gr_expj.h>
#include <repeater_tdetect_cc.h>
#include <gri_mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <string.h>

#include "cic_filter.h"

#define VERBOSE_TDETECT 0    // Used for debugging symbol timing loop

// Public constructor

repeater_tdetect_cc_sptr 
repeater_make_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length)
{
  return repeater_tdetect_cc_sptr (new repeater_tdetect_cc (samples_per_symbol, step_size, theta, cic_length));
}

repeater_tdetect_cc::repeater_tdetect_cc (int samples_per_symbol, float step_size, int theta, int cic_length)
  : gr_block ("repeater_tdetect_cc",
	      gr_make_io_signature (1, 1, sizeof (gr_complex)),
	      gr_make_io_signature (1, 1, sizeof (gr_complex))),
    d_samples_per_symbol(samples_per_symbol),
    d_half_sps(samples_per_symbol >> 1),
    d_step_size(step_size),
    d_theta(theta),
    d_cic_length(cic_length),
    d_integrator(), d_comb(cic_length), input_delay(samples_per_symbol),
    d_l2ctr(0),
    d_delta(0),
    d_delta_c(0),
    d_previous_phase_offset(0)
{
  assert((samples_per_symbol & 1) == 0);	// sps must be even
  set_relative_rate (1.0 / (float) samples_per_symbol);
  set_history(samples_per_symbol * 2);			// ensure extra input is available
}

repeater_tdetect_cc::~repeater_tdetect_cc ()
{
}

void
repeater_tdetect_cc::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
  unsigned ninputs = ninput_items_required.size();
  for (unsigned i=0; i < ninputs; i++)
    ninput_items_required[i] =
      (int) ceil((noutput_items * d_samples_per_symbol));
}

/*
 * Tone detect symbol recovery block for GR - Copyright 2012, KA1RBI
 *
 * symbol timing synchronization using tone detection
 *
 * CQPSK signals when AM-demodulated contain a strong tone at 4,800 Hz.
 * This tone is filtered (using a CIC to remove the DC offset at zero Hz).,
 * amplified, and decimated.  The resulting error signal is applied to steer
 * the symbol sampling point toward the optimum phase.
 *
 * NOTE: input samples should be normalized (AGC) such that the range of
 *       signal magnitudes is in the standard zone (0 through +1.0).
 *
 *
 * Source: Software Radios (Second Ed.) B. Farhang-Boroujeny, Sec. 10.2.3
 *
 */

int
repeater_tdetect_cc::general_work (int noutput_items,
				   gr_vector_int &ninput_items,
				   gr_vector_const_void_star &input_items,
				   gr_vector_void_star &output_items)
{
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];

  int i=0, o=0;
  gr_complex sample;

  while((o < noutput_items) && (i < ninput_items[0])) {
    sample = in[ i++ ];
    sample = input_delay.cycle(sample, d_delta);
    if (++d_l2ctr < d_half_sps)
      continue;	// decimate by sps/2
    d_l2ctr = 0;
    int64_t s = (int64_t) (262143.0 * (pow(sample.real(), 2.0) + pow(sample.imag(), 2.0))); /* mag sq */
    s = d_comb.cycle(s, d_cic_length);
    s = d_integrator.cycle(s);
    if (++d_d2ctr & 1)
      continue;	// decimate by 2
    float symbol_error = d_step_size * (float)s;

    // now adjust delta_continuous by the amount of the symbol timing error
    d_delta_c += symbol_error;
    while (d_delta_c > d_samples_per_symbol)
      d_delta_c -= d_samples_per_symbol;
    while (d_delta_c < 0)
      d_delta_c += d_samples_per_symbol;
    d_delta = (int) rint(d_delta_c);	// quantize to nearest int

    // d_theta sets optimum sampling point phase offset (delay),
    // in one-sample units
    int phase_offset = d_delta + d_theta;
    while (phase_offset > d_samples_per_symbol)
      phase_offset -= d_samples_per_symbol;
    while (phase_offset < 0)
      phase_offset += d_samples_per_symbol;

    // handle frequency mismatch between local clock and extracted clock
    // when mismatch reaches a full cycle we must either insert one "extra"
    // symbol or skip one symbol (depending on algebraic sign of mismatch)
    int dd = phase_offset - d_previous_phase_offset;
    int skip_store = 0;
    if (abs(dd) >= d_half_sps) {
      if (dd < 0 && o < noutput_items-1) {
        sample = input_delay.get(d_previous_phase_offset);
        out[o++] = sample;
      }
      if (dd > 0) {
        skip_store = 1;
      }
    }
    d_previous_phase_offset = phase_offset;

    if (!skip_store) {
      sample = input_delay.get(phase_offset);
      out[o++] = sample;
    }
  }

  consume_each(i);
  return o;
}
