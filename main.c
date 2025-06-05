#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>

#include <math.h>

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600
#define PALETTE_HEIGHT 50
#define PALETTE_CELL_SIZE 50
#define MIN_BRUSH_SIZE 2
#define MAX_BRUSH_SIZE 100

static void draw_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    for (int w = -radius; w <= radius; ++w) {
        for (int h = -radius; h <= radius; ++h) {
            if (w*w + h*h <= radius*radius) {
                SDL_RenderDrawPoint(r, cx + w, cy + h);
            }
        }
    }
}

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

    srand((unsigned)time(NULL));

    int palette_cols = window_w / PALETTE_CELL_SIZE;
    SDL_Color *palette = malloc(sizeof(SDL_Color) * palette_cols);
    for (int i = 0; i < palette_cols; ++i) {
        int unique = 0;
        while (!unique) {
            palette[i].r = rand() % 256;
            palette[i].g = rand() % 256;
            palette[i].b = rand() % 256;
            palette[i].a = 255;
            unique = 1;
            for (int j = 0; j < i; ++j) {
                if (palette[i].r == palette[j].r &&
                    palette[i].g == palette[j].g &&
                    palette[i].b == palette[j].b) {
                    unique = 0;
                    break;
                }
            }
        }
    }
    SDL_Color current_color = palette[0];
    int radius = 10;

    int canvas_w = window_w;
    int canvas_h = window_h - PALETTE_HEIGHT;
    SDL_Texture *canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET, canvas_w, canvas_h);
    SDL_SetRenderTarget(ren, canvas);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
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
                int old_w = canvas_w;
                int old_h = canvas_h;
                window_w = e.window.data1;
                window_h = e.window.data2;
                canvas_w = window_w;
                canvas_h = window_h - PALETTE_HEIGHT;

                SDL_Texture *new_canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                                           SDL_TEXTUREACCESS_TARGET, canvas_w, canvas_h);
                SDL_SetRenderTarget(ren, new_canvas);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderClear(ren);
                SDL_Rect src = {0,0, old_w, old_h};
                SDL_RenderCopy(ren, canvas, &src, &src);
                SDL_SetRenderTarget(ren, NULL);
                SDL_DestroyTexture(canvas);
                canvas = new_canvas;

                palette_cols = window_w / PALETTE_CELL_SIZE;
                palette = realloc(palette, sizeof(SDL_Color) * palette_cols);
                for (int i = 0; i < palette_cols; ++i) {
                    int unique = 0;
                    while (!unique) {
                        palette[i].r = rand() % 256;
                        palette[i].g = rand() % 256;
                        palette[i].b = rand() % 256;
                        palette[i].a = 255;
                        unique = 1;
                        for (int j = 0; j < i; ++j) {
                            if (palette[i].r == palette[j].r &&
                                palette[i].g == palette[j].g &&
                                palette[i].b == palette[j].b) {
                                unique = 0;
                                break;
                            }
                        }
                    }
                }
                needs_redraw = 1;

            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9) {
                    radius = 5 * (e.key.keysym.sym - SDLK_1 + 1);
                    if (radius > MAX_BRUSH_SIZE) radius = MAX_BRUSH_SIZE;
                    if (radius < MIN_BRUSH_SIZE) radius = MIN_BRUSH_SIZE;
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                if (e.wheel.y > 0) {
                    radius += 2;
                    if (radius > MAX_BRUSH_SIZE) radius = MAX_BRUSH_SIZE;
                } else if (e.wheel.y < 0) {
                    radius -= 2;
                    if (radius < MIN_BRUSH_SIZE) radius = MIN_BRUSH_SIZE;
                }
                needs_redraw = 1;
            } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION) {
                int mx = e.button.x;
                int my = e.button.y;
                int left = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
                int right = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);
                int middle = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_MIDDLE);

                if (my >= canvas_h && e.type == SDL_MOUSEBUTTONDOWN) {
                    int index = mx / (window_w / palette_cols);
                    if (index >= 0 && index < palette_cols)
                        current_color = palette[index];
                    needs_redraw = 1;
                } else if (middle) {
                    SDL_SetRenderTarget(ren, canvas);
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                    SDL_RenderClear(ren);
                    SDL_SetRenderTarget(ren, NULL);
                    needs_redraw = 1;
                } else if (left || right) {
                    SDL_SetRenderTarget(ren, canvas);
                    if (left) {
                        SDL_SetRenderDrawColor(ren, current_color.r, current_color.g, current_color.b, 255);
                    } else {
                        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                    }
                    draw_circle(ren, mx, my, radius);
                    SDL_SetRenderTarget(ren, NULL);
                    needs_redraw = 1;
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

            for (int i = 0; i < palette_cols; ++i) {
                SDL_Rect r = { i * (window_w / palette_cols), canvas_h, window_w / palette_cols, PALETTE_HEIGHT };
                SDL_SetRenderDrawColor(ren, palette[i].r, palette[i].g, palette[i].b, 255);
                SDL_RenderFillRect(ren, &r);
            }

            SDL_RenderPresent(ren);
            needs_redraw = 0;
        }
    }

    free(palette);
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
