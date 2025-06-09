#include "emoji_data.h"
#include "emoji_renderer.h"

// Fisher-Yates shuffle for an array of char pointers
static void shuffle_char_pointers(const char **array, int n)
{
    if (n > 1) {
        int i = n - 1;
        while (i > 0) {
            int j = rand() % (i + 1);
            const char *temp = array[j];
            array[j] = array[i];
            array[i] = temp;
            i--;
        }
    }
}

static void clear_rendered_emojis(EmojiRenderer *er)
{
    if (er->emoji_textures) {
        for (int i = 0; i < er->num_defined_emojis; ++i) {
            if (er->emoji_textures[i]) {
                SDL_DestroyTexture(er->emoji_textures[i]);
                er->emoji_textures[i] = NULL;
            }
        }
    }
    // Dimensions are part of the texture info, no separate free needed if textures are cleared
}

EmojiRenderer *emoji_renderer_create(SDL_Renderer *ren)
{
    EmojiRenderer *er = (EmojiRenderer *)malloc(sizeof(EmojiRenderer));
    if (!er) {
        SDL_Log("Failed to allocate EmojiRenderer");
        return NULL;
    }

    er->ren_ref = ren;
    er->emoji_font = TTF_OpenFont(EMOJI_FONT_PATH, EMOJI_FONT_SIZE);
    if (!er->emoji_font) {
        SDL_Log("Failed to load emoji font '%s': %s", EMOJI_FONT_PATH, SDL_GetError());
        free(er);
        return NULL;
    }

    er->num_defined_emojis = NUM_DEFAULT_EMOJIS;
    er->emoji_codepoints_shuffled = NULL;
    er->emoji_textures = NULL;
    er->emoji_texture_dims = NULL;

    if (er->num_defined_emojis > 0) {
        er->emoji_codepoints_shuffled =
            (const char **)malloc(sizeof(char *) * er->num_defined_emojis);
        if (!er->emoji_codepoints_shuffled) {
            SDL_Log("Failed to allocate memory for shuffled emoji codepoints.");
            TTF_CloseFont(er->emoji_font);
            free(er);
            return NULL;
        }
        // Copy pointers from the original static array
        for (int i = 0; i < er->num_defined_emojis; ++i) {
            er->emoji_codepoints_shuffled[i] = ORIGINAL_DEFAULT_EMOJI_CODEPOINTS[i];
        }

        er->emoji_textures =
            (SDL_Texture **)calloc(er->num_defined_emojis, sizeof(SDL_Texture *));
        er->emoji_texture_dims =
            (SDL_Point *)calloc(er->num_defined_emojis, sizeof(SDL_Point));

        if (!er->emoji_textures || !er->emoji_texture_dims) {
            SDL_Log("Failed to allocate memory for emoji textures/dims.");
            TTF_CloseFont(er->emoji_font);
            free(er->emoji_codepoints_shuffled);
            free(er->emoji_textures); // calloc initializes to NULL, so safe to free
            free(er->emoji_texture_dims);
            free(er);
            return NULL;
        }
        emoji_renderer_shuffle_and_render_all(er); // Initial shuffle and render
    }

    er->default_emoji_texture = NULL;
    er->default_emoji_texture_dims = (SDL_Point) {
        0, 0
    };
    const char *default_emoji_codepoint = "🙂";
    SDL_Color fg_color_default = {0, 0, 0, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(
                               er->emoji_font, default_emoji_codepoint, strlen(default_emoji_codepoint), fg_color_default);
    if (surface) {
        er->default_emoji_texture = SDL_CreateTextureFromSurface(er->ren_ref, surface);
        if (er->default_emoji_texture) {
            er->default_emoji_texture_dims = (SDL_Point) {
                surface->w, surface->h
            };
        } else {
            SDL_Log("Failed to create default emoji texture: %s", SDL_GetError());
        }
        SDL_DestroySurface(surface);
    } else {
        SDL_Log("Failed to render default emoji: %s", SDL_GetError());
    }

    return er;
}

void emoji_renderer_destroy(EmojiRenderer *er)
{
    if (!er) {
        return;
    }
    clear_rendered_emojis(er);
    free(er->emoji_textures);
    free(er->emoji_texture_dims);
    free(er->emoji_codepoints_shuffled); // Free the array of pointers, not the strings themselves

    if (er->default_emoji_texture) {
        SDL_DestroyTexture(er->default_emoji_texture);
    }

    if (er->emoji_font) {
        TTF_CloseFont(er->emoji_font);
    }
    free(er);
}

void emoji_renderer_shuffle_and_render_all(EmojiRenderer *er)
{
    if (!er || !er->emoji_font || !er->ren_ref || er->num_defined_emojis == 0 ||
        !er->emoji_codepoints_shuffled) {
        return;
    }

    // Shuffle the copied pointers
    shuffle_char_pointers(er->emoji_codepoints_shuffled, er->num_defined_emojis);

    // Clear any previously rendered textures
    clear_rendered_emojis(er);

    SDL_Color fg_color = {0, 0, 0, 255}; // Emojis are typically rendered with their own colors
    for (int i = 0; i < er->num_defined_emojis; ++i) {
        const char *codepoint = er->emoji_codepoints_shuffled[i];
        if (!codepoint || strlen(codepoint) == 0) {
            er->emoji_textures[i] = NULL;
            er->emoji_texture_dims[i] = (SDL_Point) {
                0, 0
            };
            continue;
        }
        SDL_Surface *surface =
            TTF_RenderText_Blended(er->emoji_font, codepoint, strlen(codepoint), fg_color);
        if (!surface) {
            SDL_Log("Failed to render emoji '%s': %s", codepoint, SDL_GetError());
            er->emoji_textures[i] = NULL;
            er->emoji_texture_dims[i] = (SDL_Point) {
                0, 0
            };
            continue;
        }
        er->emoji_textures[i] = SDL_CreateTextureFromSurface(er->ren_ref, surface);
        if (!er->emoji_textures[i]) {
            SDL_Log("Failed to create texture for emoji '%s': %s", codepoint, SDL_GetError());
            er->emoji_texture_dims[i] = (SDL_Point) {
                0, 0
            };
        } else {
            er->emoji_texture_dims[i] = (SDL_Point) {
                surface->w, surface->h
            };
        }
        SDL_DestroySurface(surface);
    }
}

bool emoji_renderer_get_texture_info(const EmojiRenderer *er,
                                     int emoji_array_idx,
                                     SDL_Texture **tex,
                                     int *w,
                                     int *h)
{
    if (!er || !tex || !w || !h || emoji_array_idx < 0 ||
        emoji_array_idx >= er->num_defined_emojis) {
        return false;
    }
    if (!er->emoji_textures || !er->emoji_texture_dims || !er->emoji_textures[emoji_array_idx]) {
        return false;
    }
    *tex = er->emoji_textures[emoji_array_idx];
    *w = er->emoji_texture_dims[emoji_array_idx].x;
    *h = er->emoji_texture_dims[emoji_array_idx].y;
    return true;
}

bool emoji_renderer_get_default_texture_info(const EmojiRenderer *er,
                                             SDL_Texture **tex,
                                             int *w,
                                             int *h)
{
    if (!er || !tex || !w || !h || !er->default_emoji_texture) {
        if (tex) {
            *tex = NULL;
        }
        return false;
    }
    *tex = er->default_emoji_texture;
    *w = er->default_emoji_texture_dims.x;
    *h = er->default_emoji_texture_dims.y;
    return true;
}

int emoji_renderer_get_num_emojis(const EmojiRenderer *er)
{
    if (!er) {
        return 0;
    }
    return er->num_defined_emojis;
}
