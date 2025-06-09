#include "app.h"

/* ------------ State Toggles & Queries ------------ */

void app_toggle_line_mode(App *app)
{
    if (!app) {
        return;
    }
    app->line_mode_toggled_on = !app->line_mode_toggled_on;
    app->needs_redraw = true;
}

bool app_is_straight_line_mode(const App *app)
{
    if (!app) {
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
    Uint32 flags = SDL_GetWindowFlags(app->win);
    if (flags & SDL_WINDOW_FULLSCREEN) {
        SDL_SetWindowFullscreen(app->win, 0);
    } else {
        SDL_SetWindowFullscreen(app->win, SDL_WINDOW_FULLSCREEN);
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
