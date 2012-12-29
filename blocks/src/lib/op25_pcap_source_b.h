/* -*- C++ -*- */

/*
 * Copyright 2010-2011 Steve Glass
 * 
 * This file is part of OP25.
 * 
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * OP25 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#ifndef INCLUDED_OP25_PCAP_SOURCE_B_H
#define INCLUDED_OP25_PCAP_SOURCE_B_H

#include <boost/shared_ptr.hpp>
#include <gr_sync_block.h>
#include <stdint.h>
#include <string>
#include <vector>

typedef boost::shared_ptr<class op25_pcap_source_b> op25_pcap_source_b_sptr;

op25_pcap_source_b_sptr op25_make_pcap_source_b(const char *path, float delay);

/**
 * op25_pcap_source_b is a GNU Radio block for reading from a
 * tcpdump-formatted capture file and producing a stream of octets.
 */
class op25_pcap_source_b : public gr_sync_block
{
public:

   /**
    * op25_pcap_source_b (virtual) destructor.
    */
   virtual ~op25_pcap_source_b();

   /**
    * Process symbols into frames.
    */
   virtual int work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);

private:

   /**
    * Expose class to public ctor. Create a new instance of
    * op25_pcap_source_b and wrap it in a shared_ptr. This is
    * effectively the public constructor.
    *
    * \param path The path to the tcpdump-formatted input file.
    * \param delay The number of seconds to delay before sending the first frame.
    */
   friend op25_pcap_source_b_sptr op25_make_pcap_source_b(const char *path, float delay);

   /**
    * op25_pcap_source_b protected constructor.
    *
    * \param path The path to the tcpdump-formatted input file.
    * \param delay The number of seconds to delay before sending the first frame.
    */
   op25_pcap_source_b(const char *path, float delay);

private:

   /**
    * The next octet to be read from the input file.
    */
   size_t loc_;

   /**
    * Symbols from the input file.
    */
   std::vector<uint8_t> octets_;

};

#endif /* INCLUDED_OP25_PCAP_SOURCE_B_H */
