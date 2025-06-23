/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* geometry.c */
#include <stdint.h>
#include <limits.h>
#include "types.h"
#include "settings.h"
#include "geometry.h"

static inline bool valid_rect(xcb_rectangle_t r)
{
	return r.width > 0 && r.height > 0 &&
	       r.x <= INT16_MAX - r.width &&
	       r.y <= INT16_MAX - r.height;
}

static inline xcb_point_t rect_max(xcb_rectangle_t r)
{
	return (xcb_point_t){r.x + r.width - 1, r.y + r.height - 1};
}

bool is_inside(xcb_point_t p, xcb_rectangle_t r)
{
	if (!valid_rect(r))
		return false;
	return p.x >= r.x && p.x < r.x + r.width &&
	       p.y >= r.y && p.y < r.y + r.height;
}

bool contains(xcb_rectangle_t a, xcb_rectangle_t b)
{
	if (!valid_rect(a) || !valid_rect(b))
		return false;
	xcb_point_t b_max = rect_max(b);
	return a.x <= b.x && a.y <= b.y &&
	       a.x + a.width >= b_max.x + 1 &&
	       a.y + a.height >= b_max.y + 1;
}

unsigned int area(xcb_rectangle_t r)
{
	if (!valid_rect(r))
		return 0;
	if (r.width > UINT_MAX / r.height)
		return UINT_MAX;
	return r.width * r.height;
}

uint32_t boundary_distance(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir)
{
	if (!valid_rect(r1) || !valid_rect(r2))
		return UINT32_MAX;

	xcb_point_t r1_max = rect_max(r1);
	xcb_point_t r2_max = rect_max(r2);

	switch (dir) {
		case DIR_NORTH:
			return r2_max.y > r1.y ? r2_max.y - r1.y : r1.y - r2_max.y;
		case DIR_WEST:
			return r2_max.x > r1.x ? r2_max.x - r1.x : r1.x - r2_max.x;
		case DIR_SOUTH:
			return r2.y < r1_max.y ? r1_max.y - r2.y : r2.y - r1_max.y;
		case DIR_EAST:
			return r2.x < r1_max.x ? r1_max.x - r2.x : r2.x - r1_max.x;
		default:
			return UINT32_MAX;
	}
}

bool on_dir_side(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir)
{
	if (!valid_rect(r1) || !valid_rect(r2))
		return false;

	xcb_point_t r1_max = rect_max(r1);
	xcb_point_t r2_max = rect_max(r2);

	/* Check if r2 is on the correct side */
	switch (directional_focus_tightness) {
		case TIGHTNESS_LOW:
			switch (dir) {
				case DIR_NORTH:
					if (r2.y > r1_max.y) return false;
					break;
				case DIR_WEST:
					if (r2.x > r1_max.x) return false;
					break;
				case DIR_SOUTH:
					if (r2_max.y < r1.y) return false;
					break;
				case DIR_EAST:
					if (r2_max.x < r1.x) return false;
					break;
				default:
					return false;
			}
			break;
		case TIGHTNESS_HIGH:
			switch (dir) {
				case DIR_NORTH:
					if (r2.y >= r1.y) return false;
					break;
				case DIR_WEST:
					if (r2.x >= r1.x) return false;
					break;
				case DIR_SOUTH:
					if (r2_max.y <= r1_max.y) return false;
					break;
				case DIR_EAST:
					if (r2_max.x <= r1_max.x) return false;
					break;
				default:
					return false;
			}
			break;
		default:
			return false;
	}

	/* Check overlap */
	switch (dir) {
		case DIR_NORTH:
		case DIR_SOUTH:
			return r2_max.x >= r1.x && r2.x <= r1_max.x;
		case DIR_WEST:
		case DIR_EAST:
			return r2_max.y >= r1.y && r2.y <= r1_max.y;
		default:
			return false;
	}
}

bool rect_eq(xcb_rectangle_t a, xcb_rectangle_t b)
{
	return a.x == b.x && a.y == b.y &&
	       a.width == b.width && a.height == b.height;
}

int rect_cmp(xcb_rectangle_t r1, xcb_rectangle_t r2)
{
	if (!valid_rect(r1) || !valid_rect(r2))
		return 0;

	if (r1.y >= r2.y + r2.height)
		return 1;
	if (r2.y >= r1.y + r1.height)
		return -1;
	if (r1.x >= r2.x + r2.width)
		return 1;
	if (r2.x >= r1.x + r1.width)
		return -1;

	unsigned int a1 = area(r1);
	unsigned int a2 = area(r2);
	return (a2 < a1) - (a1 < a2);
}
