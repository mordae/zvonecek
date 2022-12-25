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


inline static float rand_sample(void)
{
	float sample = (float)rand() / (float)RAND_MAX;
	return (sample - 0.5) * 2 * INT16_MAX;
}


void synth_string_pluck(struct synth_string *ss)
{
	ss->offset = 0;

	ss->cur_decay = ss->decay;
	ss->cur_feedback = ss->feedback;

	if (NULL == ss->buffer)
		ss->buffer = calloc(sizeof(float), ss->delay);

	for (int i = 0; i < ss->delay; i++)
		ss->buffer[i] = rand_sample();
}


void synth_string_pluck_shortly(struct synth_string *ss)
{
	synth_string_pluck(ss);
	ss->cur_decay = ss->decay * 0.99;
	ss->cur_feedback = ss->feedback;
}


void synth_string_dampen(struct synth_string *ss)
{
	ss->cur_decay = ss->cur_decay * 0.99;
}


inline static int wrap(int a, int max_)
{
	return (a + max_) % max_;
}


void synth_string_read(struct synth_string *ss, float *out, size_t len)
{
	if (NULL == ss->buffer)
		ss->buffer = calloc(sizeof(float), ss->delay);

	float fb = ss->cur_feedback;
	float nfb = (1.0 - ss->cur_feedback) * 0.5;

	int offset = ss->offset;
	int delay = ss->delay;

	float decay = 1.0 - (1.0 - ss->cur_decay) * delay * 440.0 / CONFIG_SAMPLE_FREQ;

	for (int i = 0; i < len; i++) {
		int this = wrap(offset + i, delay);
		int prev = wrap(this - 1, delay);
		int next = wrap(this + 1, delay);

		float this_sample = ss->buffer[this];
		float prev_sample = ss->buffer[prev];
		float next_sample = ss->buffer[next];

		out[i] += this_sample;

		int new = this_sample * fb
		        + prev_sample * nfb
		        + next_sample * nfb;

		ss->buffer[this] = new * decay;
	}

	ss->offset = wrap(offset + len, delay);
}
