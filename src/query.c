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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "history.h"
#include "parse.h"
#include "monitor.h"
#include "window.h"
#include "tree.h"
#include "query.h"
#include "geometry.h"

#define MAX_RECURSION_DEPTH 1000

static void query_node_depth(node_t *n, FILE *rsp, int depth);
static int query_node_ids_in_depth(node_t *n, desktop_t *d, monitor_t *m, coordinates_t *ref,
                                   coordinates_t *trg, node_select_t *sel, FILE *rsp, int depth);

void query_state(FILE *rsp)
{
	if (!rsp) return;
	
	fprintf(rsp, "{");
	fprintf(rsp, "\"focusedMonitorId\":%u,", mon ? mon->id : 0);
	if (pri_mon) {
		fprintf(rsp, "\"primaryMonitorId\":%u,", pri_mon->id);
	}
	fprintf(rsp, "\"clientsCount\":%i,", clients_count);
	fprintf(rsp, "\"monitors\":");
	fprintf(rsp, "[");
	for (monitor_t *m = mon_head; m; m = m->next) {
		query_monitor(m, rsp);
		if (m->next) {
			fprintf(rsp, ",");
		}
	}
	fprintf(rsp, "]");
	fprintf(rsp,",");
	fprintf(rsp, "\"focusHistory\":");
	query_history(rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"stackingList\":");
	query_stack(rsp);
	if (restart) {
		fprintf(rsp,",");
		fprintf(rsp, "\"eventSubscribers\":");
		query_subscribers(rsp);
	}
	fprintf(rsp, "}");
}

void query_monitor(monitor_t *m, FILE *rsp)
{
	if (!m || !rsp) return;
	
	fprintf(rsp, "{");
	fprintf(rsp, "\"name\":\"%s\",", m->name);
	fprintf(rsp, "\"id\":%u,", m->id);
	fprintf(rsp, "\"randrId\":%u,", m->randr_id);
	fprintf(rsp, "\"wired\":%s,", BOOL_STR(m->wired));
	fprintf(rsp, "\"stickyCount\":%i,", m->sticky_count);
	fprintf(rsp, "\"windowGap\":%i,", m->window_gap);
	fprintf(rsp, "\"borderWidth\":%u,", m->border_width);
	fprintf(rsp, "\"focusedDesktopId\":%u,", m->desk ? m->desk->id : 0);
	fprintf(rsp, "\"padding\":");
	query_padding(m->padding, rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"rectangle\":");
	query_rectangle(m->rectangle, rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"desktops\":");
	fprintf(rsp, "[");
	for (desktop_t *d = m->desk_head; d; d = d->next) {
		query_desktop(d, rsp);
		if (d->next) {
			fprintf(rsp,",");
		}
	}
	fprintf(rsp, "]");
	fprintf(rsp, "}");
}

void query_desktop(desktop_t *d, FILE *rsp)
{
	if (!d || !rsp) return;
	
	fprintf(rsp, "{");
	fprintf(rsp, "\"name\":\"%s\",", d->name);
	fprintf(rsp, "\"id\":%u,", d->id);
	fprintf(rsp, "\"layout\":\"%s\",", LAYOUT_STR(d->layout));
	fprintf(rsp, "\"userLayout\":\"%s\",", LAYOUT_STR(d->user_layout));
	fprintf(rsp, "\"windowGap\":%i,", d->window_gap);
	fprintf(rsp, "\"borderWidth\":%u,", d->border_width);
	fprintf(rsp, "\"focusedNodeId\":%u,", d->focus ? d->focus->id : 0);
	fprintf(rsp, "\"padding\":");
	query_padding(d->padding, rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"root\":");
	query_node_depth(d->root, rsp, 0);
	fprintf(rsp, "}");
}

static void query_node_depth(node_t *n, FILE *rsp, int depth)
{
	if (depth > MAX_RECURSION_DEPTH || !rsp) {
		fprintf(rsp, "null");
		return;
	}
	
	if (!n) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{");
		fprintf(rsp, "\"id\":%u,", n->id);
		fprintf(rsp, "\"splitType\":\"%s\",", SPLIT_TYPE_STR(n->split_type));
		fprintf(rsp, "\"splitRatio\":%lf,", n->split_ratio);
		fprintf(rsp, "\"vacant\":%s,", BOOL_STR(n->vacant));
		fprintf(rsp, "\"hidden\":%s,", BOOL_STR(n->hidden));
		fprintf(rsp, "\"sticky\":%s,", BOOL_STR(n->sticky));
		fprintf(rsp, "\"private\":%s,", BOOL_STR(n->private));
		fprintf(rsp, "\"locked\":%s,", BOOL_STR(n->locked));
		fprintf(rsp, "\"marked\":%s,", BOOL_STR(n->marked));
		fprintf(rsp, "\"presel\":");
		query_presel(n->presel, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"rectangle\":");
		query_rectangle(n->rectangle, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"constraints\":");
		query_constraints(n->constraints, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"firstChild\":");
		query_node_depth(n->first_child, rsp, depth + 1);
		fprintf(rsp,",");
		fprintf(rsp, "\"secondChild\":");
		query_node_depth(n->second_child, rsp, depth + 1);
		fprintf(rsp,",");
		fprintf(rsp, "\"client\":");
		query_client(n->client, rsp);
		fprintf(rsp, "}");
	}
}

void query_node(node_t *n, FILE *rsp)
{
	query_node_depth(n, rsp, 0);
}

