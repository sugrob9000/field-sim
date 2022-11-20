file(GLOB_RECURSE fmt-src fmt/*.cc fmt/*.h)
add_library(fmt STATIC ${fmt-src})
target_include_directories(fmt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE imgui-src imgui/*.cpp imgui/*.h)
add_library(imgui STATIC ${imgui-src})
target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})
