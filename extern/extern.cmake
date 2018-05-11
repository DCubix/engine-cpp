### PHYSFS ###

set(PHYSFS_BUILD_STATIC ON)
set(PHYSFS_BUILD_SHARED OFF)
set(PHYSFS_BUILD_TEST OFF)
add_subdirectory("extern/physfs")

add_library(extern::physfs INTERFACE IMPORTED)
set_target_properties(extern::physfs PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/extern/physfs/src
	INTERFACE_LINK_LIBRARIES physfs-static
)

### ASSIMP ###

set(ASSIMP_BUILD_NO_EXPORT ON CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "DoNotTouch")

set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_PLY_IMPORTER ON CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_BLEND_IMPORTER ON CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "DoNotTouch")

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "DoNotTouch")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "DoNotTouch")
add_subdirectory("extern/assimp")

add_library(extern::assimp INTERFACE IMPORTED)
set_target_properties(extern::assimp PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/extern/assimp/include;${CMAKE_BINARY_DIR}/extern/assimp/include"
	INTERFACE_LINK_LIBRARIES assimp
)

### SDL2 ###
set(SDL_STATIC OFF CACHE BOOL "DoNotTouch")
add_subdirectory("extern/sdl2")

add_library(extern::sdl2 INTERFACE IMPORTED)
set_target_properties(extern::sdl2 PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/extern/sdl2/include
	INTERFACE_LINK_LIBRARIES "SDL2;SDL2main"
)

add_definitions(-DSDL_MAIN_HANDLED)

### GLM ###
set(GLM_STATIC_LIBRARY_ENABLE OFF CACHE BOOL "DoNotTouch")
set(GLM_TEST_ENABLE OFF CACHE BOOL "DoNotTouch")
add_subdirectory("extern/glm")

add_library(extern::glm INTERFACE IMPORTED)
set_target_properties(extern::glm PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/extern/glm
)

add_definitions(-DGLM_FORCE_CTOR_INIT)

if (CMAKE_DL_LIBS)
	### DL ###
	add_library(extern::dl INTERFACE IMPORTED)
	set_target_properties(extern::dl PROPERTIES
		INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS}
	)
else()
	add_library(extern::dl INTERFACE IMPORTED)
endif()