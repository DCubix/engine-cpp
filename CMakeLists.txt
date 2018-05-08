cmake_minimum_required(VERSION 3.7)
project(engine C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS False)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")

include(extern/extern.cmake)

file(GLOB HEADERS
    "engine/src/*.h"
    "engine/src/core/*.h"
    "engine/src/core/*.hpp"
    "engine/src/core/logging/*.h"
    "engine/src/core/logging/*.hpp"
    "engine/src/core/im3d/*.h"
    "engine/src/core/im3d/*.hpp"
    "engine/src/gfx/*.h"
    "engine/src/gfx/glad/*.h"
    "engine/src/gfx/stb/*.h"
    "engine/src/math/*.h"
    "engine/src/components/*.h"
    "engine/src/systems/*.h"
)

file(GLOB SRC
    "engine/src/*.cpp"
    "engine/src/core/*.cpp"
    "engine/src/core/logging/*.cpp"
    "engine/src/core/im3d/*.cpp"
    "engine/src/gfx/*.cpp"
    "engine/src/gfx/glad/*.c"
    "engine/src/gfx/stb/*.c"
    "engine/src/math/*.cpp"
    "engine/src/components/*.cpp"
    "engine/src/systems/*.cpp"
)

file(GLOB SHADERS
        "engine/src/shaders/*.glsl"
)

add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${SHADERS})

target_link_libraries(${PROJECT_NAME}
    extern::assimp
    extern::physfs
    extern::sdl2
    extern::dl
)
