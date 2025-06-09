#pragma once

#include "app.h" // For App
#include <SDL3/SDL_events.h>

void handle_events(App *app, int *is_running, int sdl_wait_timeout);
