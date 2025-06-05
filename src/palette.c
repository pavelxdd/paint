// AI Summary: Manages the color and emoji palette UI for the paint application.
// Handles creation, destruction, and recreation of the palette structure.
// Generates color swatches (HSV & grayscale), calculates dynamic row layout,
// draws the complete palette (colors and emojis), and performs hit tests for tool selection.
// Delegates emoji image rendering and management to the EmojiRenderer module.
#include "palette.h"
#include "draw.h" // For draw_hollow_circle
#include "app_context.h" // For MIN_BRUSH_SIZE, CANVAS_PALETTE_SEPARATOR_HEIGHT
#include "emoji_renderer.h" // For using the emoji renderer

#include <stdlib.h>
#include <string.h> // For strlen (used indirectly by emoji_renderer)
#include <math.h> // For fabsf, fmodf, roundf
// #include <time.h>   // No longer needed for srand/rand here

// Constants for dynamic row calculation
#define MIN_DYNAMIC_COLOR_ROWS 2
#define MAX_DYNAMIC_COLOR_ROWS 6
#define MIN_DYNAMIC_EMOJI_ROWS 1
#define MAX_DYNAMIC_EMOJI_ROWS 2
#define MIN_CANVAS_HEIGHT_FOR_PALETTE_CALC (PALETTE_HEIGHT * 2) // Min canvas height to aim for when adjusting palette rows


// Fisher-Yates shuffle moved to emoji_renderer.c

// Helper function to convert HSV to RGB
static SDL_Color hsv_to_rgb(float h, float s, float v) {
    SDL_Color rgb_color;
    float r1 = 0, g1 = 0, b1 = 0;
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (s == 0.0f) {
        r1 = g1 = b1 = v;
    } else {
        float c = v * s;
        float h_prime = fmodf(h / 60.0f, 6.0f);
        if (h_prime < 0) h_prime += 6.0f;
        float x_val = c * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));
        if (h_prime >= 0 && h_prime < 1) { r1 = c; g1 = x_val; b1 = 0; }
        else if (h_prime >= 1 && h_prime < 2) { r1 = x_val; g1 = c; b1 = 0; }
        else if (h_prime >= 2 && h_prime < 3) { r1 = 0; g1 = c; b1 = x_val; }
        else if (h_prime >= 3 && h_prime < 4) { r1 = 0; g1 = x_val; b1 = c; }
        else if (h_prime >= 4 && h_prime < 5) { r1 = x_val; g1 = 0; b1 = c; }
        else { r1 = c; g1 = 0; b1 = x_val; }
        float m = v - c; r1 += m; g1 += m; b1 += m;
    }
    rgb_color.r = (Uint8)(r1 * 255.0f + 0.5f);
    rgb_color.g = (Uint8)(g1 * 255.0f + 0.5f);
    rgb_color.b = (Uint8)(b1 * 255.0f + 0.5f);
    rgb_color.a = 255;
    return rgb_color;
}

static void palette_calculate_and_set_dynamic_rows(Palette *p, int window_h) {
    // Iterate from max possible rows downwards to find the largest combination that fits
    // Prioritize fitting more rows if possible.
    for (int c_rows = MAX_DYNAMIC_COLOR_ROWS; c_rows >= MIN_DYNAMIC_COLOR_ROWS; c_rows--) {
        for (int e_rows = MAX_DYNAMIC_EMOJI_ROWS; e_rows >= MIN_DYNAMIC_EMOJI_ROWS; e_rows--) {
            int current_total_palette_rows = c_rows + e_rows;
            int palette_content_h = current_total_palette_rows * PALETTE_HEIGHT;
            
            if (c_rows > 0 && e_rows > 0 && COLOR_EMOJI_SEPARATOR_HEIGHT > 0) {
                palette_content_h += COLOR_EMOJI_SEPARATOR_HEIGHT;
            }
            
            // This is the total height the palette UI (rows + main separator) would consume
            int total_palette_ui_h = palette_content_h + CANVAS_PALETTE_SEPARATOR_HEIGHT;
            int remaining_canvas_h = window_h - total_palette_ui_h;

            if (remaining_canvas_h >= MIN_CANVAS_HEIGHT_FOR_PALETTE_CALC) {
                p->color_rows = c_rows;
                p->emoji_rows = e_rows;
                p->total_rows = p->color_rows + p->emoji_rows;
                return; 
            }
        }
    }
    // If loop finishes, it means even min_color_rows + min_emoji_rows didn't fit while reserving MIN_CANVAS_HEIGHT_FOR_PALETTE_CALC.
    // Default to absolute minimums. Canvas height might become very small or zero.
    p->color_rows = MIN_DYNAMIC_COLOR_ROWS;
    p->emoji_rows = MIN_DYNAMIC_EMOJI_ROWS;
    p->total_rows = p->color_rows + p->emoji_rows;
}


