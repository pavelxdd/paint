#include "app.h"
#include "palette.h"
#include "ui_constants.h"

void app_context_recalculate_sizes_and_limits(AppContext *ctx)
{
    if (!ctx) {
        return;
    }

    /* Ensure canvas height is up-to-date first (lives in this module) */
    app_context_update_canvas_display_height(ctx);

    int cell_dim = PALETTE_HEIGHT / 2; /* safe fallback */
    if (ctx->palette && ctx->palette->cols > 0) {
        int cell_w = ctx->window_w / ctx->palette->cols;
        cell_dim = (cell_w < PALETTE_HEIGHT ? cell_w : PALETTE_HEIGHT) / 2;
    }
    ctx->max_brush_radius = (cell_dim < MIN_BRUSH_SIZE) ? MIN_BRUSH_SIZE : cell_dim;

    if (ctx->brush_radius > ctx->max_brush_radius) {
        ctx->brush_radius = ctx->max_brush_radius;
    }
    if (ctx->brush_radius < MIN_BRUSH_SIZE) {
        ctx->brush_radius = MIN_BRUSH_SIZE;
    }
}

void app_context_update_canvas_display_height(AppContext *ctx)
{
    if (!ctx) {
        return;
    }

    int palette_h = 0;

    if (ctx->show_color_palette && ctx->palette && ctx->palette->color_rows) {
        palette_h += ctx->palette->color_rows * PALETTE_HEIGHT;
    }
    if (ctx->show_emoji_palette && ctx->palette && ctx->palette->emoji_rows) {
        palette_h += ctx->palette->emoji_rows * PALETTE_HEIGHT;
    }
    if (ctx->show_color_palette && ctx->show_emoji_palette && ctx->palette &&
        ctx->palette->color_rows && ctx->palette->emoji_rows && COLOR_EMOJI_SEPARATOR_HEIGHT) {
        palette_h += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    int ui_h = palette_h ? TOOL_SELECTOR_SEPARATOR_HEIGHT + palette_h : 0;
    ctx->canvas_display_area_h = ctx->window_h - ui_h;
    if (ctx->canvas_display_area_h < 0) {
        ctx->canvas_display_area_h = 0;
    }
}
