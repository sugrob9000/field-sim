cmake_minimum_required(VERSION 3.12)
project(field-sim CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

set(src-dir src)
set(lib-dir libsrc)
set(exec app)

find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE src-files
	${src-dir}/*.cpp
	${src-dir}/*.hpp)
add_subdirectory(${lib-dir} EXCLUDE_FROM_ALL)

add_executable(${exec} ${src-files})
target_include_directories(${exec} PRIVATE
	${src-dir}
	${lib-dir}
	${SDL2_INCLUDE_DIRS}
	${GLEW_INCLUDE_DIRS})
target_link_libraries(${exec}
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	fmt imgui)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	set(cxx-warnings
		-Wall -Wextra -Wpedantic
		-Werror=shadow -Wattributes -Wstrict-aliasing)
	target_compile_options(${exec} PRIVATE ${cxx-warnings}
		-fmax-errors=1
		-fno-exceptions -fno-rtti)
	target_compile_options(${exec} PRIVATE "$<$<CONFIG:Debug>:-Og>")
else()
	message(WARNING "Compiler other than GCC - will miss some options")
endif()
