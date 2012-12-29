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
#ifndef INCLUDED_REPEATER_CHAN_USRP_H
#define INCLUDED_REPEATER_CHAN_USRP_H

#include <gr_sync_block.h>
#include <gr_msg_queue.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <deque>

class repeater_chan_usrp;

typedef std::vector<bool> bit_vector;

#include <imbe_vocoder.h>
#include "waveforms.h"

static const int N_PHASES = 8;	// no. of points in PI/4 constellation
static const int N_PHASES_2 = N_PHASES >> 1;

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
typedef boost::shared_ptr<repeater_chan_usrp> repeater_chan_usrp_sptr;
typedef std::deque<uint8_t> dibit_queue;

/*!
 * \brief Return a shared_ptr to a new instance of repeater_chan_usrp.
 *
 * To avoid accidental use of raw pointers, repeater_chan_usrp's
 * constructor is private.  repeater_make_chan_usrp is the public
 * interface for creating new instances.
 */
repeater_chan_usrp_sptr repeater_make_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue);

class repeater_chan_usrp : public gr_block
{
private:
  // The friend declaration allows repeater_make_chan_usrp to
  // access the private constructor.

  void init_sock(int udp_port);

  friend repeater_chan_usrp_sptr repeater_make_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue);

  repeater_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue);  	// private constructor
  // internal functions
  void check_read(void);
  void append_imbe_codeword(bit_vector& frame_body, int16_t frame_vector[], unsigned int& codeword_ct);

  // internal data
  bool d_do_imbe;
  unsigned int d_expected_seq;
  int read_sock;
  struct sockaddr_in read_sock_addr;
  bool warned_select;

  unsigned int codeword_ct;

  unsigned int frame_cnt;
  bool d_keyup_state;
  time_t d_timeout_time;
  unsigned int d_timeout_value;
  bit_vector f_body;
  imbe_vocoder vocoder;

  std::deque<uint8_t> output_queue;
  std::deque<int16_t> output_queue_s;

  gr_msg_queue_sptr d_msgq;

  float d_gain;
  int d_decim;
  int d_phase;
  int d_next_samp;
  gr_complex *d_current_sym;
  const float *d_current_fsym;
  int d_active_dibit;
  int d_shift_reg;
  bool d_muted;
  bool d_do_complex;
  bool d_do_float;

  gr_complex waveforms[N_PHASES][N_WAVEFORMS][N_SPS+1];
 public:
  ~repeater_chan_usrp ();	// public destructor

  // Where all the action really happens

  int general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items);

};

#endif /* INCLUDED_REPEATER_CHAN_USRP_H */
