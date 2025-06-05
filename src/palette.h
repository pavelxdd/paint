// AI Summary: Defines the Palette structure and related constants for managing
// color and emoji selection. Provides interfaces for palette creation, drawing,
// hit testing, and querying selected tools. Relies on EmojiRenderer for emoji
// image data. Row counts are dynamic.
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // Still needed as Palette uses EmojiRenderer which uses TTF
#include "emoji_renderer.h" // For EmojiRenderer type

#define PALETTE_HEIGHT 50
#define PALETTE_CELL_MIN_SIZE 50
#define COLOR_EMOJI_SEPARATOR_HEIGHT 2
#define DEFAULT_EMOJI_CELL_PADDING 5       // Padding for emojis in palette cells

typedef struct {
    SDL_Color *colors;
    int cols;               // Number of columns, dynamically calculated
    int total_rows;         // Dynamically calculated: color_rows + emoji_rows
    int color_rows;         // Dynamically calculated number of color rows
    int emoji_rows;         // Dynamically calculated number of emoji rows

    int total_color_cells;
    int total_emoji_cells_to_display; // Number of emoji cells actually displayed in the grid
    int total_cells;        // total_color_cells + total_emoji_cells_to_display

    EmojiRenderer* emoji_renderer_instance; // Manages emoji textures and data
} Palette;

Palette *palette_create(SDL_Renderer *ren, int window_w, int window_h);
void palette_destroy(Palette *p);
void palette_recreate(Palette *p, int window_w, int window_h);
void palette_draw(const Palette *p, SDL_Renderer *ren, int palette_start_y, int window_w, int selected_idx, int brush_radius);
int palette_hit_test(const Palette *p, int mx, int my, int window_w, int palette_start_y);

SDL_Color palette_get_color(const Palette *p, int flat_index);
// Gets info for an emoji if flat_index points to an emoji cell
SDL_bool palette_get_emoji_info(const Palette *p, int flat_index, SDL_Texture** tex, int* w, int* h);

SDL_bool palette_is_color_index(const Palette *p, int flat_index);
SDL_bool palette_is_emoji_index(const Palette *p, int flat_index);

// Gets the index into the EmojiRenderer's internal (shuffled) array of emojis
// based on the flat_index from the palette grid.
int palette_get_emoji_array_idx_from_flat_idx(const Palette* p, int flat_index);
