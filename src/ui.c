#include "app.h"
#include "draw.h"
#include "ui.h"

int ui_hit_test_tool_selectors(const App *app, int mx, int my, int start_y)
{
    if (my >= start_y && my < start_y + TOOL_SELECTOR_AREA_HEIGHT) {
        // Left-side tools
        if (mx >= 0 && mx < TOOL_SELECTOR_SIZE) {
            return TOOL_BRUSH;
        }
        if (mx >= TOOL_SELECTOR_SIZE && mx < 2 * TOOL_SELECTOR_SIZE) {
            return TOOL_WATER_MARKER;
        }
        if (mx >= 2 * TOOL_SELECTOR_SIZE && mx < 3 * TOOL_SELECTOR_SIZE) {
            return TOOL_BLUR;
        }
        // Right-side tools
        int right_edge = app->window_w;
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

static void draw_backgrounds(App *app,
                             const SDL_FRect *brush_r,
                             const SDL_FRect *water_r,
                             const SDL_FRect *blur_r,
                             const SDL_FRect *line_r,
                             const SDL_FRect *emoji_r,
                             const SDL_FRect *color_r)
{
    // Brush
    if (!SDL_SetRenderDrawColor(
            app->ren, app->current_color.r, app->current_color.g, app->current_color.b, 255)) {
        SDL_Log("UI: Failed to set brush bg color: %s", SDL_GetError());
    }
    if (!SDL_RenderFillRect(app->ren, brush_r)) {
        SDL_Log("UI: Failed to fill brush bg: %s", SDL_GetError());
    }

    // Water-marker
    if (!SDL_SetRenderDrawColor(app->ren,
                                app->water_marker_color.r,
                                app->water_marker_color.g,
                                app->water_marker_color.b,
                                255)) {
        SDL_Log("UI: Failed to set water-marker bg color: %s", SDL_GetError());
    }
    if (!SDL_RenderFillRect(app->ren, water_r)) {
        SDL_Log("UI: Failed to fill water-marker bg: %s", SDL_GetError());
    }

    // Blur
    SDL_Color bg_color_blur = {40, 42, 54, 255}; // Dracula 'Background'
    if (!SDL_SetRenderDrawColor(
            app->ren, bg_color_blur.r, bg_color_blur.g, bg_color_blur.b, 255)) {
        SDL_Log("UI: Failed to set blur bg color: %s", SDL_GetError());
    }
    if (!SDL_RenderFillRect(app->ren, blur_r)) {
        SDL_Log("UI: Failed to fill blur bg: %s", SDL_GetError());
    }

    // Line Mode Toggle
    bool line_mode_disabled = (app->current_tool == TOOL_BLUR);
    if (line_mode_disabled) {
        SDL_Color bg = {68, 71, 90, 255};    // Dracula 'Current Line' (dark gray)
        SDL_Color icon = {98, 114, 164, 255}; // Dracula 'Comment' (light gray)
        if (!SDL_SetRenderDrawColor(app->ren, bg.r, bg.g, bg.b, 255)) {
            SDL_Log("UI: Failed to set line-mode disabled bg: %s", SDL_GetError());
        }
        if (!SDL_RenderFillRect(app->ren, line_r)) {
            SDL_Log("UI: Failed to fill line-mode disabled bg: %s", SDL_GetError());
        }
        if (!SDL_SetRenderDrawColor(app->ren, icon.r, icon.g, icon.b, 255)) {
            SDL_Log("UI: Failed to set line-mode disabled icon: %s", SDL_GetError());
        }
    } else if (app_is_straight_line_mode(app)) {
        SDL_Color bg = {241, 250, 140, 255}; // Dracula Yellow
        SDL_Color icon = {40, 42, 54, 255};  // Dracula Background
        if (!SDL_SetRenderDrawColor(app->ren, bg.r, bg.g, bg.b, 255)) {
            SDL_Log("UI: Failed to set line-mode on bg color: %s", SDL_GetError());
        }
        if (!SDL_RenderFillRect(app->ren, line_r)) {
            SDL_Log("UI: Failed to fill line-mode on bg: %s", SDL_GetError());
        }
        if (!SDL_SetRenderDrawColor(app->ren, icon.r, icon.g, icon.b, 255)) {
            SDL_Log("UI: Failed to set line-mode on icon color: %s", SDL_GetError());
        }
    } else {
        SDL_Color bg = {40, 42, 54, 255};   // Dracula 'Background'
        SDL_Color icon = {248, 248, 242, 255}; // Dracula 'Foreground'
        if (!SDL_SetRenderDrawColor(app->ren, bg.r, bg.g, bg.b, 255)) {
            SDL_Log("UI: Failed to set line-mode off bg color: %s", SDL_GetError());
        }
        if (!SDL_RenderFillRect(app->ren, line_r)) {
            SDL_Log("UI: Failed to fill line-mode off bg: %s", SDL_GetError());
        }
        if (!SDL_SetRenderDrawColor(app->ren, icon.r, icon.g, icon.b, 255)) {
            SDL_Log("UI: Failed to set line-mode off icon color: %s", SDL_GetError());
        }
    }

    // Draw diagonal line icon (color is set by the logic above)
    int p = TOOL_SELECTOR_SIZE / 4;
    if (!SDL_RenderLine(app->ren,
                        line_r->x + p,
                        line_r->y + line_r->h - p,
                        line_r->x + line_r->w - p,
                        line_r->y + p)) {
        SDL_Log("UI: Failed to draw line icon: %s", SDL_GetError());
    }
    if (!SDL_RenderLine(app->ren,
                        line_r->x + p + 1,
                        line_r->y + line_r->h - p,
                        line_r->x + line_r->w - p + 1,
                        line_r->y + p)) {
        SDL_Log("UI: Failed to draw line icon shadow 1: %s", SDL_GetError());
    }
    if (!SDL_RenderLine(app->ren,
                        line_r->x + p,
                        line_r->y + line_r->h - p - 1,
                        line_r->x + line_r->w - p,
                        line_r->y + p - 1)) {
        SDL_Log("UI: Failed to draw line icon shadow 2: %s", SDL_GetError());
    }

    // Emoji
    SDL_Color bg_color = {40, 42, 54, 255}; // Dracula 'Background'
    if (!SDL_SetRenderDrawColor(app->ren, bg_color.r, bg_color.g, bg_color.b, 255)) {
        SDL_Log("UI: Failed to set emoji bg color: %s", SDL_GetError());
    }
    if (!SDL_RenderFillRect(app->ren, emoji_r)) {
        SDL_Log("UI: Failed to fill emoji bg: %s", SDL_GetError());
    }

    // Color Palette Toggle (Colorful Checkerboard)
    static const SDL_Color checker_colors[] = {
        {190, 100, 130, 255},
        {210, 130, 130, 255},
        {205, 160, 110, 255},
        {220, 200, 160, 255},
        {155, 200, 150, 255},
        {130, 195, 205, 255},
        {165, 170, 220, 255},
    };
    const int num_checker_colors = sizeof(checker_colors) / sizeof(checker_colors[0]);
    int num_checkers = 5;
    float checker_size = (float)TOOL_SELECTOR_SIZE / num_checkers;
    for (int row = 0; row < num_checkers; ++row) {
        for (int col = 0; col < num_checkers; ++col) {
            SDL_FRect checker_rect = {
                color_r->x + col * checker_size,
                color_r->y + row * checker_size,
                checker_size,
                checker_size,
            };
            SDL_Color c = checker_colors[(row + 2 * col) % num_checker_colors];
            if (!SDL_SetRenderDrawColor(app->ren, c.r, c.g, c.b, 255)) {
                SDL_Log("UI: Failed to set checker color: %s", SDL_GetError());
            }
            if (!SDL_RenderFillRect(app->ren, &checker_rect)) {
                SDL_Log("UI: Failed to fill checker rect: %s", SDL_GetError());
            }
        }
    }
}

static void draw_previews(App *app,
                          const SDL_FRect *brush_r,
                          const SDL_FRect *water_r,
                          const SDL_FRect *blur_r,
                          const SDL_FRect *emoji_r)
{
    int max_preview_dim = TOOL_SELECTOR_SIZE / 2 - 3;
    int preview_radius = app->brush_radius;
    if (preview_radius > max_preview_dim) {
        preview_radius = max_preview_dim;
    }
    if (preview_radius < MIN_BRUSH_SIZE) {
        preview_radius = MIN_BRUSH_SIZE;
    }

    // Brush size preview (circle)
    Uint8 ir = 255 - app->current_color.r;
    Uint8 ig = 255 - app->current_color.g;
    Uint8 ib = 255 - app->current_color.b;
    if (!SDL_SetRenderDrawColor(app->ren, ir, ig, ib, 255)) {
        SDL_Log("UI: Failed to set brush preview color: %s", SDL_GetError());
    }
    float br_cx = brush_r->x + brush_r->w / 2.0f;
    float br_cy = brush_r->y + brush_r->h / 2.0f;
    draw_hollow_circle(app->ren, br_cx, br_cy, preview_radius);

    // Water-marker preview (hollow square)
    Uint8 w_ir = 255 - app->water_marker_color.r;
    Uint8 w_ig = 255 - app->water_marker_color.g;
    Uint8 w_ib = 255 - app->water_marker_color.b;
    if (!SDL_SetRenderDrawColor(app->ren, w_ir, w_ig, w_ib, 255)) {
        SDL_Log("UI: Failed to set water-marker preview color: %s", SDL_GetError());
    }
    float side = (float)preview_radius * 2;
    SDL_FRect wm_preview_rect_outer = {
        water_r->x + (water_r->w - side) / 2.0f,
        water_r->y + (water_r->h - side) / 2.0f,
        side,
        side,
    };
    if (!SDL_RenderRect(app->ren, &wm_preview_rect_outer)) {
        SDL_Log("UI: Failed to draw water-marker preview rect: %s", SDL_GetError());
    }
    if (side > 2) { // Draw inner rect for 2px border
        SDL_FRect wm_preview_rect_inner = {
            wm_preview_rect_outer.x + 1,
            wm_preview_rect_outer.y + 1,
            wm_preview_rect_outer.w - 2,
            wm_preview_rect_outer.h - 2,
        };
        if (!SDL_RenderRect(app->ren, &wm_preview_rect_inner)) {
            SDL_Log("UI: Failed to draw inner water-marker preview rect: %s", SDL_GetError());
        }
    }

    // Blur tool preview (blurred circle icon)
    float blur_cx = blur_r->x + blur_r->w / 2.0f;
    float blur_cy = blur_r->y + blur_r->h / 2.0f;
    if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_BLEND)) {
        SDL_Log("UI: Failed to set blend mode for blur preview: %s", SDL_GetError());
    }

    // Draw a soft, multi-layered circle to simulate a blur/glow effect.
    // This version has a more solid center for better visibility.
    int steps = preview_radius / 2;
    if (steps < 2) {
        steps = 2;
    }
    if (steps > 6) {
        steps = 6;
    }

    for (int i = 0; i < steps; ++i) {
        float ratio = (float)i / (steps - 1); // 0 to 1
        int r = preview_radius - (int)(ratio * preview_radius);
        if (r < 1) {
            r = 1;
        }
        // Alpha is higher for the innermost circle and decreases outwards.
        int alpha = 80 - (int)(ratio * 60);
        if (!SDL_SetRenderDrawColor(app->ren, 248, 248, 242, alpha)) {
            SDL_Log("UI: Failed to set color for blur preview glow: %s", SDL_GetError());
        }
        draw_circle(app->ren, blur_cx, blur_cy, r);
    }
    if (!SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_NONE)) {
        SDL_Log("UI: Failed to reset blend mode after blur preview: %s", SDL_GetError());
    }

    // Current emoji preview
    SDL_Texture *emoji_tex = NULL;
    int emoji_w = 0, emoji_h = 0;
    bool has_emoji = false;
    if (app->current_tool == TOOL_EMOJI) {
        has_emoji = palette_get_emoji_info(
                        app->palette, app->emoji_selected_palette_idx,
                        &emoji_tex, &emoji_w, &emoji_h);
    } else { // Not emoji tool, so show a default emoji
        has_emoji = emoji_renderer_get_default_texture_info(
                        app->palette->emoji_renderer_instance,
                        &emoji_tex, &emoji_w, &emoji_h);
    }

    if (has_emoji && emoji_tex) {
        float aspect_ratio = (emoji_h == 0) ? 1.0f : (float)emoji_w / emoji_h;
        float render_h = emoji_r->h - 2 * DEFAULT_EMOJI_CELL_PADDING;
        float render_w = render_h * aspect_ratio;
        if (render_w > emoji_r->w - 2 * DEFAULT_EMOJI_CELL_PADDING) {
            render_w = emoji_r->w - 2 * DEFAULT_EMOJI_CELL_PADDING;
            render_h = render_w / aspect_ratio;
        }
        if (render_w < 1) {
            render_w = 1;
        }
        if (render_h < 1) {
            render_h = 1;
        }
        SDL_FRect dst_rect = {
            emoji_r->x + (emoji_r->w - render_w) / 2.0f,
            emoji_r->y + (emoji_r->h - render_h) / 2.0f,
            render_w,
            render_h,
        };
        if (!SDL_RenderTexture(app->ren, emoji_tex, NULL, &dst_rect)) {
            SDL_Log("UI: Failed to render emoji preview texture: %s", SDL_GetError());
        }
    }
}

