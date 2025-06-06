// AI Summary: Implements the management of AppContext, the central state for the paint
// application. Handles initialization of state (window, renderer, palette, canvas, active tool),
// modifications (tool/color selection, brush size changes, palette visibility), canvas drawing
// operations (colors or emojis), and clearing the canvas.
#include "app_context.h"
#include "draw.h" // For draw_circle
#include "palette.h" // For PALETTE_HEIGHT, COLOR_EMOJI_SEPARATOR_HEIGHT
#include <stdlib.h> // For malloc, free
#include <math.h>   // For roundf

static void init_canvas_texture(AppContext *ctx, int width, int height) {
    if (ctx->canvas_texture) {
        SDL_DestroyTexture(ctx->canvas_texture);
    }
    // Canvas texture is always the full size of the window.
    ctx->canvas_texture = SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, width, height);
    if (!ctx->canvas_texture) {
        SDL_Log("Failed to create canvas texture: %s", SDL_GetError());
        ctx->canvas_texture_w = 0;
        ctx->canvas_texture_h = 0;
        return;
    }
    ctx->canvas_texture_w = width;
    ctx->canvas_texture_h = height;

    SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
    SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
    SDL_RenderClear(ctx->ren);
    SDL_SetRenderTarget(ctx->ren, NULL);
}

void app_context_update_canvas_display_height(AppContext *ctx) {
    if (!ctx || !ctx->palette) { // Guard against null palette, especially during init
        ctx->canvas_display_area_h = ctx->window_h - (PALETTE_HEIGHT * 2) - TOOL_SELECTOR_SEPARATOR_HEIGHT; // Fallback estimate
        if (ctx->canvas_display_area_h < 0) ctx->canvas_display_area_h = 0;
        return;
    }

    int total_ui_height = 0;
    
    int palette_content_h = 0;
    if (ctx->show_color_palette && ctx->palette->color_rows > 0) {
        palette_content_h += ctx->palette->color_rows * PALETTE_HEIGHT;
    }
    if (ctx->show_emoji_palette && ctx->palette->emoji_rows > 0) {
        palette_content_h += ctx->palette->emoji_rows * PALETTE_HEIGHT;
    }

    if (ctx->show_color_palette && ctx->show_emoji_palette &&
        ctx->palette->color_rows > 0 && ctx->palette->emoji_rows > 0 &&
        COLOR_EMOJI_SEPARATOR_HEIGHT > 0) {
        palette_content_h += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }

    // The main UI block is now just the palette and a separator above it.
    // The tool selectors "float" over the canvas area.
    if (palette_content_h > 0) {
        total_ui_height += TOOL_SELECTOR_SEPARATOR_HEIGHT;
        total_ui_height += palette_content_h;
    }

    ctx->canvas_display_area_h = ctx->window_h - total_ui_height;
    if (ctx->canvas_display_area_h < 0) {
        ctx->canvas_display_area_h = 0;
    }
}

AppContext* app_context_create(SDL_Window *win, SDL_Renderer *ren) {
    AppContext *ctx = (AppContext*)malloc(sizeof(AppContext));
    if (!ctx) {
        SDL_Log("Failed to allocate AppContext");
        return NULL;
    }

    ctx->win = win;
    ctx->ren = ren;
    ctx->window_w = INITIAL_WINDOW_WIDTH;
    ctx->window_h = INITIAL_WINDOW_HEIGHT;
    // Note: app_context_update_canvas_display_height will be called after palette is created.

    // Pass window_h to palette_create for dynamic row calculation
    ctx->palette = palette_create(ctx->ren, ctx->window_w, ctx->window_h);
    if (!ctx->palette) {
        SDL_Log("Failed to create palette in AppContext");
        free(ctx);
        return NULL;
    }

    // Now that palette is created and its rows are set, update canvas display height
    app_context_update_canvas_display_height(ctx);

    ctx->background_color = (SDL_Color){255, 255, 255, 255}; // Default white background

    int default_selection_idx = 0;
    if (ctx->palette->total_color_cells > 0) {
        default_selection_idx = ctx->palette->total_color_cells - 1;
    } else if (ctx->palette->total_cells > 0) { 
        default_selection_idx = ctx->palette->total_color_cells; 
    }

    ctx->selected_palette_idx = default_selection_idx;
    app_context_select_palette_tool(ctx, ctx->selected_palette_idx);

    ctx->brush_radius = 10; 
    app_context_recalculate_sizes_and_limits(ctx); // This also calls app_context_update_canvas_display_height

    // Canvas is now full window size.
    ctx->canvas_texture = NULL;
    init_canvas_texture(ctx, ctx->window_w, ctx->window_h);
    if (!ctx->canvas_texture) {
        palette_destroy(ctx->palette);
        free(ctx);
        return NULL;
    }

    ctx->needs_redraw = SDL_TRUE;
    ctx->resize_pending = SDL_FALSE;
    ctx->last_resize_timestamp = 0;

    ctx->show_color_palette = SDL_TRUE;
    ctx->show_emoji_palette = SDL_TRUE;

    return ctx;
}

