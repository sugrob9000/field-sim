cmake_minimum_required(VERSION 3.17)
project(field-sim CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

set(src-dir src)
set(libsrc-dir libsrc)
set(exec app)

find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

# Source libraries
file(GLOB_RECURSE src-files CONFIGURE_DEPENDS ${src-dir}/*.cpp)
add_subdirectory(${libsrc-dir} EXCLUDE_FROM_ALL)

# Main app
add_executable(${exec} ${src-files})

# Main app includes relative to the system include path (<SDL2/SDL.h>, not <SDL.h> etc.),
# so we do not use here ${..._INCLUDE_DIR} that find_package populates.
# 3rd-party library sources in ${libsrc-dir} have their own conventions about this,
# as reflected in their separate CMakeLists
target_include_directories(${exec} PRIVATE ${src-dir} ${libsrc-dir})

target_link_libraries(${exec}
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	fmt imgui)

# Helps incremental builds, hurts clean builds due to being a bottleneck
set(precompiled-headers ${src-dir}/math.hpp)
target_precompile_headers(${exec} PRIVATE ${precompiled-headers})

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	set(cxx-warnings -Wall -Wextra -Wpedantic -Wshadow -Wattributes -Wstrict-aliasing)
	target_compile_options(${exec} PRIVATE ${cxx-warnings})
	target_compile_options(${exec} PRIVATE -fmax-errors=1 -fno-exceptions -fno-rtti)
	target_compile_options(${exec} PRIVATE "$<$<CONFIG:Debug>:-Og>")
else()
	message(WARNING "Compiler other than GCC - will miss some options")
endif()

add_custom_target(run
	COMMAND ${exec}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
