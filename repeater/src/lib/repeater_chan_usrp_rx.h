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
#ifndef INCLUDED_REPEATER_CHAN_USRP_RX_H
#define INCLUDED_REPEATER_CHAN_USRP_RX_H

#include <gr_sync_block.h>
#include <gr_msg_queue.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <deque>

#include "chan_usrp.h"

class repeater_chan_usrp_rx;

typedef std::vector<bool> bit_vector;


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
typedef boost::shared_ptr<repeater_chan_usrp_rx> repeater_chan_usrp_rx_sptr;
typedef std::deque<uint8_t> dibit_queue;

/*!
 * \brief Return a shared_ptr to a new instance of repeater_chan_usrp_rx.
 *
 * To avoid accidental use of raw pointers, repeater_chan_usrp_rx's
 * constructor is private.  repeater_make_chan_usrp_rx is the public
 * interface for creating new instances.
 */
repeater_chan_usrp_rx_sptr repeater_make_chan_usrp_rx (const char* udp_host, int port, int debug);

class repeater_chan_usrp_rx : public gr_block
{
private:
  // The friend declaration allows repeater_make_chan_usrp_rx to
  // access the private constructor.

  void init_sock(const char* udp_host, int udp_port);

  friend repeater_chan_usrp_rx_sptr repeater_make_chan_usrp_rx (const char* udp_host, int port, int debug);

  repeater_chan_usrp_rx (const char* udp_host, int port, int debug);
  // internal functions

  // internal data
  int write_sock;
  struct sockaddr_in write_sock_addr;

  const char* d_udp_host;
  const int d_port;
  const int d_debug;

  unsigned int d_sampbuf_ct;
  unsigned int d_sendseq;

  char write_buf[ sizeof(struct _chan_usrp_bufhdr) + USRP_VOICE_FRAME_SIZE*sizeof(int16_t) ];

 public:
  ~repeater_chan_usrp_rx ();	// public destructor

  void unkey(void);

  // Where all the action really happens

  int general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items);

};

#endif /* INCLUDED_REPEATER_CHAN_USRP_RX_H */