void query_presel(presel_t *p, FILE *rsp)
{
	if (!rsp) return;
	
	if (!p) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{\"splitDir\":\"%s\",\"splitRatio\":%lf}", 
		        SPLIT_DIR_STR(p->split_dir), p->split_ratio);
	}
}

void query_client(client_t *c, FILE *rsp)
{
	if (!rsp) return;
	
	if (!c) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{");
		fprintf(rsp, "\"className\":\"%s\",", c->class_name);
		fprintf(rsp, "\"instanceName\":\"%s\",", c->instance_name);
		fprintf(rsp, "\"borderWidth\":%u,", c->border_width);
		fprintf(rsp, "\"state\":\"%s\",", STATE_STR(c->state));
		fprintf(rsp, "\"lastState\":\"%s\",", STATE_STR(c->last_state));
		fprintf(rsp, "\"layer\":\"%s\",", LAYER_STR(c->layer));
		fprintf(rsp, "\"lastLayer\":\"%s\",", LAYER_STR(c->last_layer));
		fprintf(rsp, "\"urgent\":%s,", BOOL_STR(c->urgent));
		fprintf(rsp, "\"shown\":%s,", BOOL_STR(c->shown));
		fprintf(rsp, "\"tiledRectangle\":");
		query_rectangle(c->tiled_rectangle, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"floatingRectangle\":");
		query_rectangle(c->floating_rectangle, rsp);
		fprintf(rsp, "}");
	}
}

void query_rectangle(xcb_rectangle_t r, FILE *rsp)
{
	if (!rsp) return;
	fprintf(rsp, "{\"x\":%i,\"y\":%i,\"width\":%u,\"height\":%u}", r.x, r.y, r.width, r.height);
}

void query_constraints(constraints_t c, FILE *rsp)
{
	if (!rsp) return;
	fprintf(rsp, "{\"min_width\":%u,\"min_height\":%u}", c.min_width, c.min_height);
}

void query_padding(padding_t p, FILE *rsp)
{
	if (!rsp) return;
	fprintf(rsp, "{\"top\":%i,\"right\":%i,\"bottom\":%i,\"left\":%i}", p.top, p.right, p.bottom, p.left);
}

void query_history(FILE *rsp)
{
	if (!rsp) return;
	
	fprintf(rsp, "[");
	for (history_t *h = history_head; h; h = h->next) {
		query_coordinates(&h->loc, rsp);
		if (h->next) {
			fprintf(rsp, ",");
		}
	}
	fprintf(rsp, "]");
}

void query_coordinates(coordinates_t *loc, FILE *rsp)
{
	if (!loc || !rsp) return;
	
	fprintf(rsp, "{\"monitorId\":%u,\"desktopId\":%u,\"nodeId\":%u}", 
	        loc->monitor ? loc->monitor->id : 0,
	        loc->desktop ? loc->desktop->id : 0,
	        loc->node ? loc->node->id : 0);
}

void query_stack(FILE *rsp)
{
	if (!rsp) return;
	
	fprintf(rsp, "[");
	for (stacking_list_t *s = stack_head; s; s = s->next) {
		if (s->node)
			fprintf(rsp, "%u", s->node->id);
		if (s->next) {
			fprintf(rsp, ",");
		}
	}
	fprintf(rsp, "]");
}

void query_subscribers(FILE *rsp)
{
	if (!rsp) return;
	
	fprintf(rsp, "[");
	for (subscriber_list_t *s = subscribe_head; s; s = s->next) {
		fprintf(rsp, "{\"fileDescriptor\": %i", s->stream ? fileno(s->stream) : -1);
		if (s->fifo_path) {
			fprintf(rsp, ",\"fifoPath\":\"%s\"", s->fifo_path);
		}
		fprintf(rsp, ",\"field\":%i,\"count\":%i}", s->field, s->count);
		if (s->next) {
			fprintf(rsp, ",");
		}
	}
	fprintf(rsp, "]");
}

int query_node_ids(coordinates_t *mon_ref, coordinates_t *desk_ref, coordinates_t* ref, 
                   coordinates_t *trg, monitor_select_t *mon_sel, desktop_select_t *desk_sel, 
                   node_select_t *sel, FILE *rsp)
{
	if (!rsp) return 0;
	
	int count = 0;
	for (monitor_t *m = mon_head; m; m = m->next) {
		coordinates_t loc = {m, NULL, NULL};
		if ((trg && trg->monitor && m != trg->monitor) ||
		    (mon_sel && !monitor_matches(&loc, mon_ref, mon_sel))) {
			continue;
		}
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			coordinates_t loc = {m, d, NULL};
			if ((trg && trg->desktop && d != trg->desktop) ||
			    (desk_sel && !desktop_matches(&loc, desk_ref, desk_sel))) {
				continue;
			}
			count += query_node_ids_in_depth(d->root, d, m, ref, trg, sel, rsp, 0);
		}
	}
	return count;
}

static int query_node_ids_in_depth(node_t *n, desktop_t *d, monitor_t *m, coordinates_t *ref,
                                   coordinates_t *trg, node_select_t *sel, FILE *rsp, int depth)
{
	if (depth > MAX_RECURSION_DEPTH || !n || !rsp)
		return 0;
		
	int count = 0;
	coordinates_t loc = {m, d, n};
	if ((!trg || !trg->node || n == trg->node) &&
	    (!sel || node_matches(&loc, ref, sel))) {
		fprintf(rsp, "0x%08X\n", n->id);
		count++;
	}
	count += query_node_ids_in_depth(n->first_child, d, m, ref, trg, sel, rsp, depth + 1);
	count += query_node_ids_in_depth(n->second_child, d, m, ref, trg, sel, rsp, depth + 1);
	
	return count;
}

