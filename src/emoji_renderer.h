// AI Summary: Defines the EmojiRenderer interface for managing and rendering emoji assets.
// Handles loading the emoji font, shuffling emoji codepoints, rendering them to SDL_Textures,
// and providing access to these textures and their properties.
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define EMOJI_FONT_PATH "/usr/share/fonts/noto/NotoColorEmoji.ttf" // Ensure this font is available
#define EMOJI_FONT_SIZE 48                 // Font size for rendering emojis to texture

typedef struct EmojiRenderer EmojiRenderer;

// Creates an EmojiRenderer instance.
// Loads the emoji font and prepares for rendering.
// Returns NULL on failure.
EmojiRenderer* emoji_renderer_create(SDL_Renderer* ren);

// Destroys an EmojiRenderer instance, freeing all associated resources.
void emoji_renderer_destroy(EmojiRenderer* er);

// Shuffles the internal list of emoji codepoints and re-renders all emojis to textures.
// This should be called if a new set/order of emojis is desired.
void emoji_renderer_shuffle_and_render_all(EmojiRenderer* er);

// Gets a specific emoji texture and its original dimensions.
// The index is into the shuffled list of available emojis.
// Returns SDL_FALSE if the index is invalid or texture is not available.
SDL_bool emoji_renderer_get_texture_info(const EmojiRenderer* er, int emoji_array_idx, SDL_Texture** tex, int* w, int* h);

// Gets the total number of unique emojis available and rendered by this instance.
int emoji_renderer_get_num_emojis(const EmojiRenderer* er);
