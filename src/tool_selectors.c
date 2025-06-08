#include "tool_selectors.h"
#include "draw.h"
#include "ui_constants.h"
#include <math.h>

int tool_selectors_hit_test(const AppContext *ctx, int mx, int my, int start_y)
{
    if (my >= start_y && my < start_y + TOOL_SELECTOR_AREA_HEIGHT) {
        // Left-side tools
        if (mx >= 0 && mx < TOOL_SELECTOR_SIZE) {
            return TOOL_BRUSH;
        }
        if (mx >= TOOL_SELECTOR_SIZE && mx < 2 * TOOL_SELECTOR_SIZE) {
            return TOOL_WATER_MARKER;
        }
        // Right-side tools
        int right_edge = ctx->window_w;
        if (mx >= right_edge - 3 * TOOL_SELECTOR_SIZE && mx < right_edge - 2 * TOOL_SELECTOR_SIZE) {
            return HIT_TEST_LINE_MODE_TOGGLE;
        }
        if (mx >= right_edge - 2 * TOOL_SELECTOR_SIZE && mx < right_edge - TOOL_SELECTOR_SIZE) {
            return TOOL_EMOJI;
        }
        if (mx >= right_edge - TOOL_SELECTOR_SIZE && mx < right_edge) {
            return HIT_TEST_COLOR_PALETTE_TOGGLE;
        }
    }
    return -1; // Miss
}

static void draw_backgrounds(AppContext *ctx,
                             const SDL_Rect *brush_r,
                             const SDL_Rect *water_r,
                             const SDL_Rect *line_r,
                             const SDL_Rect *emoji_r,
                             const SDL_Rect *color_r)
{
    // Brush
    SDL_SetRenderDrawColor(
        ctx->ren, ctx->current_color.r, ctx->current_color.g, ctx->current_color.b, 255);
    SDL_RenderFillRect(ctx->ren, brush_r);

    // Water-marker
    SDL_SetRenderDrawColor(ctx->ren,
                           ctx->water_marker_color.r,
                           ctx->water_marker_color.g,
                           ctx->water_marker_color.b,
                           255);
    SDL_RenderFillRect(ctx->ren, water_r);

    // Line Mode Toggle
    if (app_context_is_straight_line_mode(ctx)) {
        SDL_Color yellow = {241, 250, 140, 255}; // Dracula Yellow
        SDL_Color dark_icon = {40, 42, 54, 255};    // Dracula Background
        SDL_SetRenderDrawColor(ctx->ren, yellow.r, yellow.g, yellow.b, 255);
        SDL_RenderFillRect(ctx->ren, line_r);
        SDL_SetRenderDrawColor(ctx->ren, dark_icon.r, dark_icon.g, dark_icon.b, 255);
    } else {
        SDL_Color bg_color_line = {40, 42, 54, 255}; // Dracula 'Background'
        SDL_SetRenderDrawColor(ctx->ren, bg_color_line.r, bg_color_line.g, bg_color_line.b, 255);
        SDL_RenderFillRect(ctx->ren, line_r);
        SDL_SetRenderDrawColor(ctx->ren, 248, 248, 242, 255); // Dracula 'Foreground'
    }
    // Draw diagonal line icon
    int p = TOOL_SELECTOR_SIZE / 4;
    SDL_RenderDrawLine(ctx->ren, line_r->x + p, line_r->y + line_r->h - p, line_r->x + line_r->w - p, line_r->y + p);
    SDL_RenderDrawLine(
        ctx->ren, line_r->x + p + 1, line_r->y + line_r->h - p, line_r->x + line_r->w - p + 1, line_r->y + p);
    SDL_RenderDrawLine(
        ctx->ren, line_r->x + p, line_r->y + line_r->h - p - 1, line_r->x + line_r->w - p, line_r->y + p - 1);

    // Emoji
    SDL_Color bg_color = {40, 42, 54, 255}; // Dracula 'Background'
    SDL_SetRenderDrawColor(ctx->ren, bg_color.r, bg_color.g, bg_color.b, 255);
    SDL_RenderFillRect(ctx->ren, emoji_r);

    // Color Palette Toggle (Colorful Checkerboard)
    static const SDL_Color checker_colors[] = {{190, 100, 130, 255},
                                               {210, 130, 130, 255},
                                               {205, 160, 110, 255},
                                               {220, 200, 160, 255},
                                               {155, 200, 150, 255},
                                               {130, 195, 205, 255},
                                               {165, 170, 220, 255}};
    const int num_checker_colors = sizeof(checker_colors) / sizeof(checker_colors[0]);
    int num_checkers = 5;
    int checker_size = TOOL_SELECTOR_SIZE / num_checkers;
    for (int row = 0; row < num_checkers; ++row) {
        for (int col = 0; col < num_checkers; ++col) {
            SDL_Rect checker_rect = {color_r->x + col * checker_size,
                                     color_r->y + row * checker_size,
                                     checker_size,
                                     checker_size};
            SDL_Color c = checker_colors[(row + 2 * col) % num_checker_colors];
            SDL_SetRenderDrawColor(ctx->ren, c.r, c.g, c.b, 255);
            SDL_RenderFillRect(ctx->ren, &checker_rect);
        }
    }
}

