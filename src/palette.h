// AI Summary: Defines the Palette structure and related constants for managing
// color and emoji selection. Handles palette creation, drawing (including color
// swatches and emoji rendering with scaled previews), and hit testing.
// Requires SDL_ttf for emoji rendering.
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define PALETTE_HEIGHT 50
#define NUM_COLOR_ROWS_INC_GRAYSCALE 6 // Number of rows for colors + grayscale
#define NUM_EMOJI_ROWS 2               // Number of rows for emojis
#define PALETTE_ROWS (NUM_COLOR_ROWS_INC_GRAYSCALE + NUM_EMOJI_ROWS)
#define PALETTE_CELL_MIN_SIZE 50
#define COLOR_EMOJI_SEPARATOR_HEIGHT 2

#define EMOJI_FONT_PATH "/usr/share/fonts/noto/NotoColorEmoji.ttf" // Ensure this font is available
#define EMOJI_FONT_SIZE 48                 // Font size for rendering emojis to texture
#define DEFAULT_EMOJI_CELL_PADDING 5       // Padding for emojis in palette cells

typedef struct {
    SDL_Color *colors;
    int cols;               // Number of columns, dynamically calculated
    int total_rows;         // PALETTE_ROWS
    int color_rows;         // NUM_COLOR_ROWS_INC_GRAYSCALE
    int emoji_rows;         // NUM_EMOJI_ROWS

    int total_color_cells;
    int total_emoji_cells_to_display; // Number of emoji cells actually displayed
    int total_cells;        // total_color_cells + total_emoji_cells_to_display

    TTF_Font* emoji_font;
    const char** emoji_codepoints;  // Shuffled list of available emoji UTF-8 strings
    SDL_Texture** emoji_textures;   // Rendered textures for each defined emoji
    SDL_Point* emoji_texture_dims;  // Original w,h for each emoji texture
    int num_defined_emojis;         // Count of emojis in emoji_codepoints array

    SDL_Renderer* ren_ref; // Reference to renderer for creating textures
} Palette;

Palette *palette_create(SDL_Renderer *ren, int window_w);
void palette_destroy(Palette *p);
void palette_recreate(Palette *p, int window_w);
void palette_draw(const Palette *p, SDL_Renderer *ren, int palette_start_y, int window_w, int selected_idx, int brush_radius);
int palette_hit_test(const Palette *p, int mx, int my, int window_w, int palette_start_y);

SDL_Color palette_get_color(const Palette *p, int flat_index);
// Gets info for an emoji if flat_index points to an emoji cell
SDL_bool palette_get_emoji_info(const Palette *p, int flat_index, SDL_Texture** tex, int* w, int* h);

SDL_bool palette_is_color_index(const Palette *p, int flat_index);
SDL_bool palette_is_emoji_index(const Palette *p, int flat_index);
int palette_get_emoji_array_idx_from_flat_idx(const Palette* p, int flat_index);
