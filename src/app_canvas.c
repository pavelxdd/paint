#include "app.h"

void app_clear_canvas_with_current_bg(App *app)
{
    if (!app) {
        return;
    }

    if (!app->canvas_texture) {
        return;
    }

    if (!SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
        SDL_Log("Failed to set render target to canvas texture: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetRenderDrawColor(app->ren,
                                app->background_color.r,
                                app->background_color.g,
                                app->background_color.b,
                                app->background_color.a)) {
        SDL_Log("Failed to set draw color for canvas clear: %s", SDL_GetError());
    }
    if (!SDL_RenderClear(app->ren)) {
        SDL_Log("Failed to clear canvas: %s", SDL_GetError());
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Failed to reset render target: %s", SDL_GetError());
    }
    app->needs_redraw = true;
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

    /* clear new texture with background color */
    if (!SDL_SetRenderTarget(app->ren, new_tex)) {
        SDL_Log("Failed to set render target to new texture: %s", SDL_GetError());
    } else {
        if (!SDL_SetRenderDrawColor(app->ren,
                                    app->background_color.r,
                                    app->background_color.g,
                                    app->background_color.b,
                                    app->background_color.a)) {
            SDL_Log("Failed to set draw color for new canvas: %s", SDL_GetError());
        }
        if (!SDL_RenderClear(app->ren)) {
            SDL_Log("Failed to clear new canvas: %s", SDL_GetError());
        }
    }

    /* Destroy old canvas texture, no content is preserved on resize */
    if (app->canvas_texture) {
        SDL_DestroyTexture(app->canvas_texture);
    }

    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Failed to reset render target: %s", SDL_GetError());
    }
    app->canvas_texture = new_tex;
    app->canvas_texture_w = w;
    app->canvas_texture_h = h;

    // Recreate stroke buffer
    if (app->stroke_buffer) {
        SDL_DestroyTexture(app->stroke_buffer);
    }
    app->stroke_buffer =
        SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!app->stroke_buffer) {
        SDL_Log("Failed to create stroke buffer texture: %s", SDL_GetError());
    } else {
        if (!SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND)) {
            SDL_Log("Failed to set blend mode for stroke buffer: %s", SDL_GetError());
        }
        // Clear it to transparent
        if (!SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
            SDL_Log("Failed to set render target to stroke buffer: %s", SDL_GetError());
        } else {
            if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE)) {
                SDL_Log("Failed to set blend mode for stroke buffer clear: %s", SDL_GetError());
            }
            if (!SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0)) {
                SDL_Log("Failed to set color for stroke buffer clear: %s", SDL_GetError());
            }
            if (!SDL_RenderClear(app->ren)) {
                SDL_Log("Failed to clear stroke buffer: %s", SDL_GetError());
            }
            if (!SDL_SetRenderTarget(app->ren, NULL)) {
                SDL_Log("Failed to reset render target: %s", SDL_GetError());
            }
        }
    }

    // Recreate blur tool helper resources
    if (app->blur_source_texture) {
        SDL_DestroyTexture(app->blur_source_texture);
    }
    if (app->blur_dab_texture) {
        SDL_DestroyTexture(app->blur_dab_texture);
    }
    if (app->blur_temp_texture) {
        SDL_DestroyTexture(app->blur_temp_texture);
    }

    app->blur_dab_texture = SDL_CreateTexture(app->ren,
                                              SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET,
                                              BLUR_DAB_DOWNSCALE_SIZE, BLUR_DAB_DOWNSCALE_SIZE);

    app->blur_temp_texture = SDL_CreateTexture(app->ren,
                                               SDL_PIXELFORMAT_RGBA8888,
                                               SDL_TEXTUREACCESS_TARGET,
                                               BLUR_DAB_DOWNSCALE_SIZE, BLUR_DAB_DOWNSCALE_SIZE);

    app->blur_source_texture =
        SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

    if (!app->blur_dab_texture || !app->blur_temp_texture || !app->blur_source_texture) {
        SDL_Log("Failed to create blur helper textures.");
    } else {
        if (!SDL_SetTextureBlendMode(app->blur_source_texture, SDL_BLENDMODE_BLEND)) {
            SDL_Log("Failed to set blend mode for blur source texture: %s", SDL_GetError());
        }
    }

    app->needs_redraw = true;
}
