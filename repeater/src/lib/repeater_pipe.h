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
#ifndef INCLUDED_REPEATER_PIPE_H
#define INCLUDED_REPEATER_PIPE_H

#include <gr_sync_block.h>

class repeater_pipe;

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
typedef boost::shared_ptr<repeater_pipe> repeater_pipe_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of repeater_pipe.
 *
 * To avoid accidental use of raw pointers, repeater_pipe's
 * constructor is private.  repeater_make_pipe is the public
 * interface for creating new instances.
 */
repeater_pipe_sptr repeater_make_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);

/*!
 * \brief plumbing: OS interface
 * \ingroup block
 *
 * This uses the preferred technique: subclassing gr_sync_block.
 */

class repeater_pipe : public gr_block
{
private:
  // The friend declaration allows repeater_make_pipe to
  // access the private constructor.

  friend repeater_pipe_sptr repeater_make_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);

  repeater_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);  	// private constructor

  size_t d_input_size;
  size_t d_output_size;
  float d_io_ratio;
  const char* d_pathname;
  pid_t d_pid;
  int pipe_fd_in[2];
  int pipe_fd_out[2];
  int d_e_in;
  int d_e_out;

 public:
   virtual void forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd);
  ~repeater_pipe ();	// public destructor

  // Where all the action really happens

  int general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items);

};

#endif /* INCLUDED_REPEATER_PIPE_H */
