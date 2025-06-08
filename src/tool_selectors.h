#pragma once

#include "app.h"

#define HIT_TEST_COLOR_PALETTE_TOGGLE 100 // Values that won't conflict with ActiveTool enum
#define HIT_TEST_LINE_MODE_TOGGLE     101

void tool_selectors_draw(App *app, int start_y);

int tool_selectors_hit_test(const App *app, int mx, int my, int start_y);
