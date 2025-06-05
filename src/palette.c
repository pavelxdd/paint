#include "palette.h"
#include "draw.h"

#include <stdlib.h>
#include <math.h> // For fabsf, fmodf

// AI Summary for palette.c: Manages the color palette for the paint application.
// Handles creation, destruction, and recreation of the palette (e.g., on window resize).
// Generates colors systematically using HSV. The HSV rows are dynamically configured
// to include a central 'pure vivid' color row, with preceding rows transitioning to
// darker, saturated shades and subsequent rows transitioning to brighter, pastel shades,
// adapting to the total number of configured HSV rows. The final row is a grayscale ramp.
// Provides functions for drawing the palette and performing hit tests for color selection.

// Helper function to convert HSV to RGB
// h: hue (0-360), s: saturation (0-1), v: value (0-1)
static SDL_Color hsv_to_rgb(float h, float s, float v) {
    SDL_Color rgb_color;
    float r1 = 0, g1 = 0, b1 = 0;

    // Clamp saturation and value
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    if (s == 0.0f) { // Achromatic (grey)
        r1 = g1 = b1 = v;
    } else {
        float c = v * s;
        // h_prime will be in [0, 6). For h=360 (or multiples), h_prime=0.
        float h_prime = fmodf(h / 60.0f, 6.0f);
        if (h_prime < 0) h_prime += 6.0f; // Ensure positive for negative hues

        float x = c * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));
        
        if (h_prime >= 0 && h_prime < 1) { r1 = c; g1 = x; b1 = 0; }
        else if (h_prime >= 1 && h_prime < 2) { r1 = x; g1 = c; b1 = 0; }
        else if (h_prime >= 2 && h_prime < 3) { r1 = 0; g1 = c; b1 = x; }
        else if (h_prime >= 3 && h_prime < 4) { r1 = 0; g1 = x; b1 = c; }
        else if (h_prime >= 4 && h_prime < 5) { r1 = x; g1 = 0; b1 = c; }
        else { r1 = c; g1 = 0; b1 = x; } // h_prime >= 5 && h_prime < 6
        
        float m = v - c;
        r1 += m;
        g1 += m;
        b1 += m;
    }

    rgb_color.r = (Uint8)(r1 * 255.0f + 0.5f);
    rgb_color.g = (Uint8)(g1 * 255.0f + 0.5f);
    rgb_color.b = (Uint8)(b1 * 255.0f + 0.5f);
    rgb_color.a = 255;
    return rgb_color;
}

static void fill_palette_colors(Palette *p)
{
    const int num_hsv_palette_rows = p->rows - 1; // All rows except the last (grayscale)

    const float V_DARK_START = 0.5f;
    const float V_VIVID = 1.0f;
    const float S_VIVID = 1.0f;
    const float S_PASTEL_END = 0.4f;

    int vivid_row_index = -1;
    if (num_hsv_palette_rows > 0) {
        vivid_row_index = num_hsv_palette_rows / 2; // Integer division, places vivid row in the middle
    }

    for (int col = 0; col < p->cols; ++col) {
        // Hue is distributed across columns. p->cols is guaranteed to be >= 1 by palette_recreate.
        float hue = (360.0f * col / p->cols);

        for (int row = 0; row < p->rows; ++row) {
            int pos = row * p->cols + col;
            if (pos >= p->total) continue; // Boundary check

            if (num_hsv_palette_rows > 0 && row < num_hsv_palette_rows) { // This is an HSV row
                float current_v = V_VIVID;
                float current_s = S_VIVID;

                if (num_hsv_palette_rows == 1) {
                    // Single HSV row is pure vivid (defaults are fine)
                } else {
                    if (row < vivid_row_index) { // Rows before vivid: transition to darker
                        current_s = S_VIVID;
                        // Interpolate V from V_DARK_START up to (but not including) V_VIVID
                        // 'vivid_row_index' here acts as the number of steps/rows for this dark transition
                        if (vivid_row_index > 0) { // Avoid division by zero if vivid_row_index is 0 (e.g. N_hsv=1)
                           current_v = V_DARK_START + ((float)row / vivid_row_index) * (V_VIVID - V_DARK_START);
                        } else { // Should only occur if N_hsv=1, where this branch isn't taken due to outer 'else'
                           current_v = V_DARK_START;
                        }
                    } else if (row == vivid_row_index) {
                        // The vivid row itself (defaults are fine)
                    } else { // Rows after vivid: transition to pastel
                        current_v = V_VIVID;
                        int num_pastel_steps_total = num_hsv_palette_rows - 1 - vivid_row_index;
                        int current_pastel_step = row - vivid_row_index; // Ranges from 1 up to num_pastel_steps_total

                        if (num_pastel_steps_total > 0) { // Avoid division by zero
                            current_s = S_VIVID - ((float)current_pastel_step / num_pastel_steps_total) * (S_VIVID - S_PASTEL_END);
                        } else { // Single pastel step or no pastel steps (should be handled by num_hsv_palette_rows == 1)
                            current_s = S_PASTEL_END; 
                        }
                    }
                }
                p->colors[pos] = hsv_to_rgb(hue, current_s, current_v);

            } else { // The last row (or all rows if num_hsv_palette_rows <= 0) is grayscale
                float t = (p->cols == 1) ? 0.0f : (float)col / (p->cols - 1);
                int gray = (int)((1.0f - t) * 255.0f + 0.5f);
                p->colors[pos].r = gray;
                p->colors[pos].g = gray;
                p->colors[pos].b = gray;
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