static void draw_previews(
    AppContext *ctx, const SDL_Rect *brush_r, const SDL_Rect *water_r, const SDL_Rect *emoji_r)
{
    int max_preview_dim = TOOL_SELECTOR_SIZE / 2 - 3;
    int preview_radius = ctx->brush_radius;
    if (preview_radius > max_preview_dim) {
        preview_radius = max_preview_dim;
    }
    if (preview_radius < MIN_BRUSH_SIZE) {
        preview_radius = MIN_BRUSH_SIZE;
    }

    // Brush size preview (circle)
    Uint8 ir = 255 - ctx->current_color.r;
    Uint8 ig = 255 - ctx->current_color.g;
    Uint8 ib = 255 - ctx->current_color.b;
    SDL_SetRenderDrawColor(ctx->ren, ir, ig, ib, 255);
    int br_cx = brush_r->x + brush_r->w / 2;
    int br_cy = brush_r->y + brush_r->h / 2;
    draw_hollow_circle(ctx->ren, br_cx, br_cy, preview_radius);

    // Water-marker preview (hollow square)
    Uint8 w_ir = 255 - ctx->water_marker_color.r;
    Uint8 w_ig = 255 - ctx->water_marker_color.g;
    Uint8 w_ib = 255 - ctx->water_marker_color.b;
    SDL_SetRenderDrawColor(ctx->ren, w_ir, w_ig, w_ib, 255);
    int side = preview_radius * 2;
    SDL_Rect wm_preview_rect_outer = {
        water_r->x + (water_r->w - side) / 2, water_r->y + (water_r->h - side) / 2, side, side};
    SDL_RenderDrawRect(ctx->ren, &wm_preview_rect_outer);
    if (side > 2) { // Draw inner rect for 2px border
        SDL_Rect wm_preview_rect_inner = {wm_preview_rect_outer.x + 1,
                                          wm_preview_rect_outer.y + 1,
                                          wm_preview_rect_outer.w - 2,
                                          wm_preview_rect_outer.h - 2};
        SDL_RenderDrawRect(ctx->ren, &wm_preview_rect_inner);
    }

    // Current emoji preview
    SDL_Texture *emoji_tex = NULL;
    int emoji_w = 0, emoji_h = 0;
    SDL_bool has_emoji = SDL_FALSE;
    if (ctx->current_tool == TOOL_EMOJI) {
        has_emoji = palette_get_emoji_info(
            ctx->palette, ctx->emoji_selected_palette_idx, &emoji_tex, &emoji_w, &emoji_h);
    } else { // Not emoji tool, so show a default emoji
        has_emoji = emoji_renderer_get_default_texture_info(
            ctx->palette->emoji_renderer_instance, &emoji_tex, &emoji_w, &emoji_h);
    }

    if (has_emoji && emoji_tex) {
        float aspect_ratio = (emoji_h == 0) ? 1.0f : (float)emoji_w / emoji_h;
        int render_h = emoji_r->h - 2 * DEFAULT_EMOJI_CELL_PADDING;
        int render_w = lroundf(render_h * aspect_ratio);
        if (render_w > emoji_r->w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
            render_w = emoji_r->w - 2 * DEFAULT_EMOJI_CELL_PADDING;
            render_h = lroundf(render_w / aspect_ratio);
        }
        if (render_w < 1) {
            render_w = 1;
        }
        if (render_h < 1) {
            render_h = 1;
        }
        SDL_Rect dst_rect = {emoji_r->x + (emoji_r->w - render_w) / 2,
                             emoji_r->y + (emoji_r->h - render_h) / 2,
                             render_w,
                             render_h};
        SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst_rect);
    }
}

