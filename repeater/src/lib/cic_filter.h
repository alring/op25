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

#ifndef INCLUDED_CIC_FILTER_H
#define INCLUDED_CIC_FILTER_H 1

#include <stdint.h>
#include <assert.h>
#include <gr_complex.h>

class integrator { /* integrator */
public:
	integrator();
	~integrator();
	int64_t cycle(int64_t c1);
private:
//	int64_t d_l;
	int64_t d_sum;
};

class comb { /* comb */
public:
	comb(int max_d);
	~comb();
	int64_t cycle(int64_t c1, int idx);
private:
	int64_t *d_l;
	int d_max_d;
	int d_cur_d;
};

class delay { /* delay */
public:
	delay(int max_d);
	~delay();
	gr_complex get(int idx);
	gr_complex cycle(gr_complex c1, int idx);
private:
	gr_complex *d_l;
	int d_max_d;
	int d_cur_d;
};

#endif /* INCLUDED_CIC_FILTER_H */
