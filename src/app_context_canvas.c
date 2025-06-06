// AI Summary: Encapsulates canvas-related operations previously embedded in
// app_context.c. Functions manage clearing, drawing (brush & emoji), erasing,
// and safe texture recreation during window resizes.
#include "app_context.h"
#include "draw.h"
#include "palette.h"
#include "ui_constants.h"
#include <math.h>

/* ----------------------- Internal helpers ----------------------- */

static SDL_bool is_drawing_with_emoji(const AppContext *ctx)
{
    return ctx->current_tool == TOOL_EMOJI;
}

/* ----------------------- Public API ----------------------------- */

SDL_bool app_context_is_drawing_with_emoji(AppContext *ctx)
{
    return is_drawing_with_emoji(ctx);
}

void app_context_clear_canvas_with_current_bg(AppContext *ctx)
{
    if (!ctx || !ctx->canvas_texture) return;

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
                             int           mouse_x,
                             int           mouse_y,
                             SDL_bool      use_background_color)
{
    if (!ctx || !ctx->canvas_texture) return;
    /* keep within drawable area */
    if (mouse_y >= ctx->canvas_display_area_h ||
        ctx->canvas_display_area_h == 0) return;

    SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);

    if (use_background_color) {
        SDL_SetRenderDrawColor(ctx->ren,
                               ctx->background_color.r,
                               ctx->background_color.g,
                               ctx->background_color.b,
                               ctx->background_color.a);
        draw_circle(ctx->ren, mouse_x, mouse_y, ctx->brush_radius);
    } else if (is_drawing_with_emoji(ctx)) {
        SDL_Texture *emoji_tex = NULL; int ew = 0, eh = 0;
        if (palette_get_emoji_info(ctx->palette,
                                   ctx->selected_palette_idx,
                                   &emoji_tex, &ew, &eh) && emoji_tex) {
            float asp = (eh == 0) ? 1.0f : (float)ew / eh;
            int   h   = ctx->brush_radius * 6;
            if (h < MIN_BRUSH_SIZE * 6) h = MIN_BRUSH_SIZE * 6;
            int w = lroundf(h * asp);
            if (w == 0) w = 1;

            SDL_Rect dst = { mouse_x - w / 2, mouse_y - h / 2, w, h };
            SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst);
        }
    } else {
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
    if (!ctx) return;

    const int w = ctx->window_w;
    const int h = ctx->window_h;

    SDL_Texture *new_tex = SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET, w, h);
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

    /* copy old content if possible */
    if (ctx->canvas_texture) {
        SDL_Rect src = { 0, 0, ctx->canvas_texture_w, ctx->canvas_texture_h };
        SDL_Rect dst = src;
        if (dst.w > w) dst.w = w;
        if (dst.h > h) dst.h = h;
        if (src.w > 0 && src.h > 0 && dst.w > 0 && dst.h > 0)
            SDL_RenderCopy(ctx->ren, ctx->canvas_texture, &src, &dst);

        SDL_DestroyTexture(ctx->canvas_texture);
    }

    SDL_SetRenderTarget(ctx->ren, NULL);
    ctx->canvas_texture   = new_tex;
    ctx->canvas_texture_w = w;
    ctx->canvas_texture_h = h;
    ctx->needs_redraw     = SDL_TRUE;
}
