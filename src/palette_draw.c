#include "palette.h"
#include "ui_constants.h"
#include <math.h>

void palette_draw(const Palette *p,
                  SDL_Renderer *ren,
                  int palette_start_y,
                  int window_w,
                  int selected_idx,
                  SDL_bool show_colors,
                  SDL_bool show_emojis)
{
    if (p->cols == 0 || (!show_colors && !show_emojis)) {
        return;
    }

    int cell_width = window_w / p->cols;
    int cell_width_rem = window_w % p->cols;
    int current_y = palette_start_y;

    /* ---------- colour rows ---------- */
    if (show_colors) {
        for (int r = 0; r < p->color_rows; ++r) {
            int cx = 0;
            for (int c = 0; c < p->cols; ++c) {
                int w = cell_width + (c < cell_width_rem ? 1 : 0);
                int f_idx = r * p->cols + c;
                SDL_Rect cell_r = {cx, current_y, w, PALETTE_HEIGHT};

                /* draw cell background (colour swatch) */
                if (p->colors && f_idx < p->total_color_cells) {
                    SDL_SetRenderDrawColor(
                        ren, p->colors[f_idx].r, p->colors[f_idx].g, p->colors[f_idx].b, 255);
                } else {
                    SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
                }
                SDL_RenderFillRect(ren, &cell_r);

                /* selection highlight */
                if (f_idx == selected_idx && palette_is_color_index(p, f_idx)) {
                    Uint8 ir = 255 - p->colors[f_idx].r;
                    Uint8 ig = 255 - p->colors[f_idx].g;
                    Uint8 ib = 255 - p->colors[f_idx].b;

                    SDL_SetRenderDrawColor(ren, ir, ig, ib, 255);
                    SDL_RenderDrawRect(ren, &cell_r);
                    SDL_Rect r2 = {cell_r.x + 1, cell_r.y + 1, cell_r.w - 2, cell_r.h - 2};
                    SDL_RenderDrawRect(ren, &r2);
                }
                cx += w;
            }
            current_y += PALETTE_HEIGHT;
        }
    }

    /* ---------- separator between colours and emojis ---------- */
    SDL_bool separator_needed = show_colors && show_emojis && p->emoji_rows > 0 &&
                                p->color_rows > 0 && COLOR_EMOJI_SEPARATOR_HEIGHT > 0;
    if (separator_needed) {
        SDL_SetRenderDrawColor(ren, 68, 71, 90, 255); // Dracula 'Current Line'
        SDL_Rect sep_r = {0, current_y, window_w, COLOR_EMOJI_SEPARATOR_HEIGHT};
        SDL_RenderFillRect(ren, &sep_r);
        current_y += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    /* ---------- emoji rows ---------- */
    if (show_emojis) {
        SDL_Color chk1 = {40, 42, 54, 255}; // Dracula 'Background'
        SDL_Color chk2 = {68, 71, 90, 255}; // Dracula 'Current Line'
        int num_available_emojis = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);

        for (int er = 0; er < p->emoji_rows; ++er) {
            int cx = 0;
            for (int c = 0; c < p->cols; ++c) {
                int w = cell_width + (c < cell_width_rem ? 1 : 0);
                SDL_Rect cell_r = {cx, current_y, w, PALETTE_HEIGHT};

                SDL_SetRenderDrawColor(ren,
                                       (er + c) % 2 == 0 ? chk1.r : chk2.r,
                                       (er + c) % 2 == 0 ? chk1.g : chk2.g,
                                       (er + c) % 2 == 0 ? chk1.b : chk2.b,
                                       255);
                SDL_RenderFillRect(ren, &cell_r);

                int grid_emoji_idx = er * p->cols + c;
                int f_idx = p->total_color_cells + grid_emoji_idx;

                if (num_available_emojis > 0) {
                    int actual_idx = grid_emoji_idx % num_available_emojis;
                    SDL_Texture *tex = NULL;
                    int tex_w = 0, tex_h = 0;
                    SDL_bool has_emoji = emoji_renderer_get_texture_info(
                        p->emoji_renderer_instance, actual_idx, &tex, &tex_w, &tex_h);

                    if (has_emoji && tex) {
                        float asp = (tex_h == 0) ? 1.0f : (float)tex_w / tex_h;
                        int def_h = cell_r.h - 2 * DEFAULT_EMOJI_CELL_PADDING;
                        int def_w = lroundf(def_h * asp);
                        if (def_w > cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
                            def_w = cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING;
                            def_h = lroundf(def_w / asp);
                        }
                        if (def_w < 1) def_w = 1;
                        if (def_h < 1) def_h = 1;

                        SDL_Rect dst_r = {cell_r.x + (cell_r.w - def_w) / 2,
                                          cell_r.y + (cell_r.h - def_h) / 2,
                                          def_w,
                                          def_h};
                        SDL_RenderCopy(ren, tex, NULL, &dst_r);

                        if (f_idx == selected_idx && palette_is_emoji_index(p, f_idx)) {
                            SDL_SetRenderDrawColor(ren, 189, 147, 249, 255); // Dracula 'Purple'
                            SDL_RenderDrawRect(ren, &cell_r);
                            SDL_Rect r2 = {
                                cell_r.x + 1, cell_r.y + 1, cell_r.w - 2, cell_r.h - 2};
                            SDL_RenderDrawRect(ren, &r2);
                        }
                    } else {
                        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                        SDL_RenderDrawLine(ren,
                                           cell_r.x + 5,
                                           cell_r.y + 5,
                                           cell_r.x + cell_r.w - 5,
                                           cell_r.y + cell_r.h - 5);
                        SDL_RenderDrawLine(ren,
                                           cell_r.x + cell_r.w - 5,
                                           cell_r.y + 5,
                                           cell_r.x + 5,
                                           cell_r.y + cell_r.h - 5);
                    }
                }
                cx += w;
            }
            current_y += PALETTE_HEIGHT;
        }
    }
}
