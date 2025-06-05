// AI Summary for palette.c: Manages the color and emoji palette for the paint application.
// Handles creation (including loading emoji font, shuffling emojis, and rendering them to textures),
// destruction, and recreation (including reshuffling and re-rendering emojis) of the palette.
// Generates color rows systematically (HSV + grayscale) and emoji rows from a predefined, shuffled list.
// Provides functions for drawing the palette and performing hit tests for tool selection.
// Depends on app_context.h for MIN_BRUSH_SIZE.
#include "palette.h"
#include "draw.h" // For draw_hollow_circle
#include "app_context.h" // For MIN_BRUSH_SIZE
#include "palette_emojis_data.h" // For emoji codepoints and count

#include <stdlib.h>
#include <string.h> // For strlen
#include <math.h> // For fabsf, fmodf, roundf
#include <time.h>   // For srand, rand (though main seeds it)

// Fisher-Yates shuffle for an array of char pointers
static void shuffle_char_pointers(char **array, int n) {
    if (n > 1) {
        for (int i = n - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            char *temp = array[j];
            array[j] = array[i];
            array[i] = temp;
        }
    }
}

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

static void fill_palette_colors(Palette *p) {
    if (p->color_rows == 0) {
        return;
    }
    const int num_hsv_palette_rows = p->color_rows - 1;
    const float V_DARK_START = 0.5f, V_VIVID = 1.0f, S_VIVID = 1.0f, S_PASTEL_END = 0.4f;
    int vivid_row_index = (num_hsv_palette_rows > 0) ? (num_hsv_palette_rows / 2) : -1;
    for (int col = 0; col < p->cols; ++col) {
        float hue = (p->cols == 1) ? 0.0f : (360.0f * col / (p->cols - 1));
        if (p->cols <= 1) hue = 0;
        for (int row = 0; row < p->color_rows; ++row) {
            int pos = row * p->cols + col;
            if (num_hsv_palette_rows > 0 && row < num_hsv_palette_rows) {
                float current_v = V_VIVID, current_s = S_VIVID;
                if (num_hsv_palette_rows == 1) {
                    // Single HSV row, use vivid settings
                } else {
                    if (row < vivid_row_index) {
                        current_s = S_VIVID;
                        if (vivid_row_index > 0) current_v = V_DARK_START + ((float)row / vivid_row_index) * (V_VIVID - V_DARK_START);
                        else current_v = V_DARK_START;
                    } else if (row > vivid_row_index) {
                        current_v = V_VIVID;
                        int num_pastel_steps = num_hsv_palette_rows - 1 - vivid_row_index;
                        int current_pastel_step = row - vivid_row_index;
                        if (num_pastel_steps > 0) current_s = S_VIVID - ((float)current_pastel_step / num_pastel_steps) * (S_VIVID - S_PASTEL_END);
                        else current_s = S_PASTEL_END;
                    }
                }
                p->colors[pos] = hsv_to_rgb(hue, current_s, current_v);
            } else { // Grayscale row
                float t = (p->cols == 1) ? 0.0f : (float)col / (p->cols - 1);
                int gray = (int)((1.0f - t) * 255.0f + 0.5f);
                p->colors[pos].r = gray; p->colors[pos].g = gray; p->colors[pos].b = gray; p->colors[pos].a = 255;
            }
        }
    }
}

// Allocates and shuffles emoji_codepoints
static void allocate_and_shuffle_emoji_codepoints(Palette *p) {
    p->num_defined_emojis = NUM_DEFAULT_EMOJIS;
    if (p->num_defined_emojis == 0) {
        p->emoji_codepoints = NULL; // Ensure it's NULL if no emojis
        return;
    }

    // Free existing if any (e.g. during recreate)
    free(p->emoji_codepoints);
    p->emoji_codepoints = malloc(sizeof(char*) * p->num_defined_emojis);
    if (!p->emoji_codepoints) {
        SDL_Log("Failed to allocate memory for emoji codepoints copy.");
        p->num_defined_emojis = 0;
        return;
    }
    for (int i = 0; i < p->num_defined_emojis; ++i) {
        p->emoji_codepoints[i] = (char*)ORIGINAL_DEFAULT_EMOJI_CODEPOINTS[i];
    }
    shuffle_char_pointers((char**)p->emoji_codepoints, p->num_defined_emojis);
}

