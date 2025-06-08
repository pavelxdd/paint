#pragma once

#include "app_context.h"

/**
 * @brief Renders the entire application scene.
 *
 * This includes the main canvas, any active tool previews (like the water marker
 * stroke buffer), and all UI elements like the tool selectors and palettes.
 *
 * @param ctx The application context containing all state needed for rendering.
 */
void render_scene(AppContext *ctx);
