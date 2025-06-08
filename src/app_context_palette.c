// AI Summary: Provides palette visibility toggles, tool selection,
// and computation of drawable canvas height. Also implements helpers
// for palette cycling (next/prev) with wraparound.
#include "app_context.h"
#include "palette.h"
#include "ui_constants.h"

/* ------------ layout helper ------------ */
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

/* ------------ palette toggles ------------ */
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

/* ------------ tool selection ------------ */
void app_context_select_palette_tool(AppContext *ctx, int flat_idx)
{
    if (!ctx || flat_idx < 0 || flat_idx >= ctx->palette->total_cells) {
        return;
    }

    if (palette_is_emoji_index(ctx->palette, flat_idx)) {
        ctx->current_tool = TOOL_EMOJI;
        ctx->emoji_selected_palette_idx = flat_idx;
    } else {
        // A color was picked.
        // If current tool is emoji, switch to the last used color tool.
        if (ctx->current_tool == TOOL_EMOJI) {
            ctx->current_tool = ctx->last_color_tool;
        }

        SDL_Color selected_color = palette_get_color(ctx->palette, flat_idx);

        if (ctx->current_tool == TOOL_WATER_MARKER) {
            ctx->water_marker_color = selected_color;
            ctx->water_marker_selected_palette_idx = flat_idx;
        } else { // Must be TOOL_BRUSH
            ctx->current_color = selected_color;
            ctx->brush_selected_palette_idx = flat_idx;
        }
    }
    ctx->needs_redraw = SDL_TRUE;
}

/* ------------ Palette Cycling for Wheel Support ------------ */
// palette_type: 0 = color, 1 = emoji
void app_context_cycle_palette_selection(AppContext *ctx, int delta, int palette_type)
{
    if (!ctx || !ctx->palette)
        return;

    if (palette_type == 1) {
        // Emoji
        int n = ctx->palette->total_emoji_cells_to_display;
        if (n <= 0) return;
        int base = ctx->palette->total_color_cells;
        int cur = ctx->emoji_selected_palette_idx;
        int rel = cur - base;
        if (rel < 0 || rel >= n) rel = 0;
        rel = (rel + delta + n) % n;
        ctx->emoji_selected_palette_idx = base + rel;
        ctx->current_tool = TOOL_EMOJI;
    } else {
        // Color (for brush or water-marker)
        int n = ctx->palette->total_color_cells;
        if (n <= 0) return;
        int *sel_idx = (ctx->current_tool == TOOL_WATER_MARKER)
                           ? &ctx->water_marker_selected_palette_idx
                           : &ctx->brush_selected_palette_idx;
        int cur = *sel_idx;
        int rel = cur;
        if (rel < 0 || rel >= n) rel = 0;
        rel = (rel + delta + n) % n;
        *sel_idx = rel;
        if (ctx->current_tool == TOOL_WATER_MARKER)
            ctx->water_marker_color = palette_get_color(ctx->palette, *sel_idx);
        else
            ctx->current_color = palette_get_color(ctx->palette, *sel_idx);
    }
}

int app_context_get_current_palette_selection(AppContext *ctx)
{
    if (!ctx || !ctx->palette)
        return -1;
    if (ctx->current_tool == TOOL_EMOJI)
        return ctx->emoji_selected_palette_idx;
    else if (ctx->current_tool == TOOL_WATER_MARKER)
        return ctx->water_marker_selected_palette_idx;
    else
        return ctx->brush_selected_palette_idx;
}
