#pragma once

// Ensure this font is available
#define EMOJI_FONT_PATH "/usr/share/fonts/noto/NotoColorEmoji.ttf"
#define EMOJI_FONT_SIZE 48 // Font size for rendering emojis to texture

typedef struct EmojiRenderer {
    TTF_Font *emoji_font;
    const char **emoji_codepoints_shuffled; // Shuffled copy of original codepoints
    SDL_Texture **emoji_textures;
    SDL_Point *emoji_texture_dims;
    int num_defined_emojis;
    SDL_Renderer *ren_ref;

    // For showing a default icon in the UI when the brush tool is active
    SDL_Texture *default_emoji_texture;
    SDL_Point default_emoji_texture_dims;
} EmojiRenderer;

// Creates an EmojiRenderer instance.
// Loads the emoji font and prepares for rendering.
// Returns NULL on failure.
EmojiRenderer *emoji_renderer_create(SDL_Renderer *ren);

// Destroys an EmojiRenderer instance, freeing all associated resources.
void emoji_renderer_destroy(EmojiRenderer *er);

// Shuffles the internal list of emoji codepoints and re-renders all emojis to textures.
// This should be called if a new set/order of emojis is desired.
void emoji_renderer_shuffle_and_render_all(EmojiRenderer *er);

// Gets a specific emoji texture and its original dimensions.
// The index is into the shuffled list of available emojis.
// Returns false if the index is invalid or texture is not available.
bool emoji_renderer_get_texture_info(
    const EmojiRenderer *er, int emoji_array_idx, SDL_Texture **tex, int *w, int *h);

// Gets the texture info for the default "blank face" emoji.
bool emoji_renderer_get_default_texture_info(const EmojiRenderer *er, SDL_Texture **tex, int *w, int *h);

// Gets the total number of unique emojis available and rendered by this instance.
int emoji_renderer_get_num_emojis(const EmojiRenderer *er);
