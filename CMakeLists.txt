cmake_minimum_required(VERSION 3.26)

project(vffish VERSION 0.0.1 LANGUAGES C CXX)

# Force -std=c++17, because VMA uses constexpr.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(EXTERNAL_H "external/include/")

include_directories(include ${EXTERNAL_H})

# Find SDL3 + cglm on Windows, Linux.
if(!APPLE)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL3 REQUIRED sdl3)
	find_package(cglm REQUIRED)
endif()

# Find SDL3 + cglm on APPLE
if(APPLE)
	find_package(SDL3 REQUIRED)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(CGLM REQUIRED cglm)
	link_directories(${CGLM_LIBRARY_DIRS})
	include_directories(${CGLM_INCLUDE_DIRS})
endif()

find_package(Vulkan REQUIRED)

file(GLOB VFFISH_MAIN_SRC "src/vffish_main.c")
file(GLOB VOLK_SRC "external/include/Volk/volk.c")
file(GLOB VMA_SRC "external/include/VMA/vma_implementation.cpp")

# Silent compile warnings for VMA.
set_source_files_properties(${VMA_SRC} PROPERTIES COMPILE_FLAGS "-w")

add_executable(${PROJECT_NAME} ${VFFISH_MAIN_SRC} ${VOLK_SRC} ${VMA_SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE
	${SDL3_LIBRARIES}
	${CGLM_LIBRARIES}
	${CMAKE_DL_LIBS}
)

if(APPLE)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		"-framework Foundation"
		"-framework QuartzCore"
		"-framework Metal"
		"-framework AppKit"
	)
endif()

# Turn on flags for better debugging.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -g)
endif()

# For integration, unity testing.
enable_testing()

add_executable(run_tests
	"external/include/Unity/unity.c"
	"tests/test_basic_math.c"
)

# Make vulkan.h to not collide with volk.
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_NO_PROTOTYPES)

# Custom platform allocation extensions.
target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<PLATFORM_ID:Darwin>:ALLOCATION_OPTIONAL_EXTENSION=1>
	$<$<PLATFORM_ID:Windows>:ALLOCATION_OPTIONAL_EXTENSION=0>
	$<$<PLATFORM_ID:Linux>:ALLOCATION_OPTIONAL_EXTENSION=0>
)