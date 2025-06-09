#include "event_handler.h"
#include "app.h" // For App and its manipulation functions
#include <stdbool.h>

void handle_events(App *app, int *is_running, int sdl_wait_timeout)
{
    SDL_Event e;
    if (SDL_WaitEventTimeout(&e, sdl_wait_timeout)) {
        do {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                *is_running = 0;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                app_notify_resize_event(app, e.window.data1, e.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN:
                app_handle_keydown(app, &e.key);
                break;
            case SDL_EVENT_KEY_UP:
                app_handle_keyup(app, &e.key);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                app_handle_mousewheel(app, &e.wheel, (int)e.wheel.mouse_x, (int)e.wheel.mouse_y);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (app->is_drawing) {
                    app_draw_stroke(app,
                                    (int)e.motion.x,
                                    (int)e.motion.y,
                                    (e.motion.state & SDL_BUTTON_RMASK) ? true : false);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                app_handle_mousedown(app, &e.button);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                app_handle_mouseup(app, &e.button);
                break;
            }
        } while (SDL_PollEvent(&e)); // Process all pending events
    }
}
