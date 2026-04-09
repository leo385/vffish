cmake_minimum_required(VERSION 3.26)

project(vffish VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

# Find linker flags for specific architecture
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)
find_package(Vulkan REQUIRED)

file(GLOB VFFISH_MAIN_SRC "src/vffish_main.c")

add_executable(${PROJECT_NAME} ${VFFISH_MAIN_SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE
	${SDL3_LIBRARIES}
	Vulkan::Vulkan
)

# Turn on flags for better debugging
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# For integration, unity testing
enable_testing()

add_executable(run_tests
	"external/include/Unity/unity.c"
	"tests/test_basic_math.c"
)
