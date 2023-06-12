cmake_minimum_required(VERSION 3.17)
project(field-sim CXX)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(CMAKE_CXX_STANDARD 20)
set(src-dir src)
set(exec app)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Main app
file(GLOB_RECURSE src-files CONFIGURE_DEPENDS ${src-dir}/*.cpp ${src-dir}/*.hpp)
add_executable(${exec} ${src-files})

# Main app includes relative to the system include path (<SDL2/SDL.h>, not <SDL.h> etc.),
# so we do not use here ${..._INCLUDE_DIR} that find_package populates.
# 3rd-party library sources in ${libsrc-dir} have their own conventions about this,
# as reflected in their separate CMakeLists

target_include_directories(${exec} PRIVATE ${src-dir})

# SYSTEM to suppress warnings from libraries
target_include_directories(${exec} SYSTEM PRIVATE ${libsrc-dir})

target_link_libraries(${exec}
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	fmt::fmt)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Enabling GCC-specific configuration")

	set(gnu-cxx-warnings -Wall -Wextra -Wpedantic -Wshadow -Wattributes -Wstrict-aliasing)
	target_compile_options(${exec} PRIVATE ${gnu-cxx-warnings} -fmax-errors=1)

	set(gnu-debug-compile-options -fsanitize=undefined -Og)
	set(gnu-debug-link-options -fsanitize=undefined)

	target_compile_options(${exec} PRIVATE $<$<CONFIG:Debug>:${gnu-debug-compile-options}>)
	target_link_options(${exec} PRIVATE $<$<CONFIG:Debug>:${gnu-debug-link-options}>)
else()
	message(STATUS "Compiler other than GCC - will miss some options")
endif()

add_custom_target(run
	DEPENDS ${exec}
	COMMAND ${exec}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
