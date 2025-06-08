#include "event_handler.h"
#include "app.h" // For App and its manipulation functions

void handle_events(App *app, int *is_running, Uint32 sdl_wait_timeout)
{
    SDL_Event e;
    int last_mouse_x, last_mouse_y;
    // Grab current mouse position once, used for mouse wheel events.
    SDL_GetMouseState(&last_mouse_x, &last_mouse_y);
    if (SDL_WaitEventTimeout(&e, sdl_wait_timeout)) {
        do {
            switch (e.type) {
            case SDL_QUIT:
                *is_running = 0;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    app_notify_resize_event(app, e.window.data1, e.window.data2);
                }
                break;
            case SDL_KEYDOWN:
                app_handle_keydown(app, &e.key);
                break;
            case SDL_KEYUP:
                app_handle_keyup(app, &e.key);
                break;
            case SDL_MOUSEWHEEL:
                app_handle_mousewheel(app, &e.wheel, last_mouse_x, last_mouse_y);
                break;
            case SDL_MOUSEMOTION:
                if (app->is_drawing) {
                    app_draw_stroke(
                        app, e.motion.x, e.motion.y, (e.motion.state & SDL_BUTTON_RMASK) ? SDL_TRUE : SDL_FALSE);
                }
                // Update mouse position tracking for next mouse wheel event
                last_mouse_x = e.motion.x;
                last_mouse_y = e.motion.y;
                break;
            case SDL_MOUSEBUTTONDOWN:
                app_handle_mousedown(app, &e.button);
                break;
            case SDL_MOUSEBUTTONUP:
                app_handle_mouseup(app, &e.button);
                break;
            }
        } while (SDL_PollEvent(&e)); // Process all pending events
    }
}
