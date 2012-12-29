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
#ifndef INCLUDED_REPEATER_S2V_H
#define INCLUDED_REPEATER_S2V_H

#include <gr_block.h>

class repeater_s2v;

/*
 * We use boost::shared_ptr's instead of raw pointers for all access
 * to gr_blocks (and many other data structures).  The shared_ptr gets
 * us transparent reference counting, which greatly simplifies storage
 * management issues.  This is especially helpful in our hybrid
 * C++ / Python system.
 *
 * See http://www.boost.org/libs/smart_ptr/smart_ptr.htm
 *
 * As a convention, the _sptr suffix indicates a boost::shared_ptr
 */
typedef boost::shared_ptr<repeater_s2v> repeater_s2v_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of repeater_fsk4_slicer_fb.
 *
 * To avoid accidental use of raw pointers, repeater_s2v's
 * constructor is private.  repeater_make_s2v is the public
 * interface for creating new instances.
 */
repeater_s2v_sptr repeater_make_s2v (size_t item_size, size_t nitems_per_block);

class repeater_s2v : public gr_block
{
private:
  // The friend declaration allows repeater_make_s2v to
  // access the private constructor.

  friend repeater_s2v_sptr repeater_make_s2v (size_t item_size, size_t nitems_per_block);

  repeater_s2v (size_t item_size, size_t nitems_per_block);

  static const int DBUFSIZE = 65536;

  char d_buf[ DBUFSIZE * sizeof(gr_complex) ];
  size_t d_buf_len;

  size_t d_acct;
  size_t d_item_size;
  size_t d_nitems_per_block;

 public:
  ~repeater_s2v ();	// public destructor

  // Where all the action really happens

  int general_work (int noutput_items,
	    gr_vector_int &ninput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_REPEATER_S2V_H */
