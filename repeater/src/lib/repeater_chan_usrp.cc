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
 *
 ***********************************************************************
 *
 * chan_usrp - interface from asterisk app_rpt
 * "Fast" PI/4 DQPSK TX using a direct table lookup method
 *
 * Copyright 2010, KA1RBI
 *
 * This GR source block generates a complex output stream (at 320K rate)
 * for direct connection to the USRP sink block.  The input is a dibit
 * stream obtained from chan_usrp via a UDP channel.
 *
 * Complex output waveforms are generated directly from the dibit stream,
 * at a rate of 66.6667 SPS (nominal input rate=4800, output rate=320000).
 * The output waveform chosen for a given input symbol depends on the value
 * of that input symbol plus the values of the prior and following symbols.
 * 
 * (Other output rates than 320k are available; 320k occurs when decimation=3)
 * 
 * This results in 2 ** (3*2) = 64 distinct possible waveforms.  These are
 * initially encoded at a SPS value of 200 (corresponding to a 960K sampling
 * rate) and are scaled to standard values (-3/-1/+1/+3).
 *
 * This TX requires a separate version of the 64 waveforms for each of eight
 * possible phases.  So, at program init time we generate the actual complex
 * waveforms by
 *   - level-shifting each waveform according to the selected phase 0 to 7
 *   - clipping the resulting values (in range -4 to +4)
 *   - convert (rescale) to angle in radians
 *   - map the angles in radians to values in complex plane
 *   - multiply complex values by gain (real constant), complex samples result 
 *
 * The complex output mode is used if do_complex is set to true.  However
 * other output modes are possible.
 * -If do_imbe=true, outputs a dibit stream containing IMBE encoded voice
 * -Else, outputs raw audio (signed int16 format @ 8K)
 */

static const int phase_table[4] = {+1, +3, -1, -3};

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define INCLUDE_TX_WAVEFORMS
#include <repeater_chan_usrp.h>
#include <gr_io_signature.h>
#include <gr_prefs.h>
#include <gr_math.h>
#include <gr_expj.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <repeater_chan_usrp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include <op25_p25_frame.h>
#include <op25_imbe_frame.h>

#include "chan_usrp.h"

#ifndef min
#define min(a,b) ((a<b)?a:b)
#endif

/*
 * Create a new instance of repeater_chan_usrp and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
repeater_chan_usrp_sptr 
repeater_make_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue)
{
  return repeater_chan_usrp_sptr (new repeater_chan_usrp (listen_port, do_imbe, do_complex, do_float, gain, decim, queue));
}

/*
 * The private constructor
 */
repeater_chan_usrp::repeater_chan_usrp (int listen_port, bool do_imbe, bool do_complex, bool do_float, float gain, int decim, gr_msg_queue_sptr queue)
  : gr_block ("chan_usrp",
		   gr_make_io_signature (0, 0, 0),
		   gr_make_io_signature (1, 1, (do_float) ? sizeof(float) : ((do_complex) ? sizeof(gr_complex) : ((do_imbe) ? sizeof(char) : sizeof(int16_t))))),
    d_do_imbe(do_imbe),
    d_expected_seq(0),
    read_sock(0),
    warned_select(false),
    codeword_ct(0),
    frame_cnt(0),
    d_keyup_state(false),
    d_timeout_time(0),
    d_timeout_value(4),	// in sec.
    f_body(P25_VOICE_FRAME_SIZE),
    d_msgq(queue),
    d_gain(gain),
    d_decim(decim),
    d_phase(0),
    d_next_samp(0),
    d_current_sym(0),
    d_active_dibit(0),
    d_shift_reg(0),
    d_muted(0),
    d_do_complex(do_complex),
    d_do_float(do_float)
{
	int i, j, k;
	float t;
	float *fp;
	if (d_do_complex) {
		for (i = 0; i < N_PHASES; i++) {
			for (j = 0; j < N_WAVEFORMS; j++) {
				for (k = 0; k < N_SPS+1; k++) {
#ifdef USE_ALT_RC_FILTER
					t =  rc_syms[j][k] - N_PHASES_2 + (float)i;
#else
					t = rrc_syms[j][k] - N_PHASES_2 + (float)i;
#endif
					if (t < -N_PHASES_2) t += N_PHASES;
					if (t >  N_PHASES_2) t -= N_PHASES;
					t *= (M_PI / 4.0);
					fp = (float*)&waveforms[i][j][k];
					fp[0] = d_gain * cos(t);
					fp[1] = d_gain * sin(t);
				}
			}
		}
	}

	if (listen_port > 0)
		init_sock(listen_port);
}

