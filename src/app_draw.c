#include "app.h"
#include "draw.h"

#include "app.h"
#include "draw.h"

typedef struct {
    App *app;
    bool use_background_color;
} DabInfo;

static void app_draw_dab_callback(int x, int y, void *userdata)
{
    DabInfo *info = (DabInfo *)userdata;
    App *app = info->app;

    if (y >= app->canvas_display_area_h || app->canvas_display_area_h == 0) {
        return;
    }

    if (info->use_background_color) {
        if (!SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
            SDL_Log("Failed to set render target for eraser dab: %s", SDL_GetError());
            return;
        }
        if (!SDL_SetRenderDrawColor(app->ren,
                                    app->background_color.r,
                                    app->background_color.g,
                                    app->background_color.b,
                                    app->background_color.a)) {
            SDL_Log("Failed to set color for eraser dab: %s", SDL_GetError());
        }
        draw_circle(app->ren, (float)x, (float)y, app->brush_radius);
        if (!SDL_SetRenderTarget(app->ren, NULL)) {
            SDL_Log("Failed to reset render target after eraser dab: %s", SDL_GetError());
        }
        app->needs_redraw = true;
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
        case TOOL_BLUR:
            tool_blur_draw_dab(app, x, y);
            break;
        default:
            return; // Should not happen
    }
    app->needs_redraw = true;
}

void app_draw_line_of_dabs(App *app, float x0, float y0, float x1, float y1, bool use_background_color)
{
    DabInfo info = {app, use_background_color};
    draw_line_bresenham((int)x0, (int)y0, (int)x1, (int)y1, app_draw_dab_callback, &info);
}

void app_draw_stroke(App *app, float mouse_x, float mouse_y, bool use_background_color)
{
    if (!app || !app->canvas_texture) {
        return;
    }

    // Straight line mode is active for Brush, Water Marker, Blur or Emoji (but not for erasing)
    if (app->straight_line_stroke_latched && !use_background_color &&
        (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_WATER_MARKER ||
         app->current_tool == TOOL_EMOJI || app->current_tool == TOOL_BLUR)) {
        // --- Straight Line Preview ---
        if (!SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
            SDL_Log("Failed to set render target for preview: %s", SDL_GetError());
            return;
        }

        if (app->current_tool == TOOL_BLUR) {
            // For blur, the stroke buffer is the live canvas. We need to restore it
            // from the pristine source texture before drawing the new preview line.
            if (!SDL_SetTextureBlendMode(app->blur_source_texture, SDL_BLENDMODE_NONE)) {
                SDL_Log("Failed to set blend mode for preview restore: %s", SDL_GetError());
            }
            if (!SDL_RenderTexture(app->ren, app->blur_source_texture, NULL, NULL)) {
                SDL_Log("Failed to restore stroke buffer for preview: %s", SDL_GetError());
            }
            if (!SDL_SetTextureBlendMode(app->blur_source_texture, SDL_BLENDMODE_BLEND)) {
                // Restore default
                SDL_Log("Failed to restore blend mode for blur source: %s", SDL_GetError());
            }
        } else {
            // For other tools, the preview is an overlay, so we clear the buffer.
            if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE)) {
                SDL_Log("Failed to set blend mode for preview clear: %s", SDL_GetError());
            }
            if (!SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0)) {
                SDL_Log("Failed to set draw color for preview clear: %s", SDL_GetError());
            }
            if (!SDL_RenderClear(app->ren)) {
                SDL_Log("Failed to clear stroke buffer for preview: %s", SDL_GetError());
            }
        }

        // Get start/end points and apply snapping if Shift is held
        float x0 = app->last_stroke_x;
        float y0 = app->last_stroke_y;
        float x1 = mouse_x;
        float y1 = mouse_y;
        const bool *keyboard_state = SDL_GetKeyboardState(NULL);
        if (keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT]) {
            float dx = SDL_fabsf(x1 - x0);
            float dy = SDL_fabsf(y1 - y0);
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
            case TOOL_BLUR:
                tool_blur_draw_line_preview(app, x0, y0, x1, y1);
                break;
            default:
                break;
        }

        if (!SDL_SetRenderTarget(app->ren, NULL)) {
            SDL_Log("Failed to reset render target after preview: %s", SDL_GetError());
        }
        app->needs_redraw = true;
        return;
    }

    // --- Freehand Stroke (for Brush, Water Marker, Emoji, and Eraser) ---
    // If this is the first point of a stroke, last_stroke_x will be negative.
    float x0 = (app->last_stroke_x < 0.0f) ? mouse_x : app->last_stroke_x;
    float y0 = (app->last_stroke_y < 0.0f) ? mouse_y : app->last_stroke_y;
    float x1 = mouse_x;
    float y1 = mouse_y;

    app_draw_line_of_dabs(app, x0, y0, x1, y1, use_background_color);

    // Update the last point for the next segment of the stroke.
    app->last_stroke_x = mouse_x;
    app->last_stroke_y = mouse_y;
}
