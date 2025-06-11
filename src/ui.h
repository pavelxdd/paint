#pragma once

typedef struct App App;

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600

#define RESIZE_DEBOUNCE_MS 200 // Milliseconds for resize debouncing

/* --------------------------------------------------------------------
   Palette & Toolbar layout
   -------------------------------------------------------------------- */
#define PALETTE_HEIGHT 50                 /* Height of each palette/toolbar row */
#define PALETTE_CELL_MIN_SIZE 50          /* Minimum size of a color cell   */
#define COLOR_EMOJI_SEPARATOR_HEIGHT 2    /* Thin separator inside palette   */
#define DEFAULT_EMOJI_CELL_PADDING 5      /* Inner padding for emoji render  */

#define TOOL_SELECTOR_AREA_HEIGHT PALETTE_HEIGHT /* Height of the tool selector bar */
#define TOOL_SELECTOR_SIZE PALETTE_HEIGHT        /* Width/Height of a selector button */
#define TOOL_SELECTOR_SEPARATOR_HEIGHT 2         /* Separator below tool selectors */

/* --------------------------------------------------------------------
   Canvas / brush
   -------------------------------------------------------------------- */
#define MIN_BRUSH_SIZE 2 /* Smallest brush radius in pixels */

#define HIT_TEST_COLOR_PALETTE_TOGGLE 100 // Values that won't conflict with ActiveTool enum
#define HIT_TEST_LINE_MODE_TOGGLE     101

/* --- Tool Selectors UI --- */
void ui_draw_tool_selectors(App *app, int start_y);
int ui_hit_test_tool_selectors(const App *app, int mx, int my, int start_y);
