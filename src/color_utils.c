// AI Summary: Implements color conversion utility functions, such as HSV to RGB.
#include "color_utils.h"
#include <math.h>

/* HSV→RGB conversion (0 ≤ h < 360) */
SDL_Color hsv_to_rgb(float h, float s, float v)
{
    SDL_Color rgb = { 0, 0, 0, 255 };

    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    if (s == 0.0f) {          /* gray */
        rgb.r = rgb.g = rgb.b = lroundf(v * 255.0f);
        return rgb;
    }

    float c       = v * s;
    float h_prime = fmodf(h / 60.0f, 6.0f);
    if (h_prime < 0) h_prime += 6.0f;
    float x_val   = c * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));

    float r1 = 0, g1 = 0, b1 = 0;
    if      (h_prime < 1) { r1 = c; g1 = x_val; }
    else if (h_prime < 2) { r1 = x_val; g1 = c; }
    else if (h_prime < 3) { g1 = c; b1 = x_val; }
    else if (h_prime < 4) { g1 = x_val; b1 = c; }
    else if (h_prime < 5) { r1 = x_val; b1 = c; }
    else                  { r1 = c;            b1 = x_val; }

    float m = v - c;
    r1 += m; g1 += m; b1 += m;

    rgb.r = lroundf(r1 * 255.0f);
    rgb.g = lroundf(g1 * 255.0f);
    rgb.b = lroundf(b1 * 255.0f);
    return rgb;
}
