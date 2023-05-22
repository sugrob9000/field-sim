file(GLOB_RECURSE imgui-src imgui/*.cpp)
add_library(imgui OBJECT ${imgui-src})
target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})
target_link_libraries(imgui PRIVATE dl)
