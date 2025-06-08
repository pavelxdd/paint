#include "palette.h"
#include "color_utils.h"
#include "ui_constants.h" /* layout constants */

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
   Internal helpers
   -------------------------------------------------------------------------- */

#define MIN_DYNAMIC_COLOR_ROWS 2
#define MAX_DYNAMIC_COLOR_ROWS 6
#define MIN_DYNAMIC_EMOJI_ROWS 1
#define MAX_DYNAMIC_EMOJI_ROWS 2

/* Keep reasonable canvas height when adjusting palette rows */
#define MIN_CANVAS_HEIGHT_FOR_PALETTE_CALC (PALETTE_HEIGHT * 10)

/* Decide how many colour / emoji rows fit given current window height. */
static void palette_calculate_and_set_dynamic_rows(Palette *p, int window_h)
{
    for (int c_rows = MAX_DYNAMIC_COLOR_ROWS; c_rows >= MIN_DYNAMIC_COLOR_ROWS; --c_rows) {
        for (int e_rows = MAX_DYNAMIC_EMOJI_ROWS; e_rows >= MIN_DYNAMIC_EMOJI_ROWS; --e_rows) {
            int rows = c_rows + e_rows;
            int palette_h = rows * PALETTE_HEIGHT;

            if (c_rows && e_rows && COLOR_EMOJI_SEPARATOR_HEIGHT) {
                palette_h += COLOR_EMOJI_SEPARATOR_HEIGHT;
            }

            // The main UI block is the palette plus a separator above it.
            // The tool selectors float on the canvas and don't reduce the "drawable" area for
            // this calculation.
            int total_palette_ui_h = palette_h;
            if (palette_h > 0) {
                total_palette_ui_h += TOOL_SELECTOR_SEPARATOR_HEIGHT;
            }
            int remaining_canvas_h = window_h - total_palette_ui_h;

            if (remaining_canvas_h >= MIN_CANVAS_HEIGHT_FOR_PALETTE_CALC) {
                p->color_rows = c_rows;
                p->emoji_rows = e_rows;
                p->total_rows = rows;
                return;
            }
        }
    }

    /* Fallback minimum */
    p->color_rows = MIN_DYNAMIC_COLOR_ROWS;
    p->emoji_rows = MIN_DYNAMIC_EMOJI_ROWS;
    p->total_rows = p->color_rows + p->emoji_rows;
}

/* Fill the colour swatch array (HSV rows + grayscale row). */
static void fill_palette_colors(Palette *p)
{
    if (p->color_rows == 0) {
        return;
    }

    const int hsv_rows = p->color_rows - 1; /* last row is grayscale */

    const float V_DARK_START = 0.5f;
    const float V_VIVID = 1.0f;
    const float S_VIVID = 1.0f;
    const float S_PASTEL_END = 0.4f;
    const int vivid_row_idx = (hsv_rows > 0) ? hsv_rows / 2 : -1;

    for (int col = 0; col < p->cols; ++col) {
        float hue = (p->cols == 1) ? 0.0f : 360.0f * col / (p->cols - 1);

        for (int row = 0; row < p->color_rows; ++row) {
            int pos = row * p->cols + col;

            if (row < hsv_rows) { /* HSV rows */
                float s = S_VIVID, v = V_VIVID;

                if (hsv_rows > 1 && row < vivid_row_idx) { /* darker */
                    v = V_DARK_START +
                        ((float)row / vivid_row_idx) * (V_VIVID - V_DARK_START);
                } else if (hsv_rows > 1 && row > vivid_row_idx) { /* pastel */
                    int steps = hsv_rows - 1 - vivid_row_idx;
                    int cur = row - vivid_row_idx;
                    s = S_VIVID - ((float)cur / steps) * (S_VIVID - S_PASTEL_END);
                }

                p->colors[pos] = hsv_to_rgb(hue, s, v);
            } else { /* grayscale */
                float t = (p->cols == 1) ? 0.0f : (float)col / (p->cols - 1);
                Uint8 g = lroundf((1.0f - t) * 255.0f);
                p->colors[pos] = (SDL_Color){g, g, g, 255};
            }
        }
    }
}

/* --------------------------------------------------------------------------
   Public API implementation
   -------------------------------------------------------------------------- */

Palette *palette_create(SDL_Renderer *ren, int window_w, int window_h)
{
    Palette *p = malloc(sizeof(Palette));
    if (!p) {
        SDL_Log("Failed to allocate Palette");
        return NULL;
    }

    p->emoji_renderer_instance = emoji_renderer_create(ren);
    if (!p->emoji_renderer_instance) {
        SDL_Log("Failed to create EmojiRenderer");
        free(p);
        return NULL;
    }

    p->colors = NULL;
    palette_recreate(p, window_w, window_h);
    return p;
}

void palette_destroy(Palette *p)
{
    if (!p) {
        return;
    }

    free(p->colors);
    emoji_renderer_destroy(p->emoji_renderer_instance);
    free(p);
}

void palette_recreate(Palette *p, int window_w, int window_h)
{
    palette_calculate_and_set_dynamic_rows(p, window_h);

    p->cols = window_w / PALETTE_CELL_MIN_SIZE;
    if (p->cols < 1) {
        p->cols = 1;
    }

    p->total_color_cells = p->cols * p->color_rows;

    if (p->color_rows) {
        p->colors = realloc(p->colors, sizeof(SDL_Color) * p->total_color_cells);
        if (p->colors) {
            fill_palette_colors(p);
        } else {
            SDL_Log("Palette: realloc for colours failed");
            p->total_color_cells = 0;
        }
    } else {
        free(p->colors);
        p->colors = NULL;
        p->total_color_cells = 0;
    }

    p->total_emoji_cells_to_display = p->cols * p->emoji_rows;
    p->total_cells = p->total_color_cells + p->total_emoji_cells_to_display;

    if (p->emoji_renderer_instance) {
        emoji_renderer_shuffle_and_render_all(p->emoji_renderer_instance);
    }
}
