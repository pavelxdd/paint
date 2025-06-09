#include "app.h"
#include "palette.h"

void app_recalculate_sizes_and_limits(App *app)
{
    if (!app) {
        return;
    }

    /* Ensure canvas height is up-to-date first (lives in this module) */
    app_update_canvas_display_height(app);

    int cell_dim = PALETTE_HEIGHT / 2; /* safe fallback */
    if (app->palette && app->palette->cols > 0) {
        int cell_w = app->window_w / app->palette->cols;
        cell_dim = (cell_w < PALETTE_HEIGHT ? cell_w : PALETTE_HEIGHT) / 2;
    }
    app->max_brush_radius = (cell_dim < MIN_BRUSH_SIZE) ? MIN_BRUSH_SIZE : cell_dim;

    if (app->brush_radius > app->max_brush_radius) {
        app->brush_radius = app->max_brush_radius;
    }
    if (app->brush_radius < MIN_BRUSH_SIZE) {
        app->brush_radius = MIN_BRUSH_SIZE;
    }
}

void app_update_canvas_display_height(App *app)
{
    if (!app) {
        return;
    }

    int palette_h = 0;

    if (app->show_color_palette && app->palette && app->palette->color_rows) {
        palette_h += app->palette->color_rows * PALETTE_HEIGHT;
    }
    if (app->show_emoji_palette && app->palette && app->palette->emoji_rows) {
        palette_h += app->palette->emoji_rows * PALETTE_HEIGHT;
    }
    if (app->show_color_palette && app->show_emoji_palette && app->palette &&
        app->palette->color_rows > 0 && app->palette->emoji_rows > 0 &&
        COLOR_EMOJI_SEPARATOR_HEIGHT > 0) {
        palette_h += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    int ui_h = palette_h ? TOOL_SELECTOR_SEPARATOR_HEIGHT + palette_h : 0;
    app->canvas_display_area_h = app->window_h - ui_h;
    if (app->canvas_display_area_h < 0) {
        app->canvas_display_area_h = 0;
    }
}
