cmake_minimum_required(VERSION 3.10)
project(paint C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${CMAKE_PROJECT_NAME}
    main.c
    draw.c
    palette.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} m)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