int query_node_ids_in(node_t *n, desktop_t *d, monitor_t *m, coordinates_t *ref,
                      coordinates_t *trg, node_select_t *sel, FILE *rsp)
{
	return query_node_ids_in_depth(n, d, m, ref, trg, sel, rsp, 0);
}

int query_desktop_ids(coordinates_t* mon_ref, coordinates_t *ref, coordinates_t *trg,
                      monitor_select_t *mon_sel, desktop_select_t *sel, 
                      desktop_printer_t printer, FILE *rsp)
{
	if (!rsp || !printer) return 0;
	
	int count = 0;
	for (monitor_t *m = mon_head; m; m = m->next) {
		coordinates_t loc = {m, NULL, NULL};
		if ((trg && trg->monitor && m != trg->monitor) ||
		    (mon_sel && !monitor_matches(&loc, mon_ref, mon_sel))) {
			continue;
		}
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			coordinates_t loc = {m, d, NULL};
			if ((trg && trg->desktop && d != trg->desktop) ||
			    (sel && !desktop_matches(&loc, ref, sel))) {
				continue;
			}
			printer(d, rsp);
			count++;
		}
	}
	return count;
}

int query_monitor_ids(coordinates_t *ref, coordinates_t *trg, monitor_select_t *sel,
                      monitor_printer_t printer, FILE *rsp)
{
	if (!rsp || !printer) return 0;
	
	int count = 0;
	for (monitor_t *m = mon_head; m; m = m->next) {
		coordinates_t loc = {m, NULL, NULL};
		if ((trg && trg->monitor && m != trg->monitor) ||
		    (sel && !monitor_matches(&loc, ref, sel))) {
			continue;
		}
		printer(m, rsp);
		count++;
	}
	return count;
}

void fprint_monitor_id(monitor_t *m, FILE *rsp)
{
	if (!m || !rsp) return;
	fprintf(rsp, "0x%08X\n", m->id);
}

void fprint_monitor_name(monitor_t *m, FILE *rsp)
{
	if (!m || !rsp) return;
	fprintf(rsp, "%s\n", m->name);
}

void fprint_desktop_id(desktop_t *d, FILE *rsp)
{
	if (!d || !rsp) return;
	fprintf(rsp, "0x%08X\n", d->id);
}

void fprint_desktop_name(desktop_t *d, FILE *rsp)
{
	if (!d || !rsp) return;
	fprintf(rsp, "%s\n", d->name);
}

void print_ignore_request(state_transition_t st, FILE *rsp)
{
	if (!rsp) return;
	
	if (st == 0) {
		fprintf(rsp, "none");
	} else {
		unsigned int cnt = 0;
		if (st & STATE_TRANSITION_ENTER) {
			fprintf(rsp, "enter");
			cnt++;
		}
		if (st & STATE_TRANSITION_EXIT) {
			fprintf(rsp, "%sexit", cnt > 0 ? "," : "");
		}
	}
}

void print_modifier_mask(uint16_t m, FILE *rsp)
{
	if (!rsp) return;
	
	switch (m) {
		case XCB_MOD_MASK_SHIFT:
			fprintf(rsp, "shift");
			break;
		case XCB_MOD_MASK_CONTROL:
			fprintf(rsp, "control");
			break;
		case XCB_MOD_MASK_LOCK:
			fprintf(rsp, "lock");
			break;
		case XCB_MOD_MASK_1:
			fprintf(rsp, "mod1");
			break;
		case XCB_MOD_MASK_2:
			fprintf(rsp, "mod2");
			break;
		case XCB_MOD_MASK_3:
			fprintf(rsp, "mod3");
			break;
		case XCB_MOD_MASK_4:
			fprintf(rsp, "mod4");
			break;
		case XCB_MOD_MASK_5:
			fprintf(rsp, "mod5");
			break;
	}
}

void print_button_index(int8_t b, FILE *rsp)
{
	if (!rsp) return;
	
	switch (b) {
		case XCB_BUTTON_INDEX_ANY:
			fprintf(rsp, "any");
			break;
		case XCB_BUTTON_INDEX_1:
			fprintf(rsp, "button1");
			break;
		case XCB_BUTTON_INDEX_2:
			fprintf(rsp, "button2");
			break;
		case XCB_BUTTON_INDEX_3:
			fprintf(rsp, "button3");
			break;
		case -1:
			fprintf(rsp, "none");
			break;
	}
}

void print_pointer_action(pointer_action_t a, FILE *rsp)
{
	if (!rsp) return;
	
	switch (a) {
		case ACTION_MOVE:
			fprintf(rsp, "move");
			break;
		case ACTION_RESIZE_SIDE:
			fprintf(rsp, "resize_side");
			break;
		case ACTION_RESIZE_CORNER:
			fprintf(rsp, "resize_corner");
			break;
		case ACTION_FOCUS:
			fprintf(rsp, "focus");
			break;
		case ACTION_NONE:
			fprintf(rsp, "none");
			break;
	}
}

