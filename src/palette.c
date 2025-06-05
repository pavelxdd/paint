#include "palette.h"
#include "ui_constants.h"      /* layout constants */
#include "emoji_renderer.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* --------------------------------------------------------------------------
   Internal helpers
   -------------------------------------------------------------------------- */

#define MIN_DYNAMIC_COLOR_ROWS 2
#define MAX_DYNAMIC_COLOR_ROWS 6
#define MIN_DYNAMIC_EMOJI_ROWS 1
#define MAX_DYNAMIC_EMOJI_ROWS 2

/* Keep reasonable canvas height when adjusting palette rows */
#define MIN_CANVAS_HEIGHT_FOR_PALETTE_CALC (PALETTE_HEIGHT * 10)

/* HSV→RGB conversion (0 ≤ h < 360) */
static SDL_Color hsv_to_rgb(float h, float s, float v)
{
    SDL_Color rgb = { 0, 0, 0, 255 };

    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    if (s == 0.0f) {          /* gray */
        rgb.r = rgb.g = rgb.b = lroundf(v * 255.0f);
        return rgb;
    }

    float c       = v * s;
    float h_prime = fmodf(h / 60.0f, 6.0f);
    if (h_prime < 0) h_prime += 6.0f;
    float x_val   = c * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));

    float r1 = 0, g1 = 0, b1 = 0;
    if      (h_prime < 1) { r1 = c; g1 = x_val; }
    else if (h_prime < 2) { r1 = x_val; g1 = c; }
    else if (h_prime < 3) { g1 = c; b1 = x_val; }
    else if (h_prime < 4) { g1 = x_val; b1 = c; }
    else if (h_prime < 5) { r1 = x_val; b1 = c; }
    else                  { r1 = c;            b1 = x_val; }

    float m = v - c;
    r1 += m; g1 += m; b1 += m;

    rgb.r = lroundf(r1 * 255.0f);
    rgb.g = lroundf(g1 * 255.0f);
    rgb.b = lroundf(b1 * 255.0f);
    return rgb;
}

