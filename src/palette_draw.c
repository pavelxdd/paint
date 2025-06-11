#include "ui.h"
#include "palette.h"

static void palette_draw_colors(const Palette *p,
                                SDL_Renderer *ren,
                                int *current_y,
                                int window_w,
                                int selected_idx)
{
    int cell_width = window_w / p->cols;
    int cell_width_rem = window_w % p->cols;

    for (int r = 0; r < p->color_rows; ++r) {
        int cx = 0;
        for (int c = 0; c < p->cols; ++c) {
            int w = cell_width + (c < cell_width_rem ? 1 : 0);
            int f_idx = r * p->cols + c;
            SDL_Rect cell_r = {cx, *current_y, w, PALETTE_HEIGHT};
            SDL_FRect f_cell_r = {(float)cell_r.x, (float)cell_r.y, (float)cell_r.w, (float)cell_r.h};

            /* draw cell background (color swatch) */
            if (p->colors && f_idx < p->total_color_cells) {
                if (!SDL_SetRenderDrawColor(ren,
                                            p->colors[f_idx].r,
                                            p->colors[f_idx].g,
                                            p->colors[f_idx].b,
                                            255)) {
                    SDL_Log("Palette: Failed to set color for swatch: %s", SDL_GetError());
                }
            } else {
                if (!SDL_SetRenderDrawColor(ren, 128, 128, 128, 255)) {
                    SDL_Log("Palette: Failed to set color for empty swatch: %s", SDL_GetError());
                }
            }
            if (!SDL_RenderFillRect(ren, &f_cell_r)) {
                SDL_Log("Palette: Failed to draw color swatch: %s", SDL_GetError());
            }

            /* selection highlight */
            if (f_idx == selected_idx && palette_is_color_index(p, f_idx)) {
                Uint8 ir = 255 - p->colors[f_idx].r;
                Uint8 ig = 255 - p->colors[f_idx].g;
                Uint8 ib = 255 - p->colors[f_idx].b;

                if (!SDL_SetRenderDrawColor(ren, ir, ig, ib, 255)) {
                    SDL_Log("Palette: Failed to set color for highlight: %s", SDL_GetError());
                }
                if (!SDL_RenderRect(ren, &f_cell_r)) {
                    SDL_Log("Palette: Failed to draw highlight: %s", SDL_GetError());
                }
                SDL_FRect r2 = {f_cell_r.x + 1, f_cell_r.y + 1, f_cell_r.w - 2, f_cell_r.h - 2};
                if (!SDL_RenderRect(ren, &r2)) {
                    SDL_Log("Palette: Failed to draw inner highlight: %s", SDL_GetError());
                }
            }
            cx += w;
        }
        *current_y += PALETTE_HEIGHT;
    }
}

