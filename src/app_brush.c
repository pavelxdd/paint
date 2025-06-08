#include "app.h"
#include "ui_constants.h"

void app_context_change_brush_radius(AppContext *ctx, int delta)
{
    if (!ctx) {
        return;
    }
    ctx->brush_radius += delta;
    if (ctx->brush_radius < MIN_BRUSH_SIZE) {
        ctx->brush_radius = MIN_BRUSH_SIZE;
    }
    if (ctx->brush_radius > ctx->max_brush_radius) {
        ctx->brush_radius = ctx->max_brush_radius;
    }
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_set_brush_radius_from_key(AppContext *ctx, SDL_Keycode keycode)
{
    if (!ctx) {
        return;
    }
    // Only allow non-number keys for brush size adjustment
    // Number keys are now used for tool selection
    if (keycode == SDLK_PLUS || keycode == SDLK_EQUALS || keycode == SDLK_KP_PLUS) {
        app_context_change_brush_radius(ctx, 2);
    } else if (keycode == SDLK_MINUS || keycode == SDLK_KP_MINUS) {
        app_context_change_brush_radius(ctx, -2);
    }
}
