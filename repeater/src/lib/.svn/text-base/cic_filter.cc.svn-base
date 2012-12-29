/*
 * routines for CIC filter, delay line
 *
 * Copyright 2012, KA1RBI
 *
 * This file is part of OP25.
 *
 * OP25 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * OP25 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

using namespace std;

#include "cic_filter.h"

integrator::integrator(void) {
//	d_l = 0;
	d_sum = 0;
}

integrator::~integrator(void) {
	// no op
}

int64_t integrator::cycle(int64_t c1) {
	d_sum = c1 - d_sum;
	return (d_sum);
}

comb::comb(int max_d) {
	d_max_d = max_d;
	d_l = new int64_t[d_max_d];
	for (int i1=0; i1 < d_max_d; i1++) {
		d_l[i1]=0;
	}
	d_cur_d = 0;
}

comb::~comb(void) {
	delete [] d_l;
}

int64_t comb::cycle(int64_t c1, int idx) {
	assert (idx <= d_max_d);
	assert (idx >= 0);
	int i = d_cur_d - idx;
	if (i < 0)
		i += d_max_d;
	int64_t t1 = d_l [ i ];
	d_l [ d_cur_d ] = c1;
	d_cur_d += 1;
	if (d_cur_d >= d_max_d)
		d_cur_d = 0;
	return (c1 - t1);
}

delay::delay(int max_d) {
	d_max_d = max_d;
	d_l = new gr_complex[d_max_d];
	for (int i1=0; i1 < d_max_d; i1++) {
		d_l[i1]=0;
	}
	d_cur_d = 1;
}

delay::~delay(void) {
	delete [] d_l;
}

gr_complex delay::get(int idx) {
	assert (idx <= d_max_d);
	assert (idx >= 0);
	int i = d_cur_d - idx;
	if (i < 0)
		i += d_max_d;
	return d_l [ i ];
}

gr_complex delay::cycle(gr_complex c1, int idx) {
	assert (idx <= d_max_d);
	assert (idx >= 0);
	int i = d_cur_d - idx;
	if (i < 0)
		i += d_max_d;
	gr_complex t1 = d_l [ i ];
	d_l [ d_cur_d ] = c1;
	d_cur_d += 1;
	if (d_cur_d >= d_max_d)
		d_cur_d = 0;
	return t1;
}
