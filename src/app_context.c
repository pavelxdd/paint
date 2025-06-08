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
 * State updates & queries
 * --------------------------------------------------------------------------*/

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

/* ---------------------------------------------------------------------------
 * Resize bookkeeping
 * --------------------------------------------------------------------------*/
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
