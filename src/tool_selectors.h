#pragma once

#include "app_context.h"

#define HIT_TEST_COLOR_PALETTE_TOGGLE 100 // A value that won't conflict with ActiveTool enum

void tool_selectors_draw(AppContext *ctx, int start_y);

int tool_selectors_hit_test(const AppContext *ctx, int mx, int my, int start_y);
