// AI Summary: Implements the debounced window resize handling for the paint application.
// When a resize is pending and the debounce time has passed, it recreates the
// palette (colors and emojis, including reshuffling emojis and dynamically adjusting row counts),
// recalculates brush size limits, and creates a new canvas texture of the appropriate size,
// copying over the content from the old canvas.
#include "resize_handler.h"
#include "app_context.h" // For AppContext, RESIZE_DEBOUNCE_MS, etc.
#include "palette.h"     // For palette_recreate

void process_debounced_resize(AppContext *ctx)
{
    if (ctx->resize_pending &&
        (SDL_GetTicks() - ctx->last_resize_timestamp >= RESIZE_DEBOUNCE_MS)) {
        // Before palette is recreated, check if the last color (black) was selected.
        SDL_bool was_black_selected = SDL_FALSE;
        if (ctx->palette->total_color_cells > 0 &&
            ctx->selected_palette_idx == ctx->palette->total_color_cells - 1) {
            was_black_selected = SDL_TRUE;
        }

        // 1. Recreate palette: recalculates rows, columns, colors, and shuffles emojis
        palette_recreate(ctx->palette, ctx->window_w, ctx->window_h);

        // 2. Update canvas display height based on new window height and new palette layout
        app_context_update_canvas_display_height(ctx);

        // 3. Try to maintain selection or reset to a default color/tool
        int new_selection_idx;
        if (was_black_selected && ctx->palette->total_color_cells > 0) {
            new_selection_idx = ctx->palette->total_color_cells - 1;
        } else {
            // Fallback for other colors/emojis, which might not be preserved correctly.
            new_selection_idx = ctx->selected_palette_idx;
            if (new_selection_idx >= ctx->palette->total_cells || new_selection_idx < 0) {
                if (ctx->palette->total_color_cells > 0) {
                    new_selection_idx = ctx->palette->total_color_cells - 1; // Default to black
                } else if (ctx->palette->total_cells > 0) {
                    new_selection_idx = 0; // Default to first emoji
                } else {
                    new_selection_idx = 0; // Should not happen
                }
            }
        }
        app_context_select_palette_tool(ctx, new_selection_idx);

        // 4. Recalculate brush size limits based on new layout
        app_context_recalculate_sizes_and_limits(ctx);

        // 5. Create a new canvas texture, preserving the old content
        app_context_recreate_canvas_texture(ctx);

        ctx->resize_pending = SDL_FALSE;
        ctx->needs_redraw = SDL_TRUE;
    }
}
