#include "renderer.h"
#include "palette.h"
#include "tool_selectors.h"

void render_scene(App *app)
{
    SDL_SetRenderDrawColor(app->ren, 255, 255, 255, 255);
    SDL_RenderClear(app->ren);

    // 1. Render the canvas texture to the entire window first.
    if (app->canvas_texture) {
        SDL_RenderCopy(app->ren, app->canvas_texture, NULL, NULL);
    }

    // 2. Render tool previews from the stroke buffer if necessary.
    if (app->is_drawing && app->straight_line_stroke_latched && app->stroke_buffer) {
        // Render a straight line preview
        if (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_EMOJI) {
            // Brush and Emoji previews are opaque and on the buffer
            SDL_RenderCopy(app->ren, app->stroke_buffer, NULL, NULL);
        } else if (app->current_tool == TOOL_WATER_MARKER) {
            // Water-marker preview is semi-transparent
            SDL_SetTextureAlphaMod(app->stroke_buffer, 128);
            SDL_RenderCopy(app->ren, app->stroke_buffer, NULL, NULL);
            SDL_SetTextureAlphaMod(app->stroke_buffer, 255);
        }
    } else if (app->water_marker_stroke_active && app->stroke_buffer) {
        // Render a freehand water-marker stroke in progress
        SDL_SetTextureAlphaMod(app->stroke_buffer, 128);
        SDL_RenderCopy(app->ren, app->stroke_buffer, NULL, NULL);
        SDL_SetTextureAlphaMod(app->stroke_buffer, 255); // Reset for other potential uses
    }

    // --- UI drawing from top to bottom, overlaid on the canvas ---

    // 3. Tool selectors "float" over the canvas, just above the main UI panel.
    // Their Y position is calculated from the top of the main UI block.
    int tool_selectors_y = app->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    tool_selectors_draw(app, tool_selectors_y);

    // 4. The main UI block (palette and its separator) starts at canvas_display_area_h.
    int current_y = app->canvas_display_area_h;

    // 5. Separator between canvas/selectors and palette (if palette is visible)
    SDL_bool is_palette_content_visible =
        (app->show_color_palette && app->palette->color_rows > 0) ||
        (app->show_emoji_palette && app->palette->emoji_rows > 0);
    if (is_palette_content_visible && TOOL_SELECTOR_SEPARATOR_HEIGHT > 0) {
        SDL_SetRenderDrawColor(app->ren, 68, 71, 90, 255); // Dracula 'Current Line'
        SDL_Rect sep_rect = {0, current_y, app->window_w, TOOL_SELECTOR_SEPARATOR_HEIGHT};
        SDL_RenderFillRect(app->ren, &sep_rect);
        current_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
    }

    // 6. Palette (conditionally visible rows)
    int active_palette_idx = app_get_current_palette_selection(app);
    palette_draw(app->palette,
                 app->ren,
                 current_y,
                 app->window_w,
                 active_palette_idx,
                 app->show_color_palette,
                 app->show_emoji_palette);

    SDL_RenderPresent(app->ren);
}
