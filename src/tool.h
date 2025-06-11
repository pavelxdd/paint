#pragma once

// Forward declaration of App struct to avoid circular include with app.h.
// The full definition is available in app.h, which is included by all
// .c files that use these function declarations.
typedef struct App App;

#define BLUR_DAB_DOWNSCALE_SIZE 64 // Size of the temporary texture for the blur tool

typedef enum {
    TOOL_BRUSH,
    TOOL_WATER_MARKER,
    TOOL_BLUR,
    TOOL_EMOJI,
    TOOL_COUNT
} ActiveTool;

/* --- Drawing Tools --- */

/* --- Brush Tool --- */
void tool_brush_draw_dab(App *app, int x, int y);
void tool_brush_draw_line_preview(App *app, float x0, float y0, float x1, float y1);

/* --- Emoji Tool --- */
void tool_emoji_draw_dab(App *app, int x, int y);
void tool_emoji_draw_line_preview(App *app, float x0, float y0, float x1, float y1);

/* --- Blur Tool --- */
void tool_blur_begin_stroke(App *app);
void tool_blur_end_stroke(App *app);
void tool_blur_draw_dab(App *app, int x, int y);
void tool_blur_draw_line_preview(App *app, float x0, float y0, float x1, float y1);
void tool_blur_draw_line_of_dabs(App *app, float x0, float y0, float x1, float y1);

/* --- Water Marker Tool --- */
void tool_water_marker_begin_stroke(App *app);
void tool_water_marker_end_stroke(App *app);
void tool_water_marker_draw_dab(App *app, int x, int y);
void tool_water_marker_draw_line_preview(App *app, float x0, float y0, float x1, float y1);
