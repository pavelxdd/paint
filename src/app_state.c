#include "app.h"

/* ------------ State Toggles & Queries ------------ */

void app_toggle_line_mode(App *app)
{
    if (!app || app->current_tool == TOOL_BLUR) {
        return;
    }
    app->line_mode_toggled_on = !app->line_mode_toggled_on;
    app_mark_full_redraw(app);
}

bool app_is_straight_line_mode(const App *app)
{
    if (!app || app->current_tool == TOOL_BLUR) {
        return false;
    }
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    return app->line_mode_toggled_on || keyboard_state[SDL_SCANCODE_LCTRL] ||
           keyboard_state[SDL_SCANCODE_RCTRL];
}

void app_toggle_fullscreen(App *app)
{
    if (!app || !app->win) {
        return;
    }
    // This function toggles borderless fullscreen mode.
    bool is_fullscreen = (SDL_GetWindowFlags(app->win) & SDL_WINDOW_FULLSCREEN) != 0;
    if (!SDL_SetWindowFullscreen(app->win, !is_fullscreen)) {
        SDL_Log("Could not toggle fullscreen mode: %s", SDL_GetError());
    }
}

void app_toggle_color_palette(App *app)
{
    if (!app) {
        return;
    }
    app->show_color_palette = !app->show_color_palette;
    app_update_canvas_display_height(app);
    app_mark_full_redraw(app);
}

void app_toggle_emoji_palette(App *app)
{
    if (!app) {
        return;
    }
    app->show_emoji_palette = !app->show_emoji_palette;
    app_update_canvas_display_height(app);
    app_mark_full_redraw(app);
}

/* ------------ Dirty Rectangle Management ------------ */

void app_add_dirty_rect(App *app, float x, float y, float w, float h)
{
    if (!app) {
        return;
    }

    // Always mark that we need a redraw
    app->needs_redraw = true;

    // If we're already doing a full redraw, no need to track this rectangle
    if (app->full_redraw) {
        return;
    }

    // Ensure the rectangle has positive width and height
    if (w <= 0 || h <= 0) {
        return;
    }

    // Clamp the rectangle to the canvas bounds
    if (x < 0) {
        w += x; // Reduce width by the amount we're off-screen
        x = 0;
    }
    if (y < 0) {
        h += y; // Reduce height by the amount we're off-screen
        y = 0;
    }
    if (x + w > app->canvas_texture_w) {
        w = app->canvas_texture_w - x;
    }
    if (y + h > app->canvas_display_area_h) {
        h = app->canvas_display_area_h - y;
    }

    // If after clamping we have an invalid rectangle, return
    if (w <= 0 || h <= 0) {
        return;
    }

    // If this is the first dirty rect, initialize it
    if (!app->has_dirty_rect) {
        app->dirty_rect.x = x;
        app->dirty_rect.y = y;
        app->dirty_rect.w = w;
        app->dirty_rect.h = h;
        app->has_dirty_rect = true;
        return;
    }

    // Otherwise, expand the existing dirty rect to include this one
    // Calculate the right and bottom edges of both rectangles
    float existing_right = app->dirty_rect.x + app->dirty_rect.w;
    float new_right = x + w;
    float existing_bottom = app->dirty_rect.y + app->dirty_rect.h;
    float new_bottom = y + h;

    // Find the rightmost and bottommost points
    float right = existing_right > new_right ? existing_right : new_right;
    float bottom = existing_bottom > new_bottom ? existing_bottom : new_bottom;

    // Find the leftmost and topmost points
    app->dirty_rect.x = app->dirty_rect.x < x ? app->dirty_rect.x : x;
    app->dirty_rect.y = app->dirty_rect.y < y ? app->dirty_rect.y : y;

    // Calculate new width and height
    app->dirty_rect.w = right - app->dirty_rect.x;
    app->dirty_rect.h = bottom - app->dirty_rect.y;
}

void app_mark_full_redraw(App *app)
{
    if (!app) {
        return;
    }
    app->needs_redraw = true;
    app->has_dirty_rect = true;
    app->full_redraw = true;

    // Set the dirty rect to cover the entire canvas
    app->dirty_rect.x = 0;
    app->dirty_rect.y = 0;
    app->dirty_rect.w = app->canvas_texture_w;
    app->dirty_rect.h = app->canvas_display_area_h;
}

void app_clear_dirty_rects(App *app)
{
    if (!app) {
        return;
    }

    // Reset the dirty rectangle tracking
    app->has_dirty_rect = false;
    app->full_redraw = false;

    // Reset the dirty rect to an empty rectangle
    app->dirty_rect.x = 0;
    app->dirty_rect.y = 0;
    app->dirty_rect.w = 0;
    app->dirty_rect.h = 0;
}
