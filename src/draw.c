#include "draw.h"
#include <math.h>

// Draw filled circle using horizontal scanlines.
void draw_circle(SDL_Renderer *ren, int cx, int cy, int radius)
{
    if (radius <= 0) {
        if (radius == 0) {
            SDL_RenderPoint(ren, (float)cx, (float)cy);
        }
        return;
    }

    for (int y = -radius; y <= radius; y++) {
        // Calculate the horizontal extent (x-span) for this scanline
        int x_span = floorf(sqrtf((float)(radius * radius - y * y)));
        SDL_RenderLine(ren, (float)cx - x_span, (float)cy + y, (float)cx + x_span, (float)cy + y);
    }
}

// Draw hollow (outline only) circle (for preview)
void draw_hollow_circle(SDL_Renderer *ren, int cx, int cy, int radius)
{
    const int thickness = 2;

    if (radius < 1) {
        return;
    }

    // For very small radii, draw a filled circle as a 2px outline isn't meaningful.
    if (radius <= thickness) {
        draw_circle(ren, cx, cy, radius);
        return;
    }

    int outer_radius_sq = radius * radius;
    int inner_radius = radius - thickness;
    int inner_radius_sq = inner_radius * inner_radius;

    for (int w = -radius; w <= radius; ++w) {
        for (int h = -radius; h <= radius; ++h) {
            int dist_sq = w * w + h * h;
            if (dist_sq <= outer_radius_sq && dist_sq > inner_radius_sq) {
                SDL_RenderPoint(ren, (float)cx + w, (float)cy + h);
            }
        }
    }
}

// Draw a thick line with round caps using geometry for the shaft.
// This is much faster than drawing circles along a path.
void draw_thick_line(
    SDL_Renderer *ren, int x1, int y1, int x2, int y2, int thickness, SDL_Color color)
{
    int radius = thickness / 2;
    if (radius < 1) {
        radius = 1;
    }

    // For zero-length lines, just draw a single circle.
    if (x1 == x2 && y1 == y2) {
        SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
        draw_circle(ren, x1, y1, radius);
        return;
    }

    // Use geometry for the line shaft
    float angle = atan2(y2 - y1, x2 - x1);
    float sin_angle = sinf(angle);
    float cos_angle = cosf(angle);
    float half_thickness = (float)radius;

    SDL_FColor fcolor = {
        color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    // The four corners of the rectangle making up the line shaft
    SDL_Vertex vertices[4] = {
        {{(float)x1 - half_thickness * sin_angle, (float)y1 + half_thickness * cos_angle}, fcolor, {0, 0}},
        {{(float)x2 - half_thickness * sin_angle, (float)y2 + half_thickness * cos_angle}, fcolor, {0, 0}},
        {{(float)x2 + half_thickness * sin_angle, (float)y2 - half_thickness * cos_angle}, fcolor, {0, 0}},
        {{(float)x1 + half_thickness * sin_angle, (float)y1 - half_thickness * cos_angle}, fcolor, {0, 0}},
    };

    // The rectangle is formed by two triangles: (0, 1, 3) and (1, 2, 3).
    int indices[6] = {0, 1, 3, 1, 2, 3};
    SDL_RenderGeometry(ren, NULL, vertices, 4, indices, 6);

    // Draw circles at the ends for round caps
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    draw_circle(ren, x1, y1, radius);
    draw_circle(ren, x2, y2, radius);
}
