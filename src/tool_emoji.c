#include "app.h"
#include <math.h>

#include <stdbool.h>

static void draw_line_of_emojis(App *app, int x0, int y0, int x1, int y1)
{
    SDL_Texture *emoji_tex = NULL;
    int ew = 0, eh = 0;
    bool has_emoji = palette_get_emoji_info(
        app->palette, app->emoji_selected_palette_idx, &emoji_tex, &ew, &eh);

    if (!has_emoji || !emoji_tex) {
        return;
    }

    float asp = (eh == 0) ? 1.0f : (float)ew / eh;
    int h = app->brush_radius * 6;
    if (h < MIN_BRUSH_SIZE * 6) {
        h = MIN_BRUSH_SIZE * 6;
    }
    int w = lroundf(h * asp);
    if (w == 0) {
        w = 1;
    }
    if (h == 0) {
        h = 1;
    }

    // Draw first emoji at the start point
    SDL_FRect dst_start = {(float)x0 - w / 2, (float)y0 - h / 2, (float)w, (float)h};
    SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst_start);

    float dx = x1 - x0;
    float dy = y1 - y0;
    float line_length = sqrtf(dx * dx + dy * dy);

    // Use emoji height as spacing
    if (line_length < h) {
        return;
    }

    float ux = dx / line_length;
    float uy = dy / line_length;

    int num_emojis = floorf(line_length / h);
    for (int i = 1; i <= num_emojis; ++i) {
        int px = x0 + lroundf((float)i * h * ux);
        int py = y0 + lroundf((float)i * h * uy);
        SDL_FRect dst = {(float)px - w / 2, (float)py - h / 2, (float)w, (float)h};
        SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst);
    }
}

void tool_emoji_draw_dab(App *app, int x, int y)
{
    SDL_SetRenderTarget(app->ren, app->canvas_texture);
    SDL_Texture *emoji_tex = NULL;
    int ew = 0, eh = 0;
    bool has_emoji = palette_get_emoji_info(
        app->palette, app->emoji_selected_palette_idx, &emoji_tex, &ew, &eh);
    if (has_emoji && emoji_tex) {
        float asp = (eh == 0) ? 1.0f : (float)ew / eh;
        int h = app->brush_radius * 6;
        if (h < MIN_BRUSH_SIZE * 6) {
            h = MIN_BRUSH_SIZE * 6;
        }
        int w = lroundf(h * asp);
        if (w == 0) {
            w = 1;
        }

        SDL_FRect dst = {(float)x - w / 2, (float)y - h / 2, (float)w, (float)h};
        SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst);
    }
    SDL_SetRenderTarget(app->ren, NULL);
}

void tool_emoji_draw_line_preview(App *app, int x0, int y0, int x1, int y1)
{
    draw_line_of_emojis(app, x0, y0, x1, y1);
}
