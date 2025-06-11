#pragma once

typedef void (*BresenhamCallback)(int x, int y, void *userdata);

void draw_line_bresenham(int x0, int y0, int x1, int y1, BresenhamCallback cb, void *userdata);

void draw_circle(SDL_Renderer *ren, float cx, float cy, int radius);
void draw_hollow_circle(SDL_Renderer *ren, float cx, float cy, int radius);
void draw_thick_line(
    SDL_Renderer *ren, float x1, float y1, float x2, float y2, int thickness, SDL_Color color);
