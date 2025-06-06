// AI Summary: Orchestrates global state creation/destruction, background colour
// handling, canvas initiation, and resize bookkeeping. Brush and palette
// specifics now live in dedicated companion modules.
#include "app_context.h"
#include "palette.h"
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------*/
static void init_canvas_texture(AppContext *ctx, int width, int height)
{
    if (ctx->canvas_texture)
        SDL_DestroyTexture(ctx->canvas_texture);

    ctx->canvas_texture = SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, width, height);
    if (!ctx->canvas_texture) {
        SDL_Log("Failed to create canvas texture: %s", SDL_GetError());
        ctx->canvas_texture_w = ctx->canvas_texture_h = 0;
        return;
    }

    ctx->canvas_texture_w = width;
    ctx->canvas_texture_h = height;

    SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
    SDL_SetRenderDrawColor(ctx->ren,
                           ctx->background_color.r,
                           ctx->background_color.g,
                           ctx->background_color.b,
                           ctx->background_color.a);
    SDL_RenderClear(ctx->ren);
    SDL_SetRenderTarget(ctx->ren, NULL);
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------*/
AppContext *app_context_create(SDL_Window   *win,
                               SDL_Renderer *ren)
{
    AppContext *ctx = malloc(sizeof *ctx);
    if (!ctx) {
        SDL_Log("Failed to allocate AppContext");
        return NULL;
    }

    ctx->win      = win;
    ctx->ren      = ren;
    ctx->window_w = INITIAL_WINDOW_WIDTH;
    ctx->window_h = INITIAL_WINDOW_HEIGHT;

    ctx->background_color = (SDL_Color){ 255, 255, 255, 255 };

    ctx->palette = palette_create(ren, ctx->window_w, ctx->window_h);
    if (!ctx->palette) goto fail;

    ctx->show_color_palette = SDL_TRUE;
    ctx->show_emoji_palette = SDL_TRUE;

    app_context_update_canvas_display_height(ctx);

    ctx->selected_palette_idx =
        ctx->palette->total_color_cells ?
        ctx->palette->total_color_cells - 1 : 0;
    app_context_select_palette_tool(ctx, ctx->selected_palette_idx);

    ctx->brush_radius = 10;
    app_context_recalculate_sizes_and_limits(ctx);

    ctx->canvas_texture = NULL;
    init_canvas_texture(ctx, ctx->window_w, ctx->window_h);

    ctx->needs_redraw      = SDL_TRUE;
    ctx->resize_pending    = SDL_FALSE;
    ctx->last_resize_timestamp = 0;

    return ctx;

fail:
    if (ctx->palette) palette_destroy(ctx->palette);
    free(ctx);
    return NULL;
}

void app_context_destroy(AppContext *ctx)
{
    if (!ctx) return;
    if (ctx->canvas_texture) SDL_DestroyTexture(ctx->canvas_texture);
    palette_destroy(ctx->palette);
    free(ctx);
}

/* ---------------------------------------------------------------------------
 * Background utilities
 * --------------------------------------------------------------------------*/
void app_context_set_background_and_clear_canvas(AppContext *ctx,
                                                 SDL_Color    color)
{
    if (!ctx) return;
    ctx->background_color = color;
    app_context_clear_canvas_with_current_bg(ctx);
}

/* ---------------------------------------------------------------------------
 * Resize bookkeeping
 * --------------------------------------------------------------------------*/
void app_context_notify_resize_event(AppContext *ctx,
                                     int          new_w,
                                     int          new_h)
{
    if (!ctx) return;
    ctx->window_w = new_w < 1 ? 1 : new_w;
    ctx->window_h = new_h < 1 ? 1 : new_h;
    ctx->last_resize_timestamp = SDL_GetTicks();
    ctx->resize_pending        = SDL_TRUE;
    ctx->needs_redraw          = SDL_TRUE;
}
