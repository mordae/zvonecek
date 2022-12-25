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

#pragma once

#include <stdint.h>
#include <stdlib.h>

#if !defined(SYNTH_MAX_DELAY)
# define SYNTH_MAX_DELAY 1000
#endif

struct synth_string {
	size_t delay, offset;
	float decay, cur_decay;
	float feedback, cur_feedback;
	int16_t buffer[SYNTH_MAX_DELAY];
};

void synth_string_pluck(struct synth_string *ss);
void synth_string_pluck_shortly(struct synth_string *ss);
void synth_string_dampen(struct synth_string *ss);
void synth_string_read(struct synth_string *ss, int16_t *out, size_t len);
