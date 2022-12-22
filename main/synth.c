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

#include "config.h"

#include <math.h>
#include <stdlib.h>


void synth_string_pluck(struct synth_string *ss, int16_t strength)
{
	ss->offset = 0;
	ss->decay = 0.999;

	for (int i = 0; i < ss->delay; i++)
		ss->buffer[i] = strength * 1.0 * rand() / RAND_MAX;
}


void synth_string_pluck_shortly(struct synth_string *ss, int16_t strength)
{
	synth_string_pluck(ss, strength);
	ss->decay = 0.99;
}


inline static int wrap(int a, int max_)
{
	return (a + max_) % max_;
}


void synth_string_read(struct synth_string *ss, int16_t *out, size_t len)
{
	float fb = ss->feedback;
	float nfb = (1.0 - ss->feedback) * 0.5;

	int offset = ss->offset;
	int delay = ss->delay;

	float decay = 1.0 - (1.0 - ss->decay) * delay * 440.0 / CONFIG_SAMPLE_FREQ;

	for (int i = 0; i < len; i++) {
		int this = wrap(offset + i, delay);
		int prev = wrap(this - 1, delay);
		int next = wrap(this + 1, delay);

		int16_t this_sample = ss->buffer[this];
		int16_t prev_sample = ss->buffer[prev];
		int16_t next_sample = ss->buffer[next];

		*(out++) += this_sample;

		int new = this_sample * fb
		        + prev_sample * nfb
		        + next_sample * nfb;

		ss->buffer[this] = new * decay;
	}

	ss->offset = wrap(offset + len, delay);
}
