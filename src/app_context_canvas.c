// AI Summary: Encapsulates canvas-related operations previously embedded in
// app_context.c. Functions manage clearing, drawing (brush & emoji), erasing,
// and safe texture recreation during window resizes. It also handles stroke-buffering
// for tools like the water-marker to ensure proper transparency.
#include "app_context.h"
#include "draw.h"
#include "palette.h"
#include "ui_constants.h"
#include <math.h>

/* ----------------------- Public API ----------------------------- */

void app_context_clear_canvas_with_current_bg(AppContext *ctx)
{
    if (!ctx || !ctx->canvas_texture) {
        return;
    }

    SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
    SDL_SetRenderDrawColor(ctx->ren,
                           ctx->background_color.r,
                           ctx->background_color.g,
                           ctx->background_color.b,
                           ctx->background_color.a);
    SDL_RenderClear(ctx->ren);
    SDL_SetRenderTarget(ctx->ren, NULL);
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_draw_stroke(AppContext *ctx, int mouse_x, int mouse_y, SDL_bool use_background_color)
{
    if (!ctx || !ctx->canvas_texture) {
        return;
    }
    /* keep within drawable area */
    if (mouse_y >= ctx->canvas_display_area_h || ctx->canvas_display_area_h == 0) {
        return;
    }

    SDL_Texture *target_texture = ctx->canvas_texture;
    if (ctx->current_tool == TOOL_WATER_MARKER && !use_background_color) {
        if (!ctx->water_marker_stroke_active || !ctx->stroke_buffer) {
            return; // Not in a stroke, do nothing
        }
        target_texture = ctx->stroke_buffer;
    }

    SDL_SetRenderTarget(ctx->ren, target_texture);

    if (use_background_color) {
        SDL_SetRenderDrawColor(ctx->ren,
                               ctx->background_color.r,
                               ctx->background_color.g,
                               ctx->background_color.b,
                               ctx->background_color.a);
        draw_circle(ctx->ren, mouse_x, mouse_y, ctx->brush_radius);
    } else if (ctx->current_tool == TOOL_WATER_MARKER) {
        SDL_SetRenderDrawColor(ctx->ren,
                               ctx->water_marker_color.r,
                               ctx->water_marker_color.g,
                               ctx->water_marker_color.b,
                               255); // Opaque on buffer
        int side = lroundf(ctx->brush_radius * 2 * 1.5f);
        SDL_Rect rect = {mouse_x - side / 2, mouse_y - side / 2, side, side};
        SDL_RenderFillRect(ctx->ren, &rect);
    } else if (ctx->current_tool == TOOL_EMOJI) {
        SDL_Texture *emoji_tex = NULL;
        int ew = 0, eh = 0;
        if (palette_get_emoji_info(
                ctx->palette, ctx->emoji_selected_palette_idx, &emoji_tex, &ew, &eh) &&
            emoji_tex) {
            float asp = (eh == 0) ? 1.0f : (float)ew / eh;
            int h = ctx->brush_radius * 6;
            if (h < MIN_BRUSH_SIZE * 6) {
                h = MIN_BRUSH_SIZE * 6;
            }
            int w = lroundf(h * asp);
            if (w == 0) {
                w = 1;
            }

            SDL_Rect dst = {mouse_x - w / 2, mouse_y - h / 2, w, h};
            SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst);
        }
    } else { // TOOL_BRUSH
        SDL_SetRenderDrawColor(ctx->ren,
                               ctx->current_color.r,
                               ctx->current_color.g,
                               ctx->current_color.b,
                               255);
        draw_circle(ctx->ren, mouse_x, mouse_y, ctx->brush_radius);
    }

    SDL_SetRenderTarget(ctx->ren, NULL);
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_recreate_canvas_texture(AppContext *ctx)
{
    if (!ctx) {
        return;
    }

    const int w = ctx->window_w;
    const int h = ctx->window_h;

    SDL_Texture *new_tex = SDL_CreateTexture(
        ctx->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!new_tex) {
        SDL_Log("Failed to resize canvas texture: %s", SDL_GetError());
        return;
    }

    /* clear new texture with background colour */
    SDL_SetRenderTarget(ctx->ren, new_tex);
    SDL_SetRenderDrawColor(ctx->ren,
                           ctx->background_color.r,
                           ctx->background_color.g,
                           ctx->background_color.b,
                           ctx->background_color.a);
    SDL_RenderClear(ctx->ren);

    /* Destroy old canvas texture, no content is preserved on resize */
    if (ctx->canvas_texture) {
        SDL_DestroyTexture(ctx->canvas_texture);
    }

    SDL_SetRenderTarget(ctx->ren, NULL);
    ctx->canvas_texture = new_tex;
    ctx->canvas_texture_w = w;
    ctx->canvas_texture_h = h;

    // Recreate stroke buffer as well
    if (ctx->stroke_buffer) {
        SDL_DestroyTexture(ctx->stroke_buffer);
    }
    ctx->stroke_buffer = SDL_CreateTexture(
        ctx->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!ctx->stroke_buffer) {
        SDL_Log("Failed to create stroke buffer texture: %s", SDL_GetError());
    } else {
        SDL_SetTextureBlendMode(ctx->stroke_buffer, SDL_BLENDMODE_BLEND);
        // Clear it to transparent
        SDL_SetRenderTarget(ctx->ren, ctx->stroke_buffer);
        SDL_SetRenderDrawBlendMode(ctx->ren, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, 0);
        SDL_RenderClear(ctx->ren);
        SDL_SetRenderTarget(ctx->ren, NULL);
    }

    ctx->needs_redraw = SDL_TRUE;
}

void app_context_begin_water_marker_stroke(AppContext *ctx)
{
    if (!ctx || !ctx->stroke_buffer) {
        return;
    }
    ctx->water_marker_stroke_active = SDL_TRUE;

    // Clear the buffer to be fully transparent for the new stroke
    SDL_SetRenderTarget(ctx->ren, ctx->stroke_buffer);
    SDL_SetRenderDrawBlendMode(ctx->ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, 0);
    SDL_RenderClear(ctx->ren);
    SDL_SetRenderTarget(ctx->ren, NULL);
}

void app_context_end_water_marker_stroke(AppContext *ctx)
{
    if (!ctx || !ctx->stroke_buffer || !ctx->water_marker_stroke_active) {
        return;
    }

    // Blend the completed stroke from the buffer onto the main canvas
    SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
    SDL_SetTextureBlendMode(ctx->stroke_buffer, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(ctx->stroke_buffer, 128); // 50% alpha
    SDL_RenderCopy(ctx->ren, ctx->stroke_buffer, NULL, NULL);

    // Reset texture properties and renderer target
    SDL_SetTextureAlphaMod(ctx->stroke_buffer, 255);
    SDL_SetRenderTarget(ctx->ren, NULL);

    ctx->water_marker_stroke_active = SDL_FALSE;
    ctx->needs_redraw = SDL_TRUE;
}
