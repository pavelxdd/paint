#include "app.h"
#include "palette.h"

void app_notify_resize_event(App *app, int new_w, int new_h)
{
    if (!app) {
        return;
    }
    app->window_w = new_w < 1 ? 1 : new_w;
    app->window_h = new_h < 1 ? 1 : new_h;
    app->last_resize_timestamp = SDL_GetTicks();
    app->resize_pending = SDL_TRUE;
    app->needs_redraw = SDL_TRUE;
}

void app_process_debounced_resize(App *app)
{
    if (app->resize_pending &&
        (SDL_GetTicks() - app->last_resize_timestamp >= RESIZE_DEBOUNCE_MS)) {
        // Before palette is recreated, check for special cases to preserve them.
        SDL_bool brush_was_top_left = (app->brush_selected_palette_idx == 0);
        SDL_bool water_marker_was_top_left = (app->water_marker_selected_palette_idx == 0);
        SDL_bool water_marker_was_bottom_right = SDL_FALSE;
        if (app->palette->total_color_cells > 0) {
            water_marker_was_bottom_right =
                (app->water_marker_selected_palette_idx == app->palette->total_color_cells - 1);
        }

        // 1. Recreate palette: recalculates rows, columns, colors, and shuffles emojis
        palette_recreate(app->palette, app->window_w, app->window_h);

        // 2. Update canvas display height based on new window height and new palette layout
        app_update_canvas_display_height(app);

        // 3. Reset selections and colors, preserving special cases.
        // Brush: stays top-left if it was, otherwise defaults to bottom-right.
        if (brush_was_top_left) {
            app->brush_selected_palette_idx = 0;
        } else {
            app->brush_selected_palette_idx =
                app->palette->total_color_cells > 0 ? app->palette->total_color_cells - 1 : 0;
        }

        // Water-marker: stays top-left or bottom-right if it was, otherwise defaults to
        // top-left.
        if (water_marker_was_top_left) {
            app->water_marker_selected_palette_idx = 0;
        } else if (water_marker_was_bottom_right) {
            app->water_marker_selected_palette_idx =
                app->palette->total_color_cells > 0 ? app->palette->total_color_cells - 1 : 0;
        } else {
            app->water_marker_selected_palette_idx = 0; // Default to top-left
        }

        // Update colors from new palette
        app->current_color = palette_get_color(app->palette, app->brush_selected_palette_idx);
        app->water_marker_color =
            palette_get_color(app->palette, app->water_marker_selected_palette_idx);

        app->emoji_selected_palette_idx = app->palette->total_color_cells;

        // 4. Recalculate brush size limits based on new layout
        app_recalculate_sizes_and_limits(app);

        // 5. Create a new canvas texture, preserving the old content
        app_recreate_canvas_texture(app);

        app->resize_pending = SDL_FALSE;
        app->needs_redraw = SDL_TRUE;
    }
}
