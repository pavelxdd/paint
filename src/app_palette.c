#include "app.h"
#include "palette.h"
#include "ui_constants.h"

/* ------------ tool selection ------------ */
void app_select_palette_tool(App *app, int flat_idx)
{
    if (!app || flat_idx < 0 || flat_idx >= app->palette->total_cells) {
        return;
    }

    if (palette_is_emoji_index(app->palette, flat_idx)) {
        app->current_tool = TOOL_EMOJI;
        app->emoji_selected_palette_idx = flat_idx;
    } else {
        // A color was picked.
        // If current tool is emoji, switch to the last used color tool.
        if (app->current_tool == TOOL_EMOJI) {
            app->current_tool = app->last_color_tool;
        }

        SDL_Color selected_color = palette_get_color(app->palette, flat_idx);

        if (app->current_tool == TOOL_WATER_MARKER) {
            app->water_marker_color = selected_color;
            app->water_marker_selected_palette_idx = flat_idx;
        } else { // Must be TOOL_BRUSH or a color tool that fell through
            app->current_color = selected_color;
            app->brush_selected_palette_idx = flat_idx;
        }
    }
    app->needs_redraw = true;
}

/* ------------ Palette Navigation ------------ */
// palette_type: 0 = color, 1 = emoji
void app_cycle_palette_selection(App *app, int delta, int palette_type)
{
    if (!app || !app->palette) {
        return;
    }

    if (palette_type == 1) {
        // Emoji
        int n = app->palette->total_emoji_cells_to_display;
        if (n <= 0) {
            return;
        }
        int base = app->palette->total_color_cells;
        int cur = app->emoji_selected_palette_idx;
        int rel = cur - base;
        if (rel < 0 || rel >= n) {
            rel = 0;
        }
        rel = (rel + delta + n) % n;
        app->emoji_selected_palette_idx = base + rel;
        app->current_tool = TOOL_EMOJI;
    } else {
        // Color (for brush or water-marker)
        int n = app->palette->total_color_cells;
        if (n <= 0) {
            return;
        }
        int *sel_idx = (app->current_tool == TOOL_WATER_MARKER)
                           ? &app->water_marker_selected_palette_idx
                           : &app->brush_selected_palette_idx;

        int cur = *sel_idx;
        int rel = cur;
        if (rel < 0 || rel >= n) {
            rel = 0;
        }
        rel = (rel + delta + n) % n;
        *sel_idx = rel;
        if (app->current_tool == TOOL_WATER_MARKER) {
            app->water_marker_color = palette_get_color(app->palette, *sel_idx);
        } else {
            app->current_color = palette_get_color(app->palette, *sel_idx);
        }
    }
}

void app_move_palette_selection(App *app, SDL_Keycode key)
{
    if (!app || !app->palette) {
        return;
    }

    int *current_idx;
    int min_idx, max_idx;
    int num_rows;
    int cols = app->palette->cols;

    // Determine which tool is active and set selection index boundaries
    if (app->current_tool == TOOL_EMOJI) {
        current_idx = &app->emoji_selected_palette_idx;
        min_idx = app->palette->total_color_cells;
        max_idx =
            app->palette->total_color_cells + app->palette->total_emoji_cells_to_display - 1;
        num_rows = app->palette->emoji_rows;
    } else {
        // Brush and water-marker use color palette
        if (app->current_tool == TOOL_WATER_MARKER) {
            current_idx = &app->water_marker_selected_palette_idx;
        } else { // TOOL_BRUSH
            current_idx = &app->brush_selected_palette_idx;
        }
        min_idx = 0;
        max_idx = app->palette->total_color_cells - 1;
        num_rows = app->palette->color_rows;
    }

    // Do nothing if the current palette has no items to navigate
    if (num_rows == 0 || max_idx < min_idx) {
        return;
    }

    // Handle arrow key navigation with wrapping
    int current = *current_idx;
    int new_idx = current;

    // The indices are flat, but navigation is grid-based.
    // We work with indices relative to the start of the current grid.
    int relative_idx = current - min_idx;
    int current_row = relative_idx / cols;
    int current_col = relative_idx % cols;

    switch (key) {
    case SDLK_LEFT:
        if (current_col == 0) { // wrap to right
            new_idx = current + cols - 1;
        } else {
            new_idx = current - 1;
        }
        break;
    case SDLK_RIGHT:
        if (current_col == cols - 1) { // wrap to left
            new_idx = current - cols + 1;
        } else {
            new_idx = current + 1;
        }
        break;
    case SDLK_UP:
        if (current_row == 0) { // wrap to bottom
            new_idx = current + cols * (num_rows - 1);
        } else {
            new_idx = current - cols;
        }
        break;
    case SDLK_DOWN:
        if (current_row == num_rows - 1) { // wrap to top
            new_idx = current - cols * (num_rows - 1);
        } else {
            new_idx = current + cols;
        }
        break;
    default:
        return; // Ignore other keys
    }

    // The logic above ensures we stay within the palette's grid, so no
    // extra bounds checks are needed, assuming the palette is a full rectangle.

    // Update selection and color if applicable
    *current_idx = new_idx;

    if (app->current_tool == TOOL_WATER_MARKER) {
        app->water_marker_color = palette_get_color(app->palette, new_idx);
    } else if (app->current_tool == TOOL_BRUSH) {
        app->current_color = palette_get_color(app->palette, new_idx);
    }

    app->needs_redraw = true;
}

int app_get_current_palette_selection(App *app)
{
    if (!app || !app->palette) {
        return -1;
    }
    switch (app->current_tool) {
    case TOOL_EMOJI:
        return app->emoji_selected_palette_idx;
    case TOOL_WATER_MARKER:
        return app->water_marker_selected_palette_idx;
    case TOOL_BRUSH:
    default:
        return app->brush_selected_palette_idx;
    }
}
