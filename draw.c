// AI Summary: Implements primitive drawing functions for circles.
// Provides implementations for drawing filled and hollow circles using SDL_Renderer.
#include "draw.h"
#include <math.h>

// Draw filled circle (for painting on canvas)
void draw_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    for (int w = -radius; w <= radius; ++w) {
        for (int h = -radius; h <= radius; ++h) {
            if (w*w + h*h <= radius*radius) {
                SDL_RenderDrawPoint(r, cx + w, cy + h);
            }
        }
    }
}

// Draw hollow (outline only) circle (for preview)
void draw_hollow_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    if (radius < 1)
        return;
    for (int deg = 0; deg < 360; ++deg) {
        double rad = deg * M_PI / 180.0;
        int x = (int)(cx + radius * cos(rad));
        int y = (int)(cy + radius * sin(rad));
        SDL_RenderDrawPoint(r, x, y);
    }
}