static void draw_borders_and_highlights(App *app,
                                        int start_y,
                                        const SDL_FRect *brush_r,
                                        const SDL_FRect *water_r,
                                        const SDL_FRect *blur_r,
                                        const SDL_FRect *line_r,
                                        const SDL_FRect *emoji_r,
                                        const SDL_FRect *color_r)
{
    if (!SDL_SetRenderDrawColor(app->ren, 68, 71, 90, 255)) { // Dracula 'Current Line' for container
        SDL_Log("UI: Failed to set border color: %s", SDL_GetError());
    }

    // Left container
    SDL_FRect left_toolbar_area = {
        0, (float)start_y, 3.0f * TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_AREA_HEIGHT
    };
    if (!SDL_RenderRect(app->ren, &left_toolbar_area)) {
        SDL_Log("UI: Failed to draw left toolbar border: %s", SDL_GetError());
    }
    SDL_FRect r_inner_left = {
        left_toolbar_area.x + 1,
        left_toolbar_area.y + 1,
        left_toolbar_area.w - 2,
        left_toolbar_area.h - 2,
    };
    if (r_inner_left.w > 0 && r_inner_left.h > 0) {
        if (!SDL_RenderRect(app->ren, &r_inner_left)) {
            SDL_Log("UI: Failed to draw inner left toolbar border: %s", SDL_GetError());
        }
    }

    SDL_FRect sep_line_left1 = {
        (float)TOOL_SELECTOR_SIZE - 1, (float)start_y, 2, (float)TOOL_SELECTOR_AREA_HEIGHT
    };
    if (!SDL_RenderFillRect(app->ren, &sep_line_left1)) {
        SDL_Log("UI: Failed to draw left separator 1: %s", SDL_GetError());
    }
    SDL_FRect sep_line_left2 = {
        (float)2 * TOOL_SELECTOR_SIZE - 1, (float)start_y, 2, (float)TOOL_SELECTOR_AREA_HEIGHT
    };
    if (!SDL_RenderFillRect(app->ren, &sep_line_left2)) {
        SDL_Log("UI: Failed to draw left separator 2: %s", SDL_GetError());
    }

    // Right container
    SDL_FRect right_toolbar_area = {
        (float)app->window_w - 3 * TOOL_SELECTOR_SIZE,
        (float)start_y,
        3.0f * TOOL_SELECTOR_SIZE,
        (float)TOOL_SELECTOR_AREA_HEIGHT,
    };
    if (!SDL_RenderRect(app->ren, &right_toolbar_area)) {
        SDL_Log("UI: Failed to draw right toolbar border: %s", SDL_GetError());
    }
    SDL_FRect r_inner_right = {
        right_toolbar_area.x + 1,
        right_toolbar_area.y + 1,
        right_toolbar_area.w - 2,
        right_toolbar_area.h - 2,
    };
    if (r_inner_right.w > 0 && r_inner_right.h > 0) {
        if (!SDL_RenderRect(app->ren, &r_inner_right)) {
            SDL_Log("UI: Failed to draw inner right toolbar border: %s", SDL_GetError());
        }
    }
    SDL_FRect sep_line_right1 = {
        (float)app->window_w - 2 * TOOL_SELECTOR_SIZE - 1,
        (float)start_y,
        2,
        (float)TOOL_SELECTOR_AREA_HEIGHT,
    };
    if (!SDL_RenderFillRect(app->ren, &sep_line_right1)) {
        SDL_Log("UI: Failed to draw right separator 1: %s", SDL_GetError());
    }
    SDL_FRect sep_line_right2 = {
        (float)app->window_w - TOOL_SELECTOR_SIZE - 1,
        (float)start_y,
        2,
        (float)TOOL_SELECTOR_AREA_HEIGHT,
    };
    if (!SDL_RenderFillRect(app->ren, &sep_line_right2)) {
        SDL_Log("UI: Failed to draw right separator 2: %s", SDL_GetError());
    }

    // Active highlights
    if (app_is_straight_line_mode(app)) {
        if (!SDL_SetRenderDrawColor(app->ren, 40, 42, 54, 255)) { // Dracula 'Background' for contrast
            SDL_Log("UI: Failed to set line-mode highlight color: %s", SDL_GetError());
        }
        if (!SDL_RenderRect(app->ren, line_r)) {
            SDL_Log("UI: Failed to draw line-mode highlight: %s", SDL_GetError());
        }
        SDL_FRect r2 = {line_r->x + 1, line_r->y + 1, line_r->w - 2, line_r->h - 2};
        if (!SDL_RenderRect(app->ren, &r2)) {
            SDL_Log("UI: Failed to draw inner line-mode highlight: %s", SDL_GetError());
        }
    }
    if (app->current_tool == TOOL_BRUSH) {
        Uint8 ir = 255 - app->current_color.r;
        Uint8 ig = 255 - app->current_color.g;
        Uint8 ib = 255 - app->current_color.b;
        if (!SDL_SetRenderDrawColor(app->ren, ir, ig, ib, 255)) {
            SDL_Log("UI: Failed to set brush highlight color: %s", SDL_GetError());
        }
        if (!SDL_RenderRect(app->ren, brush_r)) {
            SDL_Log("UI: Failed to draw brush highlight: %s", SDL_GetError());
        }
        SDL_FRect r2 = {brush_r->x + 1, brush_r->y + 1, brush_r->w - 2, brush_r->h - 2};
        if (!SDL_RenderRect(app->ren, &r2)) {
            SDL_Log("UI: Failed to draw inner brush highlight: %s", SDL_GetError());
        }
    }
    if (app->current_tool == TOOL_WATER_MARKER) {
        Uint8 w_ir = 255 - app->water_marker_color.r;
        Uint8 w_ig = 255 - app->water_marker_color.g;
        Uint8 w_ib = 255 - app->water_marker_color.b;
        if (!SDL_SetRenderDrawColor(app->ren, w_ir, w_ig, w_ib, 255)) {
            SDL_Log("UI: Failed to set water-marker highlight color: %s", SDL_GetError());
        }
        if (!SDL_RenderRect(app->ren, water_r)) {
            SDL_Log("UI: Failed to draw water-marker highlight: %s", SDL_GetError());
        }
        SDL_FRect r2 = {water_r->x + 1, water_r->y + 1, water_r->w - 2, water_r->h - 2};
        if (!SDL_RenderRect(app->ren, &r2)) {
            SDL_Log("UI: Failed to draw inner water-marker highlight: %s", SDL_GetError());
        }
    }
    if (app->current_tool == TOOL_BLUR) {
        if (!SDL_SetRenderDrawColor(app->ren, 241, 250, 140, 255)) { // Dracula 'Yellow' for highlight
            SDL_Log("UI: Failed to set blur highlight color: %s", SDL_GetError());
        }
        if (!SDL_RenderRect(app->ren, blur_r)) {
            SDL_Log("UI: Failed to draw blur highlight: %s", SDL_GetError());
        }
        SDL_FRect r2 = {blur_r->x + 1, blur_r->y + 1, blur_r->w - 2, blur_r->h - 2};
        if (!SDL_RenderRect(app->ren, &r2)) {
            SDL_Log("UI: Failed to draw inner blur highlight: %s", SDL_GetError());
        }
    }
    if (app->current_tool == TOOL_EMOJI) {
        if (!SDL_SetRenderDrawColor(app->ren, 189, 147, 249, 255)) { // Dracula 'Purple'
            SDL_Log("UI: Failed to set emoji highlight color: %s", SDL_GetError());
        }
        if (!SDL_RenderRect(app->ren, emoji_r)) {
            SDL_Log("UI: Failed to draw emoji highlight: %s", SDL_GetError());
        }
        SDL_FRect r2 = {emoji_r->x + 1, emoji_r->y + 1, emoji_r->w - 2, emoji_r->h - 2};
        if (!SDL_RenderRect(app->ren, &r2)) {
            SDL_Log("UI: Failed to draw inner emoji highlight: %s", SDL_GetError());
        }
    }
    if (app->show_color_palette) {
        if (!SDL_SetRenderDrawColor(app->ren, 68, 71, 90, 255)) { // Dracula 'Current Line'
            SDL_Log("UI: Failed to set color palette toggle highlight color: %s", SDL_GetError());
        }
        if (!SDL_RenderRect(app->ren, color_r)) {
            SDL_Log("UI: Failed to draw color palette toggle highlight: %s", SDL_GetError());
        }
        SDL_FRect r2 = {color_r->x + 1, color_r->y + 1, color_r->w - 2, color_r->h - 2};
        if (!SDL_RenderRect(app->ren, &r2)) {
            SDL_Log("UI: Failed to draw inner color palette toggle highlight: %s", SDL_GetError());
        }
    }
}