repeater_chan_usrp::~repeater_chan_usrp ()
{
	if (read_sock > 0)
		close(read_sock);
}

void repeater_chan_usrp::append_imbe_codeword(bit_vector& frame_body, int16_t frame_vector[], unsigned int& codeword_ct)
{
	voice_codeword cw(voice_codeword_sz);
	// construct 144-bit codeword from 88 bits of parameters
	imbe_header_encode(cw, frame_vector[0], frame_vector[1], frame_vector[2], frame_vector[3], frame_vector[4], frame_vector[5], frame_vector[6], frame_vector[7]);

	// add codeword to voice data unit
	imbe_interleave(frame_body, cw, codeword_ct);

	// after the ninth and final codeword added, dispose of frame
	if (++codeword_ct >= nof_voice_codewords) {
		static const uint64_t hws[2] = { 0x293555ef2c653437LL, 0x293aba93bec26a2bLL };
		p25_setup_frame_header(frame_body, hws[frame_cnt & 1]);
		for (size_t i = 0; i < sizeof(imbe_ldu_ls_data_bits) / sizeof(imbe_ldu_ls_data_bits[0]); i++) {
			frame_body[imbe_ldu_ls_data_bits[i]] = 0;
		}
		// finally, output the frame
		for (uint32_t i = 0; i < P25_VOICE_FRAME_SIZE; i += 2) {
			uint8_t dibit = 
				(frame_body[i+0] << 1) +
				(frame_body[i+1]     );
			output_queue.push_back(dibit);
		}
		codeword_ct = 0;
		frame_cnt++;
	}
}

