#include "app.h"
#include "ui.h"
#include "draw.h"

void tool_brush_draw_dab(App *app, int x, int y)
{
    if (!SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
        SDL_Log("Brush: Failed to set render target: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetRenderDrawColor(
            app->ren, app->current_color.r, app->current_color.g, app->current_color.b, 255)) {
        SDL_Log("Brush: Failed to set draw color: %s", SDL_GetError());
    }
    draw_circle(app->ren, (float)x, (float)y, app->brush_radius);
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Brush: Failed to reset render target: %s", SDL_GetError());
    }
}

void tool_brush_draw_line_preview(App *app, float x0, float y0, float x1, float y1)
{
    draw_thick_line(app->ren, x0, y0, x1, y1, app->brush_radius * 2, app->current_color);
}
