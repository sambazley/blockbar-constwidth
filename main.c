/* Copyright (C) 2019 Sam Bazley
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <blockbar/blockbar.h>
#include <cairo.h>
#include <stdlib.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static PangoFontDescription *font_desc = 0;

struct block_width_data {
	int id;
	int width;
};

struct block_width_data *blk_data = 0;
int blk_count = 0;

#define S(n, t, d, ...) \
	.n = { \
		.name = #n, \
		.type = t, \
		.desc = d, \
		.def.t = __VA_ARGS__, \
		.val.t = __VA_ARGS__, \
	},

struct settings {
	struct setting reset;
} settings = {
	S(reset, INT, "Resets the width of the given block", -1)
};

static void setup_font()
{
	struct bar_settings *bar_settings = blockbar_get_settings();

	if (font_desc) {
		pango_font_description_free(font_desc);
	}

	if (bar_settings->font.val.STR) {
		font_desc = pango_font_description_from_string(bar_settings->font.val.STR);
	}
}

int init(struct module_data *data)
{
	data->name = "constwidth";
	data->settings = (struct setting *) &settings;
	data->setting_count = 1;

	setup_font();

	return 0;
}

void block_add(struct block *blk)
{
	blk_data = realloc(blk_data, sizeof(struct block_width_data) * ++blk_count);
	blk_data[blk_count-1].id = blk->id;
	blk_data[blk_count-1].width = 0;
}

void block_remove(struct block *blk)
{
	for (int i = 0; i < blk_count; i++) {
		if (blk_data[i].id == blk->id) {
			blk_data[i].id = 0;
			blk_count--;
		}
	}
}

void unload()
{
	if (blk_data) {
		free(blk_data);
	}
}

void setting_update(struct setting *setting)
{
	struct bar_settings *bar_settings = blockbar_get_settings();

	if (setting == &(bar_settings->font)) {
		setup_font();
	} else if (setting == &(settings.reset)) {
		for (int i = 0; i < blk_count; i++) {
			if (blk_data[i].id == setting->val.INT) {
				blk_data[i].width = 0;
				break;
			}
		}

		setting->val.INT = -1;
	}
}

int exec(struct block *blk, int bar, struct click *cd)
{
	(void) blk;
	(void) bar;

	if (cd) {
		if (cd->button <= 0) {
			return 1;
		}
	}

	return 0;
}

static void draw_string(cairo_t *ctx, char *str, color col, int *w)
{
	struct bar_settings *bar_settings = blockbar_get_settings();

	PangoLayout *layout = pango_cairo_create_layout(ctx);
	pango_layout_set_font_description(layout, font_desc);
	pango_layout_set_markup(layout, str, -1);

	int width, height;

	pango_layout_get_pixel_size(layout, &width, &height);

	if (*w < width) {
		*w = width;
	}

	cairo_move_to(ctx, *w / 2 - width / 2, bar_settings->height.val.INT / 2 - height / 2);

	cairo_set_source_rgba(ctx,
						  col[0] / 255.f,
						  col[1] / 255.f,
						  col[2] / 255.f,
						  col[3] / 255.f);

	pango_cairo_show_layout(ctx, layout);

	g_object_unref(layout);
}

int render(cairo_t *ctx, struct block *blk, int bar)
{
	struct bar_settings *bar_settings = blockbar_get_settings();

	char *execdata;

	if (blk->eachmon) {
		execdata = blk->data[bar].exec_data;
	} else {
		execdata = blk->data->exec_data;
	}

	if (!execdata) {
		return 0;
	}

	struct block_width_data *wdata = 0;

	for (int i = 0; i < blk_count; i++) {
		if (blk_data[i].id == blk->id) {
			wdata = &blk_data[i];
			break;
		}
	}

	if (wdata == 0) {
		return 0;
	}

	char *data = malloc(strlen(execdata) + 1);
	strcpy(data, execdata);

	color col;
	memcpy(col, bar_settings->foreground.val.COL, sizeof(color));

	int len = strlen(data);

	for (int i = 0; i < len; i++) {
		if (data[i] == '\0') {
			break;
		}

		if (data[i] == '\n') {
			if (data[i + 1] == '#') {
				char str [9] = "00000000";
				int col_len = len - i - 2;
				if (col_len == 3 || col_len == 4 || col_len == 6 || col_len == 8) {
					strncpy(str, data+i+2, col_len+1);
					blockbar_parse_color_string(str, col);
				}
			}
			data[i] = 0;
		}
	}

	draw_string(ctx, data, col, &(wdata->width));

	free(data);

	return wdata->width;
}
