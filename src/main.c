#include <stdlib.h>

#include "app.h"
#include "ui.h"
#include "event_handler.h"
#include "renderer.h"

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_srand(0);

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

    Uint64 last_render_time = 0;

    while (app->running) {
        Uint64 frame_start = SDL_GetTicks();

        // Process events with a short timeout
        int wait_timeout = 1;  // Always use a short timeout for responsiveness
        handle_events(app, wait_timeout);
        app_process_debounced_resize(app);

        // Render if needed and enough time has passed since last render
        Uint64 current_time = SDL_GetTicks();
        if (app->needs_redraw && (current_time - last_render_time >= 16)) {  // ~60 FPS
            render_scene(app);
            app->needs_redraw = false;
            last_render_time = current_time;
        }

        // Simple frame rate limiting to avoid excessive CPU usage
        Uint64 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 8) {  // Ensure we don't spin too fast
            SDL_Delay(1);
        }
    }

    app_destroy(app);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