/* Decide how many colour / emoji rows fit given current window height. */
static void palette_calculate_and_set_dynamic_rows(Palette *p, int window_h)
{
    for (int c_rows = MAX_DYNAMIC_COLOR_ROWS; c_rows >= MIN_DYNAMIC_COLOR_ROWS; --c_rows) {
        for (int e_rows = MAX_DYNAMIC_EMOJI_ROWS; e_rows >= MIN_DYNAMIC_EMOJI_ROWS; --e_rows) {
            int rows      = c_rows + e_rows;
            int palette_h = rows * PALETTE_HEIGHT;

            if (c_rows && e_rows && COLOR_EMOJI_SEPARATOR_HEIGHT)
                palette_h += COLOR_EMOJI_SEPARATOR_HEIGHT;

            int total_ui_h      = palette_h + CANVAS_PALETTE_SEPARATOR_HEIGHT;
            int remaining_canvas_h = window_h - total_ui_h;

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
    if (p->color_rows == 0) return;

    const int hsv_rows = p->color_rows - 1; /* last row is grayscale */

    const float V_DARK_START   = 0.5f;
    const float V_VIVID        = 1.0f;
    const float S_VIVID        = 1.0f;
    const float S_PASTEL_END   = 0.4f;
    const int   vivid_row_idx  = (hsv_rows > 0) ? hsv_rows / 2 : -1;

    for (int col = 0; col < p->cols; ++col) {
        float hue = (p->cols == 1) ? 0.0f : 360.0f * col / (p->cols - 1);

        for (int row = 0; row < p->color_rows; ++row) {
            int pos = row * p->cols + col;

            if (row < hsv_rows) {                   /* HSV rows */
                float s = S_VIVID, v = V_VIVID;

                if (hsv_rows > 1 && row < vivid_row_idx) {                /* darker */
                    v = V_DARK_START + ((float)row / vivid_row_idx) * (V_VIVID - V_DARK_START);
                } else if (hsv_rows > 1 && row > vivid_row_idx) {         /* pastel */
                    int steps = hsv_rows - 1 - vivid_row_idx;
                    int cur   = row - vivid_row_idx;
                    s = S_VIVID - ((float)cur / steps) * (S_VIVID - S_PASTEL_END);
                }

                p->colors[pos] = hsv_to_rgb(hue, s, v);
            } else {                                   /* grayscale */
                float t   = (p->cols == 1) ? 0.0f : (float)col / (p->cols - 1);
                Uint8 g   = lroundf((1.0f - t) * 255.0f);
                p->colors[pos] = (SDL_Color){ g, g, g, 255 };
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
    if (!p) return;

    free(p->colors);
    emoji_renderer_destroy(p->emoji_renderer_instance);
    free(p);
}

void palette_recreate(Palette *p, int window_w, int window_h)
{
    palette_calculate_and_set_dynamic_rows(p, window_h);

    p->cols = window_w / PALETTE_CELL_MIN_SIZE;
    if (p->cols < 1) p->cols = 1;

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
        p->colors            = NULL;
        p->total_color_cells = 0;
    }

    p->total_emoji_cells_to_display = p->cols * p->emoji_rows;
    p->total_cells                  = p->total_color_cells + p->total_emoji_cells_to_display;

    if (p->emoji_renderer_instance)
        emoji_renderer_shuffle_and_render_all(p->emoji_renderer_instance);
}

/* --------------------------------------------------------------------------
   Interaction & queries
   -------------------------------------------------------------------------- */

int palette_hit_test(const Palette *p, int mx, int my, int window_w, int palette_start_y)
{
    if (p->cols == 0)
        return -1;

    int colors_h = p->color_rows * PALETTE_HEIGHT;
    int sep_h    = (p->emoji_rows && p->color_rows) ? COLOR_EMOJI_SEPARATOR_HEIGHT : 0;
    int emojis_h = p->emoji_rows * PALETTE_HEIGHT;
    int total_h  = colors_h + sep_h + emojis_h;

    if (my < palette_start_y || my >= palette_start_y + total_h)
        return -1;

    /* column */
    int cell_w     = window_w / p->cols;
    int cell_w_rem = window_w % p->cols;
    int cx_edge    = 0;
    int clicked_c  = -1;
    for (int c_idx = 0; c_idx < p->cols; ++c_idx) {
        int w = cell_w + (c_idx < cell_w_rem ? 1 : 0);
        if (mx < cx_edge + w) { clicked_c = c_idx; break; }
        cx_edge += w;
    }
    if (clicked_c == -1) return -1;

    /* colour rows */
    if (my < palette_start_y + colors_h) {
        int r = (my - palette_start_y) / PALETTE_HEIGHT;
        return r * p->cols + clicked_c;
    }

    /* emoji rows */
    int emoji_start_y = palette_start_y + colors_h + sep_h;
    if (my >= emoji_start_y && my < emoji_start_y + emojis_h) {
        int grid_idx = (my - emoji_start_y) / PALETTE_HEIGHT * p->cols + clicked_c;
        int flat_idx = p->total_color_cells + grid_idx;
        if (flat_idx < p->total_cells) return flat_idx;
    }
    return -1;
}

SDL_Color palette_get_color(const Palette *p, int flat_index)
{
    return palette_is_color_index(p, flat_index) && p->colors
         ? p->colors[flat_index]
         : (SDL_Color){ 0, 0, 0, 255 };
}

SDL_bool palette_get_emoji_info(const Palette *p, int flat_index,
                                SDL_Texture **tex, int *w, int *h)
{
    if (!palette_is_emoji_index(p, flat_index) || !p->emoji_renderer_instance)
        return SDL_FALSE;

    int arr_idx = palette_get_emoji_array_idx_from_flat_idx(p, flat_index);
    return arr_idx == -1 ? SDL_FALSE
                         : emoji_renderer_get_texture_info(p->emoji_renderer_instance,
                                                           arr_idx, tex, w, h);
}

SDL_bool palette_is_color_index(const Palette *p, int flat_index)
{
    return flat_index >= 0 && flat_index < p->total_color_cells;
}

SDL_bool palette_is_emoji_index(const Palette *p, int flat_index)
{
    return flat_index >= p->total_color_cells && flat_index < p->total_cells;
}

int palette_get_emoji_array_idx_from_flat_idx(const Palette *p, int flat_index)
{
    if (!palette_is_emoji_index(p, flat_index) || !p->emoji_renderer_instance)
        return -1;

    int n = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);
    if (n == 0) return -1;

    int grid_idx = flat_index - p->total_color_cells;
    return grid_idx % n;
}
