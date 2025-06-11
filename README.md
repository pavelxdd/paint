# Simple Paint

A simple paint application created with C and SDL3.

## 🤖 Disclaimer

This repository is an experiment in AI-driven software development. Its purpose is to test various tools and workflows in which a Large Language Model (LLM) acts as the primary code author.

The development process consists of a human providing high-level tasks and objectives. In response, an AI generates all source code and subsequent modifications. This project, therefore, serves as a practical exploration of using AI as a software engineering tool.

---

![Screenshot of the paint program in action](paint.png)

## Features

- **Basic Drawing Tools**: Includes a standard brush and a semi-transparent "water marker".
- **Blur Tool**: A tool to smudge and blur parts of the drawing, creating soft, blended effects.
- **Emoji Brush**: Select from a shuffled grid of emojis to paint with.
- **Resizable Brush**: Change the brush size with the mouse wheel or keyboard shortcuts.
- **Color Palette**: A dynamic color palette is generated based on window size. Click to select a
    color, or middle-click to set the background.
- **Straight Line Mode**: Hold `Ctrl` or toggle with `Ctrl`+`Ctrl` to draw straight lines. Hold
    `Shift` while drawing a straight line to snap to horizontal or vertical axes.
- **Simple UI**: The UI is minimal, with tool selectors and palettes that adjust to the window
    size.
- **Cross-Platform**: Built with SDL3, it should compile and run on Windows, macOS, and Linux.

## Dependencies

This project requires a modern C compiler (C17), CMake, and the following libraries:

- **SDL3**
- **SDL3_ttf**

Please ensure they are installed on your system before building.

## Building

To build the project, use CMake:

```bash
cmake -B build
cmake --build build
```

## Running

Execute the binary from the build directory:

```bash
./build/paint
```

## How to Use

### Mouse Controls

- **Left Mouse Button**: Draw with the selected tool and color.
- **Right Mouse Button**: Erase (draws with the background color).
- **Middle Mouse Button**:
  - On canvas: Clear the canvas to the current background color.
  - On a color in the palette: Set that color as the new background and clear the canvas.
- **Mouse Wheel**:
  - Over canvas: Adjust brush size.
  - Over palette: Cycle through the current tool's selection (colors or emojis).

### Keyboard Shortcuts

#### Tool Selection

- `1`: Select **Brush** tool.
- `2`: Select **Water Marker** tool.
- `3`: Select **Blur** tool.
- `0`: Select **Emoji** tool.
- `Tab`: Cycle forward through tools.
- `Ctrl` + `Tab`: Cycle backward through tools.

#### Drawing Controls

- `+` / `-`: Increase/decrease brush size.
- `Ctrl` (hold): Temporarily enter straight-line drawing mode.
- `Ctrl` + `Ctrl` (press both): Toggle straight-line mode on/off.
- `Shift` (while in straight-line mode): Snap line to 90-degree angles.

#### UI & Window

- `F1`: Toggle the color palette.
- `F2`: Toggle the emoji palette.
- `Arrow Keys`: Navigate the active palette (color or emoji).
- `F`: Toggle fullscreen.