static void fill_palette_colors(Palette *p) {
    if (p->color_rows == 0) { // Use dynamic p->color_rows
        return;
    }
    const int num_hsv_palette_rows = p->color_rows - 1; // p->color_rows includes one grayscale row
    const float V_DARK_START = 0.5f, V_VIVID = 1.0f, S_VIVID = 1.0f, S_PASTEL_END = 0.4f;
    int vivid_row_index = (num_hsv_palette_rows > 0) ? (num_hsv_palette_rows / 2) : -1;
    
    for (int col = 0; col < p->cols; ++col) {
        float hue = (p->cols == 1) ? 0.0f : (360.0f * col / (p->cols - 1));
        if (p->cols <= 1) hue = 0;
        for (int row = 0; row < p->color_rows; ++row) { // Use dynamic p->color_rows
            int pos = row * p->cols + col;
            if (num_hsv_palette_rows > 0 && row < num_hsv_palette_rows) { // This is an HSV row
                float current_v = V_VIVID, current_s = S_VIVID;
                if (num_hsv_palette_rows == 1) {
                    // Single HSV row, use vivid settings
                } else {
                    if (row < vivid_row_index) { // Darker shades
                        current_s = S_VIVID;
                        if (vivid_row_index > 0) current_v = V_DARK_START + ((float)row / vivid_row_index) * (V_VIVID - V_DARK_START);
                        else current_v = V_DARK_START;
                    } else if (row > vivid_row_index) { // Lighter/Pastel shades
                        current_v = V_VIVID;
                        int num_pastel_steps = num_hsv_palette_rows - 1 - vivid_row_index;
                        int current_pastel_step = row - vivid_row_index; // Steps from vivid row towards pastels
                        if (num_pastel_steps > 0) current_s = S_VIVID - ((float)current_pastel_step / num_pastel_steps) * (S_VIVID - S_PASTEL_END);
                        else current_s = S_PASTEL_END;
                    }
                    // else row == vivid_row_index, use default V_VIVID, S_VIVID
                }
                p->colors[pos] = hsv_to_rgb(hue, current_s, current_v);
            } else { // This is the grayscale row (last color row)
                float t = (p->cols == 1) ? 0.0f : (float)col / (p->cols - 1);
                int gray = (int)((1.0f - t) * 255.0f + 0.5f);
                p->colors[pos].r = gray; p->colors[pos].g = gray; p->colors[pos].b = gray; p->colors[pos].a = 255;
            }
        }
    }
}

// allocate_and_shuffle_emoji_codepoints moved to emoji_renderer.c (as part of its create/shuffle logic)
// palette_render_emoji_textures moved to emoji_renderer.c (as part of its shuffle_and_render_all logic)


Palette *palette_create(SDL_Renderer *ren, int window_w, int window_h) {
    Palette *p = malloc(sizeof(Palette));
    if (!p) {
        SDL_Log("Failed to allocate Palette");
        return NULL;
    }
    // p->ren_ref = ren; // No longer needed in Palette struct
    
    p->emoji_renderer_instance = emoji_renderer_create(ren);
    if (!p->emoji_renderer_instance) {
        SDL_Log("Failed to create EmojiRenderer in Palette");
        free(p);
        return NULL;
    }

    p->colors = NULL;
    // color_rows, emoji_rows, total_rows will be set by palette_recreate

    // Recreate will calculate dynamic rows and finish setup
    palette_recreate(p, window_w, window_h); 
    return p;
}

