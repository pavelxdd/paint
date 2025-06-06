// AI Summary: Implements drawing and hit-testing for the tool selector UI elements
// (color/brush toggle and emoji toggle). It visualizes the current tool selection,
// color, and brush size, and handles user interaction for toggling palette visibility.
#include "tool_selectors.h"
#include "draw.h"
#include "ui_constants.h"
#include "emoji_renderer.h"
#include <math.h>

// Returns TOOL_BRUSH for color toggle, TOOL_EMOJI for emoji toggle, -1 for miss.
int tool_selectors_hit_test(const AppContext *ctx, int mx, int my, int start_y) {
    (void)ctx; // Unused for now
    if (my >= start_y && my < start_y + TOOL_SELECTOR_AREA_HEIGHT) {
        if (mx >= 0 && mx < TOOL_SELECTOR_SIZE) {
            return TOOL_BRUSH; // Hit color toggle
        }
        if (mx >= TOOL_SELECTOR_SIZE && mx < 2 * TOOL_SELECTOR_SIZE) {
            return TOOL_EMOJI; // Hit emoji toggle
        }
    }
    return -1; // Miss
}

void tool_selectors_draw(AppContext *ctx, int start_y) {
    // This function draws the entire toolbar, including backgrounds, content,
    // and all borders (container and active-state highlights).
    // The drawing is layered: backgrounds -> content -> borders.

    // --- 1. Draw Backgrounds ---
    SDL_Rect color_toggle_rect = {0, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};
    SDL_SetRenderDrawColor(ctx->ren, ctx->current_color.r, ctx->current_color.g, ctx->current_color.b, 255);
    SDL_RenderFillRect(ctx->ren, &color_toggle_rect);

    SDL_Rect emoji_toggle_rect = {TOOL_SELECTOR_SIZE, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};
    SDL_Color bg_color = { 40, 42, 54, 255 }; // Dracula 'Background'
    SDL_SetRenderDrawColor(ctx->ren, bg_color.r, bg_color.g, bg_color.b, 255);
    SDL_RenderFillRect(ctx->ren, &emoji_toggle_rect);

    // --- 2. Draw Content (Previews) ---

    // Brush size preview
    Uint8 ir = 255 - ctx->current_color.r;
    Uint8 ig = 255 - ctx->current_color.g;
    Uint8 ib = 255 - ctx->current_color.b;
    SDL_SetRenderDrawColor(ctx->ren, ir, ig, ib, 255);

    int br_cx = color_toggle_rect.x + color_toggle_rect.w / 2;
    int br_cy = color_toggle_rect.y + color_toggle_rect.h / 2;
    int max_cr = color_toggle_rect.w / 2 - 3;
    int cr = ctx->brush_radius;
    if (cr > max_cr) cr = max_cr;
    if (cr < MIN_BRUSH_SIZE) cr = MIN_BRUSH_SIZE;
    draw_hollow_circle(ctx->ren, br_cx, br_cy, cr);

    // Current emoji preview
    SDL_Texture* emoji_tex = NULL;
    int emoji_w = 0, emoji_h = 0;

    if (ctx->current_tool == TOOL_EMOJI) {
        palette_get_emoji_info(ctx->palette, ctx->selected_palette_idx, &emoji_tex, &emoji_w, &emoji_h);
    } else { // TOOL_BRUSH, so show a default emoji
        emoji_renderer_get_default_texture_info(ctx->palette->emoji_renderer_instance, &emoji_tex, &emoji_w, &emoji_h);
    }

    if (emoji_tex) {
        float aspect_ratio = (emoji_h == 0) ? 1.0f : (float)emoji_w / emoji_h;
        int render_h = emoji_toggle_rect.h - 2 * DEFAULT_EMOJI_CELL_PADDING;
        int render_w = lroundf(render_h * aspect_ratio);

        if (render_w > emoji_toggle_rect.w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
            render_w = emoji_toggle_rect.w - 2 * DEFAULT_EMOJI_CELL_PADDING;
            render_h = lroundf(render_w / aspect_ratio);
        }
        if (render_w < 1) render_w = 1;
        if (render_h < 1) render_h = 1;

        SDL_Rect dst_rect = {
            emoji_toggle_rect.x + (emoji_toggle_rect.w - render_w) / 2,
            emoji_toggle_rect.y + (emoji_toggle_rect.h - render_h) / 2,
            render_w,
            render_h
        };
        SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst_rect);
    }

    // --- 3. Draw Borders (Toolbar container and Active Highlights) ---
    SDL_Rect toolbar_area = {0, start_y, 2 * TOOL_SELECTOR_SIZE, TOOL_SELECTOR_AREA_HEIGHT};
    SDL_SetRenderDrawColor(ctx->ren, 68, 71, 90, 255); // Dracula 'Current Line' for container

    // Outer 2px border
    SDL_RenderDrawRect(ctx->ren, &toolbar_area);
    SDL_Rect r_inner = {toolbar_area.x + 1, toolbar_area.y + 1, toolbar_area.w - 2, toolbar_area.h - 2};
    if (r_inner.w > 0 && r_inner.h > 0) {
        SDL_RenderDrawRect(ctx->ren, &r_inner);
    }
    // Vertical separator line (2px)
    SDL_Rect sep_line = {TOOL_SELECTOR_SIZE - 1, start_y, 2, TOOL_SELECTOR_AREA_HEIGHT};
    SDL_RenderFillRect(ctx->ren, &sep_line);

    // Active highlights (drawn on top of the container border)
    if (ctx->current_tool == TOOL_BRUSH) {
        SDL_SetRenderDrawColor(ctx->ren, ir, ig, ib, 255); // Use inverted color for highlight
        SDL_RenderDrawRect(ctx->ren, &color_toggle_rect);
        SDL_Rect r2 = { color_toggle_rect.x + 1, color_toggle_rect.y + 1, color_toggle_rect.w - 2, color_toggle_rect.h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }

    if (ctx->current_tool == TOOL_EMOJI) {
        SDL_SetRenderDrawColor(ctx->ren, 189, 147, 249, 255); // Dracula 'Purple' for highlight
        SDL_RenderDrawRect(ctx->ren, &emoji_toggle_rect);
        SDL_Rect r2 = { emoji_toggle_rect.x + 1, emoji_toggle_rect.y + 1, emoji_toggle_rect.w - 2, emoji_toggle_rect.h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }
}
