#include "app.h"
#include "ui.h"

// Returns true if my is inside palette area, and sets out_palette_start_y.
static bool is_point_in_palette_ui(App *app, int my, int *out_palette_start_y)
{
    if (!app || !app->palette) {
        return false;
    }

    int palette_start_y = app->canvas_display_area_h;
    bool is_palette_content_visible =
        (app->show_color_palette && app->palette->color_rows > 0) ||
        (app->show_emoji_palette && app->palette->emoji_rows > 0);
    if (is_palette_content_visible) {
        palette_start_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
    }

    int colors_h = app->show_color_palette ? app->palette->color_rows * PALETTE_HEIGHT : 0;
    int sep_h = (app->show_color_palette && app->show_emoji_palette &&
                 app->palette->color_rows > 0 && app->palette->emoji_rows > 0)
                ? COLOR_EMOJI_SEPARATOR_HEIGHT
                : 0;
    int emojis_h = app->show_emoji_palette ? app->palette->emoji_rows * PALETTE_HEIGHT : 0;
    int total_h = colors_h + sep_h + emojis_h;

    if (out_palette_start_y) {
        *out_palette_start_y = palette_start_y;
    }
    if (my >= palette_start_y && my < palette_start_y + total_h) {
        return true;
    }
    return false;
}

// Handle mouse wheel over palette: cycles current palette selection (color or emoji) with
// wraparound.
static bool handle_palette_mousewheel(App *app, float mx, float my, int yscroll)
{
    int palette_start_y = 0;
    if (!is_point_in_palette_ui(app, (int)my, &palette_start_y)) {
        return false; // Not in palette area
    }

    int palette_idx = palette_hit_test(app->palette,
                                       (int)mx,
                                       (int)my,
                                       app->window_w,
                                       palette_start_y,
                                       app->show_color_palette,
                                       app->show_emoji_palette);
    if (palette_idx == -1) {
        return false;
    }

    // Only respond if hovering over a *valid* palette cell
    // Always cycle the palette corresponding to the active tool,
    // regardless of which palette cell is under the cursor.
    if (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_WATER_MARKER) {
        if (yscroll > 0) {
            app_cycle_palette_selection(app, -1, 0);
        } else if (yscroll < 0) {
            app_cycle_palette_selection(app, 1, 0);
        } else {
            return false;
        }
    } else if (app->current_tool == TOOL_EMOJI) {
        if (yscroll > 0) {
            app_cycle_palette_selection(app, -1, 1);
        } else if (yscroll < 0) {
            app_cycle_palette_selection(app, 1, 1);
        } else {
            return false;
        }
    } else {
        return false;
    }

    app->needs_redraw = true;
    return true;
}

void app_handle_mousedown(App *app, const SDL_MouseButtonEvent *mouse_event)
{
    float mx = mouse_event->x;
    float my = mouse_event->y;

    // --- UI CLICK HANDLING ---
    // Priority: 1. Tool Selectors, 2. Palette, 3. Canvas

    // 1. Check for tool selector hit first, as they float over the canvas
    int tool_selectors_y = app->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    int hit_tool = ui_hit_test_tool_selectors(app, (int)mx, (int)my, tool_selectors_y);

    if (hit_tool != -1) { // Hit test returns -1 for miss
        // Click was on a tool selector
        if (mouse_event->button == SDL_BUTTON_LEFT) {
            if (hit_tool == TOOL_BRUSH) {
                app->current_tool = TOOL_BRUSH;
                app->last_color_tool = TOOL_BRUSH;
                app->needs_redraw = true;
            } else if (hit_tool == TOOL_WATER_MARKER) {
                app->current_tool = TOOL_WATER_MARKER;
                app->last_color_tool = TOOL_WATER_MARKER;
                app->needs_redraw = true;
            } else if (hit_tool == TOOL_BLUR) {
                app->current_tool = TOOL_BLUR;
                app->needs_redraw = true;
            } else if (hit_tool == HIT_TEST_COLOR_PALETTE_TOGGLE) {
                app_toggle_color_palette(app);
            } else if (hit_tool == HIT_TEST_LINE_MODE_TOGGLE) {
                app_toggle_line_mode(app);
            } else if (hit_tool == TOOL_EMOJI) {
                app_toggle_emoji_palette(app);
            }
        }
    } else if (my >= app->canvas_display_area_h) {
        // 2. Click is on the main palette UI area (below the canvas)
        int palette_start_y = app->canvas_display_area_h;
        bool is_palette_content_visible =
            (app->show_color_palette && app->palette->color_rows > 0) ||
            (app->show_emoji_palette && app->palette->emoji_rows > 0);
        if (is_palette_content_visible) {
            palette_start_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
        }

        int palette_idx = palette_hit_test(app->palette,
                                           (int)mx,
                                           (int)my,
                                           app->window_w,
                                           palette_start_y,
                                           app->show_color_palette,
                                           app->show_emoji_palette);
        if (palette_idx != -1) {
            if (mouse_event->button == SDL_BUTTON_LEFT) {
                app_select_palette_tool(app, palette_idx);
            } else if (mouse_event->button == SDL_BUTTON_MIDDLE &&
                       !palette_is_emoji_index(app->palette, palette_idx)) {
                SDL_Color new_bg_color = palette_get_color(app->palette, palette_idx);
                app_set_background_and_clear_canvas(app, new_bg_color);
            }
        }
    } else {
        // 3. Click is on the canvas
        if (mouse_event->button == SDL_BUTTON_LEFT ||
            mouse_event->button == SDL_BUTTON_RIGHT) {
            app->is_drawing = true;
            app->last_stroke_x = mx;
            app->last_stroke_y = my;
            app->has_moved_since_mousedown = false;

            // Latch the straight-line mode for the duration of this stroke.
            // Right-click (eraser) never uses straight line mode.
            if (mouse_event->button == SDL_BUTTON_LEFT) {
                app->straight_line_stroke_latched = app_is_straight_line_mode(app);
            } else {
                app->straight_line_stroke_latched = false;
            }

            if (mouse_event->button == SDL_BUTTON_LEFT) {
                if (app->current_tool == TOOL_WATER_MARKER) {
                    tool_water_marker_begin_stroke(app);
                } else if (app->current_tool == TOOL_BLUR) {
                    tool_blur_begin_stroke(app);
                }
            }

            // If not in a latched straight-line stroke, draw the first dab immediately.
            if (!app->straight_line_stroke_latched) {
                app_draw_stroke(app, mx, my, (mouse_event->button == SDL_BUTTON_RIGHT));
            } else {
                app->needs_redraw = true; // Redraw to show preview on first move
            }
        } else if (mouse_event->button == SDL_BUTTON_MIDDLE) {
            app_clear_canvas_with_current_bg(app);
        }
    }
}

