#include "app.h"
#include "ui_constants.h"

void app_change_brush_radius(App *app, int delta)
{
    if (!app) {
        return;
    }
    app->brush_radius += delta;
    if (app->brush_radius < MIN_BRUSH_SIZE) {
        app->brush_radius = MIN_BRUSH_SIZE;
    }
    if (app->brush_radius > app->max_brush_radius) {
        app->brush_radius = app->max_brush_radius;
    }
    app->needs_redraw = true;
}

void app_set_brush_radius_from_key(App *app, SDL_Keycode keycode)
{
    if (!app) {
        return;
    }
    // Only allow non-number keys for brush size adjustment
    // Number keys are now used for tool selection
    if (keycode == SDLK_PLUS || keycode == SDLK_EQUALS || keycode == SDLK_KP_PLUS) {
        app_change_brush_radius(app, 2);
    } else if (keycode == SDLK_MINUS || keycode == SDLK_KP_MINUS) {
        app_change_brush_radius(app, -2);
    }
}
