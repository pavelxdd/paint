#include "app.h"

void app_context_handle_keydown(AppContext *ctx, const SDL_KeyboardEvent *key_event)
{
    // Handle specific keys that are not modifiers for other actions.
    switch (key_event->keysym.sym) {
    case SDLK_LCTRL:
        if (key_event->repeat == 0) {
            const Uint8 *state = SDL_GetKeyboardState(NULL);
            if (state[SDL_SCANCODE_RCTRL]) {
                app_context_toggle_line_mode(ctx);
            } else {
                ctx->needs_redraw = SDL_TRUE; // Redraw to show toggle highlight
            }
        }
        break;
    case SDLK_RCTRL:
        if (key_event->repeat == 0) {
            const Uint8 *state = SDL_GetKeyboardState(NULL);
            if (state[SDL_SCANCODE_LCTRL]) {
                app_context_toggle_line_mode(ctx);
            } else {
                ctx->needs_redraw = SDL_TRUE; // Redraw to show toggle highlight
            }
        }
        break;
    case SDLK_TAB: {
        // Cycle tools: TAB forward, CTRL+TAB backward
        int count = __TOOL_COUNT;
        int idx = (int)ctx->current_tool;

        if (key_event->keysym.mod & KMOD_CTRL) {
            idx = (idx - 1 + count) % count;
        } else {
            idx = (idx + 1) % count;
        }
        ctx->current_tool = (ActiveTool)idx;
        if (ctx->current_tool == TOOL_BRUSH || ctx->current_tool == TOOL_WATER_MARKER) {
            ctx->last_color_tool = ctx->current_tool;
        }
        ctx->needs_redraw = SDL_TRUE;
        break;
    }
    case SDLK_0:
        ctx->current_tool = TOOL_EMOJI;
        ctx->needs_redraw = SDL_TRUE;
        break;
    case SDLK_1:
        ctx->current_tool = TOOL_BRUSH;
        ctx->last_color_tool = TOOL_BRUSH;
        ctx->needs_redraw = SDL_TRUE;
        break;
    case SDLK_2:
        ctx->current_tool = TOOL_WATER_MARKER;
        ctx->last_color_tool = TOOL_WATER_MARKER;
        ctx->needs_redraw = SDL_TRUE;
        break;
    case SDLK_F1:
        app_context_toggle_color_palette(ctx);
        break;
    case SDLK_F2:
        app_context_toggle_emoji_palette(ctx);
        break;
    case SDLK_UP:
    case SDLK_DOWN:
    case SDLK_LEFT:
    case SDLK_RIGHT:
        app_context_move_palette_selection(ctx, key_event->keysym.sym);
        break;
    case SDLK_f:
        app_context_toggle_fullscreen(ctx);
        break;
    default:
        // For other keys, try to see if they are for brush size.
        app_context_set_brush_radius_from_key(ctx, key_event->keysym.sym);
        break;
    }
}

void app_context_handle_keyup(AppContext *ctx, const SDL_KeyboardEvent *key_event)
{
    switch (key_event->keysym.sym) {
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        // When a ctrl key is released, the line toggle button might change state
        // (if it was only highlighted due to the key being held).
        ctx->needs_redraw = SDL_TRUE;
        break;
    default:
        // Other keys do not affect visual state on release.
        break;
    }
}