// Renders the current p->emoji_codepoints to textures
void palette_render_emoji_textures(Palette *p) {
    if (!p->emoji_font || !p->ren_ref || p->num_defined_emojis == 0 || !p->emoji_codepoints) {
        // Free existing textures if we can't render new ones
        if (p->emoji_textures) {
            // Use p->num_defined_emojis for iteration as it should reflect the allocated size
            for (int i = 0; i < p->num_defined_emojis; ++i) { 
                if (p->emoji_textures[i]) SDL_DestroyTexture(p->emoji_textures[i]);
            }
            free(p->emoji_textures); p->emoji_textures = NULL;
        }
        free(p->emoji_texture_dims); p->emoji_texture_dims = NULL;
        return;
    }

    // Free existing textures and dims before creating new ones
    if (p->emoji_textures) {
        // Iterate based on the current num_defined_emojis which should match the allocated size
        for (int i = 0; i < p->num_defined_emojis; ++i) {
            if (p->emoji_textures[i]) SDL_DestroyTexture(p->emoji_textures[i]);
        }
        free(p->emoji_textures);
    }
    free(p->emoji_texture_dims);

    p->emoji_textures = malloc(sizeof(SDL_Texture*) * p->num_defined_emojis);
    p->emoji_texture_dims = malloc(sizeof(SDL_Point) * p->num_defined_emojis);

    if (!p->emoji_textures || !p->emoji_texture_dims) {
        SDL_Log("Failed to allocate memory for emoji textures/dims.");
        free(p->emoji_textures); p->emoji_textures = NULL;
        free(p->emoji_texture_dims); p->emoji_texture_dims = NULL;
        // num_defined_emojis remains, but textures are gone.
        return;
    }

    SDL_Color fg_color = {0, 0, 0, 255};
    for (int i = 0; i < p->num_defined_emojis; ++i) {
        if (!p->emoji_codepoints[i] || strlen(p->emoji_codepoints[i]) == 0) {
            p->emoji_textures[i] = NULL; p->emoji_texture_dims[i] = (SDL_Point){0,0};
            continue;
        }
        SDL_Surface *surface = TTF_RenderUTF8_Blended(p->emoji_font, p->emoji_codepoints[i], fg_color);
        if (!surface) {
            SDL_Log("Failed to render emoji '%s': %s", p->emoji_codepoints[i], TTF_GetError());
            p->emoji_textures[i] = NULL; p->emoji_texture_dims[i] = (SDL_Point){0,0};
            continue;
        }
        p->emoji_textures[i] = SDL_CreateTextureFromSurface(p->ren_ref, surface);
        if (!p->emoji_textures[i]) {
            SDL_Log("Failed to create texture for emoji '%s': %s", p->emoji_codepoints[i], SDL_GetError());
            p->emoji_texture_dims[i] = (SDL_Point){0,0};
        } else {
            p->emoji_texture_dims[i] = (SDL_Point){surface->w, surface->h};
        }
        SDL_FreeSurface(surface);
    }
}


Palette *palette_create(SDL_Renderer *ren, int window_w) {
    Palette *p = malloc(sizeof(Palette));
    if (!p) {
        return NULL;
    }
    p->ren_ref = ren;
    p->emoji_font = TTF_OpenFont(EMOJI_FONT_PATH, EMOJI_FONT_SIZE);
    if (!p->emoji_font) {
        SDL_Log("Failed to load emoji font '%s': %s", EMOJI_FONT_PATH, TTF_GetError());
    }
    p->total_rows = PALETTE_ROWS;
    p->color_rows = NUM_COLOR_ROWS_INC_GRAYSCALE;
    p->emoji_rows = NUM_EMOJI_ROWS;
    p->colors = NULL;
    p->emoji_codepoints = NULL;
    p->emoji_textures = NULL;
    p->emoji_texture_dims = NULL;
    p->num_defined_emojis = 0;

    allocate_and_shuffle_emoji_codepoints(p);
    palette_render_emoji_textures(p);
    palette_recreate(p, window_w); // Calculates sizes and fills colors
    return p;
}

void palette_destroy(Palette *p) {
    if (!p) {
        return;
    }
    free(p->colors);
    if (p->emoji_font) {
        TTF_CloseFont(p->emoji_font);
    }

    if (p->emoji_textures) {
        // num_defined_emojis should be the count corresponding to allocated emoji_textures
        for (int i = 0; i < p->num_defined_emojis; ++i) {
            if (p->emoji_textures[i]) {
                SDL_DestroyTexture(p->emoji_textures[i]);
            }
        }
        free(p->emoji_textures);
        p->emoji_textures = NULL; 
    }
    free(p->emoji_texture_dims);
    p->emoji_texture_dims = NULL; 

    free(p->emoji_codepoints); // Free the shuffled pointer array
    p->emoji_codepoints = NULL; 
    free(p);
}

