/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
 * Copyright 2010, KA1RBI 
 */
/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <repeater_fsk4_slicer_fb.h>
#include <gr_io_signature.h>
#include <stdio.h>

/*
 * Create a new instance of repeater_fsk4_slicer_fb and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
repeater_fsk4_slicer_fb_sptr 
repeater_make_fsk4_slicer_fb (const std::vector<float> &slice_levels)
{
  return repeater_fsk4_slicer_fb_sptr (new repeater_fsk4_slicer_fb (slice_levels));
}

/*
 * Specify constraints on number of input and output streams.
 * This info is used to construct the input and output signatures
 * (2nd & 3rd args to gr_block's constructor).  The input and
 * output signatures are used by the runtime system to
 * check that a valid number and type of inputs and outputs
 * are connected to this block.  In this case, we accept
 * only 1 input and 1 output.
 */
static const int MIN_IN = 1;	// mininum number of input streams
static const int MAX_IN = 1;	// maximum number of input streams
static const int MIN_OUT = 1;	// minimum number of output streams
static const int MAX_OUT = 1;	// maximum number of output streams

/*
 * The private constructor
 */
repeater_fsk4_slicer_fb::repeater_fsk4_slicer_fb (const std::vector<float> &slice_levels)
  : gr_sync_block ("fsk4_slicer_fb",
		   gr_make_io_signature (MIN_IN, MAX_IN, sizeof (float)),
		   gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (unsigned char)))
{
	d_slice_levels[0] = slice_levels[0];
	d_slice_levels[1] = slice_levels[1];
	d_slice_levels[2] = slice_levels[2];
	d_slice_levels[3] = slice_levels[3];
}

/*
 * Our virtual destructor.
 */
repeater_fsk4_slicer_fb::~repeater_fsk4_slicer_fb ()
{
  // nothing else required in this example
}

int 
repeater_fsk4_slicer_fb::work (int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  const float *in = (const float *) input_items[0];
  unsigned char *out = (unsigned char *) output_items[0];

  for (int i = 0; i < noutput_items; i++){
#if 0
    if (in[i] < -2.0) {
      out[i] = 3;
    } else if (in[i] < 0.0) {
      out[i] = 2;
    } else if (in[i] <  2.0) {
      out[i] = 0;
    } else {
      out[i] = 1;
    }
#endif
    uint8_t dibit;
    float sym = in[i];
    if (d_slice_levels[3] < 0) {
      dibit = 1;
      if (d_slice_levels[3] <= sym && sym < d_slice_levels[0])
        dibit = 3;
    } else {
      dibit = 3;
      if (d_slice_levels[2] <= sym && sym < d_slice_levels[3])
        dibit = 1;
    }
    if (d_slice_levels[0] <= sym && sym < d_slice_levels[1])
      dibit = 2;
    if (d_slice_levels[1] <= sym && sym < d_slice_levels[2])
      dibit = 0;
    out[i] = dibit;
  }

  // Tell runtime system how many output items we produced.
  return noutput_items;
}
