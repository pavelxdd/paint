#include "draw.h"

void draw_line_bresenham(int x0, int y0, int x1, int y1, BresenhamCallback cb, void *userdata)
{
    int dx = SDL_abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -SDL_abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    for (;;) {
        cb(x0, y0, userdata);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Draw filled circle using horizontal scanlines.
void draw_circle(SDL_Renderer *ren, float cx, float cy, int radius)
{
    if (radius <= 0) {
        if (radius == 0) {
            if (!SDL_RenderPoint(ren, cx, cy)) {
                SDL_Log("draw_circle: SDL_RenderPoint failed: %s", SDL_GetError());
            }
        }
        return;
    }

    for (int y = -radius; y <= radius; y++) {
        // Calculate the horizontal extent (x-span) for this scanline
        int x_span = (int)SDL_floorf(SDL_sqrtf((float)(radius * radius - y * y)));
        if (!SDL_RenderLine(ren, cx - x_span, cy + y, cx + x_span, cy + y)) {
            SDL_Log("draw_circle: SDL_RenderLine failed: %s", SDL_GetError());
        }
    }
}

// Draw hollow (outline only) circle (for preview)
void draw_hollow_circle(SDL_Renderer *ren, float cx, float cy, int radius)
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
                if (!SDL_RenderPoint(ren, cx + w, cy + h)) {
                    SDL_Log("draw_hollow_circle: SDL_RenderPoint failed: %s", SDL_GetError());
                }
            }
        }
    }
}

// Draw a thick line with round caps using geometry for the shaft.
// This is much faster than drawing circles along a path.
void draw_thick_line(
    SDL_Renderer *ren, float x1, float y1, float x2, float y2, int thickness, SDL_Color color)
{
    int radius = thickness / 2;
    if (radius < 1) {
        radius = 1;
    }

    // For zero-length lines, just draw a single circle.
    if (SDL_fabsf(x1 - x2) < 1e-5f && SDL_fabsf(y1 - y2) < 1e-5f) {
        if (!SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a)) {
            SDL_Log("draw_thick_line: SDL_SetRenderDrawColor failed: %s", SDL_GetError());
        }
        draw_circle(ren, x1, y1, radius);
        return;
    }

    // Use geometry for the line shaft
    float angle = SDL_atan2f(y2 - y1, x2 - x1);
    float sin_angle = SDL_sinf(angle);
    float cos_angle = SDL_cosf(angle);
    float half_thickness = (float)radius;

    SDL_FColor fcolor = {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f,
    };

    // The four corners of the rectangle making up the line shaft
    SDL_Vertex vertices[4] = {
        {{x1 - half_thickness * sin_angle, y1 + half_thickness * cos_angle}, fcolor, {0, 0}},
        {{x2 - half_thickness * sin_angle, y2 + half_thickness * cos_angle}, fcolor, {0, 0}},
        {{x2 + half_thickness * sin_angle, y2 - half_thickness * cos_angle}, fcolor, {0, 0}},
        {{x1 + half_thickness * sin_angle, y1 - half_thickness * cos_angle}, fcolor, {0, 0}},
    };

    // The rectangle is formed by two triangles: (0, 1, 3) and (1, 2, 3).
    int indices[6] = {0, 1, 3, 1, 2, 3};
    if (!SDL_RenderGeometry(ren, NULL, vertices, 4, indices, 6)) {
        SDL_Log("draw_thick_line: SDL_RenderGeometry failed: %s", SDL_GetError());
    }

    // Draw circles at the ends for round caps
    if (!SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a)) {
        SDL_Log("draw_thick_line: SDL_SetRenderDrawColor for caps failed: %s", SDL_GetError());
    }
    draw_circle(ren, x1, y1, radius);
    draw_circle(ren, x2, y2, radius);
}
