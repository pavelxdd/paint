#include "draw.h"

// Draw filled circle (for painting on canvas)
void draw_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    for (int w = -radius; w <= radius; ++w) {
        for (int h = -radius; h <= radius; ++h) {
            if (w * w + h * h <= radius * radius) {
                SDL_RenderDrawPoint(r, cx + w, cy + h);
            }
        }
    }
}

// Draw hollow (outline only) circle (for preview)
void draw_hollow_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    const int thickness = 2;

    if (radius < 1) {
        return;
    }

    // For very small radii, draw a filled circle as a 2px outline isn't meaningful.
    if (radius <= thickness) {
        draw_circle(r, cx, cy, radius);
        return;
    }

    int outer_radius_sq = radius * radius;
    int inner_radius = radius - thickness;
    int inner_radius_sq = inner_radius * inner_radius;

    for (int w = -radius; w <= radius; ++w) {
        for (int h = -radius; h <= radius; ++h) {
            int dist_sq = w * w + h * h;
            if (dist_sq <= outer_radius_sq && dist_sq > inner_radius_sq) {
                SDL_RenderDrawPoint(r, cx + w, cy + h);
            }
        }
    }
}
