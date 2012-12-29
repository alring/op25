/* -*- c++ -*- */
/*
 * Copyright 2005,2006 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <gr_prefs.h>
#include <gr_math.h>
#include <gr_expj.h>
#include <repeater_gardner_costas_cc.h>
#include <gri_mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <string.h>

#define ENABLE_COSTAS_CQPSK_HACK 1

static const float M_TWOPI = 2 * M_PI;
#define VERBOSE_GARDNER 0    // Used for debugging symbol timing loop
#define VERBOSE_COSTAS 0     // Used for debugging phase and frequency tracking
static const gr_complex PT_45 = gr_expj( M_PI / 4.0 );

// Public constructor

repeater_gardner_costas_cc_sptr 
repeater_make_gardner_costas_cc(float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq)
{
  return repeater_gardner_costas_cc_sptr (new repeater_gardner_costas_cc (samples_per_symbol, gain_mu, gain_omega, alpha, beta, max_freq, min_freq));
}

repeater_gardner_costas_cc::repeater_gardner_costas_cc (float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq)
  : gr_block ("repeater_gardner_costas_cc",
	      gr_make_io_signature (1, 1, sizeof (gr_complex)),
	      gr_make_io_signature (1, 1, sizeof (gr_complex))),
    d_mu(0),
    d_gain_omega(gain_omega),
    d_gain_mu(gain_mu),
    d_last_sample(0), d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("repeater_gardner_costas_cc", "verbose", false)),
    d_dl(0),
    d_dl_index(0),
    d_alpha(alpha), d_beta(beta), 
    d_interp_counter(0),
    d_theta(M_PI / 4.0), d_phase(0), d_freq(0), d_max_freq(max_freq)
{
  set_omega(samples_per_symbol);
  set_relative_rate (1.0 / d_omega);
  set_history(d_twice_sps);			// ensure extra input is available
}

void repeater_gardner_costas_cc::set_omega (float omega) {
    if (d_dl) {
	delete d_dl;
	d_dl = 0;
    }
    assert (omega >= 2.0);
    d_omega = omega;
    d_min_omega = omega*(1.0 - d_omega_rel);
    d_max_omega = omega*(1.0 + d_omega_rel);
    d_omega_mid = 0.5*(d_min_omega+d_max_omega);
    d_twice_sps = 2 * (int) ceilf(d_omega);
    int num_complex = std::max(d_twice_sps*2, 16);
    d_dl = new gr_complex[num_complex];
    memset(d_dl, 0, num_complex * sizeof(gr_complex));
}

repeater_gardner_costas_cc::~repeater_gardner_costas_cc ()
{
  delete d_interp;
    if (d_dl) {
	delete d_dl;
	d_dl = 0;
    }
}

void
repeater_gardner_costas_cc::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
  unsigned ninputs = ninput_items_required.size();
  for (unsigned i=0; i < ninputs; i++)
    ninput_items_required[i] =
      (int) ceil((noutput_items * d_omega) + d_interp->ntaps());
}

float   // for QPSK
repeater_gardner_costas_cc::phase_error_detector_qpsk(gr_complex sample)
{
  float phase_error = 0;
  if(fabsf(sample.real()) > fabsf(sample.imag())) {
    if(sample.real() > 0)
      phase_error = -sample.imag();
    else
      phase_error = sample.imag();
  }
  else {
    if(sample.imag() > 0)
      phase_error = sample.real();
    else
      phase_error = -sample.real();
  }
  
  return phase_error;
}

void
repeater_gardner_costas_cc::phase_error_tracking(gr_complex sample)
{
  float phase_error = 0;
#if ENABLE_COSTAS_CQPSK_HACK
  if (d_interp_counter & 1)    // every other symbol
    sample = sample * PT_45;    // rotate by +45 deg
  d_interp_counter++;
#endif /* ENABLE_COSTAS_CQPSK_HACK */

  // Make phase and frequency corrections based on sampled value
  phase_error = phase_error_detector_qpsk(sample);
    
  d_freq += d_beta*phase_error;             // adjust frequency based on error
  d_phase += d_freq + d_alpha*phase_error;  // adjust phase based on error

  // Make sure we stay within +-2pi
  while(d_phase > M_TWOPI)
    d_phase -= M_TWOPI;
  while(d_phase < -M_TWOPI)
    d_phase += M_TWOPI;
  
  // Limit the frequency range
  d_freq = gr_branchless_clip(d_freq, d_max_freq);
  
