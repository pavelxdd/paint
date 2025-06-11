#include "app.h"

/* ------------ State Toggles & Queries ------------ */

void app_toggle_line_mode(App *app)
{
    if (!app || app->current_tool == TOOL_BLUR) {
        return;
    }
    app->line_mode_toggled_on = !app->line_mode_toggled_on;
    app->needs_redraw = true;
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
    app->needs_redraw = true;
}

void app_toggle_emoji_palette(App *app)
{
    if (!app) {
        return;
    }
    app->show_emoji_palette = !app->show_emoji_palette;
    app_update_canvas_display_height(app);
    app->needs_redraw = true;
}
