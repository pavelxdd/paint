#include "app.h"
#include "draw.h"
#include "tool.h"

#define BLUR_PASSES 16

// GPU-accelerated blur.
// This is not a "true" blur, but a fast approximation using downscaling.

void tool_blur_begin_stroke(App *app)
{
    if (!app || !app->canvas_texture || !app->blur_source_texture || !app->stroke_buffer) {
        return;
    }
    app->is_buffered_stroke_active = true;

    // 1. Copy canvas to a source texture. This is our pristine source for blurring for the
    // duration of a straight-line preview.
    if (!SDL_SetRenderTarget(app->ren, app->blur_source_texture)) {
        SDL_Log("SetRenderTarget blur_source_texture failed: %s", SDL_GetError());
        return;
    }
    if (!SDL_RenderTexture(app->ren, app->canvas_texture, NULL, NULL)) {
        SDL_Log("RenderTexture to blur_source_texture failed: %s", SDL_GetError());
    }

    // 2. ALSO copy canvas to the stroke buffer. This buffer will be actively modified by the
    // blur tool and displayed in real-time.
    if (!SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
        SDL_Log("SetRenderTarget stroke_buffer failed: %s", SDL_GetError());
        SDL_SetRenderTarget(app->ren, NULL);
        return;
    }
    if (!SDL_SetTextureBlendMode(app->canvas_texture, SDL_BLENDMODE_NONE)) {
        SDL_Log("SetTextureBlendMode for canvas_texture failed: %s", SDL_GetError());
    }
    if (!SDL_RenderTexture(app->ren, app->canvas_texture, NULL, NULL)) {
        SDL_Log("RenderTexture to stroke_buffer failed: %s", SDL_GetError());
    }
    if (!SDL_SetTextureBlendMode(app->canvas_texture, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Restoring blend mode for canvas_texture failed: %s", SDL_GetError());
    }

    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Restoring default render target failed: %s", SDL_GetError());
    }
}

