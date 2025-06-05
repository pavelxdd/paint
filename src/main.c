// AI Summary: Main application logic for a simple paint program using SDL2.
// Handles window creation, rendering, event processing (mouse, keyboard, window),
// canvas management, palette interaction, brush controls, and dynamic background color.
// Core components: SDL_Window, SDL_Renderer, SDL_Texture (for canvas), main event loop.
// Interacts with draw.c for shape drawing and palette.c for color management.
#include <SDL2/SDL.h>
#include <stdlib.h>

#include "draw.h"
#include "palette.h"

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600
#define MIN_BRUSH_SIZE 2

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

    int max_brush_radius;

    Palette *palette = palette_create(window_w);
    if (!palette) {
        SDL_Log("Failed to create palette");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // Default brush color: black (from palette), select bottom-right square at startup.
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

    int canvas_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
    SDL_Texture *canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET, window_w, canvas_h);
    SDL_SetRenderTarget(ren, canvas);
    SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
    SDL_RenderClear(ren);
    SDL_SetRenderTarget(ren, NULL);

    int running = 1;
    int needs_redraw = 1;
    SDL_Event e;
    while (running) {
        if (SDL_WaitEventTimeout(&e, needs_redraw ? 16 : -1)) {
            do {
                if (e.type == SDL_QUIT) {
                    running = 0;
                } else if (e.type == SDL_WINDOWEVENT &&
                           e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    int old_w = window_w;
                    int old_h = canvas_h;
                    window_w = e.window.data1;
                    window_h = e.window.data2;
                    canvas_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);

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

                    SDL_Texture *new_canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                                                SDL_TEXTUREACCESS_TARGET, window_w, canvas_h);
                    SDL_SetRenderTarget(ren, new_canvas);
                    SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                    SDL_RenderClear(ren);
                    SDL_Rect src = {0,0, old_w, old_h};
                    SDL_RenderCopy(ren, canvas, &src, &src);
                    SDL_SetRenderTarget(ren, NULL);
                    SDL_DestroyTexture(canvas);
                    canvas = new_canvas;

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
                    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE) {
                        int mx = e.button.x;
                        int my = e.button.y;
                        int palette_idx = palette_hit_test(palette, mx, my, window_w, canvas_h);
                        if (palette_idx != -1) { // Middle-clicked on a palette color
                            default_bg_color = palette_get_color(palette, palette_idx);
                            SDL_SetRenderTarget(ren, canvas);
                            SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                            SDL_RenderClear(ren);
                            SDL_SetRenderTarget(ren, NULL);
                            needs_redraw = 1;
                        } else if (my < canvas_h) { // Middle-clicked on canvas (not palette)
                            SDL_SetRenderTarget(ren, canvas);
                            SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                            SDL_RenderClear(ren);
                            SDL_SetRenderTarget(ren, NULL);
                            needs_redraw = 1;
                        }
                    } else {
                        // Handle LMB/RMB down/drag, and LMB palette selection on down.
                        // Excludes MMB down events which are handled above.
                        int mx, my;
                        if (e.type == SDL_MOUSEBUTTONDOWN) {
                            mx = e.button.x;
                            my = e.button.y;

                            // Palette selection on LMB down
                            if (e.button.button == SDL_BUTTON_LEFT) {
                                int idx = palette_hit_test(palette, mx, my, window_w, canvas_h);
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

                        // Drawing logic if LMB or RMB is pressed (works for both DOWN and MOTION)
                        int left_button_pressed = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
                        int right_button_pressed = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);

                        if (left_button_pressed || right_button_pressed) {
                            if (my < canvas_h) { // If on canvas
                                SDL_SetRenderTarget(ren, canvas);
                                if (left_button_pressed) {
                                    SDL_SetRenderDrawColor(ren, current_color.r, current_color.g, current_color.b, 255);
                                } else { // right_button_pressed (eraser)
                                    SDL_SetRenderDrawColor(ren, default_bg_color.r, default_bg_color.g, default_bg_color.b, default_bg_color.a);
                                }
                                draw_circle(ren, mx, my, radius);
                                SDL_SetRenderTarget(ren, NULL);
                                needs_redraw = 1;
                            }
                        }
                    }
                }
            } while (SDL_PollEvent(&e));
        } else {
            // timeout without events
        }

        if (needs_redraw) {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            SDL_RenderClear(ren);

            SDL_Rect dst = {0, 0, window_w, canvas_h};
            SDL_RenderCopy(ren, canvas, NULL, &dst);

            palette_draw(palette, ren, canvas_h, window_w, selected_palette_idx, radius);

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
