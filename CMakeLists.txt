cmake_minimum_required(VERSION 3.7)
project(engine C CXX)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")

find_package(PhysFS REQUIRED)
find_package(Assimp REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/engine/deps/assimp/include
	${CMAKE_SOURCE_DIR}/engine/deps/physfs/include
	${SDL2_INCLUDE_DIRS}
	${ASSIMP_INCLUDE_DIR}
	${PHYSFS_INCLUDE_DIR}
)
link_directories(
	${CMAKE_SOURCE_DIR}/engine/deps/assimp/lib
	${CMAKE_SOURCE_DIR}/engine/deps/physfs/lib
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
	"engine/src/gfx/*.h"
	"engine/src/gfx/*.cpp"
	"engine/src/gfx/glad/*.h"
	"engine/src/gfx/glad/*.c"
	"engine/src/math/*.h"
	"engine/src/math/*.cpp"
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
	${SDL2_LIBRARIES}
	${ASSIMP_LIBRARY}
	${PHYSFS_LIBRARY}
)