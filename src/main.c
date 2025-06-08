#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // For TTF_Init, TTF_Quit
#include <stdlib.h>      // For EXIT_SUCCESS, EXIT_FAILURE

#include "app_context.h"   // For AppContext, INITIAL_WINDOW_WIDTH, RESIZE_DEBOUNCE_MS, etc.
#include "event_handler.h" // For handle_events
#include "renderer.h"      // For render_scene

int main(void)
{
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
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       INITIAL_WINDOW_WIDTH,
                                       INITIAL_WINDOW_HEIGHT,
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
            SDL_Log("Warning: Renderer is NOT accelerated. "
                    "Performance may be poor.");
        }
        if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
            SDL_Log("Renderer supports target textures.");
        } else {
            SDL_Log("Error: Renderer does NOT support target textures. "
                    "Application may not work correctly.");
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
        Uint32 wait_timeout;
        if (app_ctx->needs_redraw) {
            wait_timeout = 16;
        } else if (app_ctx->resize_pending) {
            wait_timeout = RESIZE_DEBOUNCE_MS / 4;
        } else {
            wait_timeout = (Uint32)-1;
        }

        handle_events(app_ctx, &running, wait_timeout);
        app_context_process_debounced_resize(app_ctx);

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
