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
    tool_selectors.c
    tool_brush.c
    tool_emoji.c
    tool_water_marker.c
)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
    m
)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