void ui_draw_tool_selectors(App *app, int start_y)
{
    // --- 1. Define Rects for new layout ---
    // Left-side tools
    SDL_FRect brush_toggle_rect = {0, (float)start_y, (float)TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_SIZE};
    SDL_FRect water_marker_toggle_rect = {
        (float)TOOL_SELECTOR_SIZE, (float)start_y, (float)TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_SIZE
    };
    SDL_FRect blur_toggle_rect = {
        (float)2 * TOOL_SELECTOR_SIZE, (float)start_y, (float)TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_SIZE
    };

    // Right-side tools
    SDL_FRect line_toggle_rect = {
        (float)app->window_w - 3 * TOOL_SELECTOR_SIZE, (float)start_y, (float)TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_SIZE
    };
    SDL_FRect emoji_toggle_rect = {
        (float)app->window_w - 2 * TOOL_SELECTOR_SIZE, (float)start_y, (float)TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_SIZE
    };
    SDL_FRect color_toggle_rect = {
        (float)app->window_w - TOOL_SELECTOR_SIZE, (float)start_y, (float)TOOL_SELECTOR_SIZE, (float)TOOL_SELECTOR_SIZE
    };

    draw_backgrounds(app,
                     &brush_toggle_rect,
                     &water_marker_toggle_rect,
                     &blur_toggle_rect,
                     &line_toggle_rect,
                     &emoji_toggle_rect,
                     &color_toggle_rect);
    draw_previews(app,
                  &brush_toggle_rect,
                  &water_marker_toggle_rect,
                  &blur_toggle_rect,
                  &emoji_toggle_rect);
    draw_borders_and_highlights(app,
                                start_y,
                                &brush_toggle_rect,
                                &water_marker_toggle_rect,
                                &blur_toggle_rect,
                                &line_toggle_rect,
                                &emoji_toggle_rect,
                                &color_toggle_rect);
}
