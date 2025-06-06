// AI Summary: Declares the main event handling function for the application, which
// processes the SDL event queue and updates the application state accordingly.
#pragma once

#include "app_context.h" // For AppContext
#include <SDL2/SDL_events.h> // For SDL_Keycode etc.

void handle_events(AppContext *ctx, int *is_running, Uint32 sdl_wait_timeout);