void palette_destroy(Palette *p) {
    if (!p) {
        return;
    }
    free(p->colors);
    
    if (p->emoji_renderer_instance) {
        emoji_renderer_destroy(p->emoji_renderer_instance);
    }
    free(p);
}

void palette_recreate(Palette *p, int window_w, int window_h) {
    // First, calculate and set the dynamic number of rows
    palette_calculate_and_set_dynamic_rows(p, window_h);

    p->cols = window_w / PALETTE_CELL_MIN_SIZE;
    if (p->cols < 1) {
        p->cols = 1;
    }

    p->total_color_cells = p->cols * p->color_rows;
    if (p->color_rows > 0) {
        p->colors = realloc(p->colors, sizeof(SDL_Color) * p->total_color_cells);
        if (p->colors) {
            fill_palette_colors(p);
        } else {
            SDL_Log("Failed to realloc colors for palette.");
            p->total_color_cells = 0;
        }
    } else {
        free(p->colors);
        p->colors = NULL;
        p->total_color_cells = 0;
    }

    int total_emoji_grid_cells = p->cols * p->emoji_rows;
    p->total_emoji_cells_to_display = total_emoji_grid_cells; 
    p->total_cells = p->total_color_cells + total_emoji_grid_cells;

    // Reshuffle and re-render emojis if the renderer exists.
    if (p->emoji_renderer_instance) {
        emoji_renderer_shuffle_and_render_all(p->emoji_renderer_instance);
    }
}

