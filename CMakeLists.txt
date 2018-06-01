cmake_minimum_required(VERSION 3.7)
project(engine VERSION 0.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")

find_package(Assimp REQUIRED)
find_package(SDL2 REQUIRED SDL2)
find_package(PhysFS REQUIRED)
find_package(Bullet 2.85 EXACT REQUIRED)

include_directories(
	${ASSIMP_INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${PHYSFS_INCLUDE_DIR}
	${BULLET_INCLUDE_DIRS}
)

file(GLOB HEADERS
	"engine/src/*.h"
	"engine/src/core/*.h"
	"engine/src/core/*.hpp"
	"engine/src/core/logging/*.h"
	"engine/src/core/logging/*.hpp"
	"engine/src/gfx/*.h"
	"engine/src/gfx/glad/*.h"
	"engine/src/gfx/stb/*.h"
	"engine/src/math/*.h"
	"engine/src/components/*.h"
	"engine/src/systems/*.h"
	"engine/src/imgui/*.h"
)

file(GLOB_RECURSE GLM_HEADERS
	"engine/src/math/glm/*.hpp"
)
set(HEADERS ${HEADERS} ${GLM_HEADERS})

file(GLOB SRC
	"engine/src/*.cpp"
	"engine/src/core/*.cpp"
	"engine/src/core/logging/*.cpp"
	"engine/src/gfx/*.cpp"
	"engine/src/gfx/glad/*.c"
	"engine/src/gfx/stb/*.c"
	"engine/src/math/*.cpp"
	"engine/src/components/*.cpp"
	"engine/src/systems/*.cpp"
	"engine/src/imgui/*.cpp"
	"engine/src/imgui/*.c"
)

file(GLOB SHADERS
	"engine/src/shaders/*.glsl"
)

if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${SHADERS})

target_link_libraries(${PROJECT_NAME}
	${SDL2_LIBRARY}
	${ASSIMP_LIBRARY}
	${PHYSFS_LIBRARY}
	${BULLET_LIBRARIES}
)
if (CMAKE_DL_LIBS)
	target_link_libraries(${PROJECT_NAME}
		${CMAKE_DL_LIBS}
	)
endif()
