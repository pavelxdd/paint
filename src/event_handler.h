#pragma once

#include "app.h" // For App
#include <SDL2/SDL_events.h>

void handle_events(App *app, int *is_running, Uint32 sdl_wait_timeout);