int 
repeater_chan_usrp::general_work (int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
{
	int rc;
	struct sockaddr saddr;
	char rcvbuf[1024];
	socklen_t addrlen;
	int16_t *bufdata;
	struct _chan_usrp_bufhdr *bufhdrp;
	int16_t frame_vector[8];
	unsigned int rseq;

	for (;;) {	// read all pending msgs
		addrlen = sizeof(saddr);
		rc = recvfrom(read_sock, rcvbuf, sizeof(rcvbuf), MSG_DONTWAIT,
			&saddr, &addrlen);
		if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		if (rc < 0 && !warned_select) {
			fprintf(stderr, "repeater_chan_usrp: recvfrom: %d\n", errno);
			warned_select = 1;
		}
		if (rc < 0)
			break;
		if (rc !=  sizeof(struct _chan_usrp_bufhdr) &&
		    rc != (sizeof(struct _chan_usrp_bufhdr) + USRP_VOICE_FRAME_SIZE)) {
			fprintf(stderr, "repeater_chan_usrp: error: received size %d invalid\n", rc) ;
			continue;
		}
		bufhdrp = (struct _chan_usrp_bufhdr*)rcvbuf;
		bufdata = (int16_t*) &rcvbuf[ sizeof(struct _chan_usrp_bufhdr) ];
		if (ntohl(bufhdrp->keyup) != d_keyup_state) {
			// tx keyup state change has occurred
#if 0
			fprintf(stderr, "repeater_chan_usrp: keyup state %d was %d\n", ntohl(bufhdrp->keyup), d_keyup_state);
#endif
			d_keyup_state = ntohl(bufhdrp->keyup);
			// TODO queue hdu / tdu(s)
#if 0
			if (d_msgq) {
				gr_message_sptr msg = gr_make_message(0,d_keyup_state,0,0);
				d_msgq->handle(msg);
			}
#endif
		}
		rseq = ntohl(bufhdrp->seq);
		if (rseq != d_expected_seq && rseq != 0 && d_expected_seq != 0) {
			fprintf(stderr, "repeater_chan_usrp: possible data loss, expected seq %d received %d\n", d_expected_seq, rseq);
		}
		d_expected_seq = rseq + 1;
		if (bufhdrp->keyup) {
			d_timeout_time = time(NULL) + d_timeout_value;
		}
		if ((unsigned int)rc < sizeof(struct _chan_usrp_bufhdr) + USRP_VOICE_FRAME_SIZE)
			continue;
		if (d_do_imbe || d_do_complex || d_do_float) {
			vocoder.imbe_encode(frame_vector, bufdata);
			append_imbe_codeword(f_body, frame_vector, codeword_ct);
		} else {
			for (unsigned int i=0; i < (USRP_VOICE_FRAME_SIZE>>1); i++) {
				output_queue_s.push_back(bufdata[i]);
			}
		}
	}
	if (d_do_complex) {
		gr_complex *out = (gr_complex*) output_items[0];
		int o=0;
		uint8_t new_dibit;

		while((o < noutput_items)) {
			if (d_muted) {
				if (output_queue.size())
					output_queue.clear();
				out[o++] = gr_complex(0, 0);
				continue;
			}
			if (!d_current_sym) {
				if (!output_queue.size()) {
					out[o++] = gr_complex(0, 0);
					continue;
				}
				new_dibit = output_queue.front();
				output_queue.pop_front();
				// assert(new_dibit <= 3);
				// shift reg holds three sequential dibits
				d_shift_reg = ((d_shift_reg << 2) + new_dibit) & (N_WAVEFORMS-1);
				// get pointer to next symbol waveform to be transmitted
				d_current_sym = waveforms[d_phase][d_shift_reg];
				// update phase based on current active dibit
				d_phase += phase_table[d_active_dibit];
				// clamp phase
				if (d_phase < 0) d_phase += N_PHASES;
				if (d_phase > (N_PHASES-1)) d_phase -= N_PHASES;
				// save for next pass 
				d_active_dibit = new_dibit;
			}
			out[o++] = d_current_sym[d_next_samp];
			d_next_samp += d_decim;
			if (d_next_samp >= N_SPS) {
				d_next_samp -= N_SPS;
				d_current_sym = NULL;
			}
		}
		return noutput_items;
	} else if (d_do_float) {
		float *out = (float*) output_items[0];
		int o=0;
		uint8_t new_dibit;

		while((o < noutput_items)) {
			if (d_muted) {
				if (output_queue.size())
					output_queue.clear();
				out[o++] = 0.0;
				continue;
			}
			if (!d_current_fsym) {
				if (!output_queue.size()) {
					out[o++] = 0.0;
					continue;
				}
				new_dibit = output_queue.front();
				output_queue.pop_front();
				// assert(new_dibit <= 3);
				// shift reg holds 8 sequential dibits
				d_shift_reg = ((d_shift_reg << 2) + new_dibit) & (N_FLOAT_WAVEFORMS-1);
				// get pointer to next symbol waveform to be transmitted
				d_current_fsym = float_waveforms[d_shift_reg];
			}
			out[o++] = d_current_fsym[d_next_samp];
			d_next_samp += d_decim;
			if (d_next_samp >= N_FLOAT_SPS) {
				d_next_samp -= N_FLOAT_SPS;
				d_current_fsym = NULL;
			}
		}
		return noutput_items;
	} else if (d_do_imbe) {
		char *out = (char*) output_items[0];
		// if no output available at all, output zeros
		if (output_queue.empty()) {
			for (int i = 0; i < noutput_items; i++) {
				out[i] = 0;
				// out[i] = i & 3;
			}
			return noutput_items;
		}
		int amt_move = min(output_queue.size(), (unsigned int)noutput_items);
		for (int i = 0; i < amt_move; i++) {
			out[i] = output_queue[i];
			// out[i] = i & 3;
		}
		output_queue.erase(output_queue.begin(), output_queue.begin() + amt_move);
		return amt_move;
	} else {
		int16_t *out = (int16_t*) output_items[0];
		// if no output available at all, output zeros
		if (output_queue_s.empty()) {
			for (int i = 0; i < noutput_items; i++) {
				out[i] = 0;
			}
			return noutput_items;
		}
		int amt_move = min(output_queue_s.size(), (unsigned int)noutput_items);
		for (int i = 0; i < amt_move; i++) {
			out[i] = output_queue_s[i];
		}
		output_queue_s.erase(output_queue_s.begin(), output_queue_s.begin() + amt_move);
		return amt_move;
	}
}

void repeater_chan_usrp::init_sock(int udp_port)
{
        memset (&read_sock_addr, 0, sizeof(read_sock_addr));
        read_sock = socket(PF_INET, SOCK_DGRAM, 17);   // UDP socket
        if (read_sock < 0) {
                fprintf(stderr, "op25_chan_usrp: socket: %d\n", errno);
                read_sock = 0;
		return;
        }
        read_sock_addr.sin_addr.s_addr = INADDR_ANY;
        read_sock_addr.sin_family = AF_INET;
        read_sock_addr.sin_port = htons(udp_port);
	int rc = bind(read_sock, (struct sockaddr*)&read_sock_addr, sizeof(read_sock_addr));
	if (rc < 0) {
		fprintf(stderr, "op25_chan_usrp: bind: %d\n", errno);
		close(read_sock);
                read_sock = 0;
		return;
	}
}
