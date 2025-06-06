// AI Summary: Declares functions for drawing and handling user input for the
// tool selector UI component, which contains toggles for the color and emoji palettes.
#pragma once

#include "app_context.h"

void tool_selectors_draw(AppContext *ctx, int start_y);

// Returns: TOOL_BRUSH for color toggle, TOOL_EMOJI for emoji toggle, -1 for miss.
int tool_selectors_hit_test(const AppContext *ctx, int mx, int my, int start_y);
