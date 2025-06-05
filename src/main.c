// AI Summary: Main application logic for a simple paint program using SDL2.
// Handles window creation, rendering, event processing (mouse, keyboard, window),
// canvas management, palette interaction, brush controls, and dynamic background color.
// Implements debounced window resizing for improved performance. Corrects mouse
// coordinate scaling during resize operations to ensure accurate drawing.
// Core components: SDL_Window, SDL_Renderer, SDL_Texture (for canvas), main event loop.
// Interacts with draw.c for shape drawing and palette.c for color management.
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h> // For roundf

#include "draw.h"
#include "palette.h"

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600
#define MIN_BRUSH_SIZE 2
#define RESIZE_DEBOUNCE_MS 200 // Milliseconds to wait after last resize event before processing

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return 1;
    }

    int window_w = INITIAL_WINDOW_WIDTH;
    int window_h = INITIAL_WINDOW_HEIGHT;

    SDL_Window *win = SDL_CreateWindow("Simple Paint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       window_w, window_h, SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Log("CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        SDL_Log("CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // Log renderer info
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0) {
        SDL_Log("Active renderer: %s", info.name);
        if (info.flags & SDL_RENDERER_ACCELERATED) {
            SDL_Log("Renderer is accelerated.");
        } else {
            SDL_Log("Warning: Renderer is NOT accelerated. Performance may be poor.");
        }
        if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
            SDL_Log("Renderer supports target textures.");
        } else {
            SDL_Log("Error: Renderer does NOT support target textures. Application may not work correctly.");
        }
    }


    Palette *palette = palette_create(window_w);
    if (!palette) {
        SDL_Log("Failed to create palette");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    int max_brush_radius;
    // Calculate initial max_brush_radius based on initial palette
    {
        int min_cell_width = window_w / palette->cols;
        int min_cell_dim = (min_cell_width < PALETTE_HEIGHT) ? min_cell_width : PALETTE_HEIGHT;
        max_brush_radius = min_cell_dim / 2;
        if (max_brush_radius < MIN_BRUSH_SIZE) max_brush_radius = MIN_BRUSH_SIZE;
    }

    int radius = 10;
    if (radius > max_brush_radius) radius = max_brush_radius;
    if (radius < MIN_BRUSH_SIZE) radius = MIN_BRUSH_SIZE;

    SDL_Color default_bg_color = {255, 255, 255, 255}; // Default background: white
    int selected_palette_idx = palette->total - 1; // bottom-right (last) palette cell: black
    SDL_Color current_color = palette_get_color(palette, selected_palette_idx);

    int initial_canvas_texture_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
    if (initial_canvas_texture_h < 1) initial_canvas_texture_h = 1; // Ensure positive height

    SDL_Texture *canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, window_w, initial_canvas_texture_h);
    if (!canvas) {
        SDL_Log("Failed to create initial canvas texture: %s", SDL_GetError());
        palette_destroy(palette);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    int current_canvas_texture_w = window_w;
    int current_canvas_texture_h = initial_canvas_texture_h;

    SDL_SetRenderTarget(ren, canvas);
    SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
    SDL_RenderClear(ren);
    SDL_SetRenderTarget(ren, NULL);

    int running = 1;
    int needs_redraw = 1;
    SDL_Event e;

    // Resize debouncing state
    SDL_bool resize_is_pending = SDL_FALSE;
    Uint32 last_resize_event_timestamp = 0;

    while (running) {
        Uint32 wait_timeout = needs_redraw ? 16 : (resize_is_pending ? (RESIZE_DEBOUNCE_MS / 4) : (Uint32)-1);
        if (SDL_WaitEventTimeout(&e, wait_timeout)) {
            do {
                if (e.type == SDL_QUIT) {
                    running = 0;
                } else if (e.type == SDL_WINDOWEVENT &&
                           e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window_w = e.window.data1;
                    window_h = e.window.data2;
                    if (window_w < 1) window_w = 1; // Ensure logical window size is positive
                    if (window_h < 1) window_h = 1;

                    last_resize_event_timestamp = SDL_GetTicks();
                    resize_is_pending = SDL_TRUE;
                    needs_redraw = 1;
                } else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9) {
                        radius = 5 * (e.key.keysym.sym - SDLK_1 + 1);
                        if (radius > max_brush_radius) radius = max_brush_radius;
                        if (radius < MIN_BRUSH_SIZE) radius = MIN_BRUSH_SIZE;
                        needs_redraw = 1;
                    }
                } else if (e.type == SDL_MOUSEWHEEL) {
                    if (e.wheel.y > 0) {
                        radius += 2;
                        if (radius > max_brush_radius) radius = max_brush_radius;
                    } else if (e.wheel.y < 0) {
                        radius -= 2;
                        if (radius < MIN_BRUSH_SIZE) radius = MIN_BRUSH_SIZE;
                    }
                    needs_redraw = 1;
                } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION) {
                    int canvas_display_area_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
                    // canvas_display_area_h can be <=0 if window is too small

                    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE) {
                        int mx = e.button.x;
                        int my = e.button.y;
                        // Use canvas_display_area_h (can be 0 or negative) for palette_hit_test and MMB canvas check.
                        // palette_hit_test handles negative canvas_h correctly.
                        int current_canvas_render_area_h_for_palette = (canvas_display_area_h < 0) ? 0 : canvas_display_area_h;

                        int palette_idx = palette_hit_test(palette, mx, my, window_w, current_canvas_render_area_h_for_palette);
                        if (palette_idx != -1) { // Middle-clicked on a palette color
                            default_bg_color = palette_get_color(palette, palette_idx);
                            SDL_SetRenderTarget(ren, canvas);
                            SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                            SDL_RenderClear(ren);
                            SDL_SetRenderTarget(ren, NULL);
                            needs_redraw = 1;
                        } else if (my < canvas_display_area_h) { // Middle-clicked on canvas (not palette)
                            SDL_SetRenderTarget(ren, canvas);
                            SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                            SDL_RenderClear(ren);
                            SDL_SetRenderTarget(ren, NULL);
                            needs_redraw = 1;
                        }
                    } else {
                        int mx, my;
                        if (e.type == SDL_MOUSEBUTTONDOWN) {
                            mx = e.button.x;
                            my = e.button.y;
                            if (e.button.button == SDL_BUTTON_LEFT) {
                                int current_canvas_render_area_h_for_palette = (canvas_display_area_h < 0) ? 0 : canvas_display_area_h;
                                int idx = palette_hit_test(palette, mx, my, window_w, current_canvas_render_area_h_for_palette);
                                if (idx != -1) {
                                    selected_palette_idx = idx;
                                    current_color = palette_get_color(palette, idx);
                                    needs_redraw = 1;
                                }
                            }
                        } else { // SDL_MOUSEMOTION
                            mx = e.motion.x;
                            my = e.motion.y;
                        }

                        int left_button_pressed = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
                        int right_button_pressed = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);

                        if (left_button_pressed || right_button_pressed) {
                            // canvas_display_area_h is the logical height for drawing.
                            // Check if cursor is within this logical canvas display area AND if this area has positive height.
                            if (my < canvas_display_area_h && canvas_display_area_h > 0) {
                                float final_draw_mx = (float)mx;
                                float final_draw_my = (float)my;

                                // If canvas texture dimensions differ from its display dimensions in the window,
                                // mouse coordinates need to be scaled to the texture's coordinate space.
                                // window_w is canvas_display_w, already clamped >= 1.
                                // canvas_display_area_h is checked > 0 above.
                                if (current_canvas_texture_w != window_w || current_canvas_texture_h != canvas_display_area_h) {
                                     float scale_x = (float)current_canvas_texture_w / window_w;
                                     float scale_y = (float)current_canvas_texture_h / canvas_display_area_h;
                                     final_draw_mx = mx * scale_x;
                                     final_draw_my = my * scale_y;
                                }

                                SDL_SetRenderTarget(ren, canvas);
                                if (left_button_pressed) {
                                    SDL_SetRenderDrawColor(ren, current_color.r, current_color.g, current_color.b, 255);
                                } else { // right_button_pressed (eraser)
                                    SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                                }
                                draw_circle(ren, roundf(final_draw_mx), roundf(final_draw_my), radius);
                                SDL_SetRenderTarget(ren, NULL);
                                needs_redraw = 1;
                            }
                        }
                    }
                }
            } while (SDL_PollEvent(&e));
        } else {
            // Timeout without events
        }

        // Process pending resize if debounce time has passed
        if (resize_is_pending && (SDL_GetTicks() - last_resize_event_timestamp >= RESIZE_DEBOUNCE_MS)) {
            int target_canvas_texture_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
            if (target_canvas_texture_h < 1) target_canvas_texture_h = 1;

            palette_recreate(palette, window_w);
            selected_palette_idx = palette->total - 1; // Reset selection to bottom-right (black) on resize
            current_color = palette_get_color(palette, selected_palette_idx);

            {
                int min_cell_width = window_w / palette->cols;
                int min_cell_dim = (min_cell_width < PALETTE_HEIGHT) ? min_cell_width : PALETTE_HEIGHT;
                max_brush_radius = min_cell_dim / 2;
                if (max_brush_radius < MIN_BRUSH_SIZE) max_brush_radius = MIN_BRUSH_SIZE;
            }
            if (radius > max_brush_radius) radius = max_brush_radius;


            int target_texture_w = window_w; // window_w already clamped >= 1
            // target_canvas_texture_h already clamped >= 1

            SDL_Texture *new_canvas_texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                                                SDL_TEXTUREACCESS_TARGET, target_texture_w, target_canvas_texture_h);
            if (new_canvas_texture) {
                SDL_SetRenderTarget(ren, new_canvas_texture);
                SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                SDL_RenderClear(ren);

                SDL_Rect src_rect = {0, 0, current_canvas_texture_w, current_canvas_texture_h};
                SDL_Rect dst_rect_on_new_tex = {0, 0, current_canvas_texture_w, current_canvas_texture_h};
                // Clip destination rect to new texture bounds if old texture was larger
                if(dst_rect_on_new_tex.w > target_texture_w) dst_rect_on_new_tex.w = target_texture_w;
                if(dst_rect_on_new_tex.h > target_canvas_texture_h) dst_rect_on_new_tex.h = target_canvas_texture_h;
                // Clip source rect if old texture was smaller (though typically we copy all of it)
                // No, src_rect is relative to old texture, dst_rect is relative to new. This is fine.

                SDL_RenderCopy(ren, canvas, &src_rect, &dst_rect_on_new_tex);

                SDL_SetRenderTarget(ren, NULL);
                SDL_DestroyTexture(canvas);
                canvas = new_canvas_texture;
                current_canvas_texture_w = target_texture_w;
                current_canvas_texture_h = target_canvas_texture_h;
            } else {
                SDL_Log("Failed to create new canvas texture during debounced resize: %s", SDL_GetError());
            }

            resize_is_pending = SDL_FALSE;
            needs_redraw = 1;
        }


        if (needs_redraw) {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            SDL_RenderClear(ren);

            int canvas_render_height_in_window = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
            if (canvas_render_height_in_window < 0) canvas_render_height_in_window = 0;

            SDL_Rect canvas_dst_rect_in_window = {0, 0, window_w, canvas_render_height_in_window};
            SDL_RenderCopy(ren, canvas, NULL, &canvas_dst_rect_in_window);

            palette_draw(palette, ren, canvas_render_height_in_window, window_w, selected_palette_idx, radius);

            SDL_RenderPresent(ren);
            needs_redraw = 0;
        }
    }

    palette_destroy(palette);
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
