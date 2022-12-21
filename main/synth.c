/*
 * Copyright (C) 2022 Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "synth.h"

#include <math.h>
#include <stdlib.h>


void synth_string_init(struct synth_string *ss, size_t delay)
{
	ss->delay = delay;
	ss->offset = 0;
	ss->decay = 0.99;

	for (int i = 0; i < SYNTH_MAX_DELAY; i++)
		ss->buffer[i] = 0.0;
}


void synth_string_pluck(struct synth_string *ss, float strength)
{
	ss->offset = 0;

	for (int i = 0; i < ss->delay; i++)
		ss->buffer[i] = strength * rand() / RAND_MAX;
}


inline static int wrap(int a, int min_, int max_)
{
	if (a >= max_)
		return min_;

	if (a < min_)
		return wrap(max_ - a, min_, max_);

	return a;
}


void synth_string_read(struct synth_string *ss, float *out, size_t len)
{
	for (int i = 0; i < len; i++) {
		int this = wrap(ss->offset + i, 0, ss->delay);
		int prev = wrap(this - 1, 0, ss->delay);

		float this_sample = ss->buffer[this];
		float prev_sample = ss->buffer[prev];

		*(out++) += this_sample;

		ss->buffer[this] = (this_sample + prev_sample) * 0.5 * ss->decay;
	}

	ss->offset = wrap(ss->offset + len, 0, ss->delay);
}
