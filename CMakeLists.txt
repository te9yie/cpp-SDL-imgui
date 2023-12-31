cmake_minimum_required(VERSION 3.14)
project(game)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# SDL
FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-2.28.x
    GIT_SHALLOW TRUE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL)

# SDL_image
FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image
    GIT_TAG release-2.6.x
    GIT_SHALLOW TRUE)
set(SDL2IMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL_image)

# SDL_ttf
FetchContent_Declare(
    SDL_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf
    GIT_TAG release-2.20.x
    GIT_SHALLOW TRUE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL_ttf)

# SDL_net
FetchContent_Declare(
    SDL_net
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_net
    GIT_TAG release-2.2.x
    GIT_SHALLOW TRUE)
set(SDL2NET_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL_net)

# imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.89.8)
FetchContent_MakeAvailable(imgui)

file(GLOB imgui_sources
    ${imgui_SOURCE_DIR}/*.h
    ${imgui_SOURCE_DIR}/*.cpp)
file(GLOB imgui_backends
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.*
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.*)

source_group(imgui FILES ${imgui_sources} ${imgui_backends})

# game
file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE headers src/*.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX ${PROJECT_NAME}
    FILES ${headers} ${sources})

add_executable(${PROJECT_NAME} WIN32
    ${imgui_sources} ${imgui_backends}
    ${sources})
target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_17)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-W -Wall>)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:DEBUG>>:_CRTDBG_MAP_ALLOC>)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends
    src)
target_precompile_headers(${PROJECT_NAME} PRIVATE
    src/pch.h)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2-static SDL2::SDL2main
    SDL2_image::SDL2_image-static
    SDL2_ttf::SDL2_ttf-static
    SDL2_net::SDL2_net-static)