void palette_recreate(Palette *p, int window_w) {
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
    p->total_cells = p->total_color_cells + total_emoji_grid_cells;

    // Reshuffle and re-render emojis on recreate
    if (p->emoji_codepoints && p->num_defined_emojis > 0) { // Only shuffle if codepoints were allocated
        shuffle_char_pointers((char**)p->emoji_codepoints, p->num_defined_emojis);
        palette_render_emoji_textures(p); // Re-render with new shuffled order
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
                SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
                SDL_RenderFillRect(ren, &cell_r);
            }
            if (f_idx == selected_idx) {
                Uint8 ir = 255 - p->colors[f_idx].r, ig = 255 - p->colors[f_idx].g, ib = 255 - p->colors[f_idx].b;
                SDL_SetRenderDrawColor(ren, ir, ig, ib, 255);
                SDL_RenderDrawRect(ren, &cell_r);
                SDL_Rect r2 = {cell_r.x + 1, cell_r.y + 1, cell_r.w - 2, cell_r.h - 2};
                SDL_RenderDrawRect(ren, &r2);
                int br_cx = cell_r.x + cell_r.w / 2, br_cy = cell_r.y + cell_r.h / 2, cr = brush_radius;
                int max_cr = (cell_r.w < cell_r.h ? cell_r.w : cell_r.h) / 2 - 3;
                if (cr > max_cr) cr = max_cr;
                if (cr >= MIN_BRUSH_SIZE) { // Use MIN_BRUSH_SIZE for consistency
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
            int grid_emoji_idx = er * p->cols + c, f_idx = p->total_color_cells + grid_emoji_idx;
            if (p->num_defined_emojis > 0 && p->emoji_textures) {
                int actual_emoji_idx = grid_emoji_idx % p->num_defined_emojis;
                SDL_Texture* tex = p->emoji_textures[actual_emoji_idx];
                SDL_Point dim = p->emoji_texture_dims[actual_emoji_idx];
                if (tex) {
                    SDL_Rect dst_r;
                    float asp = (dim.y == 0) ? 1.0f : (float)dim.x / dim.y;
                    
                    // Draw all emojis (selected or not) at the default padded size
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

                    if (f_idx == selected_idx) { // If selected, draw border
                        SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
                        SDL_RenderDrawRect(ren, &cell_r);
                        SDL_Rect r2 = {cell_r.x + 1, cell_r.y + 1, cell_r.w - 2, cell_r.h - 2};
                        SDL_RenderDrawRect(ren, &r2);
                    }
                } else { // Fallback if texture failed to load for some reason
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
        return -1;
    }
    if (my < palette_start_y + colors_h) {
        int r_in_cols = (my - palette_start_y) / PALETTE_HEIGHT;
        if (r_in_cols >= 0 && r_in_cols < p->color_rows) {
            return r_in_cols * p->cols + clicked_c;
        }
    }
    int emoji_start_y = palette_start_y + colors_h + sep_h;
    if (my >= emoji_start_y && my < emoji_start_y + emojis_h) {
        if (p->num_defined_emojis > 0) {
            int r_in_emojis = (my - emoji_start_y) / PALETTE_HEIGHT;
            if (r_in_emojis >= 0 && r_in_emojis < p->emoji_rows) {
                return p->total_color_cells + (r_in_emojis * p->cols + clicked_c);
            }
        }
    }
    return -1;
}

SDL_Color palette_get_color(const Palette *p, int flat_index) {
    if (palette_is_color_index(p, flat_index) && p->colors) {
        return p->colors[flat_index];
    }
    return (SDL_Color){0, 0, 0, 255}; // Default to black if out of bounds or no colors
}

SDL_bool palette_get_emoji_info(const Palette *p, int flat_index, SDL_Texture** tex, int* w, int* h) {
    if (!palette_is_emoji_index(p, flat_index) || p->num_defined_emojis == 0 || !p->emoji_textures || !p->emoji_texture_dims) {
        return SDL_FALSE;
    }
    int grid_emoji_idx = flat_index - p->total_color_cells;
    int actual_emoji_idx = grid_emoji_idx % p->num_defined_emojis;
    if (p->emoji_textures[actual_emoji_idx]) {
        *tex = p->emoji_textures[actual_emoji_idx];
        *w = p->emoji_texture_dims[actual_emoji_idx].x;
        *h = p->emoji_texture_dims[actual_emoji_idx].y;
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool palette_is_color_index(const Palette *p, int flat_index) {
    return (flat_index >= 0 && flat_index < p->total_color_cells);
}

SDL_bool palette_is_emoji_index(const Palette *p, int flat_index) {
    return (flat_index >= p->total_color_cells && flat_index < p->total_cells);
}

int palette_get_emoji_array_idx_from_flat_idx(const Palette* p, int flat_index) {
    if (!palette_is_emoji_index(p, flat_index) || p->num_defined_emojis == 0) {
        return -1;
    }
    return (flat_index - p->total_color_cells) % p->num_defined_emojis;
}
