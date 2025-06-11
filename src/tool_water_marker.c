#include "app.h"
#include "draw.h"

void tool_water_marker_begin_stroke(App *app)
{
    if (!app || !app->stroke_buffer) {
        return;
    }
    app->is_buffered_stroke_active = true;

    // Clear the buffer to be fully transparent for the new stroke
    if (!SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
        SDL_Log("Water: Failed to set render target to stroke buffer: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE)) {
        SDL_Log("Water: Failed to set blend mode for clear: %s", SDL_GetError());
    }
    if (!SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 0)) {
        SDL_Log("Water: Failed to set color for clear: %s", SDL_GetError());
    }
    if (!SDL_RenderClear(app->ren)) {
        SDL_Log("Water: Failed to clear stroke buffer: %s", SDL_GetError());
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Water: Failed to reset render target: %s", SDL_GetError());
    }
}

void tool_water_marker_end_stroke(App *app)
{
    if (!app || !app->stroke_buffer || !app->canvas_texture) {
        return;
    }

    // Blend the completed stroke from the buffer onto the main canvas
    if (!SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
        SDL_Log("Water: Failed to set render target to canvas: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Water: Failed to set blend mode for stroke buffer: %s", SDL_GetError());
    }
    if (!SDL_SetTextureAlphaMod(app->stroke_buffer, 128)) { // 50% alpha
        SDL_Log("Water: Failed to set alpha mod for stroke buffer: %s", SDL_GetError());
    }
    if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
        SDL_Log("Water: Failed to render stroke buffer to canvas: %s", SDL_GetError());
    }

    // Reset texture properties and renderer target
    if (!SDL_SetTextureAlphaMod(app->stroke_buffer, 255)) {
        SDL_Log("Water: Failed to reset alpha mod: %s", SDL_GetError());
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Water: Failed to reset render target: %s", SDL_GetError());
    }

    app->needs_redraw = true;
}

void tool_water_marker_draw_dab(App *app, int x, int y)
{
    if (!app->is_buffered_stroke_active || !app->stroke_buffer) {
        return; // Not in a stroke, do nothing
    }
    if (!SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
        SDL_Log("Water: Failed to set RT for dab: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetRenderDrawColor(app->ren,
                                app->water_marker_color.r,
                                app->water_marker_color.g,
                                app->water_marker_color.b,
                                255)) {
        SDL_Log("Water: Failed to set color for dab: %s", SDL_GetError());
    }
    int side = (int)SDL_lroundf(app->brush_radius * 2 * 1.5f);
    SDL_FRect rect = {(float)x - side / 2.0f, (float)y - side / 2.0f, (float)side, (float)side};
    if (!SDL_RenderFillRect(app->ren, &rect)) {
        SDL_Log("Water: Failed to draw dab rect: %s", SDL_GetError());
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Water: Failed to reset RT after dab: %s", SDL_GetError());
    }
}

static void draw_square_dab_callback(int x, int y, void *userdata)
{
    App *app = (App *)userdata;
    int side = (int)SDL_lroundf(app->brush_radius * 2 * 1.5f);
    SDL_FRect rect = {(float)x - side / 2.0f, (float)y - side / 2.0f, (float)side, (float)side};
    if (!SDL_RenderFillRect(app->ren, &rect)) {
        SDL_Log("Water: Failed to draw preview dab rect: %s", SDL_GetError());
    }
}

void tool_water_marker_draw_line_preview(App *app, float x0, float y0, float x1, float y1)
{
    if (!SDL_SetRenderDrawColor(app->ren,
                                app->water_marker_color.r,
                                app->water_marker_color.g,
                                app->water_marker_color.b,
                                255)) {
        SDL_Log("Water: Failed to set color for preview: %s", SDL_GetError());
    }
    draw_line_bresenham((int)x0, (int)y0, (int)x1, (int)y1, draw_square_dab_callback, app);
}
