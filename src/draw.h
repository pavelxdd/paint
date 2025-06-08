#pragma once

#include <SDL2/SDL.h>

void draw_circle(SDL_Renderer *r, int cx, int cy, int radius);
void draw_hollow_circle(SDL_Renderer *r, int cx, int cy, int radius);
void draw_thick_line(SDL_Renderer *ren, int x1, int y1, int x2, int y2, int thickness, SDL_Color color);
