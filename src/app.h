#pragma once

#include "palette.h"
#include "tool.h"

typedef struct App {
    SDL_Window *win;
    SDL_Renderer *ren;

    SDL_Texture *canvas_texture;
    int canvas_texture_w;
    int canvas_texture_h;
    // Calculated height of the canvas display area in the window
    int canvas_display_area_h;

    SDL_Texture *stroke_buffer; // For tools that need to be blended as a whole stroke
    SDL_Texture *blur_source_texture; // For the blur tool to read from
    SDL_Texture *blur_dab_texture;    // Reusable texture for individual blur dabs
    SDL_Texture *blur_temp_texture;   // For multi-pass blur

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

    bool running;
    bool needs_redraw;

    // For resize debouncing
    bool resize_pending;
    Uint64 last_resize_timestamp;

    // UI state
    bool line_mode_toggled_on;
    bool show_color_palette;
    bool show_emoji_palette;

    // Stroke state
    bool is_buffered_stroke_active;
    bool is_drawing;
    bool straight_line_stroke_latched;
    float last_stroke_x;
    float last_stroke_y;
    bool has_moved_since_mousedown;
} App;

/* --- Lifecycle (app.c) --- */
App *app_create(SDL_Window *win, SDL_Renderer *ren);
void app_destroy(App *app);

/* --- Event Handling (app_keyboard.c, app_mouse.c) --- */
void app_handle_keydown(App *app, const SDL_KeyboardEvent *key_event);
void app_handle_keyup(App *app, const SDL_KeyboardEvent *key_event);
void app_handle_mousedown(App *app, const SDL_MouseButtonEvent *mouse_event);
void app_handle_mouseup(App *app, const SDL_MouseButtonEvent *mouse_event);
void app_handle_mousewheel(
    App *app, const SDL_MouseWheelEvent *wheel_event, float mouse_x, float mouse_y);

/* --- State & Toggles (app_state.c) --- */
void app_toggle_line_mode(App *app);
bool app_is_straight_line_mode(const App *app);
void app_toggle_fullscreen(App *app);
void app_toggle_color_palette(App *app);
void app_toggle_emoji_palette(App *app);

/* --- Drawing & Canvas (app_draw.c, app_canvas.c) --- */
void app_draw_stroke(App *app, float mouse_x, float mouse_y, bool use_background_color);
void app_draw_line_of_dabs(App *app, float x0, float y0, float x1, float y1, bool use_background_color);
void app_clear_canvas_with_current_bg(App *app);
void app_set_background_and_clear_canvas(App *app, SDL_Color color);
void app_recreate_canvas_texture(App *app);

/* --- Brush (app_brush.c) --- */
void app_change_brush_radius(App *app, int delta);
void app_set_brush_radius_from_key(App *app, SDL_Keycode keycode);

/* --- Palette & Tool Selection (app_palette.c) --- */
void app_select_palette_tool(App *app, int palette_idx);
void app_move_palette_selection(App *app, SDL_Keycode key);
void app_cycle_palette_selection(App *app, int delta, int palette_type);
int app_get_current_palette_selection(App *app);

/* --- Window & Resize (app_resize.c) --- */
void app_notify_resize_event(App *app, int new_w, int new_h);
void app_process_debounced_resize(App *app);

/* --- Layout & Sizing (app_layout.c) --- */
void app_recalculate_sizes_and_limits(App *app);
void app_update_canvas_display_height(App *app);
