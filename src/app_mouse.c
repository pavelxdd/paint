#include "app.h"

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
static bool handle_palette_mousewheel(App *app, int mx, int my, int yscroll)
{
    int palette_start_y = 0;
    if (!is_point_in_palette_ui(app, my, &palette_start_y)) {
        return false; // Not in palette area
    }

    int palette_idx = palette_hit_test(app->palette,
                                       mx,
                                       my,
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
    int mx = (int)mouse_event->x;
    int my = (int)mouse_event->y;

    // --- UI CLICK HANDLING ---
    // Priority: 1. Tool Selectors, 2. Palette, 3. Canvas

    // 1. Check for tool selector hit first, as they float over the canvas
    int tool_selectors_y = app->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    int hit_tool = tool_selectors_hit_test(app, mx, my, tool_selectors_y);

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
                                           mx,
                                           my,
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
        if (mouse_event->button == SDL_BUTTON_LEFT || mouse_event->button == SDL_BUTTON_RIGHT) {
            app->is_drawing = true;
            app->last_stroke_x = mx;
            app->last_stroke_y = my;

            // Latch the straight-line mode for the duration of this stroke.
            // Right-click (eraser) never uses straight line mode.
            if (mouse_event->button == SDL_BUTTON_LEFT) {
                app->straight_line_stroke_latched = app_is_straight_line_mode(app);
            } else {
                app->straight_line_stroke_latched = false;
            }

            if (mouse_event->button == SDL_BUTTON_LEFT && app->current_tool == TOOL_WATER_MARKER) {
                tool_water_marker_begin_stroke(app);
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
    if (app->is_drawing) {
        if (app->straight_line_stroke_latched && mouse_event->button == SDL_BUTTON_LEFT &&
            (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_WATER_MARKER ||
             app->current_tool == TOOL_EMOJI)) {
            // --- Commit the straight line ---
            if (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_EMOJI) {
                // Blend the preview from the stroke buffer onto the main canvas
                SDL_SetRenderTarget(app->ren, app->canvas_texture);
                SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND);
                SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL);
                SDL_SetRenderTarget(app->ren, NULL);
            } else { // TOOL_WATER_MARKER
                // The final preview is on the stroke buffer. End the stroke to blend it.
                tool_water_marker_end_stroke(app);
            }
        } else if (app->water_marker_stroke_active && mouse_event->button == SDL_BUTTON_LEFT) {
            // --- End a freehand water-marker stroke ---
            tool_water_marker_end_stroke(app);
        }

        // Clear the stroke buffer for the next operation
        if (app->stroke_buffer) {
            SDL_SetRenderTarget(app->ren, app->stroke_buffer);
            SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0);
            SDL_RenderClear(app->ren);
            SDL_SetRenderTarget(app->ren, NULL);
        }
        app->needs_redraw = true;
    }

    // Reset drawing state on any button release
    app->is_drawing = false;
    app->straight_line_stroke_latched = false;
    app->last_stroke_x = -1;
    app->last_stroke_y = -1;
}

void app_handle_mousewheel(
    App *app, const SDL_MouseWheelEvent *wheel_event, int mouse_x, int mouse_y)
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
