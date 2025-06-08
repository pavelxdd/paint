#pragma once

#include "app.h"

/**
 * @brief Renders the entire application scene.
 *
 * This includes the main canvas, any active tool previews (like the water marker
 * stroke buffer), and all UI elements like the tool selectors and palettes.
 *
 * @param app The application context containing all state needed for rendering.
 */
void render_scene(App *app);
