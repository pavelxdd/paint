// AI Summary: Renders the palette UI (color swatches and emojis), including selection
// highlights, brush-size preview, and separator lines.  Extracted from palette.c to
// isolate rendering concerns and keep palette logic/data separate.
#include "palette.h"
#include "draw.h"
#include "ui_constants.h"   // MIN_BRUSH_SIZE & CANVAS_PALETTE_SEPARATOR_HEIGHT
#include <math.h>

void palette_draw(const Palette *p,
                  SDL_Renderer   *ren,
                  int             palette_start_y,
                  int             window_w,
                  int             selected_idx,
                  int             brush_radius)
{
    if (p->cols == 0) {
        return;
    }

    /* ---------- colour rows ---------- */
    int cell_width      = window_w / p->cols;
    int cell_width_rem  = window_w % p->cols;
    int current_y       = palette_start_y;

    for (int r = 0; r < p->color_rows; ++r) {
        int cx = 0;
        for (int c = 0; c < p->cols; ++c) {
            int w      = cell_width + (c < cell_width_rem ? 1 : 0);
            int f_idx  = r * p->cols + c;
            SDL_Rect cell_r = { cx, current_y, w, PALETTE_HEIGHT };

            /* draw cell background (colour swatch) */
            if (p->colors && f_idx < p->total_color_cells) {
                SDL_SetRenderDrawColor(ren,
                                       p->colors[f_idx].r,
                                       p->colors[f_idx].g,
                                       p->colors[f_idx].b,
                                       255);
            } else {
                SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
            }
            SDL_RenderFillRect(ren, &cell_r);

            /* selection & brush preview */
            if (f_idx == selected_idx && palette_is_color_index(p, f_idx)) {
                Uint8 ir = 255 - p->colors[f_idx].r;
                Uint8 ig = 255 - p->colors[f_idx].g;
                Uint8 ib = 255 - p->colors[f_idx].b;

                SDL_SetRenderDrawColor(ren, ir, ig, ib, 255);
                SDL_RenderDrawRect(ren, &cell_r);
                SDL_Rect r2 = { cell_r.x + 1, cell_r.y + 1,
                                cell_r.w - 2, cell_r.h - 2 };
                SDL_RenderDrawRect(ren, &r2);

                /* brush radius preview circle */
                int br_cx  = cell_r.x + cell_r.w / 2;
                int br_cy  = cell_r.y + cell_r.h / 2;
                int cr     = brush_radius;
                int max_cr = (cell_r.w < cell_r.h ? cell_r.w : cell_r.h) / 2 - 3;

                if (cr > max_cr) cr = max_cr;
                if (cr < MIN_BRUSH_SIZE) cr = MIN_BRUSH_SIZE;
                if (cr >= MIN_BRUSH_SIZE) {
                    SDL_SetRenderDrawColor(ren, ir, ig, ib, 255);
                    draw_hollow_circle(ren, br_cx, br_cy, cr);
                }
            }
            cx += w;
        }
        current_y += PALETTE_HEIGHT;
    }

    /* ---------- separator between colours and emojis ---------- */
    if (p->emoji_rows > 0 && p->color_rows > 0 && COLOR_EMOJI_SEPARATOR_HEIGHT > 0) {
        SDL_SetRenderDrawColor(ren, 180, 180, 180, 255);
        SDL_Rect sep_r = { 0, current_y, window_w, COLOR_EMOJI_SEPARATOR_HEIGHT };
        SDL_RenderFillRect(ren, &sep_r);
        current_y += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    /* ---------- emoji rows ---------- */
    SDL_Color chk1 = { 220, 220, 220, 255 };
    SDL_Color chk2 = { 235, 235, 235, 255 };

    int num_available_emojis = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);

    for (int er = 0; er < p->emoji_rows; ++er) {
        int cx = 0;
        for (int c = 0; c < p->cols; ++c) {
            int w = cell_width + (c < cell_width_rem ? 1 : 0);
            SDL_Rect cell_r = { cx, current_y, w, PALETTE_HEIGHT };

            SDL_SetRenderDrawColor(ren,
                                   (er + c) % 2 == 0 ? chk1.r : chk2.r,
                                   (er + c) % 2 == 0 ? chk1.g : chk2.g,
                                   (er + c) % 2 == 0 ? chk1.b : chk2.b,
                                   255);
            SDL_RenderFillRect(ren, &cell_r);

            int grid_emoji_idx = er * p->cols + c;
            int f_idx          = p->total_color_cells + grid_emoji_idx;

            if (num_available_emojis > 0) {
                int actual_idx   = grid_emoji_idx % num_available_emojis;
                SDL_Texture *tex = NULL;
                int tex_w = 0, tex_h = 0;

                if (emoji_renderer_get_texture_info(p->emoji_renderer_instance,
                                                    actual_idx, &tex, &tex_w, &tex_h) && tex) {

                    /* keep aspect ratio inside cell, with padding */
                    float asp       = (tex_h == 0) ? 1.0f : (float)tex_w / tex_h;
                    int   def_h     = cell_r.h - 2 * DEFAULT_EMOJI_CELL_PADDING;
                    int   def_w     = lroundf(def_h * asp);

                    if (def_w > cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
                        def_w = cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING;
                        def_h = lroundf(def_w / asp);
                    }
                    if (def_w < 1) def_w = 1;
                    if (def_h < 1) def_h = 1;

                    SDL_Rect dst_r = { cell_r.x + (cell_r.w - def_w) / 2,
                                       cell_r.y + (cell_r.h - def_h) / 2,
                                       def_w, def_h };
                    SDL_RenderCopy(ren, tex, NULL, &dst_r);

                    /* selection rectangle around emoji */
                    if (f_idx == selected_idx && palette_is_emoji_index(p, f_idx)) {
                        SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
                        SDL_RenderDrawRect(ren, &cell_r);
                        SDL_Rect r2 = { cell_r.x + 1, cell_r.y + 1,
                                        cell_r.w - 2, cell_r.h - 2 };
                        SDL_RenderDrawRect(ren, &r2);
                    }
                } else {
                    /* fallback – failed texture */
                    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                    SDL_RenderDrawLine(ren, cell_r.x + 5, cell_r.y + 5,
                                       cell_r.x + cell_r.w - 5, cell_r.y + cell_r.h - 5);
                    SDL_RenderDrawLine(ren, cell_r.x + cell_r.w - 5, cell_r.y + 5,
                                       cell_r.x + 5, cell_r.y + cell_r.h - 5);
                }
            }
            cx += w;
        }
        current_y += PALETTE_HEIGHT;
    }
}
