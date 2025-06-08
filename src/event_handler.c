// AI Summary: Implements SDL event polling and processing for the paint application.
// Dispatches events (quit, window, keyboard, mouse) to appropriate handlers using a
// switch statement. Complex logic like mouse clicks is handled in helper functions
// to improve readability. Interacts with AppContext to modify application state.
#include "event_handler.h"
#include "app_context.h"    // For AppContext and its manipulation functions
#include "palette.h"        // For palette_hit_test, palette_get_color, etc.
#include "tool_selectors.h" // For hit testing the new tool toggles and constants
#include "ui_constants.h"   // For layout constants

static void handle_mouse_down(AppContext *ctx, const SDL_MouseButtonEvent *e)
{
    int mx = e->x;
    int my = e->y;

    // --- UI CLICK HANDLING ---
    // Priority: 1. Tool Selectors, 2. Palette, 3. Canvas

    // 1. Check for tool selector hit first, as they float over the canvas
    int tool_selectors_y = ctx->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    int hit_tool = tool_selectors_hit_test(ctx, mx, my, tool_selectors_y);

    if (hit_tool != -1) { // Hit test returns -1 for miss
        // Click was on a tool selector
        if (e->button == SDL_BUTTON_LEFT) {
            if (hit_tool == TOOL_BRUSH) {
                ctx->current_tool = TOOL_BRUSH;
                ctx->last_color_tool = TOOL_BRUSH;
                ctx->needs_redraw = SDL_TRUE;
            } else if (hit_tool == TOOL_WATER_MARKER) {
                ctx->current_tool = TOOL_WATER_MARKER;
                ctx->last_color_tool = TOOL_WATER_MARKER;
                ctx->needs_redraw = SDL_TRUE;
            } else if (hit_tool == HIT_TEST_COLOR_PALETTE_TOGGLE) {
                app_context_toggle_color_palette(ctx);
            } else if (hit_tool == TOOL_EMOJI) {
                app_context_toggle_emoji_palette(ctx);
            }
        }
    } else if (my >= ctx->canvas_display_area_h) {
        // 2. Click is on the main palette UI area (below the canvas)
        int palette_start_y = ctx->canvas_display_area_h;
        SDL_bool is_palette_content_visible =
            (ctx->show_color_palette && ctx->palette->color_rows > 0) ||
            (ctx->show_emoji_palette && ctx->palette->emoji_rows > 0);
        if (is_palette_content_visible) {
            palette_start_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
        }

        int palette_idx = palette_hit_test(ctx->palette,
                                           mx,
                                           my,
                                           ctx->window_w,
                                           palette_start_y,
                                           ctx->show_color_palette,
                                           ctx->show_emoji_palette);
        if (palette_idx != -1) {
            if (e->button == SDL_BUTTON_LEFT) {
                app_context_select_palette_tool(ctx, palette_idx);
            } else if (e->button == SDL_BUTTON_MIDDLE &&
                       !palette_is_emoji_index(ctx->palette, palette_idx)) {
                SDL_Color new_bg_color = palette_get_color(ctx->palette, palette_idx);
                app_context_set_background_and_clear_canvas(ctx, new_bg_color);
            }
        }
    } else {
        // 3. Click is on the canvas
        if (e->button == SDL_BUTTON_LEFT) {
            if (ctx->current_tool == TOOL_WATER_MARKER) {
                app_context_begin_water_marker_stroke(ctx);
            }
            app_context_draw_stroke(ctx, mx, my, SDL_FALSE); // Draw with current tool
        } else if (e->button == SDL_BUTTON_RIGHT) {
            app_context_draw_stroke(ctx, mx, my, SDL_TRUE); // Erase with background color
        } else if (e->button == SDL_BUTTON_MIDDLE) {
            app_context_clear_canvas_with_current_bg(ctx);
        }
    }
}

 // Returns SDL_TRUE if my is inside palette area, and sets out_palette_start_y.
