#include "app.h"

void app_clear_canvas_with_current_bg(App *app)
{
    if (!app || !app->canvas_texture) {
        return;
    }

    SDL_SetRenderTarget(app->ren, app->canvas_texture);
    SDL_SetRenderDrawColor(app->ren,
                           app->background_color.r,
                           app->background_color.g,
                           app->background_color.b,
                           app->background_color.a);
    SDL_RenderClear(app->ren);
    SDL_SetRenderTarget(app->ren, NULL);
    app->needs_redraw = SDL_TRUE;
}

void app_recreate_canvas_texture(App *app)
{
    if (!app) {
        return;
    }

    const int w = app->window_w;
    const int h = app->window_h;

    SDL_Texture *new_tex =
        SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!new_tex) {
        SDL_Log("Failed to resize canvas texture: %s", SDL_GetError());
        return;
    }

    /* clear new texture with background colour */
    SDL_SetRenderTarget(app->ren, new_tex);
    SDL_SetRenderDrawColor(app->ren,
                           app->background_color.r,
                           app->background_color.g,
                           app->background_color.b,
                           app->background_color.a);
    SDL_RenderClear(app->ren);

    /* Destroy old canvas texture, no content is preserved on resize */
    if (app->canvas_texture) {
        SDL_DestroyTexture(app->canvas_texture);
    }

    SDL_SetRenderTarget(app->ren, NULL);
    app->canvas_texture = new_tex;
    app->canvas_texture_w = w;
    app->canvas_texture_h = h;

    // Recreate stroke buffer as well
    if (app->stroke_buffer) {
        SDL_DestroyTexture(app->stroke_buffer);
    }
    app->stroke_buffer =
        SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!app->stroke_buffer) {
        SDL_Log("Failed to create stroke buffer texture: %s", SDL_GetError());
    } else {
        SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND);
        // Clear it to transparent
        SDL_SetRenderTarget(app->ren, app->stroke_buffer);
        SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0);
        SDL_RenderClear(app->ren);
        SDL_SetRenderTarget(app->ren, NULL);
    }

    app->needs_redraw = SDL_TRUE;
}
