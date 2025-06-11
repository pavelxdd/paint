#include "app.h"

void app_handle_keydown(App *app, const SDL_KeyboardEvent *key_event)
{
    // Handle specific keys that are not modifiers for other actions.
    switch (key_event->key) {
        case SDLK_ESCAPE:
            app->running = 0;
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            if (key_event->repeat == 0) {
                const bool *state = SDL_GetKeyboardState(NULL);
                // Toggle on press of the *second* control key.
                if ((key_event->key == SDLK_LCTRL && state[SDL_SCANCODE_RCTRL]) ||
                    (key_event->key == SDLK_RCTRL && state[SDL_SCANCODE_LCTRL])) {
                    app_toggle_line_mode(app);
                } else {
                    app->needs_redraw = true; // Redraw to show toggle highlight
                }
            }
            break;
        case SDLK_TAB: {
            // Cycle tools: TAB forward, CTRL+TAB backward
            int count = TOOL_COUNT;
            int idx = (int)app->current_tool;

            if (key_event->mod & SDL_KMOD_CTRL) {
                idx = (idx - 1 + count) % count;
            } else {
                idx = (idx + 1) % count;
            }
            app->current_tool = (ActiveTool)idx;
            if (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_WATER_MARKER) {
                app->last_color_tool = app->current_tool;
            }
            app->needs_redraw = true;
            break;
        }
        case SDLK_0:
            app->current_tool = TOOL_EMOJI;
            app->needs_redraw = true;
            break;
        case SDLK_1:
            app->current_tool = TOOL_BRUSH;
            app->last_color_tool = TOOL_BRUSH;
            app->needs_redraw = true;
            break;
        case SDLK_2:
            app->current_tool = TOOL_WATER_MARKER;
            app->last_color_tool = TOOL_WATER_MARKER;
            app->needs_redraw = true;
            break;
        case SDLK_3:
            app->current_tool = TOOL_BLUR;
            app->needs_redraw = true;
            break;
        case SDLK_F1:
            app_toggle_color_palette(app);
            break;
        case SDLK_F2:
            app_toggle_emoji_palette(app);
            break;
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            app_move_palette_selection(app, key_event->key);
            break;
        case SDLK_F:
            app_toggle_fullscreen(app);
            break;
        default:
            // For other keys, try to see if they are for brush size.
            app_set_brush_radius_from_key(app, key_event->key);
            break;
    }
}

void app_handle_keyup(App *app, const SDL_KeyboardEvent *key_event)
{
    switch (key_event->key) {
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            // When a ctrl key is released, the line toggle button might change state
            // (if it was only highlighted due to the key being held).
            app->needs_redraw = true;
            break;
        default:
            // Other keys do not affect visual state on release.
            break;
    }
}
