cmake_minimum_required(VERSION 3.17)
project(field-sim CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
	message(STATUS "LTO enabled")
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(CMAKE_CXX_STANDARD 20)

option(ENABLE_CLANG_TIDY OFF)
if(ENABLE_CLANG_TIDY)
	find_program(CLANG_TIDY clang-tidy)
	if(CLANG_TIDY)
		file(STRINGS clang-tidy-checks.txt checks)
		list(JOIN checks "," checks-joined)
		set_property(TARGET ${exec} PROPERTY
			CXX_CLANG_TIDY ${CLANG_TIDY}
			--quiet
			--header-filter="^${CMAKE_SOURCE_DIR}/${src-dir}"  # shut up about libraries
			--checks=${checks-joined}
			-p ${CMAKE_BINARY_DIR})
	endif()
endif()

set(src-dir src)
set(libsrc-dir libsrc)
set(exec app)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(${libsrc-dir} EXCLUDE_FROM_ALL)

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
	fmt imgui)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	message(STATUS "Enabling GCC-specific configuration")

	set(cxx-warnings -Wall -Wextra -Wpedantic -Wshadow -Wattributes -Wstrict-aliasing)
	target_compile_options(${exec} PRIVATE ${cxx-warnings})
	target_compile_options(${exec} PRIVATE -fmax-errors=1 -fno-exceptions -fno-rtti)
	target_compile_options(${exec} PRIVATE "$<$<CONFIG:Debug>:-Og>")

	if (CMAKE_BUILD_TYPE STREQUAL Debug)
		target_compile_options(${exec} PRIVATE -fsanitize=undefined)
		target_link_options(${exec} PRIVATE -fsanitize=undefined)
	endif()
else()
	message(STATUS "Compiler other than GCC - will miss some options")
endif()

add_custom_target(run
	DEPENDS ${exec}
	COMMAND ${exec}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
