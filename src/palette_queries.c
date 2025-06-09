#include "palette.h"
#include <stdbool.h>

int palette_hit_test(const Palette *p,
                     int mx,
                     int my,
                     int window_w,
                     int palette_start_y,
                     bool show_colors,
                     bool show_emojis)
{
    if (p->cols == 0) {
        return -1;
    }

    int colors_h = show_colors ? p->color_rows * PALETTE_HEIGHT : 0;
    int sep_h = (show_colors && show_emojis && p->color_rows > 0 && p->emoji_rows > 0)
                    ? COLOR_EMOJI_SEPARATOR_HEIGHT
                    : 0;
    int emojis_h = show_emojis ? p->emoji_rows * PALETTE_HEIGHT : 0;
    int total_h = colors_h + sep_h + emojis_h;

    if (my < palette_start_y || my >= palette_start_y + total_h) {
        return -1;
    }

    /* column */
    int cell_w = window_w / p->cols;
    int cell_w_rem = window_w % p->cols;
    int cx_edge = 0;
    int clicked_c = -1;
    for (int c_idx = 0; c_idx < p->cols; ++c_idx) {
        int w = cell_w + (c_idx < cell_w_rem ? 1 : 0);
        if (mx < cx_edge + w) {
            clicked_c = c_idx;
            break;
        }
        cx_edge += w;
    }
    if (clicked_c == -1) {
        return -1;
    }

    /* colour rows */
    if (show_colors && my < palette_start_y + colors_h) {
        int r = (my - palette_start_y) / PALETTE_HEIGHT;
        return r * p->cols + clicked_c;
    }

    /* emoji rows */
    int emoji_start_y = palette_start_y + colors_h + sep_h;
    if (show_emojis && my >= emoji_start_y && my < emoji_start_y + emojis_h) {
        int grid_idx = (my - emoji_start_y) / PALETTE_HEIGHT * p->cols + clicked_c;
        int flat_idx = p->total_color_cells + grid_idx;
        if (flat_idx < p->total_cells) {
            return flat_idx;
        }
    }
    return -1;
}

SDL_Color palette_get_color(const Palette *p, int flat_index)
{
    if (palette_is_color_index(p, flat_index) && p->colors) {
        return p->colors[flat_index];
    }
    return (SDL_Color){0, 0, 0, 255};
}

bool
palette_get_emoji_info(const Palette *p, int flat_index, SDL_Texture **tex, int *w, int *h)
{
    if (!palette_is_emoji_index(p, flat_index) || !p->emoji_renderer_instance) {
        return false;
    }

    int arr_idx = palette_get_emoji_array_idx_from_flat_idx(p, flat_index);
    if (arr_idx == -1) {
        return false;
    }

    EmojiRenderer *er = p->emoji_renderer_instance;
    return emoji_renderer_get_texture_info(er, arr_idx, tex, w, h);
}

bool palette_is_color_index(const Palette *p, int flat_index)
{
    return flat_index >= 0 && flat_index < p->total_color_cells;
}

bool palette_is_emoji_index(const Palette *p, int flat_index)
{
    return flat_index >= p->total_color_cells && flat_index < p->total_cells;
}

int palette_get_emoji_array_idx_from_flat_idx(const Palette *p, int flat_index)
{
    if (!palette_is_emoji_index(p, flat_index) || !p->emoji_renderer_instance) {
        return -1;
    }
    EmojiRenderer *er = p->emoji_renderer_instance;
    int n = emoji_renderer_get_num_emojis(er);
    if (n == 0) {
        return -1;
    }

    int grid_idx = flat_index - p->total_color_cells;
    return grid_idx % n;
}
