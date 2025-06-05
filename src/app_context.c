// AI Summary: Implements the management of AppContext, the central state for the paint
// application. Handles initialization of state (window, renderer, palette, canvas),
// modifications (color selection, brush size changes), canvas drawing operations
// (including coordinate scaling), and clearing the canvas.
#include "app_context.h"
#include "draw.h" // For draw_circle
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
        // Consider how to handle this error; perhaps set a flag in AppContext
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
    ctx->canvas_display_area_h = ctx->window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
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


    ctx->palette = palette_create(ctx->window_w);
    if (!ctx->palette) {
        SDL_Log("Failed to create palette in AppContext");
        free(ctx);
        return NULL;
    }

    ctx->background_color = (SDL_Color){255, 255, 255, 255}; // Default white background
    ctx->selected_palette_idx = ctx->palette->total - 1;     // Default black color
    ctx->current_color = palette_get_color(ctx->palette, ctx->selected_palette_idx);
    
    ctx->brush_radius = 10; // Default brush radius
    // max_brush_radius will be set by app_context_recalculate_sizes_and_limits

    app_context_recalculate_sizes_and_limits(ctx); // Calculates max_brush_radius and clamps brush_radius

    int initial_canvas_h = ctx->window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
    if (initial_canvas_h < 1) initial_canvas_h = 1;
    ctx->canvas_texture = NULL; // init_canvas_texture handles existing texture
    init_canvas_texture(ctx, ctx->window_w, initial_canvas_h);
    if (!ctx->canvas_texture) { // Check if texture creation failed
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
    if (!ctx) return;
    if (ctx->canvas_texture) {
        SDL_DestroyTexture(ctx->canvas_texture);
    }
    if (ctx->palette) {
        palette_destroy(ctx->palette);
    }
    free(ctx);
}

void app_context_select_palette_color(AppContext *ctx, int palette_idx) {
    if (palette_idx >= 0 && palette_idx < ctx->palette->total) {
        ctx->selected_palette_idx = palette_idx;
        ctx->current_color = palette_get_color(ctx->palette, palette_idx);
        ctx->needs_redraw = SDL_TRUE;
    }
}

void app_context_set_background_and_clear_canvas(AppContext *ctx, SDL_Color color) {
    ctx->background_color = color;
    app_context_clear_canvas_with_current_bg(ctx); // This will also set needs_redraw
}

void app_context_clear_canvas_with_current_bg(AppContext *ctx) {
    if (!ctx->canvas_texture) return;
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

    if (ctx->palette->cols > 0) { // Ensure palette->cols is positive
        int min_cell_width = ctx->window_w / ctx->palette->cols;
        int min_cell_dim = (min_cell_width < PALETTE_HEIGHT) ? min_cell_width : PALETTE_HEIGHT;
        ctx->max_brush_radius = min_cell_dim / 2;
    } else { // Fallback if palette has no columns (e.g. window_w too small)
        ctx->max_brush_radius = PALETTE_HEIGHT / 2;
    }

    if (ctx->max_brush_radius < MIN_BRUSH_SIZE) {
        ctx->max_brush_radius = MIN_BRUSH_SIZE;
    }
    
    // Clamp current brush_radius to the new limits
    if (ctx->brush_radius > ctx->max_brush_radius) {
        ctx->brush_radius = ctx->max_brush_radius;
    }
    if (ctx->brush_radius < MIN_BRUSH_SIZE) {
        ctx->brush_radius = MIN_BRUSH_SIZE;
    }
}

void app_context_draw_stroke(AppContext *ctx, int mouse_x, int mouse_y, SDL_bool use_background_color) {
    // canvas_display_area_h is the logical height for drawing.
    // Check if cursor is within this logical canvas display area AND if this area has positive height.
    if (mouse_y < ctx->canvas_display_area_h && ctx->canvas_display_area_h > 0 && ctx->canvas_texture) {
        float final_draw_mx = (float)mouse_x;
        float final_draw_my = (float)mouse_y;

        // If canvas texture dimensions differ from its display dimensions in the window,
        // mouse coordinates need to be scaled to the texture's coordinate space.
        // ctx->window_w is canvas_display_w, assumed >= 1 from event handler or resize.
        // ctx->canvas_display_area_h is checked > 0 above.
        if (ctx->canvas_texture_w != ctx->window_w || ctx->canvas_texture_h != ctx->canvas_display_area_h) {
             if (ctx->window_w > 0 && ctx->canvas_display_area_h > 0) { // Avoid division by zero
                 float scale_x = (float)ctx->canvas_texture_w / ctx->window_w;
                 float scale_y = (float)ctx->canvas_texture_h / ctx->canvas_display_area_h;
                 final_draw_mx = mouse_x * scale_x;
                 final_draw_my = mouse_y * scale_y;
             }
        }

        SDL_SetRenderTarget(ctx->ren, ctx->canvas_texture);
        if (use_background_color) {
            SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
        } else {
            SDL_SetRenderDrawColor(ctx->ren, ctx->current_color.r, ctx->current_color.g, ctx->current_color.b, 255);
        }
        draw_circle(ctx->ren, roundf(final_draw_mx), roundf(final_draw_my), ctx->brush_radius);
        SDL_SetRenderTarget(ctx->ren, NULL);
        ctx->needs_redraw = SDL_TRUE;
    }
}

void app_context_notify_resize_event(AppContext *ctx, int new_w, int new_h) {
    ctx->window_w = (new_w < 1) ? 1 : new_w;
    ctx->window_h = (new_h < 1) ? 1 : new_h;
    // canvas_display_area_h will be updated by recalculate_sizes_and_limits in resize_handler
    // or if called directly: app_context_update_canvas_display_height(ctx);

    ctx->last_resize_timestamp = SDL_GetTicks();
    ctx->resize_pending = SDL_TRUE;
    ctx->needs_redraw = SDL_TRUE; // Redraw to show intermediate state or final resized state
}
