#include "app.h"
#include "draw.h"

void tool_brush_draw_dab(App *app, int x, int y)
{
    SDL_SetRenderTarget(app->ren, app->canvas_texture);
    SDL_SetRenderDrawColor(
        app->ren, app->current_color.r, app->current_color.g, app->current_color.b, 255);
    draw_circle(app->ren, x, y, app->brush_radius);
    SDL_SetRenderTarget(app->ren, NULL);
}

void tool_brush_draw_line_preview(App *app, int x0, int y0, int x1, int y1)
{
    draw_thick_line(app->ren, x0, y0, x1, y1, app->brush_radius * 2, app->current_color);
}