#if VERBOSE_COSTAS
  printf("cl: phase_error: %f  phase: %f  freq: %f  sample: %f+j%f  constellation: %f+j%f\n",
	 phase_error, d_phase, d_freq, sample.real(), sample.imag(), 
	 d_constellation[d_current_const_point].real(), d_constellation[d_current_const_point].imag());
#endif
}

int
repeater_gardner_costas_cc::general_work (int noutput_items,
				   gr_vector_int &ninput_items,
				   gr_vector_const_void_star &input_items,
				   gr_vector_void_star &output_items)
{
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];

  int i=0, o=0;
  gr_complex symbol, sample, nco;

  while((o < noutput_items) && (i < ninput_items[0])) {
    while((d_mu > 1.0) && (i < ninput_items[0]))  {
	d_mu --;

        d_phase += d_freq;
  // Keep phase clamped and not walk to infinity
  while(d_phase > M_TWOPI)
    d_phase -= M_TWOPI;
  while(d_phase < -M_TWOPI)
    d_phase += M_TWOPI;
  
        nco = gr_expj(d_phase+d_theta);   // get the NCO value for derotating the curr
        symbol = in[i];
        sample = nco*symbol;      // get the downconverted symbol

	d_dl[d_dl_index] = sample;
	d_dl[d_dl_index + d_twice_sps] = sample;
	d_dl_index ++;
	d_dl_index = d_dl_index % d_twice_sps;

	i++;
    }
    
    if(i < ninput_items[0]) {
		float half_omega = d_omega / 2.0;
		int half_sps = (int) floorf(half_omega);
		float half_mu = d_mu + half_omega - (float) half_sps;
		if (half_mu > 1.0) {
			half_mu -= 1.0;
			half_sps += 1;
		}
		// at this point half_sps represents the whole part, and
		// half_mu the fractional part, of the halfway mark.
		// locate two points, separated by half of one symbol time
		// interp_samp is (we hope) at the optimum sampling point 
		gr_complex interp_samp_mid = d_interp->interpolate(&d_dl[ d_dl_index ], d_mu);
		gr_complex interp_samp = d_interp->interpolate(&d_dl[ d_dl_index + half_sps], half_mu);

		float error_real = (d_last_sample.real() - interp_samp.real()) * interp_samp_mid.real();
		float error_imag = (d_last_sample.imag() - interp_samp.imag()) * interp_samp_mid.imag();
		d_last_sample = interp_samp;	// save for next time
		float symbol_error = error_real + error_imag; // Gardner loop error
		if (isnan(symbol_error)) symbol_error = 0.0;
		if (symbol_error < -1.0) symbol_error = -1.0;
		if (symbol_error >  1.0) symbol_error =  1.0;

		d_omega = d_omega + d_gain_omega * symbol_error;  // update omega based on loop error
		d_omega = d_omega_mid + gr_branchless_clip(d_omega-d_omega_mid, d_omega_rel);   // make sure we don't walk away
#if VERBOSE_GARDNER
		printf("%f\t%f\t%f\t%f\t%f\n", symbol_error, d_mu, d_omega, error_real, error_imag);
#endif
		d_mu += d_omega + d_gain_mu * symbol_error;   // update mu based on loop error

		phase_error_tracking(interp_samp);
  
      out[o++] = interp_samp;
    }
  }

  #if 0
  printf("ninput_items: %d   noutput_items: %d   consuming: %d   returning: %d\n",
	 ninput_items[0], noutput_items, i, o);
  #endif

  consume_each(i);
  return o;
}
