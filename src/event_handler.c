#include "app.h"
#include "event_handler.h"

void handle_events(App *app, int sdl_wait_timeout)
{
    SDL_Event e;
    if (SDL_WaitEventTimeout(&e, sdl_wait_timeout)) {
        do {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    app->running = false;
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
                    app_handle_mousewheel(app, &e.wheel, e.wheel.mouse_x, e.wheel.mouse_y);
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (app->is_drawing) {
                        app->has_moved_since_mousedown = true;
                        app_draw_stroke(
                            app, e.motion.x, e.motion.y, (e.motion.state & SDL_BUTTON_RMASK) != 0);
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
