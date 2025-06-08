#include "app.h"
#include "draw.h"
#include <math.h>

static void app_draw_dab(App *app, int x, int y, SDL_bool use_background_color)
{
    if (y >= app->canvas_display_area_h || app->canvas_display_area_h == 0) {
        return;
    }

    if (use_background_color) {
        SDL_SetRenderTarget(app->ren, app->canvas_texture);
        SDL_SetRenderDrawColor(app->ren,
                               app->background_color.r,
                               app->background_color.g,
                               app->background_color.b,
                               app->background_color.a);
        draw_circle(app->ren, x, y, app->brush_radius);
        SDL_SetRenderTarget(app->ren, NULL);
        app->needs_redraw = SDL_TRUE;
        return;
    }

    switch (app->current_tool) {
    case TOOL_BRUSH:
        tool_brush_draw_dab(app, x, y);
        break;
    case TOOL_WATER_MARKER:
        tool_water_marker_draw_dab(app, x, y);
        break;
    case TOOL_EMOJI:
        tool_emoji_draw_dab(app, x, y);
        break;
    default:
        return; // Should not happen
    }
    app->needs_redraw = SDL_TRUE;
}

void app_draw_stroke(App *app, int mouse_x, int mouse_y, SDL_bool use_background_color)
{
    if (!app || !app->canvas_texture) {
        return;
    }

    // Straight line mode is active for Brush, Water Marker, or Emoji (but not for erasing)
    if (app->straight_line_stroke_latched && !use_background_color &&
        (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_WATER_MARKER ||
         app->current_tool == TOOL_EMOJI)) {
        // --- Straight Line Preview ---
        SDL_SetRenderTarget(app->ren, app->stroke_buffer);
        SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0);
        SDL_RenderClear(app->ren);

        // Get start/end points and apply snapping if Shift is held
        int x0 = app->last_stroke_x;
        int y0 = app->last_stroke_y;
        int x1 = mouse_x;
        int y1 = mouse_y;
        const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
        if (keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT]) {
            int dx = abs(x1 - x0);
            int dy = abs(y1 - y0);
            if (dx > dy) {
                y1 = y0; // Snap to horizontal
            } else {
                x1 = x0; // Snap to vertical
            }
        }

        // Draw the preview line based on the active tool
        switch (app->current_tool) {
        case TOOL_BRUSH:
            tool_brush_draw_line_preview(app, x0, y0, x1, y1);
            break;
        case TOOL_WATER_MARKER:
            tool_water_marker_draw_line_preview(app, x0, y0, x1, y1);
            break;
        case TOOL_EMOJI:
            tool_emoji_draw_line_preview(app, x0, y0, x1, y1);
            break;
        default:
            break;
        }

        SDL_SetRenderTarget(app->ren, NULL);
        app->needs_redraw = SDL_TRUE;
        return;
    }

    // --- Freehand Stroke (for Brush, Water Marker, Emoji, and Eraser) ---
    // If this is the first point of a stroke, last_stroke_x will be -1.
    int x0 = (app->last_stroke_x == -1) ? mouse_x : app->last_stroke_x;
    int y0 = (app->last_stroke_y == -1) ? mouse_y : app->last_stroke_y;
    int x1 = mouse_x;
    int y1 = mouse_y;

    // Bresenham's line algorithm to draw a continuous line of "dabs".
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    for (;;) {
        app_draw_dab(app, x0, y0, use_background_color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }

    // Update the last point for the next segment of the stroke.
    app->last_stroke_x = mouse_x;
    app->last_stroke_y = mouse_y;
}
