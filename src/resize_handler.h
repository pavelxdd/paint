// AI Summary: Handles debounced window resize logic for the paint application.
// Recreates the canvas texture to match new window dimensions, preserving existing
// content. Also reconfigures the color palette and updates brush size constraints.
#pragma once

#include "app_context.h" // For AppContext

void process_debounced_resize(AppContext *ctx);
