#include "app_context.h"
#include "palette.h"
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------*/
AppContext *app_context_create(SDL_Window *win, SDL_Renderer *ren)
{
    AppContext *ctx = malloc(sizeof *ctx);
    if (!ctx) {
        SDL_Log("Failed to allocate AppContext");
        return NULL;
    }

    ctx->win = win;
    ctx->ren = ren;
    ctx->window_w = INITIAL_WINDOW_WIDTH;
    ctx->window_h = INITIAL_WINDOW_HEIGHT;

    ctx->background_color = (SDL_Color){255, 255, 255, 255};

    ctx->palette = palette_create(ren, ctx->window_w, ctx->window_h);
    if (!ctx->palette) {
        goto fail;
    }

    ctx->show_color_palette = SDL_TRUE;
    ctx->show_emoji_palette = SDL_TRUE;

    app_context_update_canvas_display_height(ctx);

    // Set default colors and tool
    ctx->current_tool = TOOL_BRUSH;
    ctx->last_color_tool = TOOL_BRUSH;

    // Default water-marker to red (top-left)
    ctx->water_marker_selected_palette_idx = 0;
    if (ctx->palette->total_color_cells > 0) {
        ctx->water_marker_color =
            palette_get_color(ctx->palette, ctx->water_marker_selected_palette_idx);
    } else {
        ctx->water_marker_color = (SDL_Color){255, 0, 0, 255}; // Fallback red
    }

    // Default brush to black (bottom-right)
    ctx->brush_selected_palette_idx =
        ctx->palette->total_color_cells ? ctx->palette->total_color_cells - 1 : 0;
    if (ctx->palette->total_color_cells > 0) {
        ctx->current_color = palette_get_color(ctx->palette, ctx->brush_selected_palette_idx);
    } else {
        ctx->current_color = (SDL_Color){0, 0, 0, 255}; // Fallback black
    }

    // Default emoji to first one
    ctx->emoji_selected_palette_idx = ctx->palette->total_color_cells;

    ctx->brush_radius = 10;
    app_context_recalculate_sizes_and_limits(ctx);

    ctx->canvas_texture = NULL;
    ctx->stroke_buffer = NULL;
    app_context_recreate_canvas_texture(ctx);

    ctx->needs_redraw = SDL_TRUE;
    ctx->resize_pending = SDL_FALSE;
    ctx->last_resize_timestamp = 0;
    ctx->water_marker_stroke_active = SDL_FALSE;
    ctx->line_mode_toggled_on = SDL_FALSE;
    ctx->is_drawing = SDL_FALSE;
    ctx->straight_line_stroke_latched = SDL_FALSE;
    ctx->last_stroke_x = -1;
    ctx->last_stroke_y = -1;

    return ctx;

fail:
    if (ctx->palette) {
        palette_destroy(ctx->palette);
    }
    free(ctx);
    return NULL;
}

void app_context_destroy(AppContext *ctx)
{
    if (!ctx) {
        return;
    }
    if (ctx->canvas_texture) {
        SDL_DestroyTexture(ctx->canvas_texture);
    }
    if (ctx->stroke_buffer) {
        SDL_DestroyTexture(ctx->stroke_buffer);
    }
    palette_destroy(ctx->palette);
    free(ctx);
}

/* ---------------------------------------------------------------------------
 * Background utilities
 * --------------------------------------------------------------------------*/
void app_context_set_background_and_clear_canvas(AppContext *ctx, SDL_Color color)
{
    if (!ctx) {
        return;
    }
    ctx->background_color = color;
    app_context_clear_canvas_with_current_bg(ctx);
}
