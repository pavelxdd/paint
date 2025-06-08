#include "app.h"
#include "draw.h"
#include "palette.h"
#include "ui_constants.h"
#include <math.h>

static void draw_line_of_emojis(App *app, int x0, int y0, int x1, int y1)
{
    SDL_Texture *emoji_tex = NULL;
    int ew = 0, eh = 0;
    SDL_bool has_emoji = palette_get_emoji_info(
        app->palette, app->emoji_selected_palette_idx, &emoji_tex, &ew, &eh);

    if (!has_emoji || !emoji_tex) {
        return;
    }

    float asp = (eh == 0) ? 1.0f : (float)ew / eh;
    int h = app->brush_radius * 6;
    if (h < MIN_BRUSH_SIZE * 6) {
        h = MIN_BRUSH_SIZE * 6;
    }
    int w = lroundf(h * asp);
    if (w == 0) {
        w = 1;
    }
    if (h == 0) {
        h = 1;
    }

    // Draw first emoji at the start point
    SDL_Rect dst_start = {x0 - w / 2, y0 - h / 2, w, h};
    SDL_RenderCopy(app->ren, emoji_tex, NULL, &dst_start);

    float dx = x1 - x0;
    float dy = y1 - y0;
    float line_length = sqrtf(dx * dx + dy * dy);

    // Use emoji height as spacing
    if (line_length < h) {
        return;
    }

    float ux = dx / line_length;
    float uy = dy / line_length;

    int num_emojis = floorf(line_length / h);
    for (int i = 1; i <= num_emojis; ++i) {
        int px = x0 + lroundf((float)i * h * ux);
        int py = y0 + lroundf((float)i * h * uy);
        SDL_Rect dst = {px - w / 2, py - h / 2, w, h};
        SDL_RenderCopy(app->ren, emoji_tex, NULL, &dst);
    }
}

static void draw_line_of_square_dabs(App *app, int x0, int y0, int x1, int y1)
{
    SDL_SetRenderDrawColor(app->ren,
                           app->water_marker_color.r,
                           app->water_marker_color.g,
                           app->water_marker_color.b,
                           255);
    int side = lroundf(app->brush_radius * 2 * 1.5f);

    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    for (;;) {
        SDL_Rect rect = {x0 - side / 2, y0 - side / 2, side, side};
        SDL_RenderFillRect(app->ren, &rect);
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
}

static void draw_dab_at_point(App *app, int x, int y, SDL_bool use_background_color)
{
    if (y >= app->canvas_display_area_h || app->canvas_display_area_h == 0) {
        return;
    }

    SDL_Texture *target_texture = app->canvas_texture;

    if (use_background_color) {
        SDL_SetRenderTarget(app->ren, target_texture);
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
    case TOOL_BRUSH: {
        SDL_SetRenderTarget(app->ren, target_texture);
        SDL_SetRenderDrawColor(app->ren,
                               app->current_color.r,
                               app->current_color.g,
                               app->current_color.b,
                               255);
        draw_circle(app->ren, x, y, app->brush_radius);
    } break;

    case TOOL_WATER_MARKER: {
        if (!app->water_marker_stroke_active || !app->stroke_buffer) {
            return; // Not in a stroke, do nothing
        }
        target_texture = app->stroke_buffer;
        SDL_SetRenderTarget(app->ren, target_texture);
        SDL_SetRenderDrawColor(
            app->ren, app->water_marker_color.r, app->water_marker_color.g, app->water_marker_color.b, 255);
        int side = lroundf(app->brush_radius * 2 * 1.5f);
        SDL_Rect rect = {x - side / 2, y - side / 2, side, side};
        SDL_RenderFillRect(app->ren, &rect);
    } break;

    case TOOL_EMOJI: {
        SDL_SetRenderTarget(app->ren, target_texture);
        SDL_Texture *emoji_tex = NULL;
        int ew = 0, eh = 0;
        SDL_bool has_emoji = palette_get_emoji_info(
            app->palette, app->emoji_selected_palette_idx, &emoji_tex, &ew, &eh);
        if (has_emoji && emoji_tex) {
            float asp = (eh == 0) ? 1.0f : (float)ew / eh;
            int h = app->brush_radius * 6;
            if (h < MIN_BRUSH_SIZE * 6) {
                h = MIN_BRUSH_SIZE * 6;
            }
            int w = lroundf(h * asp);
            if (w == 0) {
                w = 1;
            }

            SDL_Rect dst = {x - w / 2, y - h / 2, w, h};
            SDL_RenderCopy(app->ren, emoji_tex, NULL, &dst);
        }
    } break;

    default:
        return; // Should not happen
    }

    SDL_SetRenderTarget(app->ren, NULL);
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
        if (app->current_tool == TOOL_BRUSH) {
            draw_thick_line(
                app->ren, x0, y0, x1, y1, app->brush_radius * 2, app->current_color);
        } else if (app->current_tool == TOOL_WATER_MARKER) {
            draw_line_of_square_dabs(app, x0, y0, x1, y1);
        } else { // TOOL_EMOJI
            draw_line_of_emojis(app, x0, y0, x1, y1);
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
        draw_dab_at_point(app, x0, y0, use_background_color);
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

void app_begin_water_marker_stroke(App *app)
{
    if (!app || !app->stroke_buffer) {
        return;
    }
    app->water_marker_stroke_active = SDL_TRUE;

    // Clear the buffer to be fully transparent for the new stroke
    SDL_SetRenderTarget(app->ren, app->stroke_buffer);
    SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0);
    SDL_RenderClear(app->ren);
    SDL_SetRenderTarget(app->ren, NULL);
}

void app_end_water_marker_stroke(App *app)
{
    if (!app || !app->stroke_buffer || !app->water_marker_stroke_active) {
        return;
    }

    // Blend the completed stroke from the buffer onto the main canvas
    SDL_SetRenderTarget(app->ren, app->canvas_texture);
    SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(app->stroke_buffer, 128); // 50% alpha
    SDL_RenderCopy(app->ren, app->stroke_buffer, NULL, NULL);

    // Reset texture properties and renderer target
    SDL_SetTextureAlphaMod(app->stroke_buffer, 255);
    SDL_SetRenderTarget(app->ren, NULL);

    app->water_marker_stroke_active = SDL_FALSE;
    app->needs_redraw = SDL_TRUE;
}
