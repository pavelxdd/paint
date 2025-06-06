// AI Summary: Declares color utility functions, such as for converting from HSV to RGB color space.
#pragma once

#include <SDL2/SDL.h>

/**
 * @brief Converts a color from HSV to RGB color space.
 * 
 * @param h Hue, in degrees [0, 360).
 * @param s Saturation, [0, 1].
 * @param v Value, [0, 1].
 * @return SDL_Color The equivalent RGB color.
 */
SDL_Color hsv_to_rgb(float h, float s, float v);
