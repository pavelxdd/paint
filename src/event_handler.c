// AI Summary: Implements SDL event polling (using SDL_WaitEventTimeout) and processing
// for the paint application. It checks for various event types like quit, window
// resize, key presses (for brush size), mouse wheel (for brush size), and mouse
// button/motion events for drawing, tool/color picking, and background changes.
// Interacts with AppContext to modify application state based on events.
#include "event_handler.h"
#include "app_context.h" // For AppContext and its manipulation functions
#include "palette.h"     // For palette_hit_test, palette_get_color, palette_is_emoji_index etc.

void handle_events(AppContext *ctx, int *is_running, Uint32 sdl_wait_timeout) {
    SDL_Event e;
    if (SDL_WaitEventTimeout(&e, sdl_wait_timeout)) {
        do {
            if (e.type == SDL_QUIT) {
                *is_running = 0;
            } else if (e.type == SDL_WINDOWEVENT &&
                       e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                app_context_notify_resize_event(ctx, e.window.data1, e.window.data2);
            } else if (e.type == SDL_KEYDOWN) {
                app_context_set_brush_radius_from_key(ctx, e.key.keysym.sym);
            } else if (e.type == SDL_MOUSEWHEEL) {
                if (e.wheel.y > 0) { // Scroll up
                    app_context_change_brush_radius(ctx, 2);
                } else if (e.wheel.y < 0) { // Scroll down
                    app_context_change_brush_radius(ctx, -2);
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION) {
                // canvas_display_area_h is already up-to-date in ctx

                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE) {
                    int mx = e.button.x;
                    int my = e.button.y;

                    int palette_interaction_start_y = ctx->canvas_display_area_h + CANVAS_PALETTE_SEPARATOR_HEIGHT;
                    int palette_idx = palette_hit_test(ctx->palette, mx, my, ctx->window_w, palette_interaction_start_y);
                    if (palette_idx != -1) { // Middle-clicked on a palette color
                        SDL_Color new_bg_color = palette_get_color(ctx->palette, palette_idx);
                        app_context_set_background_and_clear_canvas(ctx, new_bg_color);
                    } else if (my < ctx->canvas_display_area_h) { // Middle-clicked on canvas (not palette)
                        app_context_clear_canvas_with_current_bg(ctx);
                    }
                } else { // Left/Right button down or motion
                    int mx, my;
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        mx = e.button.x;
                        my = e.button.y;
                        if (e.button.button == SDL_BUTTON_LEFT) {
                            int palette_interaction_start_y = ctx->canvas_display_area_h + CANVAS_PALETTE_SEPARATOR_HEIGHT;
                            int palette_idx = palette_hit_test(ctx->palette, mx, my, ctx->window_w, palette_interaction_start_y);
                            if (palette_idx != -1) {
                                app_context_select_palette_tool(ctx, palette_idx);
                            }
                        }
                    } else { // SDL_MOUSEMOTION
                        mx = e.motion.x;
                        my = e.motion.y;
                    }

                    int left_button_pressed = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
                    int right_button_pressed = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);

                    if (left_button_pressed || right_button_pressed) {
                        // Drawing logic is in app_context_draw_stroke, which also checks if mouse is within canvas display area.
                        app_context_draw_stroke(ctx, mx, my, right_button_pressed); // true for erase (use_background_color)
                    }
                }
            }
        } while (SDL_PollEvent(&e)); // Process all pending events
    } else {
        // Timeout without events
    }
}
