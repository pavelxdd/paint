#include "app.h"

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
        SDL_FRect rect = {(float)(x0 - side / 2), (float)(y0 - side / 2), (float)side, (float)side};
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

void tool_water_marker_begin_stroke(App *app)
{
    if (!app || !app->stroke_buffer) {
        return;
    }
    app->water_marker_stroke_active = true;

    // Clear the buffer to be fully transparent for the new stroke
    SDL_SetRenderTarget(app->ren, app->stroke_buffer);
    SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0);
    SDL_RenderClear(app->ren);
    SDL_SetRenderTarget(app->ren, NULL);
}

void tool_water_marker_end_stroke(App *app)
{
    if (!app || !app->stroke_buffer || !app->water_marker_stroke_active) {
        return;
    }

    // Blend the completed stroke from the buffer onto the main canvas
    SDL_SetRenderTarget(app->ren, app->canvas_texture);
    SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(app->stroke_buffer, 128); // 50% alpha
    SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL);

    // Reset texture properties and renderer target
    SDL_SetTextureAlphaMod(app->stroke_buffer, 255);
    SDL_SetRenderTarget(app->ren, NULL);

    app->water_marker_stroke_active = false;
    app->needs_redraw = true;
}

void tool_water_marker_draw_dab(App *app, int x, int y)
{
    if (!app->water_marker_stroke_active || !app->stroke_buffer) {
        return; // Not in a stroke, do nothing
    }
    SDL_SetRenderTarget(app->ren, app->stroke_buffer);
    SDL_SetRenderDrawColor(app->ren,
                           app->water_marker_color.r,
                           app->water_marker_color.g,
                           app->water_marker_color.b,
                           255);
    int side = lroundf(app->brush_radius * 2 * 1.5f);
    SDL_FRect rect = {(float)(x - side / 2), (float)(y - side / 2), (float)side, (float)side};
    SDL_RenderFillRect(app->ren, &rect);
    SDL_SetRenderTarget(app->ren, NULL);
}

void tool_water_marker_draw_line_preview(App *app, int x0, int y0, int x1, int y1)
{
    draw_line_of_square_dabs(app, x0, y0, x1, y1);
}
