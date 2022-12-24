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


#include "scene.h"
#include "config.h"


struct scene *scene_stack = NULL;


void scene_push(struct scene *scene, const void *arg)
{
	scene->next = scene_stack;
	scene_stack = scene;
	scene->on_activate(arg);
	scene->on_top();
}


void scene_pop(void)
{
	if (scene_stack) {
		scene_stack->on_deactivate();
		scene_stack = scene_stack->next;
	}

	if (scene_stack)
		scene_stack->on_top();
}


void scene_replace(struct scene *scene, const void *arg)
{
	if (scene_stack) {
		scene_stack->on_deactivate();
		scene_stack = scene_stack->next;
	}

	scene_push(scene, arg);
}


bool scene_handle_key_pressed(int key)
{
	for (struct scene *scene = scene_stack; scene; scene = scene->next)
		if (scene->on_key_pressed(key))
			return true;

	return false;
}


bool scene_handle_key_released(int key)
{
	for (struct scene *scene = scene_stack; scene; scene = scene->next)
		if (scene->on_key_released(key))
			return true;

	return false;
}


unsigned scene_idle(unsigned max)
{
	int i = 0;

	for (struct scene *scene = scene_stack; scene; scene = scene->next) {
		unsigned sleep = scene->on_idle(i++);
		max = sleep < max ? sleep : max;
	}

	return max;
}
