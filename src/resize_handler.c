// AI Summary: Implements the debounced window resize handling for the paint application.
// When a resize is pending and the debounce time has passed, it recreates the
// color palette, recalculates brush size limits, and creates a new canvas texture
// of the appropriate size, copying over the content from the old canvas.
#include "resize_handler.h"
#include "app_context.h" // For AppContext, RESIZE_DEBOUNCE_MS, PALETTE_ROWS, PALETTE_HEIGHT
#include "palette.h"     // For palette_recreate, palette_get_color

void process_debounced_resize(AppContext *ctx) {
    if (ctx->resize_pending && (SDL_GetTicks() - ctx->last_resize_timestamp >= RESIZE_DEBOUNCE_MS)) {
        // Update canvas display height based on new window height BEFORE recalculating palette/canvas texture
        app_context_update_canvas_display_height(ctx);

        int target_canvas_texture_h = ctx->canvas_display_area_h; // Use the calculated display height
        if (target_canvas_texture_h < 1) target_canvas_texture_h = 1;

        palette_recreate(ctx->palette, ctx->window_w);
        // Reset selection to bottom-right (black) on resize, or maintain current if possible
        // Current behavior: reset to black
        app_context_select_palette_color(ctx, ctx->palette->total - 1); 

        app_context_recalculate_sizes_and_limits(ctx); // Recalculates max_brush_radius and clamps brush_radius

        int target_texture_w = ctx->window_w; // window_w already clamped >= 1 by app_context_notify_resize_event

        SDL_Texture *new_canvas_texture = SDL_CreateTexture(ctx->ren, SDL_PIXELFORMAT_RGBA8888,
                                                            SDL_TEXTUREACCESS_TARGET, target_texture_w, target_canvas_texture_h);
        if (new_canvas_texture) {
            SDL_SetRenderTarget(ctx->ren, new_canvas_texture);
            SDL_SetRenderDrawColor(ctx->ren, ctx->background_color.r, ctx->background_color.g, ctx->background_color.b, ctx->background_color.a);
            SDL_RenderClear(ctx->ren);

            if (ctx->canvas_texture) { // Only copy if old texture exists
                SDL_Rect src_rect = {0, 0, ctx->canvas_texture_w, ctx->canvas_texture_h};
                SDL_Rect dst_rect_on_new_tex = {0, 0, ctx->canvas_texture_w, ctx->canvas_texture_h};
                
                // Clip destination rect to new texture bounds if old texture was larger
                if(dst_rect_on_new_tex.w > target_texture_w) dst_rect_on_new_tex.w = target_texture_w;
                if(dst_rect_on_new_tex.h > target_canvas_texture_h) dst_rect_on_new_tex.h = target_canvas_texture_h;
                
                // Ensure source rect is also clipped if it's larger than what can be drawn on dest
                if(src_rect.w > dst_rect_on_new_tex.w) src_rect.w = dst_rect_on_new_tex.w;
                if(src_rect.h > dst_rect_on_new_tex.h) src_rect.h = dst_rect_on_new_tex.h;

                if (src_rect.w > 0 && src_rect.h > 0) { // Only copy if there's something to copy
                   SDL_RenderCopy(ctx->ren, ctx->canvas_texture, &src_rect, &dst_rect_on_new_tex);
                }
            }

            SDL_SetRenderTarget(ctx->ren, NULL);
            if (ctx->canvas_texture) {
                SDL_DestroyTexture(ctx->canvas_texture);
            }
            ctx->canvas_texture = new_canvas_texture;
            ctx->canvas_texture_w = target_texture_w;
            ctx->canvas_texture_h = target_canvas_texture_h;
        } else {
            SDL_Log("Failed to create new canvas texture during debounced resize: %s", SDL_GetError());
            // Old canvas texture is still intact if new one failed.
        }

        ctx->resize_pending = SDL_FALSE;
        ctx->needs_redraw = SDL_TRUE;
    }
}