void palette_draw(const Palette *p, SDL_Renderer *ren, int palette_start_y, int window_w, int selected_idx, int brush_radius) {
    if (p->cols == 0) {
        return;
    }
    int cell_width = window_w / p->cols, cell_width_rem = window_w % p->cols, current_y = palette_start_y;

    for (int r = 0; r < p->color_rows; ++r) {
        int cx = 0;
        for (int c = 0; c < p->cols; ++c) {
            int w = cell_width + (c < cell_width_rem ? 1 : 0), f_idx = r * p->cols + c;
            SDL_Rect cell_r = {cx, current_y, w, PALETTE_HEIGHT};
            if (p->colors && f_idx < p->total_color_cells) {
                SDL_SetRenderDrawColor(ren, p->colors[f_idx].r, p->colors[f_idx].g, p->colors[f_idx].b, 255);
                SDL_RenderFillRect(ren, &cell_r);
            } else {
                SDL_SetRenderDrawColor(ren, 128, 128, 128, 255); // Should not happen if total_color_cells is correct
                SDL_RenderFillRect(ren, &cell_r);
            }
            if (f_idx == selected_idx && palette_is_color_index(p, f_idx)) {
                Uint8 ir = 255 - p->colors[f_idx].r, ig = 255 - p->colors[f_idx].g, ib = 255 - p->colors[f_idx].b;
                SDL_SetRenderDrawColor(ren, ir, ig, ib, 255);
                SDL_RenderDrawRect(ren, &cell_r);
                SDL_Rect r2 = {cell_r.x + 1, cell_r.y + 1, cell_r.w - 2, cell_r.h - 2};
                SDL_RenderDrawRect(ren, &r2);
                int br_cx = cell_r.x + cell_r.w / 2, br_cy = cell_r.y + cell_r.h / 2, cr = brush_radius;
                int max_cr = (cell_r.w < cell_r.h ? cell_r.w : cell_r.h) / 2 - 3;
                if (cr > max_cr) cr = max_cr;
                if (cr < MIN_BRUSH_SIZE) cr = MIN_BRUSH_SIZE; // Ensure visible preview for small brushes
                if (cr >= MIN_BRUSH_SIZE) { 
                    SDL_SetRenderDrawColor(ren, ir, ig, ib, 255);
                    draw_hollow_circle(ren, br_cx, br_cy, cr);
                }
            }
            cx += w;
        }
        current_y += PALETTE_HEIGHT;
    }

    if (p->emoji_rows > 0 && p->color_rows > 0 && COLOR_EMOJI_SEPARATOR_HEIGHT > 0) {
        SDL_SetRenderDrawColor(ren, 180, 180, 180, 255);
        SDL_Rect sep_r = {0, current_y, window_w, COLOR_EMOJI_SEPARATOR_HEIGHT};
        SDL_RenderFillRect(ren, &sep_r);
        current_y += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    SDL_Color chk1 = {220, 220, 220, 255}, chk2 = {235, 235, 235, 255};
    int num_available_emojis = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);

    for (int er = 0; er < p->emoji_rows; ++er) {
        int cx = 0;
        for (int c = 0; c < p->cols; ++c) {
            int w = cell_width + (c < cell_width_rem ? 1 : 0);
            SDL_Rect cell_r = {cx, current_y, w, PALETTE_HEIGHT};
            if ((er + c) % 2 == 0) {
                SDL_SetRenderDrawColor(ren, chk1.r, chk1.g, chk1.b, chk1.a);
            } else {
                SDL_SetRenderDrawColor(ren, chk2.r, chk2.g, chk2.b, chk2.a);
            }
            SDL_RenderFillRect(ren, &cell_r);
            
            int grid_emoji_idx = er * p->cols + c;
            int f_idx = p->total_color_cells + grid_emoji_idx;

            if (num_available_emojis > 0) {
                int actual_emoji_array_idx = grid_emoji_idx % num_available_emojis; // Wrap around available emojis
                SDL_Texture* tex = NULL;
                SDL_Point dim = {0,0};
                
                if (emoji_renderer_get_texture_info(p->emoji_renderer_instance, actual_emoji_array_idx, &tex, &dim.x, &dim.y) && tex) {
                    SDL_Rect dst_r;
                    float asp = (dim.y == 0) ? 1.0f : (float)dim.x / dim.y;
                    
                    int defh = cell_r.h - 2 * DEFAULT_EMOJI_CELL_PADDING;
                    int defw = roundf(defh * asp);
                    if (defw > cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
                        defw = cell_r.w - 2 * DEFAULT_EMOJI_CELL_PADDING;
                        defh = roundf(defw / asp);
                    }
                    if (defw < 1) defw = 1;
                    if (defh < 1) defh = 1;
                    dst_r = (SDL_Rect){cell_r.x + (cell_r.w - defw) / 2, cell_r.y + (cell_r.h - defh) / 2, defw, defh};
                    SDL_RenderCopy(ren, tex, NULL, &dst_r);

                    if (f_idx == selected_idx && palette_is_emoji_index(p, f_idx)) { 
                        SDL_SetRenderDrawColor(ren, 0, 0, 255, 255); // Blue selection box for emojis
                        SDL_RenderDrawRect(ren, &cell_r);
                        SDL_Rect r2 = {cell_r.x + 1, cell_r.y + 1, cell_r.w - 2, cell_r.h - 2};
                        SDL_RenderDrawRect(ren, &r2);
                    }
                } else { 
                    // Draw placeholder if emoji failed to load/render (should be rare)
                    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255); 
                    SDL_RenderDrawLine(ren, cell_r.x + 5, cell_r.y + 5, cell_r.x + cell_r.w - 5, cell_r.y + cell_r.h - 5);
                    SDL_RenderDrawLine(ren, cell_r.x + cell_r.w - 5, cell_r.y + 5, cell_r.x + 5, cell_r.y + cell_r.h - 5);
                }
            }
            cx += w;
        }
        current_y += PALETTE_HEIGHT;
    }
}

