add_executable(${CMAKE_PROJECT_NAME}
    main.c
    app_context.c
    color_utils.c
    draw.c
    emoji_data.c
    emoji_renderer.c
    event_handler.c
    palette.c
    palette_draw.c
    resize_handler.c
    tool_selectors.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    SDL2::SDL2          # Use imported target for SDL2
    SDL2_ttf::SDL2_ttf  # Use imported target for SDL2_ttf
    m                   # Math library
)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
