// AI Summary: SDL2 paint program with dynamic 2-row palette, precise cell sizing, color selection highlight, and brush size preview.
// Core features: canvas resizing, palette fills width exactly, black default color, selection highlighting, brush size circle drawn on selected palette color.

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600
#define PALETTE_HEIGHT 50
#define PALETTE_ROWS 5
#define PALETTE_CELL_MIN_SIZE 50
#define MIN_BRUSH_SIZE 2
#define MAX_BRUSH_SIZE 100

// Draw filled circle (for painting on canvas)
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

// Draw hollow (outline only) circle (for preview)
static void draw_hollow_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    if (radius < 1)
        return;
    for (int deg = 0; deg < 360; ++deg) {
        double rad = deg * M_PI / 180.0;
        int x = (int)(cx + radius * cos(rad));
        int y = (int)(cy + radius * sin(rad));
        SDL_RenderDrawPoint(r, x, y);
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

    // Set up palette cell geometry for window width; must fill row exactly.
    int palette_cols = window_w / PALETTE_CELL_MIN_SIZE;
    if (palette_cols < 1) palette_cols = 1;
    int palette_total = palette_cols * PALETTE_ROWS;

    SDL_Color *palette = malloc(sizeof(SDL_Color) * palette_total);
    // Systematically fill palette: last row is grayscale ramp, others as HSV.
    const float S_MIN = 0.2f;
    for (int col = 0; col < palette_cols; ++col) {
        float hue = 360.0f * col / palette_cols;
        for (int row = 0; row < PALETTE_ROWS; ++row) {
            int pos = row * palette_cols + col;
            if (row == PALETTE_ROWS - 1) {
                // Grayscale: left=white, right=black
                float t = (palette_cols == 1) ? 0.0f : (float)col / (palette_cols - 1);
                int gray = (int)((1.0f - t) * 255.0f + 0.5f);
                palette[pos].r = gray;
                palette[pos].g = gray;
                palette[pos].b = gray;
                palette[pos].a = 255;
            } else {
                float sat = S_MIN + (1.0f - S_MIN) * (1.0f - (float)row / (PALETTE_ROWS - 1));
                float val = 1.0f;
                // HSV to RGB
                float c = val * sat;
                float h_ = hue / 60.0f;
                float x = c * (1 - fabsf(fmodf(h_, 2) - 1));
                float r1, g1, b1;
                if      (h_ >= 0 && h_ < 1) { r1 = c; g1 = x; b1 = 0; }
                else if (h_ >= 1 && h_ < 2) { r1 = x; g1 = c; b1 = 0; }
                else if (h_ >= 2 && h_ < 3) { r1 = 0; g1 = c; b1 = x; }
                else if (h_ >= 3 && h_ < 4) { r1 = 0; g1 = x; b1 = c; }
                else if (h_ >= 4 && h_ < 5) { r1 = x; g1 = 0; b1 = c; }
                else if (h_ >= 5 && h_ < 6) { r1 = c; g1 = 0; b1 = x; }
                else                        { r1 = g1 = b1 = 0; }
                float m = val - c;
                palette[pos].r = (int)((r1 + m) * 255.0f + 0.5f);
                palette[pos].g = (int)((g1 + m) * 255.0f + 0.5f);
                palette[pos].b = (int)((b1 + m) * 255.0f + 0.5f);
                palette[pos].a = 255;
            }
        }
    }

    // Default brush color: black (from palette), select bottom-right square at startup.
    SDL_Color current_color = (SDL_Color){0, 0, 0, 255};
    int radius = 10;
    int selected_palette = palette_total - 1; // bottom-right (last) palette cell: black

    int canvas_w = window_w;
    int canvas_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);
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
                canvas_h = window_h - (PALETTE_HEIGHT * PALETTE_ROWS);

                // Recompute palette geometry and color table
                palette_cols = window_w / PALETTE_CELL_MIN_SIZE;
                if (palette_cols < 1) palette_cols = 1;
                palette_total = palette_cols * PALETTE_ROWS;
                palette = realloc(palette, sizeof(SDL_Color) * palette_total);
                // Systematically fill palette: last row is grayscale ramp, others as HSV (matches initialization).
                const float S_MIN = 0.2f;
                for (int col = 0; col < palette_cols; ++col) {
                    float hue = 360.0f * col / palette_cols;
                    for (int row = 0; row < PALETTE_ROWS; ++row) {
                        int pos = row * palette_cols + col;
                        if (row == PALETTE_ROWS - 1) {
                            // Grayscale: left=white, right=black
                            float t = (palette_cols == 1) ? 0.0f : (float)col / (palette_cols - 1);
                            int gray = (int)((1.0f - t) * 255.0f + 0.5f);
                            palette[pos].r = gray;
                            palette[pos].g = gray;
                            palette[pos].b = gray;
                            palette[pos].a = 255;
                        } else {
                            float sat = S_MIN + (1.0f - S_MIN) * (1.0f - (float)row / (PALETTE_ROWS - 1));
                            float val = 1.0f;
                            // HSV to RGB
                            float c = val * sat;
                            float h_ = hue / 60.0f;
                            float x = c * (1 - fabsf(fmodf(h_, 2) - 1));
                            float r1, g1, b1;
                            if      (h_ >= 0 && h_ < 1) { r1 = c; g1 = x; b1 = 0; }
                            else if (h_ >= 1 && h_ < 2) { r1 = x; g1 = c; b1 = 0; }
                            else if (h_ >= 2 && h_ < 3) { r1 = 0; g1 = c; b1 = x; }
                            else if (h_ >= 3 && h_ < 4) { r1 = 0; g1 = x; b1 = c; }
                            else if (h_ >= 4 && h_ < 5) { r1 = x; g1 = 0; b1 = c; }
                            else if (h_ >= 5 && h_ < 6) { r1 = c; g1 = 0; b1 = x; }
                            else                        { r1 = g1 = b1 = 0; }
                            float m = val - c;
                            palette[pos].r = (int)((r1 + m) * 255.0f + 0.5f);
                            palette[pos].g = (int)((g1 + m) * 255.0f + 0.5f);
                            palette[pos].b = (int)((b1 + m) * 255.0f + 0.5f);
                            palette[pos].a = 255;
                        }
                    }
                }
                selected_palette = palette_total - 1; // Reset selection to bottom-right (black) on resize
                current_color = (SDL_Color){0, 0, 0, 255}; // Reset brush color to black
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

                int palette_area_top = canvas_h;
                int palette_area_bottom = window_h;

                // Compute geometry for palette cells (fill row exactly)
                int cell_width = window_w / palette_cols;
                int cell_width_rem = window_w % palette_cols;

                if (my >= palette_area_top && my < palette_area_bottom && e.type == SDL_MOUSEBUTTONDOWN) {
                    int row = (my - palette_area_top) / PALETTE_HEIGHT;
                    // Find column by traversing cell edges, to deal with non-divisible width
                    int x = 0, col = 0, found = 0;
                    for (col = 0; col < palette_cols; ++col) {
                        int w = cell_width + (col < cell_width_rem ? 1 : 0);
                        if (mx < x + w) {
                            found = 1;
                            break;
                        }
                        x += w;
                    }
                    if (found && row >= 0 && row < PALETTE_ROWS) {
                        int idx = row * palette_cols + col;
                        if (idx >= 0 && idx < palette_total) {
                            selected_palette = idx;
                            current_color = palette[idx];
                        }
                    }
                    needs_redraw = 1;
                } else if (middle) {
                    SDL_SetRenderTarget(ren, canvas);
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                    SDL_RenderClear(ren);
                    SDL_SetRenderTarget(ren, NULL);
                    needs_redraw = 1;
                } else if (left || right) {
                    if (my < canvas_h) {
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

            // Compute geometry for palette cells (fill row exactly)
            int cell_width = window_w / palette_cols;
            int cell_width_rem = window_w % palette_cols;

            // Draw palette
            int cell_y = canvas_h;
            for (int row = 0; row < PALETTE_ROWS; ++row) {
                int cell_x = 0;
                for (int col = 0; col < palette_cols; ++col) {
                    int i = row * palette_cols + col;
                    if (i >= palette_total)
                        continue;
                    int w = cell_width + (col < cell_width_rem ? 1 : 0);
                    SDL_Rect r = {cell_x, cell_y, w, PALETTE_HEIGHT};
                    // Fill color
                    SDL_SetRenderDrawColor(ren, palette[i].r, palette[i].g, palette[i].b, 255);
                    SDL_RenderFillRect(ren, &r);

                    // Draw selection frame if this is the selected palette square
                    if (i == selected_palette) {
                        // Use inverted color for selection frame for maximum contrast
                        Uint8 inv_r = 255 - palette[i].r;
                        Uint8 inv_g = 255 - palette[i].g;
                        Uint8 inv_b = 255 - palette[i].b;
                        SDL_SetRenderDrawColor(ren, inv_r, inv_g, inv_b, 255);
                        SDL_RenderDrawRect(ren, &r);
                        SDL_Rect r2 = {r.x + 1, r.y + 1, r.w - 2, r.h - 2};
                        SDL_RenderDrawRect(ren, &r2);
                        // Draw hollow circle for brush size preview (center of cell)
                        int cx = r.x + r.w/2;
                        int cy = r.y + r.h/2;
                        // Constrain circle radius so it fits inside cell
                        int circle_r = radius;
                        int max_cell_radius = (r.w < r.h ? r.w : r.h) / 2 - 3;
                        if (circle_r > max_cell_radius) circle_r = max_cell_radius;
                        if (circle_r >= 2)
                        {
                            // Invert the palette color for visibility
                            SDL_SetRenderDrawColor(
                                ren,
                                inv_r,
                                inv_g,
                                inv_b,
                                255
                            );
                            draw_hollow_circle(ren, cx, cy, circle_r);
                        }
                    }
                    cell_x += w;
                }
                cell_y += PALETTE_HEIGHT;
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
