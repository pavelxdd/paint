#pragma once

#include <SDL2/SDL.h>
#include "palette.h" // For Palette struct and PALETTE_HEIGHT
#include "ui_constants.h"          // Shared UI layout constants

typedef enum {
    TOOL_BRUSH,
    TOOL_WATER_MARKER,
    TOOL_EMOJI,
    __TOOL_COUNT
} ActiveTool;

typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;

    SDL_Texture *canvas_texture;
    int canvas_texture_w;
    int canvas_texture_h;
    // Calculated height of the canvas display area in the window
    int canvas_display_area_h;

    SDL_Texture *stroke_buffer; // For tools that need to be blended as a whole stroke

    Palette *palette;
    int brush_selected_palette_idx;
    int water_marker_selected_palette_idx;
    int emoji_selected_palette_idx;
    ActiveTool current_tool;    // Current drawing tool (brush, emoji, or water-marker)
    ActiveTool last_color_tool; // Remembers brush vs water-marker when switching to emoji
    SDL_Color current_color;      // Current drawing color (if current_tool is TOOL_BRUSH)
    SDL_Color water_marker_color; // Current drawing color for water-marker tool
    SDL_Color background_color;   // Current canvas background color

    int brush_radius;
    int max_brush_radius; // Max allowed brush radius, dynamically calculated

    int window_w;
    int window_h;

    SDL_bool needs_redraw;

    // For resize debouncing
    SDL_bool resize_pending;
    Uint32 last_resize_timestamp;

    // UI visibility state
    SDL_bool show_color_palette;
    SDL_bool show_emoji_palette;

    // Stroke state
    SDL_bool water_marker_stroke_active;
    SDL_bool is_drawing;
    int last_stroke_x;
    int last_stroke_y;
} AppContext;

// Initialization and Cleanup
AppContext *app_context_create(SDL_Window *win, SDL_Renderer *ren);
void app_context_destroy(AppContext *ctx);

// State Modifiers & Operations
void app_context_select_palette_tool(AppContext *ctx, int palette_idx);
void app_context_set_background_and_clear_canvas(AppContext *ctx, SDL_Color color);
void app_context_clear_canvas_with_current_bg(AppContext *ctx);
void app_context_toggle_color_palette(AppContext *ctx);
void app_context_toggle_emoji_palette(AppContext *ctx);
void app_context_toggle_fullscreen(AppContext *ctx);
void app_context_change_brush_radius(AppContext *ctx, int delta);
void app_context_set_brush_radius_from_key(AppContext *ctx, SDL_Keycode keycode);
void app_context_recalculate_sizes_and_limits(AppContext *ctx);
void app_context_draw_stroke(
    AppContext *ctx, int mouse_x, int mouse_y, SDL_bool use_background_color);

// Event Handling
void app_context_handle_keydown(AppContext *ctx, const SDL_KeyboardEvent *key_event);
void app_context_handle_mousedown(AppContext *ctx,
                                  const SDL_MouseButtonEvent *mouse_event);
void app_context_handle_mouseup(AppContext *ctx, const SDL_MouseButtonEvent *mouse_event);
void app_context_handle_mousewheel(AppContext *ctx,
                                   const SDL_MouseWheelEvent *wheel_event,
                                   int mouse_x,
                                   int mouse_y);

// For window resize event notification
void app_context_notify_resize_event(AppContext *ctx, int new_w, int new_h);
void app_context_process_debounced_resize(AppContext *ctx);

// Stroke management (for water-marker tool)
void app_context_begin_water_marker_stroke(AppContext *ctx);
void app_context_end_water_marker_stroke(AppContext *ctx);

// Palette Navigation
void app_context_move_palette_selection(AppContext *ctx, SDL_Keycode key);
void app_context_cycle_palette_selection(AppContext *ctx, int delta, int palette_type);
int app_context_get_current_palette_selection(AppContext *ctx);

// Internal helpers
void app_context_update_canvas_display_height(AppContext *ctx);
void app_context_recreate_canvas_texture(AppContext *ctx);

