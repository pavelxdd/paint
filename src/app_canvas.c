#include "app.h"

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
