// AI Summary: Initializes SDL and SDL_ttf, creates the main application window and renderer,
// and sets up the AppContext. Runs the main application loop, orchestrating
// event handling, debounced resize processing, and scene rendering (including the tool selectors and palettes).
// Delegates specific logic to app_context, event_handler, and resize_handler modules.
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> // For TTF_Init, TTF_Quit
#include <stdlib.h>       // For EXIT_SUCCESS, EXIT_FAILURE

#include "app_context.h"    // For AppContext, INITIAL_WINDOW_WIDTH, RESIZE_DEBOUNCE_MS, etc.
#include "event_handler.h"  // For handle_events
#include "palette.h"        // For palette_draw (used in render_scene)
#include "resize_handler.h" // For process_debounced_resize
#include "tool_selectors.h" // For drawing the tool toggles

static void render_scene(AppContext *ctx)
{
    SDL_SetRenderDrawColor(ctx->ren, 255, 255, 255, 255);
    SDL_RenderClear(ctx->ren);

    // 1. Render the canvas texture to the entire window first.
    if (ctx->canvas_texture) {
        SDL_RenderCopy(ctx->ren, ctx->canvas_texture, NULL, NULL);
    }

    // 2. If a water-marker stroke is in progress, render it semi-transparently for live preview.
    if (ctx->water_marker_stroke_active && ctx->stroke_buffer) {
        SDL_SetTextureAlphaMod(ctx->stroke_buffer, 128);
        SDL_RenderCopy(ctx->ren, ctx->stroke_buffer, NULL, NULL);
        SDL_SetTextureAlphaMod(ctx->stroke_buffer, 255); // Reset for other potential uses
    }

    // --- UI drawing from top to bottom, overlaid on the canvas ---

    // 3. Tool selectors "float" over the canvas, just above the main UI panel.
    // Their Y position is calculated from the top of the main UI block.
    int tool_selectors_y = ctx->canvas_display_area_h - TOOL_SELECTOR_AREA_HEIGHT;
    tool_selectors_draw(ctx, tool_selectors_y);

    // 4. The main UI block (palette and its separator) starts at canvas_display_area_h.
    int current_y = ctx->canvas_display_area_h;

    // 5. Separator between canvas/selectors and palette (if palette is visible)
    SDL_bool is_palette_content_visible =
        (ctx->show_color_palette && ctx->palette->color_rows > 0) ||
        (ctx->show_emoji_palette && ctx->palette->emoji_rows > 0);
    if (is_palette_content_visible && TOOL_SELECTOR_SEPARATOR_HEIGHT > 0) {
        SDL_SetRenderDrawColor(ctx->ren, 68, 71, 90, 255); // Dracula 'Current Line'
        SDL_Rect sep_rect = {0, current_y, ctx->window_w, TOOL_SELECTOR_SEPARATOR_HEIGHT};
        SDL_RenderFillRect(ctx->ren, &sep_rect);
        current_y += TOOL_SELECTOR_SEPARATOR_HEIGHT;
    }

    // 6. Palette (conditionally visible rows)
    int active_palette_idx = -1;
    switch (ctx->current_tool) {
    case TOOL_BRUSH:
        active_palette_idx = ctx->brush_selected_palette_idx;
        break;
    case TOOL_WATER_MARKER:
        active_palette_idx = ctx->water_marker_selected_palette_idx;
        break;
    case TOOL_EMOJI:
        active_palette_idx = ctx->emoji_selected_palette_idx;
        break;
    }
    palette_draw(ctx->palette,
                 ctx->ren,
                 current_y,
                 ctx->window_w,
                 active_palette_idx,
                 ctx->show_color_palette,
                 ctx->show_emoji_palette);

    SDL_RenderPresent(ctx->ren);
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init error: %s", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Window *win = SDL_CreateWindow("Simple Paint",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       INITIAL_WINDOW_WIDTH,
                                       INITIAL_WINDOW_HEIGHT,
                                       SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Log("CreateWindow error: %s", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        SDL_Log("CreateRenderer error: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Log renderer info
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0) {
        SDL_Log("Active renderer: %s", info.name);
        if (info.flags & SDL_RENDERER_ACCELERATED) {
            SDL_Log("Renderer is accelerated.");
        } else {
            SDL_Log("Warning: Renderer is NOT accelerated. "
                    "Performance may be poor.");
        }
        if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
            SDL_Log("Renderer supports target textures.");
        } else {
            SDL_Log("Error: Renderer does NOT support target textures. "
                    "Application may not work correctly.");
            // This is a critical error, consider exiting if absolutely necessary
        }
    }

    AppContext *app_ctx = app_context_create(win, ren);
    if (!app_ctx) {
        SDL_Log("Failed to create AppContext");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    int running = 1;
    while (running) {
        Uint32 wait_timeout;
        if (app_ctx->needs_redraw) {
            wait_timeout = 16;
        } else if (app_ctx->resize_pending) {
            wait_timeout = RESIZE_DEBOUNCE_MS / 4;
        } else {
            wait_timeout = (Uint32)-1;
        }

        handle_events(app_ctx, &running, wait_timeout);
        process_debounced_resize(app_ctx);

        if (app_ctx->needs_redraw) {
            render_scene(app_ctx);
            app_ctx->needs_redraw = SDL_FALSE;
        }
    }

    app_context_destroy(app_ctx);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