void resolve_rule_consequence(rule_consequence_t *csq)
{
	if (!csq) return;
	
	coordinates_t ref = {mon, mon ? mon->desk : NULL, mon && mon->desk ? mon->desk->focus : NULL};
	coordinates_t dst = {NULL, NULL, NULL};
	monitor_t *monitor = monitor_from_desc(csq->monitor_desc, &ref, &dst) != SELECTOR_OK ? NULL : dst.monitor;
	desktop_t *desktop = desktop_from_desc(csq->desktop_desc, &ref, &dst) != SELECTOR_OK ? NULL : dst.desktop;
	node_t *node = node_from_desc(csq->node_desc, &ref, &dst) != SELECTOR_OK ? NULL : dst.node;

#define PRINT_OBJECT_ID(name) \
	if (!name) { \
		csq->name##_desc[0] = '\0'; \
	} else { \
		snprintf(csq->name##_desc, 11, "0x%08X", name->id); \
	}
	PRINT_OBJECT_ID(monitor)
	PRINT_OBJECT_ID(desktop)
	PRINT_OBJECT_ID(node)
#undef PRINT_OBJECT_ID
}

void print_rule_consequence(char **buf, rule_consequence_t *csq)
{
	if (!buf || !csq) return;
	
	char *rect_buf = NULL;
	print_rectangle(&rect_buf, csq->rect);
	if (!rect_buf) {
		rect_buf = malloc(1);
		if (rect_buf)
			*rect_buf = '\0';
	}

	int ret = asprintf(buf, "monitor=%s desktop=%s node=%s state=%s layer=%s honor_size_hints=%s "
	                   "split_dir=%s split_ratio=%lf hidden=%s sticky=%s private=%s locked=%s "
	                   "marked=%s center=%s follow=%s manage=%s focus=%s border=%s rectangle=%s",
	        csq->monitor_desc, csq->desktop_desc, csq->node_desc,
	        csq->state ? STATE_STR(*csq->state) : "",
	        csq->layer ? LAYER_STR(*csq->layer) : "",
	        csq->honor_size_hints == HONOR_SIZE_HINTS_DEFAULT ? "" : HSH_MODE_STR(csq->honor_size_hints),
	        csq->split_dir ? SPLIT_DIR_STR(*csq->split_dir) : "", csq->split_ratio,
	        ON_OFF_STR(csq->hidden), ON_OFF_STR(csq->sticky), ON_OFF_STR(csq->private),
	        ON_OFF_STR(csq->locked), ON_OFF_STR(csq->marked), ON_OFF_STR(csq->center),
	        ON_OFF_STR(csq->follow), ON_OFF_STR(csq->manage), ON_OFF_STR(csq->focus),
	        ON_OFF_STR(csq->border), rect_buf ? rect_buf : "");
	
	free(rect_buf);
	
	if (ret < 0)
		*buf = NULL;
}

void print_rectangle(char **buf, xcb_rectangle_t *rect)
{
	if (!buf) return;
	
	if (rect) {
		if (asprintf(buf, "%hux%hu+%hi+%hi", rect->width, rect->height, rect->x, rect->y) < 0)
			*buf = NULL;
	} else {
		*buf = NULL;
	}
}

node_select_t make_node_select(void)
{
	node_select_t sel = {
		.automatic = OPTION_NONE,
		.focused = OPTION_NONE,
		.active = OPTION_NONE,
		.local = OPTION_NONE,
		.leaf = OPTION_NONE,
		.window = OPTION_NONE,
		.tiled = OPTION_NONE,
		.pseudo_tiled = OPTION_NONE,
		.floating = OPTION_NONE,
		.fullscreen = OPTION_NONE,
		.hidden = OPTION_NONE,
		.sticky = OPTION_NONE,
		.private = OPTION_NONE,
		.locked = OPTION_NONE,
		.marked = OPTION_NONE,
		.urgent = OPTION_NONE,
		.same_class = OPTION_NONE,
		.descendant_of = OPTION_NONE,
		.ancestor_of = OPTION_NONE,
		.below = OPTION_NONE,
		.normal = OPTION_NONE,
		.above = OPTION_NONE,
		.horizontal = OPTION_NONE,
		.vertical = OPTION_NONE
	};
	return sel;
}

desktop_select_t make_desktop_select(void)
{
	desktop_select_t sel = {
		.occupied = OPTION_NONE,
		.focused = OPTION_NONE,
		.active = OPTION_NONE,
		.urgent = OPTION_NONE,
		.local = OPTION_NONE,
		.tiled = OPTION_NONE,
		.monocle = OPTION_NONE,
		.user_tiled = OPTION_NONE,
		.user_monocle = OPTION_NONE
	};
	return sel;
}

monitor_select_t make_monitor_select(void)
{
	monitor_select_t sel = {
		.occupied = OPTION_NONE,
		.focused = OPTION_NONE
	};
	return sel;
}

int node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
	if (!desc || !ref || !dst)
		return SELECTOR_BAD_DESCRIPTOR;
		
	if (strlen(desc) > MAXLEN)
		return SELECTOR_BAD_DESCRIPTOR;

	dst->node = NULL;

	coordinates_t ref_copy = *ref;
	ref = &ref_copy;
	char desc_copy[MAXLEN];
	strncpy(desc_copy, desc, MAXLEN - 1);
	desc_copy[MAXLEN - 1] = '\0';
	desc = desc_copy;

	char *hash = strrchr(desc, '#');
	char *path = strrchr(desc, '@');
	char *colon = strrchr(desc, ':');

	if (hash && colon && path && path < hash && hash < colon) {
		if (path > desc && *(path - 1) == '#') {
			hash = path - 1;
		} else {
			hash = NULL;
		}
	}

	if (hash) {
		*hash = '\0';
		int ret;
		coordinates_t tmp = {mon, mon ? mon->desk : NULL, 
		                    mon && mon->desk ? mon->desk->focus : NULL};
		if ((ret = node_from_desc(desc, &tmp, ref)) == SELECTOR_OK) {
			desc = hash + 1;
		} else {
			return ret;
		}
	}

	if (colon && hash && colon < hash) {
		colon = NULL;
	}

	node_select_t sel = make_node_select();

	if (!parse_node_modifiers(colon ? colon : desc, &sel)) {
		return SELECTOR_BAD_MODIFIERS;
	}

	direction_t dir;
	cycle_dir_t cyc;
	history_dir_t hdi;
	if (parse_direction(desc, &dir)) {
		find_nearest_neighbor(ref, dst, dir, &sel);
	} else if (parse_cycle_direction(desc, &cyc)) {
		find_closest_node(ref, dst, cyc, &sel);
	} else if (parse_history_direction(desc, &hdi)) {
		history_find_node(hdi, ref, dst, &sel);
	} else if (streq("any", desc)) {
		find_any_node(ref, dst, &sel);
	} else if (streq("first_ancestor", desc)) {
		find_first_ancestor(ref, dst, &sel);
	} else if (streq("last", desc)) {
		history_find_node(HISTORY_OLDER, ref, dst, &sel);
	} else if (streq("newest", desc)) {
		history_find_newest_node(ref, dst, &sel);
	} else if (streq("biggest", desc)) {
		find_by_area(AREA_BIGGEST, ref, dst, &sel);
	} else if (streq("smallest", desc)) {
		find_by_area(AREA_SMALLEST, ref, dst, &sel);
	} else if (streq("pointed", desc)) {
		xcb_window_t win = XCB_NONE;
		query_pointer(&win, NULL);
		if (locate_leaf(win, dst) && node_matches(dst, ref, &sel)) {
			return SELECTOR_OK;
		} else {
			return SELECTOR_INVALID;
		}
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, mon ? mon->desk : NULL, 
		                    mon && mon->desk ? mon->desk->focus : NULL};
		if (node_matches(&loc, ref, &sel)) {
			*dst = loc;
		}
	} else if (*desc == '@') {
		desc++;
		*dst = *ref;
		if (colon) {
			*colon = '\0';
			int ret;
			if ((ret = desktop_from_desc(desc, ref, dst)) == SELECTOR_OK) {
				dst->node = dst->desktop ? dst->desktop->focus : NULL;
				desc = colon + 1;
			} else {
				return ret;
			}
		}
		if (*desc == '/') {
			dst->node = dst->desktop ? dst->desktop->root : NULL;
		}
		char *move = strtok(desc, PTH_TOK);
		while (move && dst->node) {
			if (streq("first", move) || streq("1", move)) {
				dst->node = dst->node->first_child;
			} else if (streq("second", move) || streq("2", move)) {
				dst->node = dst->node->second_child;
			} else if (streq("parent", move)) {
				dst->node = dst->node->parent;
			} else if (streq("brother", move)) {
				dst->node = brother_tree(dst->node);
			} else {
				direction_t dir;
				if (parse_direction(move, &dir)) {
					dst->node = find_fence(dst->node, dir);
				} else {
					return SELECTOR_BAD_DESCRIPTOR;
				}
			}
			move = strtok(NULL, PTH_TOK);
		}
		if (dst->node) {
			if (node_matches(dst, ref, &sel)) {
				return SELECTOR_OK;
			} else {
				return SELECTOR_INVALID;
			}
		} else if (dst->desktop && dst->desktop->root) {
			return SELECTOR_INVALID;
		}
		return SELECTOR_OK;
	} else {
		uint32_t id;
		if (parse_id(desc, &id)) {
			if (find_by_id(id, dst) && node_matches(dst, ref, &sel)) {
				return SELECTOR_OK;
			} else {
				return SELECTOR_INVALID;
			}
		} else {
			return SELECTOR_BAD_DESCRIPTOR;
		}
	}

	if (!dst->node) {
		return SELECTOR_INVALID;
	}

	return SELECTOR_OK;
}

int desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
	if (!desc || !ref || !dst)
		return SELECTOR_BAD_DESCRIPTOR;
		
	if (strlen(desc) > MAXLEN)
		return SELECTOR_BAD_DESCRIPTOR;

	dst->desktop = NULL;

	if (*desc == '%') {
		locate_desktop(desc + 1, dst);
		goto end;
	}

	coordinates_t ref_copy = *ref;
	ref = &ref_copy;
	char desc_copy[MAXLEN];
	strncpy(desc_copy, desc, MAXLEN - 1);
	desc_copy[MAXLEN - 1] = '\0';
	desc = desc_copy;

	char *hash = strrchr(desc, '#');
	char *colon = strrchr(desc, ':');

	if (hash && colon && hash < colon) {
		hash = NULL;
	}

	if (hash) {
		*hash = '\0';
		int ret;
		coordinates_t tmp = {mon, mon ? mon->desk : NULL, NULL};
		if ((ret = desktop_from_desc(desc, &tmp, ref)) == SELECTOR_OK) {
			desc = hash + 1;
		} else {
			return ret;
		}
	}

	if (colon && hash && colon < hash) {
		colon = NULL;
	}

	desktop_select_t sel = make_desktop_select();

	if (!parse_desktop_modifiers(colon ? colon : desc, &sel)) {
		return SELECTOR_BAD_MODIFIERS;
	}

	cycle_dir_t cyc;
	history_dir_t hdi;
	uint16_t idx;
	uint32_t id;
	if (parse_cycle_direction(desc, &cyc)) {
		find_closest_desktop(ref, dst, cyc, &sel);
	} else if (parse_history_direction(desc, &hdi)) {
		history_find_desktop(hdi, ref, dst, &sel);
	} else if (streq("any", desc)) {
		find_any_desktop(ref, dst, &sel);
	} else if (streq("last", desc)) {
		history_find_desktop(HISTORY_OLDER, ref, dst, &sel);
	} else if (streq("newest", desc)) {
		history_find_newest_desktop(ref, dst, &sel);
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, mon ? mon->desk : NULL, NULL};
		if (desktop_matches(&loc, ref, &sel)) {
			*dst = loc;
		}
	} else if (colon) {
		*colon = '\0';
		int ret;
		if ((ret = monitor_from_desc(desc, ref, dst)) == SELECTOR_OK) {
			if (streq("focused", colon + 1)) {
				coordinates_t loc = {dst->monitor, 
				                    dst->monitor ? dst->monitor->desk : NULL, NULL};
				if (desktop_matches(&loc, ref, &sel)) {
					*dst = loc;
				}
			} else if (parse_index(colon + 1, &idx)) {
				if (desktop_from_index(idx, dst, dst->monitor) && desktop_matches(dst, ref, &sel)) {
					return SELECTOR_OK;
				} else {
					return SELECTOR_INVALID;
				}
			} else {
				return SELECTOR_BAD_DESCRIPTOR;
			}
		} else {
			return ret;
		}
	} else if (parse_index(desc, &idx) && desktop_from_index(idx, dst, NULL)) {
		if (desktop_matches(dst, ref, &sel)) {
			return SELECTOR_OK;
		} else {
			return SELECTOR_INVALID;
		}
	} else if (parse_id(desc, &id) && desktop_from_id(id, dst, NULL)) {
		if (desktop_matches(dst, ref, &sel)) {
			return SELECTOR_OK;
		} else {
			return SELECTOR_INVALID;
		}
	} else {
		int hits = 0;
		if (desktop_from_name(desc, ref, dst, &sel, &hits)) {
			return SELECTOR_OK;
		} else {
			if (hits > 0) {
				return SELECTOR_INVALID;
			} else {
				return SELECTOR_BAD_DESCRIPTOR;
			}
		}
	}

