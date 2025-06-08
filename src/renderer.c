#include "renderer.h"
#include "palette.h"
#include "tool_selectors.h"

void render_scene(AppContext *ctx)
{
    SDL_SetRenderDrawColor(ctx->ren, 255, 255, 255, 255);
    SDL_RenderClear(ctx->ren);

    // 1. Render the canvas texture to the entire window first.
    if (ctx->canvas_texture) {
        SDL_RenderCopy(ctx->ren, ctx->canvas_texture, NULL, NULL);
    }

    // 2. Render tool previews from the stroke buffer if necessary.
    if (ctx->is_drawing && ctx->straight_line_stroke_latched && ctx->stroke_buffer) {
        // Render a straight line preview
        if (ctx->current_tool == TOOL_BRUSH || ctx->current_tool == TOOL_EMOJI) {
            // Brush and Emoji previews are opaque and on the buffer
            SDL_RenderCopy(ctx->ren, ctx->stroke_buffer, NULL, NULL);
        } else if (ctx->current_tool == TOOL_WATER_MARKER) {
            // Water-marker preview is semi-transparent
            SDL_SetTextureAlphaMod(ctx->stroke_buffer, 128);
            SDL_RenderCopy(ctx->ren, ctx->stroke_buffer, NULL, NULL);
            SDL_SetTextureAlphaMod(ctx->stroke_buffer, 255);
        }
    } else if (ctx->water_marker_stroke_active && ctx->stroke_buffer) {
        // Render a freehand water-marker stroke in progress
        SDL_SetTextureAlphaMod(ctx->stroke_buffer, 128);
        SDL_RenderCopy(ctx->ren, ctx->stroke_buffer, NULL, NULL);
        SDL_SetTextureAlphaMod(ctx->stroke_buffer, 255); // Reset for other potential uses
    }

    // --- UI drawing from top to bottom, overlaid on the canvas ---

    // 3. Tool selectors "float" over the canvas, just above the main UI panel.
    // Their Y position is calculated from the top of the main UI block.
    int tool_selectors_y = ctx->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    tool_selectors_draw(ctx, tool_selectors_y);

    // 4. The main UI block (palette and its separator) starts at canvas_display_area_h.
    int current_y = ctx->canvas_display_area_h;

    // 5. Separator between canvas/selectors and palette (if palette is visible)
    SDL_bool is_palette_content_visible =
        (ctx->show_color_palette && ctx->palette->color_rows > 0) ||
        (ctx->show_emoji_palette && ctx->palette->emoji_rows > 0);
    if (is_palette_content_visible && TOOL_SELECTOR_SEPARATOR_HEIGHT > 0) {
        SDL_SetRenderDrawColor(ctx->ren, 68, 71, 90, 255); // Dracula 'Current Line'
        SDL_Rect sep_rect = {0, current_y, ctx->window_w, TOOL_SELECTOR_SEPARATOR_HEIGHT};
        SDL_RenderFillRect(ctx->ren, &sep_rect);
        current_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
    }

    // 6. Palette (conditionally visible rows)
    int active_palette_idx = app_context_get_current_palette_selection(ctx);
    palette_draw(ctx->palette,
                 ctx->ren,
                 current_y,
                 ctx->window_w,
                 active_palette_idx,
                 ctx->show_color_palette,
                 ctx->show_emoji_palette);

    SDL_RenderPresent(ctx->ren);
}
