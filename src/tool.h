#pragma once

// Forward declaration of App struct to avoid circular include with app.h.
// The full definition is available in app.h, which is included by all
// .c files that use these function declarations.
typedef struct App App;

typedef enum {
    TOOL_BRUSH,
    TOOL_WATER_MARKER,
    TOOL_EMOJI,
    TOOL_COUNT
} ActiveTool;

#define HIT_TEST_COLOR_PALETTE_TOGGLE 100 // Values that won't conflict with ActiveTool enum
#define HIT_TEST_LINE_MODE_TOGGLE     101

/* --- Drawing Tools --- */

/* --- Brush Tool --- */
void tool_brush_draw_dab(App *app, int x, int y);
void tool_brush_draw_line_preview(App *app, int x0, int y0, int x1, int y1);

/* --- Emoji Tool --- */
void tool_emoji_draw_dab(App *app, int x, int y);
void tool_emoji_draw_line_preview(App *app, int x0, int y0, int x1, int y1);

/* --- Water Marker Tool --- */
void tool_water_marker_begin_stroke(App *app);
void tool_water_marker_end_stroke(App *app);
void tool_water_marker_draw_dab(App *app, int x, int y);
void tool_water_marker_draw_line_preview(App *app, int x0, int y0, int x1, int y1);

/* --- Tool Selectors UI --- */
void tool_selectors_draw(App *app, int start_y);
int tool_selectors_hit_test(const App *app, int mx, int my, int start_y);