static void palette_draw_emojis(const Palette *p,
                                SDL_Renderer *ren,
                                int *current_y,
                                int window_w,
                                int selected_idx)
{
    SDL_Color chk1 = {40, 42, 54, 255}; // Dracula 'Background'
    SDL_Color chk2 = {68, 71, 90, 255}; // Dracula 'Current Line'
    int num_available_emojis = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);
    int cell_width = window_w / p->cols;
    int cell_width_rem = window_w % p->cols;

    for (int er = 0; er < p->emoji_rows; ++er) {
        int cx = 0;
        for (int c = 0; c < p->cols; ++c) {
            int w = cell_width + (c < cell_width_rem ? 1 : 0);
            SDL_Rect cell_r = {cx, *current_y, w, PALETTE_HEIGHT};
            SDL_FRect f_cell_r = {(float)cell_r.x, (float)cell_r.y, (float)cell_r.w, (float)cell_r.h};

            if (!SDL_SetRenderDrawColor(ren,
                                        (er + c) % 2 == 0 ? chk1.r : chk2.r,
                                        (er + c) % 2 == 0 ? chk1.g : chk2.g,
                                        (er + c) % 2 == 0 ? chk1.b : chk2.b,
                                        255)) {
                SDL_Log("Palette: Failed to set color for emoji cell: %s", SDL_GetError());
            }
            if (!SDL_RenderFillRect(ren, &f_cell_r)) {
                SDL_Log("Palette: Failed to draw emoji cell: %s", SDL_GetError());
            }

            int grid_emoji_idx = er * p->cols + c;
            int f_idx = p->total_color_cells + grid_emoji_idx;

            if (num_available_emojis > 0) {
                int actual_idx = grid_emoji_idx % num_available_emojis;
                SDL_Texture *tex = NULL;
                int tex_w = 0, tex_h = 0;
                bool has_emoji = emoji_renderer_get_texture_info(
                                     p->emoji_renderer_instance, actual_idx,
                                     &tex, &tex_w, &tex_h);

                if (has_emoji && tex) {
                    float asp = (tex_h == 0) ? 1.0f : (float)tex_w / tex_h;
                    float def_h = (float)cell_r.h - 2 * DEFAULT_EMOJI_CELL_PADDING;
                    float def_w = def_h * asp;
                    if (def_w > cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
                        def_w = (float)cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING;
                        def_h = def_w / asp;
                    }
                    if (def_w < 1.0f) {
                        def_w = 1.0f;
                    }
                    if (def_h < 1.0f) {
                        def_h = 1.0f;
                    }

                    SDL_FRect dst_r = {
                        (float)cell_r.x + (cell_r.w - def_w) / 2.0f,
                        (float)cell_r.y + (cell_r.h - def_h) / 2.0f,
                        def_w,
                        def_h,
                    };
                    if (!SDL_RenderTexture(ren, tex, NULL, &dst_r)) {
                        SDL_Log("Palette: Failed to render emoji texture: %s", SDL_GetError());
                    }

                    if (f_idx == selected_idx && palette_is_emoji_index(p, f_idx)) {
                        if (!SDL_SetRenderDrawColor(ren, 189, 147, 249, 255)) { // Dracula 'Purple'
                            SDL_Log("Palette: Failed to set color for emoji highlight: %s", SDL_GetError());
                        }
                        if (!SDL_RenderRect(ren, &f_cell_r)) {
                            SDL_Log("Palette: Failed to draw emoji highlight: %s", SDL_GetError());
                        }
                        SDL_FRect r2 =
                        {f_cell_r.x + 1, f_cell_r.y + 1, f_cell_r.w - 2, f_cell_r.h - 2};
                        if (!SDL_RenderRect(ren, &r2)) {
                            SDL_Log("Palette: Failed to draw inner emoji highlight: %s", SDL_GetError());
                        }
                    }
                } else {
                    if (!SDL_SetRenderDrawColor(ren, 255, 0, 0, 255)) {
                        SDL_Log("Palette: Failed to set error color: %s", SDL_GetError());
                    }
                    if (!SDL_RenderLine(ren,
                                        (float)cell_r.x + 5,
                                        (float)cell_r.y + 5,
                                        (float)cell_r.x + cell_r.w - 5,
                                        (float)cell_r.y + cell_r.h - 5)) {
                        SDL_Log("Palette: Failed to draw error line 1: %s", SDL_GetError());
                    }
                    if (!SDL_RenderLine(ren,
                                        (float)cell_r.x + cell_r.w - 5,
                                        (float)cell_r.y + 5,
                                        (float)cell_r.x + 5,
                                        (float)cell_r.y + cell_r.h - 5)) {
                        SDL_Log("Palette: Failed to draw error line 2: %s", SDL_GetError());
                    }
                }
            }
            cx += w;
        }
        *current_y += PALETTE_HEIGHT;
    }
}

void palette_draw(const Palette *p,
                  SDL_Renderer *ren,
                  int palette_start_y,
                  int window_w,
                  int selected_idx,
                  bool show_colors,
                  bool show_emojis)
{
    if (p->cols == 0 || (!show_colors && !show_emojis)) {
        return;
    }

    int current_y = palette_start_y;

    /* ---------- color rows ---------- */
    if (show_colors && p->color_rows > 0) {
        palette_draw_colors(p, ren, &current_y, window_w, selected_idx);
    }

    /* ---------- separator between colors and emojis ---------- */
    bool separator_needed = show_colors && show_emojis && p->emoji_rows > 0 &&
                            p->color_rows > 0 && COLOR_EMOJI_SEPARATOR_HEIGHT > 0;
    if (separator_needed) {
        if (!SDL_SetRenderDrawColor(ren, 68, 71, 90, 255)) { // Dracula 'Current Line'
            SDL_Log("Palette: Failed to set color for separator: %s", SDL_GetError());
        }
        SDL_FRect sep_r = {
            0, (float)current_y, (float)window_w, (float)COLOR_EMOJI_SEPARATOR_HEIGHT
        };
        if (!SDL_RenderFillRect(ren, &sep_r)) {
            SDL_Log("Palette: Failed to draw separator: %s", SDL_GetError());
        }
        current_y += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    /* ---------- emoji rows ---------- */
    if (show_emojis && p->emoji_rows > 0) {
        palette_draw_emojis(p, ren, &current_y, window_w, selected_idx);
    }
}
