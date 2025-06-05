#pragma once

#include <SDL2/SDL.h>

#define PALETTE_HEIGHT 50
#define PALETTE_ROWS 6
#define PALETTE_CELL_MIN_SIZE 50

typedef struct {
    SDL_Color *colors;
    int cols;
    int rows;
    int total;
} Palette;

Palette *palette_create(int window_w);
void palette_destroy(Palette *p);
void palette_recreate(Palette *p, int window_w);
void palette_draw(const Palette *p, SDL_Renderer *ren, int canvas_h, int window_w, int selected_idx, int brush_radius);
int palette_hit_test(const Palette *p, int mx, int my, int window_w, int canvas_h);
SDL_Color palette_get_color(const Palette *p, int index);
