cmake_minimum_required(VERSION 3.26)

project(vffish VERSION 0.0.1 LANGUAGES C)

# Find linker flags for specific architecture
find_package(SDL3 REQUIRED)

set(SDL3_DIR ${CMAKE_SOURCE_DIR}/external/include)
include_directories(include ${SDL3_DIR})

file(GLOB VFFISH_MAIN_SRC "vffish_main.c")

add_executable(${PROJECT_NAME} ${VFFISH_MAIN_SRC})
target_link_libraries(vffish PRIVATE SDL3::SDL3)