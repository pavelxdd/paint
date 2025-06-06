// AI Summary: Provides palette visibility toggles, tool selection,
// and computation of drawable canvas height.
#include "app_context.h"
#include "palette.h"
#include "ui_constants.h"

/* ------------ layout helper ------------ */
void app_context_update_canvas_display_height(AppContext *ctx)
{
    if (!ctx) return;

    int palette_h = 0;

    if (ctx->show_color_palette && ctx->palette && ctx->palette->color_rows)
        palette_h += ctx->palette->color_rows * PALETTE_HEIGHT;
    if (ctx->show_emoji_palette && ctx->palette && ctx->palette->emoji_rows)
        palette_h += ctx->palette->emoji_rows * PALETTE_HEIGHT;
    if (ctx->show_color_palette && ctx->show_emoji_palette &&
        ctx->palette && ctx->palette->color_rows && ctx->palette->emoji_rows &&
        COLOR_EMOJI_SEPARATOR_HEIGHT)
        palette_h += COLOR_EMOJI_SEPARATOR_HEIGHT;

    int ui_h = palette_h ? TOOL_SELECTOR_SEPARATOR_HEIGHT + palette_h : 0;
    ctx->canvas_display_area_h = ctx->window_h - ui_h;
    if (ctx->canvas_display_area_h < 0)
        ctx->canvas_display_area_h = 0;
}

/* ------------ palette toggles ------------ */
void app_context_toggle_color_palette(AppContext *ctx)
{
    if (!ctx) return;
    ctx->show_color_palette = !ctx->show_color_palette;
    app_context_update_canvas_display_height(ctx);
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_toggle_emoji_palette(AppContext *ctx)
{
    if (!ctx) return;
    ctx->show_emoji_palette = !ctx->show_emoji_palette;
    app_context_update_canvas_display_height(ctx);
    ctx->needs_redraw = SDL_TRUE;
}

/* ------------ tool selection ------------ */
void app_context_select_palette_tool(AppContext *ctx, int flat_idx)
{
    if (!ctx || flat_idx < 0 || flat_idx >= ctx->palette->total_cells) return;

    ctx->selected_palette_idx = flat_idx;

    if (palette_is_emoji_index(ctx->palette, flat_idx)) {
        ctx->current_tool  = TOOL_EMOJI;
        ctx->current_color = (SDL_Color){ 40, 42, 54, 255 };
    } else {
        ctx->current_tool  = TOOL_BRUSH;
        ctx->current_color = palette_get_color(ctx->palette, flat_idx);
    }
    ctx->needs_redraw = SDL_TRUE;
}
