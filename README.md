# paint

![Screenshot of the paint program in action](paint.png)

Welcome to `paint`, the revolutionary digital canvas where creativity knows no bounds! This isn't just another paint program; it's a masterpiece of modern software engineering, fully vibe-coded with the power of AI to deliver an unparalleled artistic experience. From procedurally generated color palettes to a delightful array of emojis, every feature has been meticulously crafted to inspire joy and unleash your inner artist. Prepare to be amazed!

## Building

This project uses CMake.

### Install Dependencies

* CMake (e.g., `sudo apt install cmake` on Debian/Ubuntu, `brew install cmake` on macOS)
* SDL2 development libraries (e.g., `sudo apt install libsdl2-dev` on Debian/Ubuntu, `brew install sdl2` on macOS)
* SDL2_ttf development libraries (e.g., `sudo apt install libsdl2-ttf-dev` on Debian/Ubuntu, `brew install sdl2_ttf` on macOS)

### Compile

Create a build directory and run CMake and Make from there:

```bash
mkdir build
cd build
cmake ..
make
```

The executable `paint` will be created in the `build` directory.

## Running

After successful compilation, navigate to the `build` directory and run the executable:

```bash
cd build
./paint
```
