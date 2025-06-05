// AI Summary: Provides primitive drawing functions for circles.
// Includes functions for drawing both filled and hollow circles on an SDL_Renderer.
#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>

void draw_circle(SDL_Renderer *r, int cx, int cy, int radius);
void draw_hollow_circle(SDL_Renderer *r, int cx, int cy, int radius);

#endif // DRAW_H