end:
	if (!dst->desktop) {
		return SELECTOR_INVALID;
	}

	return SELECTOR_OK;
}

int monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
	if (!desc || !ref || !dst)
		return SELECTOR_BAD_DESCRIPTOR;
		
	if (strlen(desc) > MAXLEN)
		return SELECTOR_BAD_DESCRIPTOR;

	dst->monitor = NULL;

	if (*desc == '%') {
		locate_monitor(desc + 1, dst);
		goto end;
	}

	coordinates_t ref_copy = *ref;
	ref = &ref_copy;
	char desc_copy[MAXLEN];
	strncpy(desc_copy, desc, MAXLEN - 1);
	desc_copy[MAXLEN - 1] = '\0';
	desc = desc_copy;

	char *hash = strrchr(desc, '#');

	if (hash) {
		*hash = '\0';
		int ret;
		coordinates_t tmp = {mon, NULL, NULL};
		if ((ret = monitor_from_desc(desc, &tmp, ref)) == SELECTOR_OK) {
			desc = hash + 1;
		} else {
			return ret;
		}
	}

	monitor_select_t sel = make_monitor_select();

	if (!parse_monitor_modifiers(desc, &sel)) {
		return SELECTOR_BAD_MODIFIERS;
	}

	direction_t dir;
	cycle_dir_t cyc;
	history_dir_t hdi;
	uint16_t idx;
	uint32_t id;
	if (parse_direction(desc, &dir)) {
		dst->monitor = nearest_monitor(ref->monitor, dir, &sel);
	} else if (parse_cycle_direction(desc, &cyc)) {
		dst->monitor = closest_monitor(ref->monitor, cyc, &sel);
	} else if (parse_history_direction(desc, &hdi)) {
		history_find_monitor(hdi, ref, dst, &sel);
	} else if (streq("any", desc)) {
		find_any_monitor(ref, dst, &sel);
	} else if (streq("last", desc)) {
		history_find_monitor(HISTORY_OLDER, ref, dst, &sel);
	} else if (streq("newest", desc)) {
		history_find_newest_monitor(ref, dst, &sel);
	} else if (streq("primary", desc)) {
		if (pri_mon) {
			coordinates_t loc = {pri_mon, NULL, NULL};
			if (monitor_matches(&loc, ref, &sel)) {
				dst->monitor = pri_mon;
			}
		}
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, NULL, NULL};
		if (monitor_matches(&loc, ref, &sel)) {
			dst->monitor = mon;
		}
	} else if (streq("pointed", desc)) {
		xcb_point_t pointer;
		query_pointer(NULL, &pointer);
		for (monitor_t *m = mon_head; m; m = m->next) {
			if (is_inside(pointer, m->rectangle)) {
				dst->monitor = m;
				break;
			}
		}
	} else if (parse_index(desc, &idx) && monitor_from_index(idx, dst)) {
		if (monitor_matches(dst, ref, &sel)) {
			return SELECTOR_OK;
		} else {
			return SELECTOR_INVALID;
		}
	} else if (parse_id(desc, &id) && monitor_from_id(id, dst)) {
		if (monitor_matches(dst, ref, &sel)) {
			return SELECTOR_OK;
		} else {
			return SELECTOR_INVALID;
		}
	} else {
		if (locate_monitor(desc, dst)) {
			if (monitor_matches(dst, ref, &sel)) {
				return SELECTOR_OK;
			} else {
				return SELECTOR_INVALID;
			}
		} else {
			return SELECTOR_BAD_DESCRIPTOR;
		}
	}