void app_context_destroy(AppContext *ctx) {
    if (!ctx) {
        return;
    }
    if (ctx->canvas_texture) {
        SDL_DestroyTexture(ctx->canvas_texture);
    }
    if (ctx->palette) {
        palette_destroy(ctx->palette);
    }
    free(ctx);
}

void app_context_select_palette_tool(AppContext *ctx, int palette_idx) {
    if (palette_idx >= 0 && palette_idx < ctx->palette->total_cells) {
        ctx->selected_palette_idx = palette_idx;
        if (palette_is_emoji_index(ctx->palette, palette_idx)) {
            ctx->current_tool = TOOL_EMOJI;
            // Reset color to a default when emoji tool is picked.
            ctx->current_color = (SDL_Color){40, 42, 54, 255}; // Dracula 'Background'
        } else if (palette_is_color_index(ctx->palette, palette_idx)) {
            ctx->current_tool = TOOL_BRUSH;
            ctx->current_color = palette_get_color(ctx->palette, palette_idx);
        } else {
            // Fallback case (e.g., empty palette)
            ctx->current_tool = TOOL_BRUSH;
            ctx->current_color = (SDL_Color){0, 0, 0, 255};
        }
        ctx->needs_redraw = SDL_TRUE;
    }
}

void app_context_set_background_and_clear_canvas(AppContext *ctx, SDL_Color color) {
    ctx->background_color = color;
    app_context_clear_canvas_with_current_bg(ctx);
}

