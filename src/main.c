#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_properties.h>
#include <SDL3_ttf/SDL_ttf.h> // For TTF_Init, TTF_Quit
#include <stdbool.h>
#include <stdlib.h> // For EXIT_SUCCESS, EXIT_FAILURE

#include "app.h"   // For App, INITIAL_WINDOW_WIDTH, RESIZE_DEBOUNCE_MS, etc.
#include "event_handler.h" // For handle_events
#include "renderer.h"      // For render_scene

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("TTF_Init error: %s", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Window *win = SDL_CreateWindow("Simple Paint",
                                       INITIAL_WINDOW_WIDTH,
                                       INITIAL_WINDOW_HEIGHT,
                                       SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Log("CreateWindow error: %s", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);
    if (!ren) {
        SDL_Log("CreateRenderer error: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Log renderer info
    SDL_PropertiesID props = SDL_GetRendererProperties(ren);
    if (props) {
        const char *name = SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, NULL);
        SDL_Log("Active renderer: %s", name ? name : "Unknown");

        if (name && SDL_strcmp(name, "software") == 0) {
            SDL_Log("Warning: Renderer is NOT accelerated. "
                    "Performance may be poor.");
        }
    }

    App *app = app_create(win, ren);
    if (!app) {
        SDL_Log("Failed to create App");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    int running = 1;
    while (running) {
        int wait_timeout;
        if (app->needs_redraw) {
            wait_timeout = 16;
        } else if (app->resize_pending) {
            wait_timeout = RESIZE_DEBOUNCE_MS / 4;
        } else {
            wait_timeout = -1;
        }

        handle_events(app, &running, wait_timeout);
        app_process_debounced_resize(app);

        if (app->needs_redraw) {
            render_scene(app);
            app->needs_redraw = false;
        }
    }

    app_destroy(app);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
