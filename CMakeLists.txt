cmake_minimum_required(VERSION 3.7)
project(engine C CXX)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")

if (NOT EXISTS "extern/physfs" OR NOT EXISTS "extern/assimp")
	add_custom_target(
		GitSubmoduleInit
		COMMAND git submodule update --init --recursive
	)
	add_dependencies(${engine} GitSubmoduleInit)
endif()

set(PHYSFS_BUILD_STATIC ON)
set(PHYSFS_BUILD_SHARED OFF)
set(PHYSFS_BUILD_TEST OFF)
add_subdirectory("extern/physfs")

set(ASSIMP_BUILD_NO_EXPORT ON)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)

set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_FBX_IMPORTER ON)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_PLY_IMPORTER ON)
set(ASSIMP_BUILD_BLEND_IMPORTER ON)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
add_subdirectory("extern/assimp")

find_package(SDL2 REQUIRED)

include_directories(
	${SDL2_INCLUDE_DIRS}
	${ASSIMP_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/extern/physfs/src
	${CMAKE_SOURCE_DIR}/extern/assimp/include
)

file(GLOB SRC
	"engine/src/*.h"
	"engine/src/*.cpp"
	"engine/src/core/*.h"
	"engine/src/core/*.hpp"
	"engine/src/core/*.cpp"
	"engine/src/core/logging/*.h"
	"engine/src/core/logging/*.cpp"
	"engine/src/core/logging/*.hpp"
	"engine/src/core/im3d/*.h"
	"engine/src/core/im3d/*.cpp"
	"engine/src/core/im3d/*.hpp"
	"engine/src/gfx/*.h"
	"engine/src/gfx/*.cpp"
	"engine/src/gfx/glad/*.h"
	"engine/src/gfx/glad/*.c"
	"engine/src/gfx/stb/*.h"
	"engine/src/gfx/stb/*.c"
	"engine/src/math/*.h"
	"engine/src/math/*.cpp"
	"engine/src/components/*.h"
	"engine/src/components/*.cpp"
	"engine/src/systems/*.h"
	"engine/src/systems/*.cpp"
	"engine/src/shaders/*.glsl"
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
	physfs-static
	assimp
	${SDL2_LIBRARIES}
)