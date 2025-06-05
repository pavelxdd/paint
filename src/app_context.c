// AI Summary: Implements the management of AppContext, the central state for the paint
// application. Handles initialization of state (window, renderer, palette, canvas, active tool),
// modifications (tool/color selection, brush size changes), canvas drawing operations
// (colors or emojis, including coordinate scaling), and clearing the canvas.
#include "app_context.h"
#include "draw.h" // For draw_circle
#include "palette.h" // For PALETTE_ROWS, PALETTE_HEIGHT, NUM_EMOJI_ROWS, NUM_COLOR_ROWS_INC_GRAYSCALE, COLOR_EMOJI_SEPARATOR_HEIGHT
#include <stdlib.h> // For malloc, free
#include <math.h>   // For roundf

static void init_canvas_texture(AppContext *ctx, int width, int height) {
    if (ctx->canvas_texture) {
        SDL_DestroyTexture(ctx->canvas_texture);
    }
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
    // Total height of all cells + height of separator between color/emoji rows
    int palette_internal_content_height = (PALETTE_HEIGHT * PALETTE_ROWS);
    if (NUM_EMOJI_ROWS > 0 && NUM_COLOR_ROWS_INC_GRAYSCALE > 0) { // Separator only if both types of rows exist
        palette_internal_content_height += COLOR_EMOJI_SEPARATOR_HEIGHT;
    }
    
    ctx->canvas_display_area_h = ctx->window_h - palette_internal_content_height - CANVAS_PALETTE_SEPARATOR_HEIGHT;
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
    app_context_update_canvas_display_height(ctx);

    ctx->palette = palette_create(ctx->ren, ctx->window_w);
    if (!ctx->palette) {
        SDL_Log("Failed to create palette in AppContext");
        free(ctx);
        return NULL;
    }

    ctx->background_color = (SDL_Color){255, 255, 255, 255}; // Default white background

    // Default to the last color in the palette (usually black from grayscale row)
    int default_selection_idx = 0;
    if (ctx->palette->total_color_cells > 0) {
        default_selection_idx = ctx->palette->total_color_cells - 1;
    } else if (ctx->palette->total_cells > 0) { // Fallback to first emoji if no colors
        default_selection_idx = ctx->palette->total_color_cells; // First emoji
    }
    // If palette is totally empty (should not happen with valid creation), default_selection_idx is 0.

    ctx->selected_palette_idx = default_selection_idx;
    app_context_select_palette_tool(ctx, ctx->selected_palette_idx);

    ctx->brush_radius = 10; // Default brush radius
    app_context_recalculate_sizes_and_limits(ctx);

    int initial_canvas_h = ctx->canvas_display_area_h; 
    if (initial_canvas_h < 1) {
        initial_canvas_h = 1;
    }
    ctx->canvas_texture = NULL;
    init_canvas_texture(ctx, ctx->window_w, initial_canvas_h);
    if (!ctx->canvas_texture) {
        palette_destroy(ctx->palette);
        free(ctx);
        return NULL;
    }

    ctx->needs_redraw = SDL_TRUE;
    ctx->resize_pending = SDL_FALSE;
    ctx->last_resize_timestamp = 0;

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
            // current_color is not used for TOOL_EMOJI
        } else if (palette_is_color_index(ctx->palette, palette_idx)) {
            ctx->current_tool = TOOL_BRUSH;
            ctx->current_color = palette_get_color(ctx->palette, palette_idx);
        } else {
            // Should not happen if palette_idx is valid and maps to a cell type
            // Default to brush with black if something is wrong
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
    app_context_update_canvas_display_height(ctx);

    if (ctx->palette && ctx->palette->cols > 0) {
        int min_cell_width = ctx->window_w / ctx->palette->cols;
        int min_cell_dim = (min_cell_width < PALETTE_HEIGHT) ? min_cell_width : PALETTE_HEIGHT;
        // For circle brush, radius is half of cell dim
        // For emojis, this radius translates to ~half cell height for preview
        ctx->max_brush_radius = min_cell_dim / 2; 
    } else {
        ctx->max_brush_radius = PALETTE_HEIGHT / 2;
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
    if (mouse_y < ctx->canvas_display_area_h && ctx->canvas_display_area_h > 0 && ctx->canvas_texture) {
        float final_draw_mx = (float)mouse_x;
        float final_draw_my = (float)mouse_y;

        if (ctx->canvas_texture_w != ctx->window_w || ctx->canvas_texture_h != ctx->canvas_display_area_h) {
             if (ctx->window_w > 0 && ctx->canvas_display_area_h > 0) {
                 float scale_x = (float)ctx->canvas_texture_w / ctx->window_w;
                 float scale_y = (float)ctx->canvas_texture_h / ctx->canvas_display_area_h;
                 final_draw_mx = mouse_x * scale_x;
                 final_draw_my = mouse_y * scale_y;
             }
        }

        SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);

        if (use_background_color) { // Erasing always uses background color
            SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
            draw_circle(ctx->ren, roundf(final_draw_mx), roundf(final_draw_my), ctx->brush_radius);
        } else {
            if (app_context_is_drawing_with_emoji(ctx)) {
                SDL_Texture* emoji_tex = NULL;
                int emoji_w = 0, emoji_h = 0;
                if (palette_get_emoji_info(ctx->palette, ctx->selected_palette_idx, &emoji_tex, &emoji_w, &emoji_h) && emoji_tex) {
                    if (emoji_w > 0 && emoji_h > 0) {
                        float aspect_ratio = (float)emoji_w / emoji_h;
                        // brush_radius determines the base size, tripled for canvas drawing
                        int render_h = ctx->brush_radius * 6; 
                        if (render_h < MIN_BRUSH_SIZE * 6) {
                             render_h = MIN_BRUSH_SIZE * 6; // Minimum sensible size (scaled)
                        }

                        int render_w = roundf(render_h * aspect_ratio);
                        if (render_w == 0) {
                            render_w = 1; // Ensure non-zero width
                        }

                        SDL_Rect dst_rect = {
                            roundf(final_draw_mx - render_w / 2.0f),
                            roundf(final_draw_my - render_h / 2.0f),
                            render_w,
                            render_h
                        };
                        SDL_RenderCopy(ctx->ren, emoji_tex, NULL, &dst_rect);
                    }
                }
            } else { // Drawing with color brush
                SDL_SetRenderDrawColor(ctx->ren, ctx->current_color.r, ctx->current_color.g, ctx->current_color.b, 255);
                draw_circle(ctx->ren, roundf(final_draw_mx), roundf(final_draw_my), ctx->brush_radius);
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
