// AI Summary: Implements color palette management for the paint program.
// Handles palette creation, dynamic resizing based on window width, color generation (HSV and grayscale),
// rendering the palette, and processing clicks for color selection.
#include "palette.h"
#include "draw.h"
#include <stdlib.h>
#include <math.h>

static void fill_palette_colors(Palette *p)
{
    // Systematically fill palette: last row is grayscale ramp, others as HSV.
    const float S_MIN = 0.2f;
    for (int col = 0; col < p->cols; ++col) {
        float hue = 360.0f * col / p->cols;
        for (int row = 0; row < p->rows; ++row) {
            int pos = row * p->cols + col;
            if (row == p->rows - 1) {
                // Grayscale: left=white, right=black
                float t = (p->cols == 1) ? 0.0f : (float)col / (p->cols - 1);
                int gray = (int)((1.0f - t) * 255.0f + 0.5f);
                p->colors[pos].r = gray;
                p->colors[pos].g = gray;
                p->colors[pos].b = gray;
                p->colors[pos].a = 255;
            } else {
                float sat = S_MIN + (1.0f - S_MIN) * (1.0f - (float)row / (p->rows - 1));
                float val = 1.0f;
                // HSV to RGB
                float c = val * sat;
                float h_ = hue / 60.0f;
                float x = c * (1 - fabsf(fmodf(h_, 2) - 1));
                float r1, g1, b1;
                if      (h_ >= 0 && h_ < 1) { r1 = c; g1 = x; b1 = 0; }
                else if (h_ >= 1 && h_ < 2) { r1 = x; g1 = c; b1 = 0; }
                else if (h_ >= 2 && h_ < 3) { r1 = 0; g1 = c; b1 = x; }
                else if (h_ >= 3 && h_ < 4) { r1 = 0; g1 = x; b1 = c; }
                else if (h_ >= 4 && h_ < 5) { r1 = x; g1 = 0; b1 = c; }
                else if (h_ >= 5 && h_ < 6) { r1 = c; g1 = 0; b1 = x; }
                else                      { r1 = g1 = b1 = 0; }
                float m = val - c;
                p->colors[pos].r = (int)((r1 + m) * 255.0f + 0.5f);
                p->colors[pos].g = (int)((g1 + m) * 255.0f + 0.5f);
                p->colors[pos].b = (int)((b1 + m) * 255.0f + 0.5f);
                p->colors[pos].a = 255;
            }
        }
    }
}

Palette *palette_create(int window_w)
{
    Palette *p = malloc(sizeof(Palette));
    if (!p) return NULL;
    p->rows = PALETTE_ROWS;
    p->colors = NULL;
    palette_recreate(p, window_w);
    return p;
}

void palette_destroy(Palette *p)
{
    if (!p) return;
    free(p->colors);
    free(p);
}

void palette_recreate(Palette *p, int window_w)
{
    p->cols = window_w / PALETTE_CELL_MIN_SIZE;
    if (p->cols < 1) p->cols = 1;
    p->total = p->cols * p->rows;
    p->colors = realloc(p->colors, sizeof(SDL_Color) * p->total);
    fill_palette_colors(p);
}

void palette_draw(const Palette *p, SDL_Renderer *ren, int canvas_h, int window_w, int selected_idx, int brush_radius)
{
    // Compute geometry for palette cells (fill row exactly)
    int cell_width = window_w / p->cols;
    int cell_width_rem = window_w % p->cols;

    // Draw palette
    int cell_y = canvas_h;
    for (int row = 0; row < p->rows; ++row) {
        int cell_x = 0;
        for (int col = 0; col < p->cols; ++col) {
            int i = row * p->cols + col;
            if (i >= p->total)
                continue;
            int w = cell_width + (col < cell_width_rem ? 1 : 0);
            SDL_Rect r = {cell_x, cell_y, w, PALETTE_HEIGHT};
            // Fill color
            SDL_SetRenderDrawColor(ren, p->colors[i].r, p->colors[i].g, p->colors[i].b, 255);
            SDL_RenderFillRect(ren, &r);

            // Draw selection frame if this is the selected palette square
            if (i == selected_idx) {
                // Use inverted color for selection frame for maximum contrast
                Uint8 inv_r = 255 - p->colors[i].r;
                Uint8 inv_g = 255 - p->colors[i].g;
                Uint8 inv_b = 255 - p->colors[i].b;
                SDL_SetRenderDrawColor(ren, inv_r, inv_g, inv_b, 255);
                SDL_RenderDrawRect(ren, &r);
                SDL_Rect r2 = {r.x + 1, r.y + 1, r.w - 2, r.h - 2};
                SDL_RenderDrawRect(ren, &r2);
                // Draw hollow circle for brush size preview (center of cell)
                int cx = r.x + r.w/2;
                int cy = r.y + r.h/2;
                // Constrain circle radius so it fits inside cell
                int circle_r = brush_radius;
                int max_cell_radius = (r.w < r.h ? r.w : r.h) / 2 - 3;
                if (circle_r > max_cell_radius) circle_r = max_cell_radius;
                if (circle_r >= 2)
                {
                    // Invert the palette color for visibility
                    SDL_SetRenderDrawColor(ren, inv_r, inv_g, inv_b, 255);
                    draw_hollow_circle(ren, cx, cy, circle_r);
                }
            }
            cell_x += w;
        }
        cell_y += PALETTE_HEIGHT;
    }
}

int palette_hit_test(const Palette *p, int mx, int my, int window_w, int canvas_h)
{
    int palette_area_top = canvas_h;
    int palette_area_bottom = canvas_h + (p->rows * PALETTE_HEIGHT);

    if (my < palette_area_top || my >= palette_area_bottom) {
        return -1;
    }

    int row = (my - palette_area_top) / PALETTE_HEIGHT;

    // Compute geometry for palette cells (fill row exactly)
    int cell_width = window_w / p->cols;
    int cell_width_rem = window_w % p->cols;

    // Find column by traversing cell edges, to deal with non-divisible width
    int x = 0;
    for (int col = 0; col < p->cols; ++col) {
        int w = cell_width + (col < cell_width_rem ? 1 : 0);
        if (mx < x + w) {
            if (row >= 0 && row < p->rows) {
                int idx = row * p->cols + col;
                if (idx >= 0 && idx < p->total) {
                    return idx;
                }
            }
            return -1;
        }
        x += w;
    }

    return -1;
}

SDL_Color palette_get_color(const Palette *p, int index)
{
    if (index >= 0 && index < p->total) {
        return p->colors[index];
    }
    return (SDL_Color){0, 0, 0, 255}; // Default to black
}

