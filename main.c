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

static PangoFontDescription *fontDesc = 0;

struct BlockWidthData {
    int id;
    int width;
};

struct BlockWidthData *blkData = 0;
int blkCount = 0;

#define S(n, t, d, ...) \
    .n = { \
        .name = #n, \
        .type = t, \
        .desc = d, \
        .def.t = __VA_ARGS__, \
        .val.t = __VA_ARGS__, \
    },

struct Settings {
    struct Setting reset;
} settings = {
    S(reset, INT, "Resets the width of the given block", -1)
};

static void setupFont() {
    struct BarSettings *barSettings = blockbarGetSettings();

    if (fontDesc) {
        pango_font_description_free(fontDesc);
    }

    if (barSettings->font.val.STR) {
        fontDesc = pango_font_description_from_string(barSettings->font.val.STR);
    }
}

int init(struct ModuleData *data) {
    data->name = "constwidth";
    data->settings = (struct Setting *) &settings;
    data->settingCount = 1;

    setupFont();

    return 0;
}

void blockAdd(struct Block *blk) {
    blkData = realloc(blkData, sizeof(struct BlockWidthData) * ++blkCount);
    blkData[blkCount-1].id = blk->id;
    blkData[blkCount-1].width = 0;
}

void blockRemove(struct Block *blk) {
    for (int i = 0; i < blkCount; i++) {
        if (blkData[i].id == blk->id) {
            blkData[i].id = 0;
            blkCount--;
        }
    }
}

void unload() {
    if (blkData) {
        free(blkData);
    }
}

void settingUpdate(struct Setting *setting) {
    struct BarSettings *barSettings = blockbarGetSettings();

    if (setting == &(barSettings->font)) {
        setupFont();
    } else if (setting == &(settings.reset)) {
        for (int i = 0; i < blkCount; i++) {
            if (blkData[i].id == setting->val.INT) {
                blkData[i].width = 0;
                break;
            }
        }

        setting->val.INT = -1;
    }
}

int exec(struct Block *blk, int bar, struct Click *cd) {
    (void) blk;
    (void) bar;

    if (cd) {
        if (cd->button <= 0) {
            return 1;
        }
    }

    return 0;
}

static void drawString(cairo_t *ctx, char *str, color col, int *w) {
    struct BarSettings *barSettings = blockbarGetSettings();

    PangoLayout *layout = pango_cairo_create_layout(ctx);
    pango_layout_set_font_description(layout, fontDesc);
    pango_layout_set_markup(layout, str, -1);

    int width, height;

    pango_layout_get_pixel_size(layout, &width, &height);

    if (*w < width) {
        *w = width;
    }

    cairo_move_to(ctx, *w / 2 - width / 2, barSettings->height.val.INT / 2 - height / 2);

    cairo_set_source_rgba(ctx,
                          col[0] / 255.f,
                          col[1] / 255.f,
                          col[2] / 255.f,
                          col[3] / 255.f);

    pango_cairo_show_layout(ctx, layout);

    g_object_unref(layout);
}

int render(cairo_t *ctx, struct Block *blk, int bar) {
    struct BarSettings *barSettings = blockbarGetSettings();

    char *execdata;

    if (blk->eachmon) {
        execdata = blk->data[bar].execData;
    } else {
        execdata = blk->data->execData;
    }

    if (!execdata) {
        return 0;
    }

    struct BlockWidthData *wdata = 0;

    for (int i = 0; i < blkCount; i++) {
        if (blkData[i].id == blk->id) {
            wdata = &blkData[i];
            break;
        }
    }

    if (wdata == 0) {
        return 0;
    }

    char *data = malloc(strlen(execdata) + 1);
    strcpy(data, execdata);

    color col;
    memcpy(col, barSettings->foreground.val.COL, sizeof(color));

    int len = strlen(data);

    for (int i = 0; i < len; i++) {
        if (data[i] == '\0') {
            break;
        }

        if (data[i] == '\n') {
            if (data[i + 1] == '#') {
                char str [9] = "00000000";
                int colLen = len - i - 2;
                if (colLen == 3 || colLen == 4 || colLen == 6 || colLen == 8) {
                    strncpy(str, data+i+2, colLen+1);
                    blockbarParseColorString(str, col);
                }
            }
            data[i] = 0;
        }
    }

    drawString(ctx, data, col, &(wdata->width));

    free(data);

    return wdata->width;
}