int palette_hit_test(const Palette *p, int mx, int my, int window_w, int palette_start_y) {
    if (p->cols == 0) {
        return -1;
    }
    int colors_h = p->color_rows * PALETTE_HEIGHT;
    int sep_h = (p->emoji_rows > 0 && p->color_rows > 0) ? COLOR_EMOJI_SEPARATOR_HEIGHT : 0;
    int emojis_h = p->emoji_rows * PALETTE_HEIGHT;
    int total_ui_h = colors_h + sep_h + emojis_h;

    if (my < palette_start_y || my >= palette_start_y + total_ui_h) {
        return -1;
    }

    int cell_w = window_w / p->cols, cell_w_rem = window_w % p->cols, cur_x_edge = 0, clicked_c = -1;
    for (int c_idx = 0; c_idx < p->cols; ++c_idx) {
        int w = cell_w + (c_idx < cell_w_rem ? 1 : 0);
        if (mx < cur_x_edge + w) {
            clicked_c = c_idx;
            break;
        }
        cur_x_edge += w;
    }

    if (clicked_c == -1) {
        return -1; // Click was to the right of all columns (e.g. if window_w % p->cols != 0 and rounding)
    }

    // Check color rows
    if (my < palette_start_y + colors_h) {
        int r_in_cols_region = (my - palette_start_y) / PALETTE_HEIGHT;
        if (r_in_cols_region >= 0 && r_in_cols_region < p->color_rows) {
            return r_in_cols_region * p->cols + clicked_c;
        }
    }

    // Check emoji rows
    int emoji_region_start_y = palette_start_y + colors_h + sep_h;
    if (my >= emoji_region_start_y && my < emoji_region_start_y + emojis_h) {
        int num_available_emojis = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);
        if (num_available_emojis > 0) { 
            int r_in_emojis_region = (my - emoji_region_start_y) / PALETTE_HEIGHT;
            if (r_in_emojis_region >= 0 && r_in_emojis_region < p->emoji_rows) {
                int flat_idx = p->total_color_cells + (r_in_emojis_region * p->cols + clicked_c);
                if (flat_idx < p->total_cells) { // Ensure click is within the grid defined by emoji_rows * cols
                    return flat_idx;
                }
            }
        }
    }
    return -1;
}

SDL_Color palette_get_color(const Palette *p, int flat_index) {
    if (palette_is_color_index(p, flat_index) && p->colors) {
        return p->colors[flat_index];
    }
    return (SDL_Color){0, 0, 0, 255}; // Default black if not a valid color index
}

SDL_bool palette_get_emoji_info(const Palette *p, int flat_index, SDL_Texture** tex, int* w, int* h) {
    if (!palette_is_emoji_index(p, flat_index) || !p->emoji_renderer_instance) {
        return SDL_FALSE;
    }
    int emoji_array_idx = palette_get_emoji_array_idx_from_flat_idx(p, flat_index);
    if (emoji_array_idx == -1) {
        return SDL_FALSE;
    }
    return emoji_renderer_get_texture_info(p->emoji_renderer_instance, emoji_array_idx, tex, w, h);
}

SDL_bool palette_is_color_index(const Palette *p, int flat_index) {
    return (flat_index >= 0 && flat_index < p->total_color_cells);
}

SDL_bool palette_is_emoji_index(const Palette *p, int flat_index) {
    // p->total_cells is total_color_cells + (cols * emoji_rows)
    return (flat_index >= p->total_color_cells && flat_index < p->total_cells);
}

int palette_get_emoji_array_idx_from_flat_idx(const Palette* p, int flat_index) {
    if (!palette_is_emoji_index(p, flat_index) || !p->emoji_renderer_instance) {
        return -1;
    }
    int num_available_emojis = emoji_renderer_get_num_emojis(p->emoji_renderer_instance);
    if (num_available_emojis == 0) {
        return -1;
    }
    // The index into the p->emoji_textures array (which has size p->num_defined_emojis)
    // is based on the position in the visible grid, modulo the number of actual emojis.
    int grid_emoji_idx = flat_index - p->total_color_cells; // This is the 0-based index within the emoji grid cells
    return grid_emoji_idx % num_available_emojis;
}
