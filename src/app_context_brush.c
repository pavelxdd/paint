// AI Summary: Encapsulates all brush-size operations and limit recomputation
// based on current palette layout and window metrics.
#include "app_context.h"
#include "palette.h"
#include "ui_constants.h"
#include <math.h>

void app_context_change_brush_radius(AppContext *ctx, int delta)
{
    if (!ctx) return;
    ctx->brush_radius += delta;
    if (ctx->brush_radius < MIN_BRUSH_SIZE)
        ctx->brush_radius = MIN_BRUSH_SIZE;
    if (ctx->brush_radius > ctx->max_brush_radius)
        ctx->brush_radius = ctx->max_brush_radius;
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_set_brush_radius_from_key(AppContext *ctx, SDL_Keycode keycode)
{
    if (!ctx) return;
    if (keycode < SDLK_1 || keycode > SDLK_9) return;

    ctx->brush_radius = 5 * (keycode - SDLK_1 + 1);
    if (ctx->brush_radius < MIN_BRUSH_SIZE)
        ctx->brush_radius = MIN_BRUSH_SIZE;
    if (ctx->brush_radius > ctx->max_brush_radius)
        ctx->brush_radius = ctx->max_brush_radius;
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_recalculate_sizes_and_limits(AppContext *ctx)
{
    if (!ctx) return;

    /* Ensure canvas height is up-to-date first (lives in palette module) */
    app_context_update_canvas_display_height(ctx);

    int cell_dim = PALETTE_HEIGHT / 2; /* safe fallback */
    if (ctx->palette && ctx->palette->cols > 0) {
        int cell_w  = ctx->window_w / ctx->palette->cols;
        cell_dim    = (cell_w < PALETTE_HEIGHT ? cell_w : PALETTE_HEIGHT) / 2;
    }
    ctx->max_brush_radius = (cell_dim < MIN_BRUSH_SIZE) ? MIN_BRUSH_SIZE : cell_dim;

    if (ctx->brush_radius > ctx->max_brush_radius)
        ctx->brush_radius = ctx->max_brush_radius;
    if (ctx->brush_radius < MIN_BRUSH_SIZE)
        ctx->brush_radius = MIN_BRUSH_SIZE;
}