end:
	if (!dst->monitor) {
		return SELECTOR_INVALID;
	}

	return SELECTOR_OK;
}

bool locate_leaf(xcb_window_t win, coordinates_t *loc)
{
	if (!loc) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			for (node_t *n = first_extrema(d->root); n; n = next_leaf(n, d->root)) {
				if (n->id == win) {
					loc->monitor = m;
					loc->desktop = d;
					loc->node = n;
					return true;
				}
			}
		}
	}
	return false;
}

bool locate_window(xcb_window_t win, coordinates_t *loc)
{
	if (!loc) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			for (node_t *n = first_extrema(d->root); n; n = next_leaf(n, d->root)) {
				if (!n->client)
					continue;
				if (n->id == win) {
					loc->monitor = m;
					loc->desktop = d;
					loc->node = n;
					return true;
				}
			}
		}
	}
	return false;
}

bool locate_desktop(char *name, coordinates_t *loc)
{
	if (!name || !loc) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			if (streq(d->name, name)) {
				loc->monitor = m;
				loc->desktop = d;
				return true;
			}
		}
	}
	return false;
}

bool locate_monitor(char *name, coordinates_t *loc)
{
	if (!name || !loc) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		if (streq(m->name, name)) {
			loc->monitor = m;
			return true;
		}
	}
	return false;
}

bool desktop_from_id(uint32_t id, coordinates_t *loc, monitor_t *mm)
{
	if (!loc) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		if (mm && m != mm)
			continue;
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			if (d->id == id) {
				loc->monitor = m;
				loc->desktop = d;
				loc->node = NULL;
				return true;
			}
		}
	}
	return false;
}

bool desktop_from_name(char *name, coordinates_t *ref, coordinates_t *dst, 
                       desktop_select_t *sel, int *hits)
{
	if (!name || !dst) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			if (streq(d->name, name)) {
				if (hits)
					(*hits)++;
				coordinates_t loc = {m, d, NULL};
				if (desktop_matches(&loc, ref, sel)) {
					dst->monitor = m;
					dst->desktop = d;
					return true;
				}
			}
		}
	}
	return false;
}

bool desktop_from_index(uint16_t idx, coordinates_t *loc, monitor_t *mm)
{
	if (!loc || idx == 0) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		if (mm && m != mm)
			continue;
		for (desktop_t *d = m->desk_head; d; d = d->next, idx--) {
			if (idx == 1) {
				loc->monitor = m;
				loc->desktop = d;
				loc->node = NULL;
				return true;
			}
		}
	}
	return false;
}

bool monitor_from_id(uint32_t id, coordinates_t *loc)
{
	if (!loc) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next) {
		if (m->id == id) {
			loc->monitor = m;
			loc->desktop = NULL;
			loc->node = NULL;
			return true;
		}
	}
	return false;
}

bool monitor_from_index(int idx, coordinates_t *loc)
{
	if (!loc || idx <= 0) return false;
	
	for (monitor_t *m = mon_head; m; m = m->next, idx--) {
		if (idx == 1) {
			loc->monitor = m;
			loc->desktop = NULL;
			loc->node = NULL;
			return true;
		}
	}
	return false;
}

