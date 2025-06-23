#include "renderer.h"
#include "ui.h"

void render_scene(App *app)
{
    // Always start with a clean slate - no clipping
    if (!SDL_SetRenderClipRect(app->ren, NULL)) {
        SDL_Log("Failed to reset clip rect: %s", SDL_GetError());
    }

    // Set white background color for the entire window
    if (!SDL_SetRenderDrawColor(app->ren, 255, 255, 255, 255)) {
        SDL_Log("Failed to set draw color: %s", SDL_GetError());
    }

    // Clear the entire window
    if (!SDL_RenderClear(app->ren)) {
        SDL_Log("Failed to clear renderer: %s", SDL_GetError());
    }

    // The correct behavior of dirty rectangle implementation
    // is to always render the full canvas texture to the backbuffer,
    // then render any previews and UI elements on top.

    // 1. Render the main canvas content
    if (app->is_drawing && app->current_tool == TOOL_BLUR && app->is_buffered_stroke_active) {
        // For blur, the stroke_buffer is the "live" canvas
        if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
            SDL_Log("Failed to render stroke_buffer for blur: %s", SDL_GetError());
        }
    } else if (app->canvas_texture) {
        // Default: render the main canvas
        if (!SDL_RenderTexture(app->ren, app->canvas_texture, NULL, NULL)) {
            SDL_Log("Failed to render canvas texture: %s", SDL_GetError());
        }
    }

    // 2. Render tool previews from the stroke buffer if necessary (for non-blur tools).
    // For these operations, we need to render on top of what's already been drawn
    if (app->is_drawing && app->straight_line_stroke_latched && app->stroke_buffer) {
        // For straight line previews, we need to render the entire stroke buffer
        // since it contains the temporary line preview
        if (app->current_tool == TOOL_BRUSH || app->current_tool == TOOL_EMOJI) {
            if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
                SDL_Log("Render: Failed to render stroke buffer preview: %s", SDL_GetError());
            }
        } else if (app->current_tool == TOOL_WATER_MARKER) {
            if (!SDL_SetTextureAlphaMod(app->stroke_buffer, 128)) {
                SDL_Log("Render: Failed to set alpha for water marker preview: %s", SDL_GetError());
            }
            if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
                SDL_Log("Render: Failed to render water marker preview: %s", SDL_GetError());
            }
            if (!SDL_SetTextureAlphaMod(app->stroke_buffer, 255)) {
                SDL_Log("Render: Failed to reset alpha for water marker: %s", SDL_GetError());
            }
        }
    } else if (app->is_drawing && app->is_buffered_stroke_active &&
               !app->straight_line_stroke_latched) {
        // Render a freehand stroke in progress for buffered tools (e.g., water marker)
        if (app->current_tool == TOOL_WATER_MARKER) {
            if (!SDL_SetTextureAlphaMod(app->stroke_buffer, 128)) {
                SDL_Log("Render: Failed to set alpha for water marker stroke: %s", SDL_GetError());
            }

            // Always render the full stroke buffer for water marker strokes
            if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
                SDL_Log("Render: Failed to render water marker stroke: %s", SDL_GetError());
            }

            if (!SDL_SetTextureAlphaMod(app->stroke_buffer, 255)) { // Reset
                SDL_Log("Render: Failed to reset alpha for water marker stroke: %s", SDL_GetError());
            }
        }
    }

    // Always clear the UI area (below canvas_display_area_h) to ensure it's drawn correctly
    // This is outside the canvas area, so we need to clear it regardless of dirty rectangles
    SDL_FRect ui_rect = {
        0, (float)app->canvas_display_area_h,
        (float)app->window_w, (float)(app->window_h - app->canvas_display_area_h)
    };

    if (!SDL_SetRenderDrawColor(app->ren, 255, 255, 255, 255)) {
        SDL_Log("Failed to set draw color for UI area: %s", SDL_GetError());
    }
    if (!SDL_RenderFillRect(app->ren, &ui_rect)) {
        SDL_Log("Failed to clear UI area: %s", SDL_GetError());
    }

    // --- UI drawing from top to bottom, overlaid on the canvas ---

    // 3. Tool selectors "float" over the canvas, just above the main UI panel.
    int tool_selectors_y = app->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    ui_draw_tool_selectors(app, tool_selectors_y);

    // 4. The main UI block (palette and its separator) starts at canvas_display_area_h.
    int current_y = app->canvas_display_area_h;

    // 5. Separator between canvas/selectors and palette (if palette is visible)
    bool is_palette_content_visible =
        (app->show_color_palette && app->palette->color_rows > 0) ||
        (app->show_emoji_palette && app->palette->emoji_rows > 0);
    if (is_palette_content_visible && TOOL_SELECTOR_SEPARATOR_HEIGHT > 0) {
        if (!SDL_SetRenderDrawColor(app->ren, 68, 71, 90, 255)) { // Dracula 'Current Line'
            SDL_Log("Render: Failed to set color for separator: %s", SDL_GetError());
        }
        SDL_FRect sep_rect = {
            0, (float)current_y, (float)app->window_w, (float)TOOL_SELECTOR_SEPARATOR_HEIGHT
        };
        if (!SDL_RenderFillRect(app->ren, &sep_rect)) {
            SDL_Log("Render: Failed to fill separator: %s", SDL_GetError());
        }
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

    if (!SDL_RenderPresent(app->ren)) {
        SDL_Log("SDL_RenderPresent failed: %s", SDL_GetError());
    }

    // Clear dirty rectangles after rendering
    app_clear_dirty_rects(app);
}
