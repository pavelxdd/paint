// AI Summary: Contains the static definition of the emoji codepoint array and its count for the palette.
#include "emoji_data.h"
#include <stddef.h> // For sizeof

// Predefined list of emojis - curated for a children's painting application.
// This list is shuffled on startup.
const char* ORIGINAL_DEFAULT_EMOJI_CODEPOINTS[] = {
    // -- Faces & People --
    "😀", "😂", "😍", "🥳", "😊", "🥺", "🤩", "🤪", "🤠", "🤡", "👻", "👽", "🤖", "😇", "🎃", // Fun faces
    "🦸", "🦸‍♀️", "🦸‍♂️", "🦹", "🦹‍♀️", "🦹‍♂️", // Superheroes
    "🧑‍🎨", "🧑‍🚀", "🧑‍🚒", "👮", "👷", "💂", "🕵️", // Professions

    // -- Animals & Mythical Creatures --
    "🐶", "🐱", "🐭", "🐹", "🐰", "🦊", "🐻", "🐼", "🐨", "🐯", "🦁", "🐮", "🐷", "🐸", "🐒", "🐔", // Mammals & farm
    "🐧", "🐦", "🐤", "🦆", "🦅", "🦉", "🦇", "🐺", "🐴", // Birds & others
    "🐝", "🐛", "🦋", "🐌", "🐞", "🐜", // Insects
    "🐢", "🐍", "🦎", "🦖", "🦕", "🐊", // Reptiles & Dinos
    "🦈", "🐠", "🐟", "🐡", "🐬", "🐳", "🐙", "🦀", // Sea creatures
    "🦄", "🐉", "🧜‍♀️", "🧜‍♂️", "🧚‍♀️", "🧚‍♂️", "🧞‍♀️", "🧞‍♂️", "🧝‍♀️", "🧝‍♂️", // Fantasy

    // -- Nature & Weather --
    "☀️", "☁️", "🌧️", "❄️", "⚡", "🔥", "💧", "🌈", "⭐", "✨", "☄️", "🪐", // Sky & Weather
    "🌷", "🌹", "🌻", "🌸", "🌼", "🌺", "🍀", "🍄", "🌵", "🌲", "🌳", "🌴", // Plants
    "🌍", "🌋", "⛰️", "🏕️", // Places

    // -- Food & Drink --
    "🍎", "🍊", "🍌", "🍉", "🍇", "🍓", "🥝", "🍍", "🍑", "🍒", "🥥", "🥑", "🥕", "🌽", // Fruits & Veg
    "🍕", "🍔", "🍟", "🌭", "🌮", "🧇", "🥞", "🥨", "🧀", "🥚", // Meals
    "🍦", "🍧", "🍨", "🍩", "🍪", "🎂", "🍰", "🧁", "🥧", "🍫", "🍬", "🍭", "🍿", // Sweets
    "🥤", "🧃", "🥛", // Drinks

    // -- Activities & Hobbies --
    "⚽", "🏀", "🏈", "⚾", "🎾", "🏐", "🎳", "⛳", "🛹", "🚲", "🎨", "🎭", // Sports & Hobbies
    "🎤", "🎧", "🎼", "🎹", "🥁", "🎷", "🎺", "🎸", "🎻", "🎲", "🧩", "🎮", // Music & Games
    "🪄", "🔮", "💎", "👑", "💍", "🎁", "🎈", "🎉", "🎊", // Magic & Party

    // -- Travel & Objects --
    "🚗", "🚕", "🚓", "🚑", "🚒", "🚚", "🚜", "🚲", "🛵", "✈️", "🚀", "🛸", "🚁", "⛵", "⚓", "🚂", // Vehicles
    "🏠", "🏡", "🏰", "⛺", "🎡", "🎢", "🎠", // Buildings & Places
    "❤️", "🧡", "💛", "💚", "💙", "💜", "💖", "❤️‍🔥", "💯", // Hearts & Symbols
    "💡", "💣", "🔑", "🛡️", "🧸", "🪁" // Misc Objects
};
const int NUM_DEFAULT_EMOJIS = sizeof(ORIGINAL_DEFAULT_EMOJI_CODEPOINTS) / sizeof(ORIGINAL_DEFAULT_EMOJI_CODEPOINTS[0]);
