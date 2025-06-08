#include "app_context.h"
#include "palette.h"
#include "tool_selectors.h"
#include "ui_constants.h"

// Returns SDL_TRUE if my is inside palette area, and sets out_palette_start_y.
static SDL_bool is_point_in_palette_ui(AppContext *ctx, int my, int *out_palette_start_y)
{
    if (!ctx || !ctx->palette) {
        return SDL_FALSE;
    }

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

    if (out_palette_start_y) {
        *out_palette_start_y = palette_start_y;
    }
    if (my >= palette_start_y && my < palette_start_y + total_h) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

// Handle mouse wheel over palette: cycles current palette selection (color or emoji) with
// wraparound.
static SDL_bool handle_palette_mousewheel(AppContext *ctx, int mx, int my, int yscroll)
{
    int palette_start_y = 0;
    if (!is_point_in_palette_ui(ctx, my, &palette_start_y)) {
        return SDL_FALSE; // Not in palette area
    }

    int palette_idx = palette_hit_test(ctx->palette,
                                       mx,
                                       my,
                                       ctx->window_w,
                                       palette_start_y,
                                       ctx->show_color_palette,
                                       ctx->show_emoji_palette);
    if (palette_idx == -1) {
        return SDL_FALSE;
    }

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

void app_context_handle_mousedown(AppContext *ctx, const SDL_MouseButtonEvent *mouse_event)
{
    int mx = mouse_event->x;
    int my = mouse_event->y;

    // --- UI CLICK HANDLING ---
    // Priority: 1. Tool Selectors, 2. Palette, 3. Canvas

    // 1. Check for tool selector hit first, as they float over the canvas
    int tool_selectors_y = ctx->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    int hit_tool = tool_selectors_hit_test(ctx, mx, my, tool_selectors_y);

    if (hit_tool != -1) { // Hit test returns -1 for miss
        // Click was on a tool selector
        if (mouse_event->button == SDL_BUTTON_LEFT) {
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
            } else if (hit_tool == HIT_TEST_LINE_MODE_TOGGLE) {
                app_context_toggle_line_mode(ctx);
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
            if (mouse_event->button == SDL_BUTTON_LEFT) {
                app_context_select_palette_tool(ctx, palette_idx);
            } else if (mouse_event->button == SDL_BUTTON_MIDDLE &&
                       !palette_is_emoji_index(ctx->palette, palette_idx)) {
                SDL_Color new_bg_color = palette_get_color(ctx->palette, palette_idx);
                app_context_set_background_and_clear_canvas(ctx, new_bg_color);
            }
        }
    } else {
        // 3. Click is on the canvas
        if (mouse_event->button == SDL_BUTTON_LEFT || mouse_event->button == SDL_BUTTON_RIGHT) {
            ctx->is_drawing = SDL_TRUE;
            ctx->last_stroke_x = mx;
            ctx->last_stroke_y = my;

            // Latch the straight-line mode for the duration of this stroke.
            // Right-click (eraser) never uses straight line mode.
            if (mouse_event->button == SDL_BUTTON_LEFT) {
                ctx->straight_line_stroke_latched = app_context_is_straight_line_mode(ctx);
            } else {
                ctx->straight_line_stroke_latched = SDL_FALSE;
            }

            if (mouse_event->button == SDL_BUTTON_LEFT && ctx->current_tool == TOOL_WATER_MARKER) {
                app_context_begin_water_marker_stroke(ctx);
            }

            // If not in a latched straight-line stroke, draw the first dab immediately.
            if (!ctx->straight_line_stroke_latched) {
                app_context_draw_stroke(ctx, mx, my, (mouse_event->button == SDL_BUTTON_RIGHT));
            } else {
                ctx->needs_redraw = SDL_TRUE; // Redraw to show preview on first move
            }
        } else if (mouse_event->button == SDL_BUTTON_MIDDLE) {
            app_context_clear_canvas_with_current_bg(ctx);
        }
    }
}

void app_context_handle_mouseup(AppContext *ctx, const SDL_MouseButtonEvent *mouse_event)
{
    if (ctx->is_drawing) {
        if (ctx->straight_line_stroke_latched && mouse_event->button == SDL_BUTTON_LEFT &&
            (ctx->current_tool == TOOL_BRUSH || ctx->current_tool == TOOL_WATER_MARKER ||
             ctx->current_tool == TOOL_EMOJI)) {
            // --- Commit the straight line ---
            if (ctx->current_tool == TOOL_BRUSH || ctx->current_tool == TOOL_EMOJI) {
                // Blend the preview from the stroke buffer onto the main canvas
                SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
                SDL_SetTextureBlendMode(ctx->stroke_buffer, SDL_BLENDMODE_BLEND);
                SDL_RenderCopy(ctx->ren, ctx->stroke_buffer, NULL, NULL);
                SDL_SetRenderTarget(ctx->ren, NULL);
            } else { // TOOL_WATER_MARKER
                // The final preview is on the stroke buffer. End the stroke to blend it.
                app_context_end_water_marker_stroke(ctx);
            }
        } else if (ctx->water_marker_stroke_active && mouse_event->button == SDL_BUTTON_LEFT) {
            // --- End a freehand water-marker stroke ---
            app_context_end_water_marker_stroke(ctx);
        }

        // Clear the stroke buffer for the next operation
        if (ctx->stroke_buffer) {
            SDL_SetRenderTarget(ctx->ren, ctx->stroke_buffer);
            SDL_SetRenderDrawBlendMode(ctx->ren, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, 0);
            SDL_RenderClear(ctx->ren);
            SDL_SetRenderTarget(ctx->ren, NULL);
        }
        ctx->needs_redraw = SDL_TRUE;
    }

    // Reset drawing state on any button release
    ctx->is_drawing = SDL_FALSE;
    ctx->straight_line_stroke_latched = SDL_FALSE;
    ctx->last_stroke_x = -1;
    ctx->last_stroke_y = -1;
}

void app_context_handle_mousewheel(AppContext *ctx,
                                   const SDL_MouseWheelEvent *wheel_event,
                                   int mouse_x,
                                   int mouse_y)
{
    // Mouse wheel events are delivered globally, not per-window region.
    // To implement cycling palette, must check if mouse is in palette area.
    // If yes, perform palette selection cycling and prevent brush size change.
    // If not, apply original behavior (brush size adjust).
    if (!handle_palette_mousewheel(ctx, mouse_x, mouse_y, wheel_event->y)) {
        // Only fall back to brush size adjust if not handled by palette hover
        if (wheel_event->y > 0) { // Scroll up
            app_context_change_brush_radius(ctx, 2);
        } else if (wheel_event->y < 0) { // Scroll down
            app_context_change_brush_radius(ctx, -2);
        }
    }
}
