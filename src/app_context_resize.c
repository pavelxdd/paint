#include "app_context.h"
#include "palette.h"

void app_context_notify_resize_event(AppContext *ctx, int new_w, int new_h)
{
    if (!ctx) {
        return;
    }
    ctx->window_w = new_w < 1 ? 1 : new_w;
    ctx->window_h = new_h < 1 ? 1 : new_h;
    ctx->last_resize_timestamp = SDL_GetTicks();
    ctx->resize_pending = SDL_TRUE;
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_process_debounced_resize(AppContext *ctx)
{
    if (ctx->resize_pending &&
        (SDL_GetTicks() - ctx->last_resize_timestamp >= RESIZE_DEBOUNCE_MS)) {
        // Before palette is recreated, check for special cases to preserve them.
        SDL_bool brush_was_top_left = (ctx->brush_selected_palette_idx == 0);
        SDL_bool water_marker_was_top_left = (ctx->water_marker_selected_palette_idx == 0);
        SDL_bool water_marker_was_bottom_right = SDL_FALSE;
        if (ctx->palette->total_color_cells > 0) {
            water_marker_was_bottom_right =
                (ctx->water_marker_selected_palette_idx == ctx->palette->total_color_cells - 1);
        }

        // 1. Recreate palette: recalculates rows, columns, colors, and shuffles emojis
        palette_recreate(ctx->palette, ctx->window_w, ctx->window_h);

        // 2. Update canvas display height based on new window height and new palette layout
        app_context_update_canvas_display_height(ctx);

        // 3. Reset selections and colors, preserving special cases.
        // Brush: stays top-left if it was, otherwise defaults to bottom-right.
        if (brush_was_top_left) {
            ctx->brush_selected_palette_idx = 0;
        } else {
            ctx->brush_selected_palette_idx =
                ctx->palette->total_color_cells > 0 ? ctx->palette->total_color_cells - 1 : 0;
        }

        // Water-marker: stays top-left or bottom-right if it was, otherwise defaults to
        // top-left.
        if (water_marker_was_top_left) {
            ctx->water_marker_selected_palette_idx = 0;
        } else if (water_marker_was_bottom_right) {
            ctx->water_marker_selected_palette_idx =
                ctx->palette->total_color_cells > 0 ? ctx->palette->total_color_cells - 1 : 0;
        } else {
            ctx->water_marker_selected_palette_idx = 0; // Default to top-left
        }

        // Update colors from new palette
        ctx->current_color = palette_get_color(ctx->palette, ctx->brush_selected_palette_idx);
        ctx->water_marker_color =
            palette_get_color(ctx->palette, ctx->water_marker_selected_palette_idx);

        ctx->emoji_selected_palette_idx = ctx->palette->total_color_cells;

        // 4. Recalculate brush size limits based on new layout
        app_context_recalculate_sizes_and_limits(ctx);

        // 5. Create a new canvas texture, preserving the old content
        app_context_recreate_canvas_texture(ctx);

        ctx->resize_pending = SDL_FALSE;
        ctx->needs_redraw = SDL_TRUE;
    }
}
