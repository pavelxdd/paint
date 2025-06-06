// AI Summary: Implements SDL event polling (using SDL_WaitEventTimeout) and processing
// for the paint application. It checks for various event types like quit, window
// resize, key presses (for brush size), mouse wheel (for brush size), and mouse
// button/motion events for drawing, tool/color picking, toggling palettes, and background changes.
// Interacts with AppContext to modify application state based on events.
#include "event_handler.h"
#include "app_context.h" // For AppContext and its manipulation functions
#include "palette.h"     // For palette_hit_test, palette_get_color, etc.
#include "tool_selectors.h" // For hit testing the new tool toggles

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
            } else if (e.type == SDL_MOUSEMOTION) {
                int mx = e.motion.x;
                int my = e.motion.y;
                int left_button_pressed = e.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT);
                int right_button_pressed = e.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT);

                if (left_button_pressed || right_button_pressed) {
                    app_context_draw_stroke(ctx, mx, my, right_button_pressed);
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;

                // --- UI CLICK HANDLING ---
                // Priority: 1. Tool Selectors, 2. Palette, 3. Canvas
                
                // 1. Check for tool selector hit first, as they float over the canvas
                int tool_selectors_y = ctx->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
                int hit_tool = tool_selectors_hit_test(ctx, mx, my, tool_selectors_y);

                if (hit_tool >= 0) {
                    // Click was on a tool selector
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (hit_tool == TOOL_BRUSH) app_context_toggle_color_palette(ctx);
                        else if (hit_tool == TOOL_EMOJI) app_context_toggle_emoji_palette(ctx);
                    }
                } else if (my >= ctx->canvas_display_area_h) {
                    // 2. Click is on the main palette UI area (below the canvas)
                    int palette_start_y = ctx->canvas_display_area_h;
                    SDL_bool is_palette_content_visible = (ctx->show_color_palette && ctx->palette->color_rows > 0) || (ctx->show_emoji_palette && ctx->palette->emoji_rows > 0);
                    if (is_palette_content_visible) {
                        palette_start_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
                    }

                    int palette_idx = palette_hit_test(ctx->palette, mx, my, ctx->window_w, palette_start_y, ctx->show_color_palette, ctx->show_emoji_palette);
                    if (palette_idx != -1) {
                        if (e.button.button == SDL_BUTTON_LEFT) {
                            app_context_select_palette_tool(ctx, palette_idx);
                        } else if (e.button.button == SDL_BUTTON_MIDDLE && !palette_is_emoji_index(ctx->palette, palette_idx)) {
                            SDL_Color new_bg_color = palette_get_color(ctx->palette, palette_idx);
                            app_context_set_background_and_clear_canvas(ctx, new_bg_color);
                        }
                    }
                } else {
                    // 3. Click is on the canvas
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        app_context_draw_stroke(ctx, mx, my, SDL_FALSE); // Draw with current tool
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        app_context_draw_stroke(ctx, mx, my, SDL_TRUE); // Erase with background color
                    } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                        app_context_clear_canvas_with_current_bg(ctx);
                    }
                }
            }
        } while (SDL_PollEvent(&e)); // Process all pending events
    } else {
        // Timeout without events
    }
}
