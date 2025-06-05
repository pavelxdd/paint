// AI Summary: Defines the AppContext structure holding the application's global state
// for the paint program. Manages window/renderer, canvas, palette, brush, colors,
// and flags for UI updates and resize handling. Provides functions for
// initializing, destroying, and modifying this state.
#pragma once

#include <SDL2/SDL.h>
#include "palette.h" // For Palette struct and PALETTE_ROWS, PALETTE_HEIGHT

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600
#define MIN_BRUSH_SIZE 2
// MAX_BRUSH_SIZE is dynamically calculated based on palette and window size.
#define RESIZE_DEBOUNCE_MS 200 // Milliseconds for resize debouncing
#define CANVAS_PALETTE_SEPARATOR_HEIGHT 2 // Height of the separator line

typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;

    SDL_Texture *canvas_texture;
    int canvas_texture_w;
    int canvas_texture_h;
    // Calculated height of the canvas display area in the window
    int canvas_display_area_h;


    Palette *palette;
    int selected_palette_idx;
    SDL_Color current_color;    // Current drawing color
    SDL_Color background_color; // Current canvas background color

    int brush_radius;
    int max_brush_radius;       // Max allowed brush radius, dynamically calculated

    int window_w;
    int window_h;

    SDL_bool needs_redraw;

    // For resize debouncing
    SDL_bool resize_pending;
    Uint32 last_resize_timestamp;
} AppContext;

// Initialization and Cleanup
AppContext* app_context_create(SDL_Window *win, SDL_Renderer *ren);
void app_context_destroy(AppContext *ctx);

// State Modifiers & Operations
void app_context_select_palette_color(AppContext *ctx, int palette_idx);
void app_context_set_background_and_clear_canvas(AppContext *ctx, SDL_Color color);
void app_context_clear_canvas_with_current_bg(AppContext *ctx);
void app_context_change_brush_radius(AppContext *ctx, int delta);
void app_context_set_brush_radius_from_key(AppContext *ctx, SDL_Keycode keycode);
void app_context_recalculate_sizes_and_limits(AppContext *ctx);
void app_context_draw_stroke(AppContext *ctx, int mouse_x, int mouse_y, SDL_bool use_background_color);

// For window resize event notification
void app_context_notify_resize_event(AppContext *ctx, int new_w, int new_h);

// Helper to calculate canvas display height (called internally and by resize handler)
void app_context_update_canvas_display_height(AppContext *ctx);
