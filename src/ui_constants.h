// AI Summary: Centralised header for UI/layout constants shared across modules.
// Avoids cyclic includes by making every high-level constant available here.
#pragma once

/* --------------------------------------------------------------------
   Palette & Toolbar layout
   -------------------------------------------------------------------- */
#define PALETTE_HEIGHT                50   /* Height of each palette/toolbar row */
#define PALETTE_CELL_MIN_SIZE         50   /* Minimum size of a colour cell   */
#define COLOR_EMOJI_SEPARATOR_HEIGHT   2   /* Thin separator inside palette   */
#define DEFAULT_EMOJI_CELL_PADDING     5   /* Inner padding for emoji render  */

#define TOOL_SELECTOR_AREA_HEIGHT     PALETTE_HEIGHT /* Height of the tool selector bar */
#define TOOL_SELECTOR_SIZE            PALETTE_HEIGHT /* Width/Height of a selector button */
#define TOOL_SELECTOR_SEPARATOR_HEIGHT 2   /* Separator below tool selectors */

/* --------------------------------------------------------------------
   Canvas / brush
   -------------------------------------------------------------------- */
#define MIN_BRUSH_SIZE                  2  /* Smallest brush radius in pixels */
