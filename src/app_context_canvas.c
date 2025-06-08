#include "app_context.h"
#include "draw.h"
#include "palette.h"
#include "ui_constants.h"
#include <math.h>

static void draw_dab_at_point(AppContext *ctx, int x, int y, SDL_bool use_background_color)
{
    if (y >= ctx->canvas_display_area_h || ctx->canvas_display_area_h == 0) {
        return;
    }

    SDL_Texture *target_texture = ctx->canvas_texture;

    if (use_background_color) {
        SDL_SetRenderTarget(ctx->ren, target_texture);
        SDL_SetRenderDrawColor(ctx->ren,
                               ctx->background_color.r,
                               ctx->background_color.g,
                               ctx->background_color.b,
                               ctx->background_color.a);
        draw_circle(ctx->ren, x, y, ctx->brush_radius);
        SDL_SetRenderTarget(ctx->ren, NULL);
        ctx->needs_redraw = SDL_TRUE;
        return;
    }

    switch (ctx->current_tool) {
    case TOOL_BRUSH: {
        SDL_SetRenderTarget(ctx->ren, target_texture);
        SDL_SetRenderDrawColor(ctx->ren,
                               ctx->current_color.r,
                               ctx->current_color.g,
                               ctx->current_color.b,
                               255);
        draw_circle(ctx->ren, x, y, ctx->brush_radius);
    } break;

    case TOOL_WATER_MARKER: {
        if (!ctx->water_marker_stroke_active || !ctx->stroke_buffer) {
            return; // Not in a stroke, do nothing
        }
        target_texture = ctx->stroke_buffer;
        SDL_SetRenderTarget(ctx->ren, target_texture);
        SDL_SetRenderDrawColor(
            ctx->ren, ctx->water_marker_color.r, ctx->water_marker_color.g, ctx->water_marker_color.b, 255);
        int side = lroundf(ctx->brush_radius * 2 * 1.5f);
        SDL_Rect rect = {x - side / 2, y - side / 2, side, side};
        SDL_RenderFillRect(ctx->ren, &rect);
    } break;

    case TOOL_EMOJI: {
        SDL_SetRenderTarget(ctx->ren, target_texture);
        SDL_Texture *emoji_tex = NULL;
        int ew = 0, eh = 0;
        SDL_bool has_emoji = palette_get_emoji_info(
            ctx->palette, ctx->emoji_selected_palette_idx, &emoji_tex, &ew, &eh);
        if (has_emoji && emoji_tex) {
            float asp = (eh == 0) ? 1.0f : (float)ew / eh;
            int h = ctx->brush_radius * 6;
            if (h < MIN_BRUSH_SIZE * 6) {
                h = MIN_BRUSH_SIZE * 6;
            }
            int w = lroundf(h * asp);
            if (w == 0) {
                w = 1;
            }

            SDL_Rect dst = {x - w / 2, y - h / 2, w, h};
            SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst);
        }
    } break;

    default:
        return; // Should not happen
    }

    SDL_SetRenderTarget(ctx->ren, NULL);
    ctx->needs_redraw = SDL_TRUE;
}

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

void app_context_draw_stroke(AppContext *ctx,
                             int mouse_x,
                             int mouse_y,
                             SDL_bool use_background_color)
{
    if (!ctx || !ctx->canvas_texture) {
        return;
    }

    // Emojis are stamped, not drawn as a continuous line.
    // However, right-clicking with the emoji tool should still erase with a circle.
    if (ctx->current_tool == TOOL_EMOJI && !use_background_color) {
        draw_dab_at_point(ctx, mouse_x, mouse_y, use_background_color);
        return;
    }

    // If this is the first point of a stroke, last_stroke_x will be -1.
    int x0 = (ctx->last_stroke_x == -1) ? mouse_x : ctx->last_stroke_x;
    int y0 = (ctx->last_stroke_y == -1) ? mouse_y : ctx->last_stroke_y;
    int x1 = mouse_x;
    int y1 = mouse_y;

    // Bresenham's line algorithm to draw a continuous line of "dabs".
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    for (;;) {
        draw_dab_at_point(ctx, x0, y0, use_background_color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }

    // Update the last point for the next segment of the stroke.
    ctx->last_stroke_x = mouse_x;
    ctx->last_stroke_y = mouse_y;
}

void app_context_recreate_canvas_texture(AppContext *ctx)
{
    if (!ctx) {
        return;
    }

    const int w = ctx->window_w;
    const int h = ctx->window_h;

    SDL_Texture *new_tex =
        SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
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
    ctx->stroke_buffer =
        SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
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
