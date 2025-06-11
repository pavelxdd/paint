add_executable(${CMAKE_PROJECT_NAME}
    app.c
    app_brush.c
    app_canvas.c
    app_draw.c
    app_keyboard.c
    app_layout.c
    app_mouse.c
    app_palette.c
    app_resize.c
    app_state.c
    color_utils.c
    draw.c
    emoji_data.c
    emoji_renderer.c
    event_handler.c
    main.c
    palette.c
    palette_draw.c
    palette_queries.c
    renderer.c
    tool_brush.c
    tool_blur.c
    tool_emoji.c
    tool_water_marker.c
    ui.c
)

target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE pch.h)

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL3_INCLUDE_DIRS}
    ${SDL3_TTF_INCLUDE_DIRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    m
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    -D_GNU_SOURCE
    -D_LARGE_FILES
    -D_LARGEFILE_SOURCE
    -D_FILE_OFFSET_BITS=64
    -D_TIME_BITS=64
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -pipe
    -funsigned-char
    -fno-math-errno
    -fno-common
    -Werror
    -Wfatal-errors
    -Wall
    -Wextra
    -Wundef
    -Wpedantic
    -pedantic-errors
    -Wstrict-prototypes
    -Wmissing-braces
    -Wmissing-prototypes
    -Wmissing-declarations
    -Wimplicit-fallthrough
    -Wdouble-promotion
    -Wpointer-arith
    -Wfloat-equal
    -Winline
    -Wshadow
    -Walloca
    -Wformat=2
    -Wwrite-strings
    -Wstrict-overflow
    -Wno-missing-field-initializers
    -Wno-unused-parameter
    -Wno-strict-aliasing
    -Wno-format-nonliteral
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wduplicated-branches
        -Wduplicated-cond
        -Wlogical-op
        -Wnormalized
        -Wstringop-overflow=4
        -Wformat-overflow=2
        -Wformat-truncation
        -Wformat-signedness
        -Wno-format-zero-length
        -Wno-clobbered
    )
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wno-gnu
        -Wno-language-extension-token
        -Wno-builtin-requires-header
        -Wno-format-pedantic
    )
endif()