void tool_blur_end_stroke(App *app)
{
    if (!app || !app->stroke_buffer || !app->canvas_texture) {
        return;
    }

    // Copy the completed stroke from the buffer onto the main canvas.
    if (!SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
        SDL_Log("SetRenderTarget canvas_texture failed: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_NONE)) {
        SDL_Log("SetTextureBlendMode for stroke_buffer failed: %s", SDL_GetError());
    }
    if (!SDL_RenderTexture(app->ren, app->stroke_buffer, NULL, NULL)) {
        SDL_Log("Rendering stroke_buffer to canvas failed: %s", SDL_GetError());
    }

    // Restore default blend mode for the texture and reset renderer target
    if (!SDL_SetTextureBlendMode(app->stroke_buffer, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Restoring blend mode for stroke_buffer failed: %s", SDL_GetError());
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Restoring default render target failed: %s", SDL_GetError());
    }

    app->needs_redraw = true;
}

void tool_blur_draw_dab(App *app, int x, int y)
{
    if (!app->is_buffered_stroke_active || !app->blur_dab_texture || !app->blur_temp_texture) {
        return;
    }

    int visual_radius = app->brush_radius * 2;
    if (visual_radius < 1) {
        visual_radius = 1;
    }

    SDL_Rect src_rect = {
        x - visual_radius,
        y - visual_radius,
        visual_radius * 2,
        visual_radius * 2,
    };

    SDL_Rect canvas_rect = {0, 0, app->canvas_texture_w, app->canvas_texture_h};
    SDL_GetRectIntersection(&src_rect, &canvas_rect, &src_rect);
    if (SDL_RectEmpty(&src_rect)) {
        return;
    }

    SDL_FRect f_src_rect;
    SDL_RectToFRect(&src_rect, &f_src_rect);

    // --- Step 1: Copy the region to be blurred from the LIVE stroke_buffer to a small texture ---
    if (!SDL_SetRenderTarget(app->ren, app->blur_dab_texture)) {
        SDL_Log("Blur: Failed to set RT to dab texture: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetTextureScaleMode(app->stroke_buffer, SDL_SCALEMODE_LINEAR)) { // Use linear for downscale blur
        SDL_Log("Blur: Failed to set scale mode for stroke buffer: %s", SDL_GetError());
    }
    if (!SDL_RenderTexture(app->ren, app->stroke_buffer, &f_src_rect, NULL)) {
        SDL_Log("Blur: Failed to render to dab texture: %s", SDL_GetError());
    }

    // --- Step 2: Perform multiple blur passes ---
    SDL_Texture *blur_src = app->blur_dab_texture;
    SDL_Texture *blur_dst = app->blur_temp_texture;

    if (!SDL_SetTextureScaleMode(blur_src, SDL_SCALEMODE_LINEAR) ||
        !SDL_SetTextureScaleMode(blur_dst, SDL_SCALEMODE_LINEAR)) {
        SDL_Log("Blur: Failed to set scale mode for blur textures: %s", SDL_GetError());
        return;
    }

    for (int i = 0; i < BLUR_PASSES; ++i) {
        if (!SDL_SetRenderTarget(app->ren, blur_dst)) {
            SDL_Log("Blur: Failed to set RT for pass %d: %s", i, SDL_GetError());
            break;
        }
        if (!SDL_RenderTexture(app->ren, blur_src, NULL, NULL)) {
            SDL_Log("Blur: Failed to render for pass %d: %s", i, SDL_GetError());
        }

        SDL_Texture *tmp = blur_src;
        blur_src = blur_dst;
        blur_dst = tmp;
    }

    // --- Step 3: Blend the blurred texture back onto the live stroke_buffer ---
    if (!SDL_SetRenderTarget(app->ren, app->stroke_buffer)) {
        SDL_Log("Blur: Failed to set RT to stroke buffer: %s", SDL_GetError());
        return;
    }
    if (!SDL_SetTextureBlendMode(blur_src, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Blur: Failed to set blend mode for blurred texture: %s", SDL_GetError());
    }
    if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Blur: Failed to set blend mode for renderer: %s", SDL_GetError());
    }

    const int circle_segments = 16;
    SDL_Vertex vertices[circle_segments + 2];
    SDL_zeroa(vertices);

    SDL_FColor center_color = {1.0f, 1.0f, 1.0f, 1.0f};
    SDL_FColor outer_color = {1.0f, 1.0f, 1.0f, 0.0f};

    vertices[0].position.x = (float)x;
    vertices[0].position.y = (float)y;
    vertices[0].tex_coord.x = 0.5f;
    vertices[0].tex_coord.y = 0.5f;
    vertices[0].color = center_color;

    const float tex_margin = 0.5f / BLUR_DAB_DOWNSCALE_SIZE;
    const float tex_radius = 0.5f - tex_margin;

    for (int i = 0; i <= circle_segments; ++i) {
        float angle = (float)i / (float)circle_segments * 2.0f * SDL_PI_F;
        vertices[i + 1].position.x = x + SDL_cosf(angle) * (float)visual_radius;
        vertices[i + 1].position.y = y + SDL_sinf(angle) * (float)visual_radius;
        vertices[i + 1].tex_coord.x = 0.5f + SDL_cosf(angle) * tex_radius;
        vertices[i + 1].tex_coord.y = 0.5f + SDL_sinf(angle) * tex_radius;
        vertices[i + 1].color = outer_color;
    }

    int indices[circle_segments * 3];
    for (int i = 0; i < circle_segments; ++i) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    if (!SDL_RenderGeometry(app->ren, blur_src,
                            vertices, SDL_arraysize(vertices),
                            indices, SDL_arraysize(indices))) {
        SDL_Log("RenderGeometry for blur dab failed: %s", SDL_GetError());
    }

    // Reset state
    if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE)) {
        SDL_Log("Blur: Failed to reset renderer blend mode: %s", SDL_GetError());
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Blur: Failed to reset render target: %s", SDL_GetError());
    }
}

static void tool_blur_dab_callback(int x, int y, void *userdata)
{
    tool_blur_draw_dab((App *)userdata, x, y);
}

void tool_blur_draw_line_of_dabs(App *app, float x0, float y0, float x1, float y1)
{
    draw_line_bresenham((int)x0, (int)y0, (int)x1, (int)y1, tool_blur_dab_callback, app);
}

void tool_blur_draw_line_preview(App *app, float x0, float y0, float x1, float y1)
{
    tool_blur_draw_line_of_dabs(app, x0, y0, x1, y1);
}