bool node_matches(coordinates_t *loc, coordinates_t *ref, node_select_t *sel)
{
	if (!loc || !loc->node || !sel)
		return false;

	if (sel->focused != OPTION_NONE &&
	    loc->node != (mon && mon->desk ? mon->desk->focus : NULL)
	    ? sel->focused == OPTION_TRUE
	    : sel->focused == OPTION_FALSE) {
		return false;
	}

	if (sel->active != OPTION_NONE &&
	    loc->node != (loc->desktop ? loc->desktop->focus : NULL)
	    ? sel->active == OPTION_TRUE
	    : sel->active == OPTION_FALSE) {
		return false;
	}

	if (sel->automatic != OPTION_NONE &&
	    loc->node->presel
	    ? sel->automatic == OPTION_FALSE
	    : sel->automatic == OPTION_TRUE) {
		return false;
	}

	if (sel->local != OPTION_NONE &&
	    loc->desktop != (ref ? ref->desktop : NULL)
	    ? sel->local == OPTION_TRUE
	    : sel->local == OPTION_FALSE) {
		return false;
	}

	if (sel->active != OPTION_NONE &&
	    loc->desktop != (loc->monitor ? loc->monitor->desk : NULL)
	    ? sel->active == OPTION_TRUE
	    : sel->active == OPTION_FALSE) {
		return false;
	}

	if (sel->leaf != OPTION_NONE &&
	    !is_leaf(loc->node)
	    ? sel->leaf == OPTION_TRUE
	    : sel->leaf == OPTION_FALSE) {
		return false;
	}

	if (sel->window != OPTION_NONE &&
	    !loc->node->client
	    ? sel->window == OPTION_TRUE
	    : sel->window == OPTION_FALSE) {
		return false;
	}

#define NFLAG(p) \
	if (sel->p != OPTION_NONE && \
	    !loc->node->p \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	NFLAG(hidden)
	NFLAG(sticky)
	NFLAG(private)
	NFLAG(locked)
	NFLAG(marked)
#undef NFLAG

#define NSPLIT(p, e) \
	if (sel->p != OPTION_NONE && \
	    loc->node->split_type != e \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	NSPLIT(horizontal, TYPE_HORIZONTAL)
	NSPLIT(vertical, TYPE_VERTICAL)
#undef NSPLIT

	if (sel->descendant_of != OPTION_NONE &&
	    !is_descendant(loc->node, ref ? ref->node : NULL)
	    ? sel->descendant_of == OPTION_TRUE
	    : sel->descendant_of == OPTION_FALSE) {
		return false;
	}

	if (sel->ancestor_of != OPTION_NONE &&
	    !is_descendant(ref ? ref->node : NULL, loc->node)
	    ? sel->ancestor_of == OPTION_TRUE
	    : sel->ancestor_of == OPTION_FALSE) {
		return false;
	}

	if (!loc->node->client) {
		if (sel->same_class == OPTION_TRUE ||
		    sel->tiled == OPTION_TRUE ||
		    sel->pseudo_tiled == OPTION_TRUE ||
		    sel->floating == OPTION_TRUE ||
		    sel->fullscreen == OPTION_TRUE ||
		    sel->below == OPTION_TRUE ||
		    sel->normal == OPTION_TRUE ||
		    sel->above == OPTION_TRUE ||
		    sel->urgent == OPTION_TRUE) {
			return false;
		}
		return true;
	}

	if (ref && ref->node && ref->node->client &&
	    sel->same_class != OPTION_NONE &&
	    streq(loc->node->client->class_name, ref->node->client->class_name)
	    ? sel->same_class == OPTION_FALSE
	    : sel->same_class == OPTION_TRUE) {
		return false;
	}

#define WSTATE(p, e) \
	if (sel->p != OPTION_NONE && \
	    loc->node->client->state != e \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	WSTATE(tiled, STATE_TILED)
	WSTATE(pseudo_tiled, STATE_PSEUDO_TILED)
	WSTATE(floating, STATE_FLOATING)
	WSTATE(fullscreen, STATE_FULLSCREEN)
#undef WSTATE

#define WLAYER(p, e) \
	if (sel->p != OPTION_NONE && \
	    loc->node->client->layer != e \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	WLAYER(below, LAYER_BELOW)
	WLAYER(normal, LAYER_NORMAL)
	WLAYER(above, LAYER_ABOVE)
#undef WLAYER

#define WFLAG(p) \
	if (sel->p != OPTION_NONE && \
	    !loc->node->client->p \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	WFLAG(urgent)
#undef WFLAG

	return true;
}

bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t *sel)
{
	if (!loc || !loc->desktop || !sel)
		return false;

	if (sel->occupied != OPTION_NONE &&
	    !loc->desktop->root
	    ? sel->occupied == OPTION_TRUE
	    : sel->occupied == OPTION_FALSE) {
		return false;
	}

	if (sel->focused != OPTION_NONE &&
	    loc->desktop != (mon ? mon->desk : NULL)
	    ? sel->focused == OPTION_TRUE
	    : sel->focused == OPTION_FALSE) {
		return false;
	}

	if (sel->active != OPTION_NONE &&
	    loc->desktop != (loc->monitor ? loc->monitor->desk : NULL)
	    ? sel->active == OPTION_TRUE
	    : sel->active == OPTION_FALSE) {
		return false;
	}

	if (sel->urgent != OPTION_NONE &&
	    !is_urgent(loc->desktop)
	    ? sel->urgent == OPTION_TRUE
	    : sel->urgent == OPTION_FALSE) {
		return false;
	}

	if (sel->local != OPTION_NONE &&
	    loc->monitor != (ref && ref->monitor ? ref->monitor : NULL)
	    ? sel->local == OPTION_TRUE
	    : sel->local == OPTION_FALSE) {
		return false;
	}

#define DLAYOUT(p, e) \
	if (sel->p != OPTION_NONE && \
	    loc->desktop->layout != e \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	DLAYOUT(tiled, LAYOUT_TILED)
	DLAYOUT(monocle, LAYOUT_MONOCLE)
#undef DLAYOUT

#define DUSERLAYOUT(p, e) \
	if (sel->p != OPTION_NONE && \
	    loc->desktop->user_layout != e \
	    ? sel->p == OPTION_TRUE \
	    : sel->p == OPTION_FALSE) { \
		return false; \
	}
	DUSERLAYOUT(user_tiled, LAYOUT_TILED)
	DUSERLAYOUT(user_monocle, LAYOUT_MONOCLE)
#undef DUSERLAYOUT

	return true;
}

bool monitor_matches(coordinates_t *loc, __attribute__((unused)) coordinates_t *ref, monitor_select_t *sel)
{
	if (!loc || !loc->monitor || !sel)
		return false;

	if (sel->occupied != OPTION_NONE &&
	    (!loc->monitor->desk || !loc->monitor->desk->root)
	    ? sel->occupied == OPTION_TRUE
	    : sel->occupied == OPTION_FALSE) {
		return false;
	}

	if (sel->focused != OPTION_NONE &&
	    loc->monitor != mon
	    ? sel->focused == OPTION_TRUE
	    : sel->focused == OPTION_FALSE) {
		return false;
	}

	return true;
}