void app_handle_mouseup(App *app, const SDL_MouseButtonEvent *mouse_event)
{
    if (app->is_drawing && mouse_event->button == SDL_BUTTON_LEFT) {
        if (app->straight_line_stroke_latched) {
            if (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_EMOJI) {
                if (SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
                    if (!SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND)) {
                        SDL_Log("MUP:Failed to set blend mode for stroke buffer: %s", SDL_GetError());
                    }
                    if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
                        SDL_Log("MUP:Failed to render stroke buffer: %s", SDL_GetError());
                    }
                    if (!SDL_SetRenderTarget(app->ren, NULL)) {
                        SDL_Log("MUP:Failed to reset render target: %s", SDL_GetError());
                    }
                } else {
                    SDL_Log("MUP:Failed to set render target to canvas: %s", SDL_GetError());
                }
            } else if (app->current_tool == TOOL_WATER_MARKER) {
                tool_water_marker_end_stroke(app);
            } else if (app->current_tool == TOOL_BLUR) {
                // The preview is now the real deal, just finalize.
                tool_blur_end_stroke(app);
            }
        } else { // Freehand stroke for buffered tools
            if (app->is_buffered_stroke_active) {
                switch (app->current_tool) {
                    case TOOL_WATER_MARKER:
                        tool_water_marker_end_stroke(app);
                        break;
                    case TOOL_BLUR:
                        if (!app->has_moved_since_mousedown) {
                            // Single click: apply more dabs to make it feel substantial
                            for (int i = 0; i < 9; ++i) {
                                tool_blur_draw_dab(app, (int)mouse_event->x, (int)mouse_event->y);
                            }
                        }
                        tool_blur_end_stroke(app);
                        break;
                    default:
                        // This case can be hit if tool is switched mid-stroke.
                        // For now, do nothing.
                        break;
                }
            }
        }
    }

    // Clear the stroke buffer for the next operation
    if (app->stroke_buffer) {
        if (SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
            if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE)) {
                SDL_Log("MUP:Failed to set blend mode for clear: %s", SDL_GetError());
            }
            if (!SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0)) {
                SDL_Log("MUP:Failed to set color for clear: %s", SDL_GetError());
            }
            if (!SDL_RenderClear(app->ren)) {
                SDL_Log("MUP:Failed to clear stroke buffer: %s", SDL_GetError());
            }
            if (!SDL_SetRenderTarget(app->ren, NULL)) {
                SDL_Log("MUP:Failed to reset render target after clear: %s", SDL_GetError());
            }
        } else {
            SDL_Log("MUP:Failed to set render target to stroke buffer: %s", SDL_GetError());
        }
    }

    // Reset drawing state on any button release
    app->is_drawing = false;
    app->straight_line_stroke_latched = false;
    app->is_buffered_stroke_active = false;
    app->last_stroke_x = -1.0f;
    app->last_stroke_y = -1.0f;
    app->has_moved_since_mousedown = false;
    app->needs_redraw = true;
}

void app_handle_mousewheel(
    App *app, const SDL_MouseWheelEvent *wheel_event, float mouse_x, float mouse_y)
{
    // Mouse wheel events are delivered globally, not per-window region.
    // To implement cycling palette, must check if mouse is in palette area.
    // If yes, perform palette selection cycling and prevent brush size change.
    // If not, apply original behavior (brush size adjust).
    if (!handle_palette_mousewheel(app, mouse_x, mouse_y, (int)wheel_event->y)) {
        // Only fall back to brush size adjust if not handled by palette hover
        if (wheel_event->y > 0) { // Scroll up
            app_change_brush_radius(app, 2);
        } else if (wheel_event->y < 0) { // Scroll down
            app_change_brush_radius(app, -2);
        }
    }
}
