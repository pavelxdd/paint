// AI Summary: Centralised header for UI/layout constants shared across modules.
// Avoids cyclic includes by making every high-level constant available here.
#pragma once

/* --------------------------------------------------------------------
   Palette layout                                                          
   -------------------------------------------------------------------- */
#define PALETTE_HEIGHT                50   /* Height of each palette row      */
#define PALETTE_CELL_MIN_SIZE         50   /* Minimum size of a colour cell   */
#define COLOR_EMOJI_SEPARATOR_HEIGHT   2   /* Thin separator inside palette   */
#define DEFAULT_EMOJI_CELL_PADDING     5   /* Inner padding for emoji render  */

/* --------------------------------------------------------------------
   Canvas / brush                                                         
   -------------------------------------------------------------------- */
#define CANVAS_PALETTE_SEPARATOR_HEIGHT 2  /* Gap between canvas & palette    */
#define MIN_BRUSH_SIZE                  2  /* Smallest brush radius in pixels */