static void draw_borders_and_highlights(AppContext *ctx,
                                        int start_y,
                                        const SDL_Rect *brush_r,
                                        const SDL_Rect *water_r,
                                        const SDL_Rect *line_r,
                                        const SDL_Rect *emoji_r,
                                        const SDL_Rect *color_r)
{
    SDL_SetRenderDrawColor(ctx->ren, 68, 71, 90, 255); // Dracula 'Current Line' for container

    // Left container
    SDL_Rect left_toolbar_area = {0, start_y, 2 * TOOL_SELECTOR_SIZE, TOOL_SELECTOR_AREA_HEIGHT};
    SDL_RenderDrawRect(ctx->ren, &left_toolbar_area);
    SDL_Rect r_inner_left = {left_toolbar_area.x + 1,
                             left_toolbar_area.y + 1,
                             left_toolbar_area.w - 2,
                             left_toolbar_area.h - 2};
    if (r_inner_left.w > 0 && r_inner_left.h > 0) {
        SDL_RenderDrawRect(ctx->ren, &r_inner_left);
    }

    SDL_Rect sep_line_left = {TOOL_SELECTOR_SIZE - 1, start_y, 2, TOOL_SELECTOR_AREA_HEIGHT};
    SDL_RenderFillRect(ctx->ren, &sep_line_left);

    // Right container
    SDL_Rect right_toolbar_area = {ctx->window_w - 3 * TOOL_SELECTOR_SIZE,
                                   start_y,
                                   3 * TOOL_SELECTOR_SIZE,
                                   TOOL_SELECTOR_AREA_HEIGHT};
    SDL_RenderDrawRect(ctx->ren, &right_toolbar_area);
    SDL_Rect r_inner_right = {right_toolbar_area.x + 1,
                              right_toolbar_area.y + 1,
                              right_toolbar_area.w - 2,
                              right_toolbar_area.h - 2};
    if (r_inner_right.w > 0 && r_inner_right.h > 0) {
        SDL_RenderDrawRect(ctx->ren, &r_inner_right);
    }
    SDL_Rect sep_line_right1 = {
        ctx->window_w - 2 * TOOL_SELECTOR_SIZE - 1, start_y, 2, TOOL_SELECTOR_AREA_HEIGHT};
    SDL_RenderFillRect(ctx->ren, &sep_line_right1);
    SDL_Rect sep_line_right2 = {
        ctx->window_w - TOOL_SELECTOR_SIZE - 1, start_y, 2, TOOL_SELECTOR_AREA_HEIGHT};
    SDL_RenderFillRect(ctx->ren, &sep_line_right2);

    // Active highlights
    if (app_context_is_straight_line_mode(ctx)) {
        SDL_SetRenderDrawColor(ctx->ren, 40, 42, 54, 255); // Dracula 'Background' for contrast
        SDL_RenderDrawRect(ctx->ren, line_r);
        SDL_Rect r2 = {line_r->x + 1, line_r->y + 1, line_r->w - 2, line_r->h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }
    if (ctx->current_tool == TOOL_BRUSH) {
        Uint8 ir = 255 - ctx->current_color.r;
        Uint8 ig = 255 - ctx->current_color.g;
        Uint8 ib = 255 - ctx->current_color.b;
        SDL_SetRenderDrawColor(ctx->ren, ir, ig, ib, 255);
        SDL_RenderDrawRect(ctx->ren, brush_r);
        SDL_Rect r2 = {brush_r->x + 1, brush_r->y + 1, brush_r->w - 2, brush_r->h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }
    if (ctx->current_tool == TOOL_WATER_MARKER) {
        Uint8 w_ir = 255 - ctx->water_marker_color.r;
        Uint8 w_ig = 255 - ctx->water_marker_color.g;
        Uint8 w_ib = 255 - ctx->water_marker_color.b;
        SDL_SetRenderDrawColor(ctx->ren, w_ir, w_ig, w_ib, 255);
        SDL_RenderDrawRect(ctx->ren, water_r);
        SDL_Rect r2 = {water_r->x + 1, water_r->y + 1, water_r->w - 2, water_r->h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }
    if (ctx->current_tool == TOOL_EMOJI) {
        SDL_SetRenderDrawColor(ctx->ren, 189, 147, 249, 255); // Dracula 'Purple'
        SDL_RenderDrawRect(ctx->ren, emoji_r);
        SDL_Rect r2 = {emoji_r->x + 1, emoji_r->y + 1, emoji_r->w - 2, emoji_r->h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }
    if (ctx->show_color_palette) {
        SDL_SetRenderDrawColor(ctx->ren, 68, 71, 90, 255); // Dracula 'Current Line'
        SDL_RenderDrawRect(ctx->ren, color_r);
        SDL_Rect r2 = {color_r->x + 1, color_r->y + 1, color_r->w - 2, color_r->h - 2};
        SDL_RenderDrawRect(ctx->ren, &r2);
    }
}

void tool_selectors_draw(AppContext *ctx, int start_y)
{
    // --- 1. Define Rects for new layout ---
    // Left-side tools
    SDL_Rect brush_toggle_rect = {0, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};
    SDL_Rect water_marker_toggle_rect = {
        TOOL_SELECTOR_SIZE, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};
    // Right-side tools
    SDL_Rect line_toggle_rect = {
        ctx->window_w - 3 * TOOL_SELECTOR_SIZE, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};
    SDL_Rect emoji_toggle_rect = {
        ctx->window_w - 2 * TOOL_SELECTOR_SIZE, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};
    SDL_Rect color_toggle_rect = {
        ctx->window_w - TOOL_SELECTOR_SIZE, start_y, TOOL_SELECTOR_SIZE, TOOL_SELECTOR_SIZE};

    draw_backgrounds(ctx,
                     &brush_toggle_rect,
                     &water_marker_toggle_rect,
                     &line_toggle_rect,
                     &emoji_toggle_rect,
                     &color_toggle_rect);
    draw_previews(ctx, &brush_toggle_rect, &water_marker_toggle_rect, &emoji_toggle_rect);
    draw_borders_and_highlights(ctx,
                                start_y,
                                &brush_toggle_rect,
                                &water_marker_toggle_rect,
                                &line_toggle_rect,
                                &emoji_toggle_rect,
                                &color_toggle_rect);
}
