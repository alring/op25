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
#ifndef INCLUDED_REPEATER_P25_FRAME_ASSEMBLER_H
#define INCLUDED_REPEATER_P25_FRAME_ASSEMBLER_H

#include <gr_sync_block.h>
#include <gr_msg_queue.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <deque>

class repeater_p25_frame_assembler;
class p25_framer;

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
typedef boost::shared_ptr<repeater_p25_frame_assembler> repeater_p25_frame_assembler_sptr;
typedef std::deque<uint8_t> dibit_queue;

/*!
 * \brief Return a shared_ptr to a new instance of repeater_p25_frame_assembler.
 *
 * To avoid accidental use of raw pointers, repeater_p25_frame_assembler's
 * constructor is private.  repeater_make_p25_frame_assembler is the public
 * interface for creating new instances.
 */
repeater_p25_frame_assembler_sptr repeater_make_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue);

/*!
 * \brief produce a stream of dibits, given a stream of floats in [-3,-1,1,3]
 * \ingroup block
 *
 * This uses the preferred technique: subclassing gr_sync_block.
 */

class repeater_p25_frame_assembler : public gr_block
{
private:
  // The friend declaration allows repeater_make_p25_frame_assembler to
  // access the private constructor.

  void init_sock(const char* udp_host, int udp_port);

  friend repeater_p25_frame_assembler_sptr repeater_make_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue);

  repeater_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue);  	// private constructor
  // internal functions
	typedef std::vector<bool> bit_vector;
	bool header_codeword(uint64_t acc, uint32_t& nac, uint32_t& duid);
	void proc_voice_unit(bit_vector& frame_body) ;
  // internal instance variables and state
	int write_bufp;
	int write_sock;
	struct sockaddr_in write_sock_addr;
	char write_buf[512];
	const char* d_udp_host;
	int d_port;
	int d_debug;
	bool d_do_imbe;
	bool d_do_output;
	bool d_do_msgq;
	gr_msg_queue_sptr d_msg_queue;
	dibit_queue symbol_queue;
	p25_framer* framer;

 public:
   virtual void forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd);
  ~repeater_p25_frame_assembler ();	// public destructor

  // Where all the action really happens

  int general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items);

};

#endif /* INCLUDED_REPEATER_P25_FRAME_ASSEMBLER_H */
