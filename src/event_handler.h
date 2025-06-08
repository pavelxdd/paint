#pragma once

#include "app_context.h" // For AppContext
#include <SDL2/SDL_events.h> // For SDL_Keycode etc.

void handle_events(AppContext *ctx, int *is_running, Uint32 sdl_wait_timeout);
