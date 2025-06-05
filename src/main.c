// AI Summary: Initializes SDL and SDL_ttf, creates the main application window and renderer,
// and sets up the AppContext. Runs the main application loop, orchestrating
// event handling, debounced resize processing, and scene rendering (including emojis).
// Delegates specific logic to app_context, event_handler, and resize_handler modules.
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // For TTF_Init, TTF_Quit
#include <stdlib.h> // For EXIT_SUCCESS, EXIT_FAILURE

#include "app_context.h"    // For AppContext, INITIAL_WINDOW_WIDTH, RESIZE_DEBOUNCE_MS, etc.
#include "event_handler.h"  // For handle_events
#include "resize_handler.h" // For process_debounced_resize
#include "palette.h"        // For palette_draw (used in render_scene)

static void render_scene(AppContext *ctx) {
    SDL_SetRenderDrawColor(ctx->ren, 255, 255, 255, 255); 
    SDL_RenderClear(ctx->ren);

    // canvas_display_area_h is already calculated and stored in ctx
    SDL_Rect canvas_dst_rect_in_window = {0, 0, ctx->window_w, ctx->canvas_display_area_h};

    // Only render canvas if its display area is positive and texture exists
    if (ctx->canvas_display_area_h > 0 && ctx->canvas_texture) {
        SDL_RenderCopy(ctx->ren, ctx->canvas_texture, NULL, &canvas_dst_rect_in_window);
    }

    // Draw separator line
    if (CANVAS_PALETTE_SEPARATOR_HEIGHT > 0) {
        SDL_SetRenderDrawColor(ctx->ren, 237, 237, 237, 255); // #ededed
        SDL_Rect sep_rect = {0, ctx->canvas_display_area_h, ctx->window_w, CANVAS_PALETTE_SEPARATOR_HEIGHT};
        SDL_RenderFillRect(ctx->ren, &sep_rect);
    }

    int palette_start_y = ctx->canvas_display_area_h + CANVAS_PALETTE_SEPARATOR_HEIGHT;
    palette_draw(ctx->palette, ctx->ren, palette_start_y, ctx->window_w, ctx->selected_palette_idx, ctx->brush_radius);

    SDL_RenderPresent(ctx->ren);
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init error: %s", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Window *win = SDL_CreateWindow("Simple Paint",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT,
                                       SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Log("CreateWindow error: %s", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        SDL_Log("CreateRenderer error: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Log renderer info
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0) {
        SDL_Log("Active renderer: %s", info.name);
        if (info.flags & SDL_RENDERER_ACCELERATED) {
            SDL_Log("Renderer is accelerated.");
        } else {
            SDL_Log("Warning: Renderer is NOT accelerated. Performance may be poor.");
        }
        if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
            SDL_Log("Renderer supports target textures.");
        } else {
            SDL_Log("Error: Renderer does NOT support target textures. Application may not work correctly.");
            // This is a critical error, consider exiting if absolutely necessary
        }
    }

    AppContext *app_ctx = app_context_create(win, ren);
    if (!app_ctx) {
        SDL_Log("Failed to create AppContext");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    int running = 1;
    while (running) {
        Uint32 wait_timeout = app_ctx->needs_redraw ? 16 : (app_ctx->resize_pending ? (RESIZE_DEBOUNCE_MS / 4) : (Uint32)-1);

        handle_events(app_ctx, &running, wait_timeout);
        process_debounced_resize(app_ctx);

        if (app_ctx->needs_redraw) {
            render_scene(app_ctx);
            app_ctx->needs_redraw = SDL_FALSE;
        }
    }

    app_context_destroy(app_ctx);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
