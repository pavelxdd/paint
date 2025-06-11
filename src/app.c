#include "app.h"
#include "ui.h"
#include "palette.h"

/* ---------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------*/
App *app_create(SDL_Window *win, SDL_Renderer *ren)
{
    App *app = SDL_malloc(sizeof * app);
    if (!app) {
        SDL_Log("Failed to allocate App");
        return NULL;
    }

    app->win = win;
    app->ren = ren;
    app->window_w = INITIAL_WINDOW_WIDTH;
    app->window_h = INITIAL_WINDOW_HEIGHT;

    app->background_color = (SDL_Color) {
        255, 255, 255, 255
    };

    app->palette = palette_create(ren, app->window_w, app->window_h);
    if (!app->palette) {
        goto fail;
    }

    app->show_color_palette = true;
    app->show_emoji_palette = true;

    app_update_canvas_display_height(app);

    // Set default colors and tool
    app->current_tool = TOOL_BRUSH;
    app->last_color_tool = TOOL_BRUSH;

    // Default water-marker to red (top-left)
    app->water_marker_selected_palette_idx = 0;
    if (app->palette->total_color_cells > 0) {
        app->water_marker_color =
            palette_get_color(app->palette, app->water_marker_selected_palette_idx);
    } else {
        app->water_marker_color = (SDL_Color) {
            255, 0, 0, 255
        }; // Fallback red
    }

    // Default brush to black (bottom-right)
    app->brush_selected_palette_idx =
        app->palette->total_color_cells ? app->palette->total_color_cells - 1 : 0;
    if (app->palette->total_color_cells > 0) {
        app->current_color = palette_get_color(app->palette, app->brush_selected_palette_idx);
    } else {
        app->current_color = (SDL_Color) {
            0, 0, 0, 255
        }; // Fallback black
    }

    // Default emoji to first one
    app->emoji_selected_palette_idx = app->palette->total_color_cells;

    app->brush_radius = 10;
    app_recalculate_sizes_and_limits(app);

    app->canvas_texture = NULL;
    app->stroke_buffer = NULL;
    app->blur_source_texture = NULL;
    app->blur_dab_texture = NULL;
    app_recreate_canvas_texture(app);

    app->needs_redraw = true;
    app->resize_pending = false;
    app->last_resize_timestamp = 0;
    app->is_buffered_stroke_active = false;
    app->line_mode_toggled_on = false;
    app->is_drawing = false;
    app->straight_line_stroke_latched = false;
    app->last_stroke_x = -1.0f;
    app->last_stroke_y = -1.0f;
    app->has_moved_since_mousedown = false;

    return app;

fail:
    if (app->palette) {
        palette_destroy(app->palette);
    }
    SDL_free(app);
    return NULL;
}

void app_destroy(App *app)
{
    if (!app) {
        return;
    }
    if (app->canvas_texture) {
        SDL_DestroyTexture(app->canvas_texture);
    }
    if (app->stroke_buffer) {
        SDL_DestroyTexture(app->stroke_buffer);
    }
    if (app->blur_source_texture) {
        SDL_DestroyTexture(app->blur_source_texture);
    }
    if (app->blur_dab_texture) {
        SDL_DestroyTexture(app->blur_dab_texture);
    }
    if (app->blur_temp_texture) {
        SDL_DestroyTexture(app->blur_temp_texture);
    }
    palette_destroy(app->palette);
    SDL_free(app);
}

/* ---------------------------------------------------------------------------
 * Background utilities
 * --------------------------------------------------------------------------*/
void app_set_background_and_clear_canvas(App *app, SDL_Color color)
{
    if (!app) {
        return;
    }
    app->background_color = color;
    app_clear_canvas_with_current_bg(app);
}
