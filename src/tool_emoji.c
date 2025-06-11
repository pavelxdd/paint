#include "app.h"
#include "ui.h"

static void draw_line_of_emojis(App *app, float x0, float y0, float x1, float y1)
{
    SDL_Texture *emoji_tex = NULL;
    int ew = 0, eh = 0;
    bool has_emoji = palette_get_emoji_info(
                         app->palette, app->emoji_selected_palette_idx,
                         &emoji_tex, &ew, &eh);

    if (!has_emoji || !emoji_tex) {
        return;
    }

    float asp = (eh == 0) ? 1.0f : (float)ew / eh;
    int h = app->brush_radius * 6;
    if (h < MIN_BRUSH_SIZE * 6) {
        h = MIN_BRUSH_SIZE * 6;
    }
    int w = (int)SDL_lroundf(h * asp);
    if (w == 0) {
        w = 1;
    }
    if (h == 0) {
        h = 1;
    }

    // Draw first emoji at the start point
    SDL_FRect dst_start = {x0 - w / 2.0f, y0 - h / 2.0f, (float)w, (float)h};
    if (!SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst_start)) {
        SDL_Log("Emoji: Failed to render start emoji: %s", SDL_GetError());
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float line_length = SDL_sqrtf(dx * dx + dy * dy);

    // Use emoji height as spacing
    if (line_length < h) {
        return;
    }

    float ux = dx / line_length;
    float uy = dy / line_length;

    int num_emojis = (int)SDL_floorf(line_length / h);
    for (int i = 1; i <= num_emojis; ++i) {
        float px = x0 + (float)i * h * ux;
        float py = y0 + (float)i * h * uy;
        SDL_FRect dst = {px - w / 2.0f, py - h / 2.0f, (float)w, (float)h};
        if (!SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst)) {
            SDL_Log("Emoji: Failed to render line emoji: %s", SDL_GetError());
        }
    }
}

void tool_emoji_draw_dab(App *app, int x, int y)
{
    if (!SDL_SetRenderTarget(app->ren, app->canvas_texture)) {
        SDL_Log("Emoji: Failed to set render target for dab: %s", SDL_GetError());
        return;
    }
    SDL_Texture *emoji_tex = NULL;
    int ew = 0, eh = 0;
    bool has_emoji = palette_get_emoji_info(
                         app->palette, app->emoji_selected_palette_idx,
                         &emoji_tex, &ew, &eh);
    if (has_emoji && emoji_tex) {
        float asp = (eh == 0) ? 1.0f : (float)ew / eh;
        int h = app->brush_radius * 6;
        if (h < MIN_BRUSH_SIZE * 6) {
            h = MIN_BRUSH_SIZE * 6;
        }
        int w = (int)SDL_lroundf(h * asp);
        if (w == 0) {
            w = 1;
        }

        SDL_FRect dst = {(float)x - w / 2.0f, (float)y - h / 2.0f, (float)w, (float)h};
        if (!SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst)) {
            SDL_Log("Emoji: Failed to render dab texture: %s", SDL_GetError());
        }
    }
    if (!SDL_SetRenderTarget(app->ren, NULL)) {
        SDL_Log("Emoji: Failed to reset render target after dab: %s", SDL_GetError());
    }
}

void tool_emoji_draw_line_preview(App *app, float x0, float y0, float x1, float y1)
{
    draw_line_of_emojis(app, x0, y0, x1, y1);
}
