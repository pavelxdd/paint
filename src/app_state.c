#include "app.h"

/* ------------ State Toggles & Queries ------------ */

void app_context_toggle_line_mode(AppContext *ctx)
{
    if (!ctx) {
        return;
    }
    ctx->line_mode_toggled_on = !ctx->line_mode_toggled_on;
    ctx->needs_redraw = SDL_TRUE;
}

SDL_bool app_context_is_straight_line_mode(const AppContext *ctx)
{
    if (!ctx) {
        return SDL_FALSE;
    }
    const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
    return ctx->line_mode_toggled_on || keyboard_state[SDL_SCANCODE_LCTRL] ||
           keyboard_state[SDL_SCANCODE_RCTRL];
}

void app_context_toggle_fullscreen(AppContext *ctx)
{
    if (!ctx || !ctx->win) {
        return;
    }
    Uint32 flags = SDL_GetWindowFlags(ctx->win);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(ctx->win, 0);
    } else {
        SDL_SetWindowFullscreen(ctx->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void app_context_toggle_color_palette(AppContext *ctx)
{
    if (!ctx) {
        return;
    }
    ctx->show_color_palette = !ctx->show_color_palette;
    app_context_update_canvas_display_height(ctx);
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_toggle_emoji_palette(AppContext *ctx)
{
    if (!ctx) {
        return;
    }
    ctx->show_emoji_palette = !ctx->show_emoji_palette;
    app_context_update_canvas_display_height(ctx);
    ctx->needs_redraw = SDL_TRUE;
}
