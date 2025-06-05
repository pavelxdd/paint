add_executable(${CMAKE_PROJECT_NAME}
    main.c
    draw.c
    palette.c
    app_context.c
    event_handler.c
    resize_handler.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} m)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
