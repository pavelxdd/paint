# paint

## Building

This project uses CMake.

1.  **Install Dependencies:**
    *   CMake (e.g., `sudo apt install cmake` on Debian/Ubuntu, `brew install cmake` on macOS)
    *   SDL2 development libraries (e.g., `sudo apt install libsdl2-dev` on Debian/Ubuntu, `brew install sdl2` on macOS)

2.  **Compile:**
    Create a build directory and run CMake and Make from there:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
    The executable `main` will be created in the `build` directory.

## Running

After successful compilation, navigate to the `build` directory and run the executable:
```bash
./paint
```