static SDL_bool is_point_in_palette_ui(AppContext *ctx, int my, int *out_palette_start_y)
{
    if (!ctx || !ctx->palette) return SDL_FALSE;

    int palette_start_y = ctx->canvas_display_area_h;
    SDL_bool is_palette_content_visible =
        (ctx->show_color_palette && ctx->palette->color_rows > 0) ||
        (ctx->show_emoji_palette && ctx->palette->emoji_rows > 0);
    if (is_palette_content_visible) {
        palette_start_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
    }

    int colors_h = ctx->show_color_palette ? ctx->palette->color_rows * PALETTE_HEIGHT : 0;
    int sep_h = (ctx->show_color_palette && ctx->show_emoji_palette &&
                 ctx->palette->color_rows > 0 && ctx->palette->emoji_rows > 0)
                    ? COLOR_EMOJI_SEPARATOR_HEIGHT
                    : 0;
    int emojis_h = ctx->show_emoji_palette ? ctx->palette->emoji_rows * PALETTE_HEIGHT : 0;
    int total_h = colors_h + sep_h + emojis_h;

    if (out_palette_start_y) *out_palette_start_y = palette_start_y;
    if (my >= palette_start_y && my < palette_start_y + total_h)
        return SDL_TRUE;
    return SDL_FALSE;
}

// Handle mouse wheel over palette: cycles current palette selection (color or emoji) with wraparound.
static SDL_bool handle_palette_mousewheel(AppContext *ctx, int mx, int my, int yscroll)
{
    int palette_start_y = 0;
    if (!is_point_in_palette_ui(ctx, my, &palette_start_y))
        return SDL_FALSE; // Not in palette area

    int palette_idx = palette_hit_test(ctx->palette,
                                       mx,
                                       my,
                                       ctx->window_w,
                                       palette_start_y,
                                       ctx->show_color_palette,
                                       ctx->show_emoji_palette);
    if (palette_idx == -1)
        return SDL_FALSE;

    // Only respond if hovering over a *valid* palette cell
    // Always cycle the palette corresponding to the active tool,
    // regardless of which palette cell is under the cursor.
    if (ctx->current_tool == TOOL_BRUSH || ctx->current_tool == TOOL_WATER_MARKER) {
        if (yscroll > 0) {
            app_context_cycle_palette_selection(ctx, -1, 0);
        } else if (yscroll < 0) {
            app_context_cycle_palette_selection(ctx, 1, 0);
        } else {
            return SDL_FALSE;
        }
    } else if (ctx->current_tool == TOOL_EMOJI) {
        if (yscroll > 0) {
            app_context_cycle_palette_selection(ctx, -1, 1);
        } else if (yscroll < 0) {
            app_context_cycle_palette_selection(ctx, 1, 1);
        } else {
            return SDL_FALSE;
        }
    } else {
        return SDL_FALSE;
    }

    ctx->needs_redraw = SDL_TRUE;
    return SDL_TRUE;
}

