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

#include <stdlib.h>
#include <stdbool.h>


/*
 * Scene virtual function table.
 *
 * It has no private data, because we assume they are stored statically
 * inside the respective module.
 */
struct scene {
	/* Called once at the start of the program. */
	void (*on_init)(void);

	/* Called when the scene appears on the top of the stack. */
	void (*on_top)(void);

	/* Called when the scene enters the scene stack. Before on_top. */
	void (*on_activate)(const void *arg);

	/* Called when the scene leaves the scene stack. */
	void (*on_deactivate)(void);

	/*
	 * Called for active scene to do its work.
	 *
	 * The `depth` argument is the level of the scene stack this
	 * scene is currently at. Value 0 indicates that the scene is
	 * at the very top.
	 *
	 * It returns the number of milliseconds to sleep before it should
	 * be called again. The scene that returns the lowest number dictates
	 * the final sleep duration, though.
	 */
	unsigned (*on_idle)(unsigned depth);

	/*
	 * Called when a key gets pressed while the scene is active.
	 *
	 * Returns `true` to indicate that the event has been handled.
	 * Otherwise the next scene in the stack will get it.
	 */
	bool (*on_key_pressed)(int key);

	/*
	 * Called when a key gets depressed while the scene is active.
	 *
	 * Returns `true` to indicate that the event has been handled.
	 * Otherwise the next scene in the stack will get it.
	 */
	bool (*on_key_released)(int key);

	/* Next scene in the stack. */
	struct scene *next;
};


/*
 * Stack of scenes.
 */
extern struct scene *scene_stack;


/*
 * Push a new scene to the scene stack.
 * Same scene must not be pushed to the scene stack twice!
 */
void scene_push(struct scene *scene, const void *arg);


/* Pop a scene from the scene stack. */
void scene_pop(void);


/*
 * Replace the scene at the top of the scene stack.
 *
 * Equivalent to scene_pop() followed by a scene_push(),
 * except the scene below the one being replaced does not
 * receive `on_top`.
 */
void scene_replace(struct scene *scene, const void *arg);


/* Let scene stack handle key press. */
bool scene_handle_key_pressed(int key);


/* Let scene stack handle key release. */
bool scene_handle_key_released(int key);


/*
 * Run scene stack idle tasks and return number of milliseconds to sleep.
 * The `max` parameter gives the maximum amount of milliseconds to return.
 */
unsigned scene_idle(unsigned max);


/* Allows one to play freely or switch to Learning. */
extern struct scene Keyboard;

/* Plays a tune and then prompts user to play using LEDs. */
extern struct scene Learning;
extern const char *const learning_songs[4];