void app_context_toggle_color_palette(AppContext *ctx) {
    ctx->show_color_palette = !ctx->show_color_palette;
    app_context_update_canvas_display_height(ctx);
    // Do not refresh texture, just redraw scene with new UI layout
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_toggle_emoji_palette(AppContext *ctx) {
    ctx->show_emoji_palette = !ctx->show_emoji_palette;
    app_context_update_canvas_display_height(ctx);
    // Do not refresh texture, just redraw scene with new UI layout
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_clear_canvas_with_current_bg(AppContext *ctx) {
    if (!ctx->canvas_texture) {
        return;
    }
    SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
    SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
    SDL_RenderClear(ctx->ren);
    SDL_SetRenderTarget(ctx->ren, NULL);
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_change_brush_radius(AppContext *ctx, int delta) {
    ctx->brush_radius += delta;
    if (ctx->brush_radius < MIN_BRUSH_SIZE) {
        ctx->brush_radius = MIN_BRUSH_SIZE;
    }
    if (ctx->brush_radius > ctx->max_brush_radius) {
        ctx->brush_radius = ctx->max_brush_radius;
    }
    ctx->needs_redraw = SDL_TRUE;
}

void app_context_set_brush_radius_from_key(AppContext *ctx, SDL_Keycode keycode) {
    if (keycode >= SDLK_1 && keycode <= SDLK_9) {
        ctx->brush_radius = 5 * (keycode - SDLK_1 + 1);
        if (ctx->brush_radius < MIN_BRUSH_SIZE) {
            ctx->brush_radius = MIN_BRUSH_SIZE;
        }
        if (ctx->brush_radius > ctx->max_brush_radius) {
            ctx->brush_radius = ctx->max_brush_radius;
        }
        ctx->needs_redraw = SDL_TRUE;
    }
}

void app_context_recalculate_sizes_and_limits(AppContext *ctx) {
    app_context_update_canvas_display_height(ctx); // Ensure canvas height is current

    if (ctx->palette && ctx->palette->cols > 0) {
        int min_cell_width = ctx->window_w / ctx->palette->cols;
        int min_cell_dim = (min_cell_width < PALETTE_HEIGHT) ? min_cell_width : PALETTE_HEIGHT;
        ctx->max_brush_radius = min_cell_dim / 2; 
    } else {
        ctx->max_brush_radius = PALETTE_HEIGHT / 2; // Fallback
    }

    if (ctx->max_brush_radius < MIN_BRUSH_SIZE) {
        ctx->max_brush_radius = MIN_BRUSH_SIZE;
    }

    if (ctx->brush_radius > ctx->max_brush_radius) {
        ctx->brush_radius = ctx->max_brush_radius;
    }
    if (ctx->brush_radius < MIN_BRUSH_SIZE) {
        ctx->brush_radius = MIN_BRUSH_SIZE;
    }
}

SDL_bool app_context_is_drawing_with_emoji(AppContext* ctx) {
    return ctx->current_tool == TOOL_EMOJI;
}

void app_context_draw_stroke(AppContext *ctx, int mouse_x, int mouse_y, SDL_bool use_background_color) {
    // Check if the draw coordinates are within the visible canvas area.
    if (mouse_y < ctx->canvas_display_area_h && ctx->canvas_display_area_h > 0 && ctx->canvas_texture) {
        // No scaling needed as canvas texture is 1:1 with window.
        SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);

        if (use_background_color) {
            SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
            draw_circle(ctx->ren, mouse_x, mouse_y, ctx->brush_radius);
        } else {
            if (app_context_is_drawing_with_emoji(ctx)) {
                SDL_Texture* emoji_tex = NULL;
                int emoji_w = 0, emoji_h = 0;
                if (palette_get_emoji_info(ctx->palette, ctx->selected_palette_idx, &emoji_tex, &emoji_w, &emoji_h) && emoji_tex) {
                    if (emoji_w > 0 && emoji_h > 0) {
                        float aspect_ratio = (float)emoji_w / emoji_h;
                        int render_h = ctx->brush_radius * 6;
                        if (render_h < MIN_BRUSH_SIZE * 6) {
                             render_h = MIN_BRUSH_SIZE * 6;
                        }

                        int render_w = roundf(render_h * aspect_ratio);
                        if (render_w == 0) {
                            render_w = 1;
                        }

                        SDL_Rect dst_rect = {
                            (int)roundf(mouse_x - render_w / 2.0f),
                            (int)roundf(mouse_y - render_h / 2.0f),
                            render_w,
                            render_h
                        };
                        SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst_rect);
                    }
                }
            } else {
                SDL_SetRenderDrawColor(ctx->ren, ctx->current_color.r, ctx->current_color.g, ctx->current_color.b, 255);
                draw_circle(ctx->ren, mouse_x, mouse_y, ctx->brush_radius);
            }
        }
        SDL_SetRenderTarget(ctx->ren, NULL);
        ctx->needs_redraw = SDL_TRUE;
    }
}

void app_context_notify_resize_event(AppContext *ctx, int new_w, int new_h) {
    ctx->window_w = (new_w < 1) ? 1 : new_w;
    ctx->window_h = (new_h < 1) ? 1 : new_h;

    ctx->last_resize_timestamp = SDL_GetTicks();
    ctx->resize_pending = SDL_TRUE;
    ctx->needs_redraw = SDL_TRUE;
}

// This function is only called after a debounced window resize.
// It creates a new canvas texture of the new window size and preserves the old content.
void app_context_recreate_canvas_texture(AppContext *ctx) {
    int target_texture_w = ctx->window_w;
    int target_texture_h = ctx->window_h;

    SDL_Texture *new_canvas_texture = SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888,
                                                        SDL_TEXTUREACCESS_TARGET, target_texture_w, target_texture_h);
    if (new_canvas_texture) {
        SDL_SetRenderTarget(ctx->ren, new_canvas_texture);
        SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
        SDL_RenderClear(ctx->ren);

        if (ctx->canvas_texture) {
            SDL_Rect src_rect = {0, 0, ctx->canvas_texture_w, ctx->canvas_texture_h};
            SDL_Rect dst_rect_on_new_tex = {0, 0, ctx->canvas_texture_w, ctx->canvas_texture_h};

            // Clamp copy area to the smaller of the two textures
            if (dst_rect_on_new_tex.w > target_texture_w) dst_rect_on_new_tex.w = target_texture_w;
            if (dst_rect_on_new_tex.h > target_texture_h) dst_rect_on_new_tex.h = target_texture_h;
            if (src_rect.w > target_texture_w) src_rect.w = target_texture_w;
            if (src_rect.h > target_texture_h) src_rect.h = target_texture_h;

            if (src_rect.w > 0 && src_rect.h > 0) {
               SDL_RenderCopy(ctx->ren, ctx->canvas_texture, &src_rect, &dst_rect_on_new_tex);
            }
        }

        SDL_SetRenderTarget(ctx->ren, NULL);
        if (ctx->canvas_texture) {
            SDL_DestroyTexture(ctx->canvas_texture);
        }
        ctx->canvas_texture = new_canvas_texture;
        ctx->canvas_texture_w = target_texture_w;
        ctx->canvas_texture_h = target_texture_h;
    } else {
        SDL_Log("Failed to create new canvas texture during resize: %s", SDL_GetError());
    }
}