void handle_events(AppContext *ctx, int *is_running, Uint32 sdl_wait_timeout)
{
    SDL_Event e;
    int last_mouse_x = -1, last_mouse_y = -1;
    // Grab current mouse position once, used for mouse wheel events.
    {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        last_mouse_x = mx;
        last_mouse_y = my;
    }
    if (SDL_WaitEventTimeout(&e, sdl_wait_timeout)) {
        do {
            switch (e.type) {
            case SDL_QUIT:
                *is_running = 0;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    app_context_notify_resize_event(ctx, e.window.data1, e.window.data2);
                }
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_TAB) {
                    // Cycle tools: TAB forward, CTRL+TAB backward
                    int count = __TOOL_COUNT;
                    int idx = 0;
                    for (int j = 0; j < count; ++j) {
                        if ((int)ctx->current_tool == j) {
                            idx = j;
                            break;
                        }
                    }
                    if (e.key.keysym.mod & KMOD_CTRL) {
                        idx = (idx + count - 1) % count;
                    } else {
                        idx = (idx + 1) % count;
                    }
                    ctx->current_tool = idx;
                    if (ctx->current_tool == TOOL_BRUSH || 
                        ctx->current_tool == TOOL_WATER_MARKER) {
                        ctx->last_color_tool = ctx->current_tool;
                    }
                    ctx->needs_redraw = SDL_TRUE;
                } else
                // Handle tool selection by number keys
                if (e.key.keysym.sym == SDLK_0) {
                    ctx->current_tool = TOOL_EMOJI;
                    ctx->needs_redraw = SDL_TRUE;
                } else if (e.key.keysym.sym == SDLK_1) {
                    ctx->current_tool = TOOL_BRUSH;
                    ctx->last_color_tool = TOOL_BRUSH;
                    ctx->needs_redraw = SDL_TRUE;
                } else if (e.key.keysym.sym == SDLK_2) {
                    ctx->current_tool = TOOL_WATER_MARKER;
                    ctx->last_color_tool = TOOL_WATER_MARKER;
                    ctx->needs_redraw = SDL_TRUE;
                } else if (e.key.keysym.sym == SDLK_F1) {
                    app_context_toggle_color_palette(ctx);
                    ctx->needs_redraw = SDL_TRUE;
                } else if (e.key.keysym.sym == SDLK_F2) {
                    app_context_toggle_emoji_palette(ctx);
                    ctx->needs_redraw = SDL_TRUE;
                } else if (e.key.keysym.sym == SDLK_UP ||
                           e.key.keysym.sym == SDLK_DOWN ||
                           e.key.keysym.sym == SDLK_LEFT ||
                           e.key.keysym.sym == SDLK_RIGHT) {
                    app_context_move_palette_selection(ctx, e.key.keysym.sym);
                    ctx->needs_redraw = SDL_TRUE;
                } else if (e.key.keysym.sym == SDLK_f) {
                    // Toggle fullscreen mode
                    Uint32 flags = SDL_GetWindowFlags(ctx->win);
                    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        SDL_SetWindowFullscreen(ctx->win, 0);
                    } else {
                        SDL_SetWindowFullscreen(ctx->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                    ctx->needs_redraw = SDL_TRUE;
                } else {
                    // Only allow non-number keys for brush size adjustment
                    app_context_set_brush_radius_from_key(ctx, e.key.keysym.sym);
                }
                break;
            case SDL_MOUSEWHEEL: {
                // Mouse wheel events are delivered globally, not per-window region.
                // To implement cycling palette, must check if mouse is in palette area.
                // If yes, perform palette selection cycling and prevent brush size change.
                // If not, apply original behavior (brush size adjust).
                int mx = last_mouse_x, my = last_mouse_y;
                if (!handle_palette_mousewheel(ctx, mx, my, e.wheel.y)) {
                    // Only fall back to brush size adjust if not handled by palette hover
                    if (e.wheel.y > 0) { // Scroll up
                        app_context_change_brush_radius(ctx, 2);
                    } else if (e.wheel.y < 0) { // Scroll down
                        app_context_change_brush_radius(ctx, -2);
                    }
                }
                break;
            }
            case SDL_MOUSEMOTION:
                if (e.motion.state & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) {
                    app_context_draw_stroke(
                        ctx,
                        e.motion.x,
                        e.motion.y,
                        (e.motion.state & SDL_BUTTON_RMASK) ? SDL_TRUE : SDL_FALSE);
                }
                // Update mouse position tracking for next mouse wheel event
                last_mouse_x = e.motion.x;
                last_mouse_y = e.motion.y;
                break;
            case SDL_MOUSEBUTTONDOWN:
                handle_mouse_down(ctx, &e.button);
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT && ctx->water_marker_stroke_active) {
                    app_context_end_water_marker_stroke(ctx);
                }
                break;
            }
        } while (SDL_PollEvent(&e)); // Process all pending events
    }
}
