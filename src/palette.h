// AI Summary: Declares the Palette structure and its public API. This includes
// functions for lifecycle management (create, destroy, recreate), drawing, and
// querying state (hit-test, get color/emoji). Constants are in ui_constants.h.
#pragma once

#include "emoji_renderer.h"
#include "ui_constants.h" /* Shared UI constants */
#include <SDL2/SDL.h>

typedef struct {
    SDL_Color *colors;                   /* Dynamic array of colour cells             */
    int cols;                            /* Column count (screen-width dependent)      */
    int total_rows;                      /* colour_rows + emoji_rows                   */
    int color_rows;                      /* Number of colour rows                      */
    int emoji_rows;                      /* Number of emoji rows                       */

    int total_color_cells;               /* colour_rows * cols                    */
    int total_emoji_cells_to_display;    /* emoji_rows  * cols                    */
    int total_cells;                     /* total_color_cells + total_emoji_cells */

    EmojiRenderer *emoji_renderer_instance; /* Renders and caches emoji textures     */
} Palette;

/* Construction / teardown -------------------------------------------------- */
Palette *palette_create(SDL_Renderer *ren, int window_w, int window_h);
void palette_destroy(Palette *p);
void palette_recreate(Palette *p, int window_w, int window_h);

/* Rendering ---------------------------------------------------------------- */
void palette_draw(const Palette *p,
                  SDL_Renderer *ren,
                  int palette_start_y,
                  int window_w,
                  int selected_idx,
                  SDL_bool show_colors,
                  SDL_bool show_emojis);

/* Interaction helpers ------------------------------------------------------ */
int palette_hit_test(const Palette *p,
                     int mx,
                     int my,
                     int window_w,
                     int palette_start_y,
                     SDL_bool show_colors,
                     SDL_bool show_emojis);

SDL_Color palette_get_color(const Palette *p, int flat_index);

SDL_bool
palette_get_emoji_info(const Palette *p, int flat_index, SDL_Texture **tex, int *w, int *h);

SDL_bool palette_is_color_index(const Palette *p, int flat_index);
SDL_bool palette_is_emoji_index(const Palette *p, int flat_index);

int palette_get_emoji_array_idx_from_flat_idx(const Palette *p, int flat_index);
