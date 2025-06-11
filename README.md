# Simple Paint

A simple painting application created with C and SDL3. It is designed to be a fun, kid-friendly drawing program with basic tools, colors, and emojis.

![Screenshot of the paint program in action](paint.png)

## 🤖 Disclaimer

This repository is an experiment in AI-driven software development. Its purpose is to test various tools and workflows in which a Large Language Model (LLM) acts as the primary code author.

The development process consists of a human providing high-level tasks and objectives. In response, an AI generates all source code and subsequent modifications. This project, therefore, serves as a practical exploration of using AI as a software engineering tool.

---

## Features

- **Multiple Tools**: Brush, Water Marker, Blur, and Emoji stamper.
- **Color Palette**: A dynamically generated palette of colors.
- **Emoji Palette**: A shuffled grid of fun emojis to stamp on the canvas.
- **Brush Controls**: Adjustable brush size.
- **Straight Line Mode**: Draw straight lines with any tool.
- **Fullscreen Mode**: Toggle fullscreen for an immersive drawing experience.
- **Eraser**: Use the right mouse button to erase.
- **Dynamic UI**: The user interface adapts to the window size.

---

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

---

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

- `Escape`: Exit the application.
- `F1`: Toggle the color palette.
- `F2`: Toggle the emoji palette.
- `Arrow Keys`: Navigate the active palette (color or emoji).
- `F`: Toggle fullscreen.
