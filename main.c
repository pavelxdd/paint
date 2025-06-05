#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PALETTE_COLS 8
#define PALETTE_HEIGHT 50

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

    SDL_Window *win = SDL_CreateWindow("Simple Paint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, 0);
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
    SDL_Color palette[PALETTE_COLS];
    for (int i = 0; i < PALETTE_COLS; ++i) {
        palette[i].r = rand() % 256;
        palette[i].g = rand() % 256;
        palette[i].b = rand() % 256;
        palette[i].a = 255;
    }
    SDL_Color current_color = palette[0];
    int radius = 10;

    int canvas_h = WINDOW_HEIGHT - PALETTE_HEIGHT;
    SDL_Texture *canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, canvas_h);
    SDL_SetRenderTarget(ren, canvas);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);
    SDL_SetRenderTarget(ren, NULL);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9) {
                    radius = 5 * (e.key.keysym.sym - SDLK_1 + 1);
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION) {
                int mx = e.button.x;
                int my = e.button.y;
                int left = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
                int right = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);
                int middle = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_MIDDLE);

                if (my >= canvas_h && e.type == SDL_MOUSEBUTTONDOWN) {
                    int index = mx / (WINDOW_WIDTH / PALETTE_COLS);
                    if (index >= 0 && index < PALETTE_COLS)
                        current_color = palette[index];
                } else if (middle) {
                    SDL_SetRenderTarget(ren, canvas);
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                    SDL_RenderClear(ren);
                    SDL_SetRenderTarget(ren, NULL);
                } else if (left || right) {
                    SDL_SetRenderTarget(ren, canvas);
                    if (left) {
                        SDL_SetRenderDrawColor(ren, current_color.r, current_color.g, current_color.b, 255);
                    } else {
                        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                    }
                    draw_circle(ren, mx, my, radius);
                    SDL_SetRenderTarget(ren, NULL);
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);

        SDL_RenderCopy(ren, canvas, NULL, NULL);

        for (int i = 0; i < PALETTE_COLS; ++i) {
            SDL_Rect r = { i * (WINDOW_WIDTH / PALETTE_COLS), canvas_h, WINDOW_WIDTH / PALETTE_COLS, PALETTE_HEIGHT };
            SDL_SetRenderDrawColor(ren, palette[i].r, palette[i].g, palette[i].b, 255);
            SDL_RenderFillRect(ren, &r);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
