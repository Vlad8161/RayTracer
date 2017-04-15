cmake_minimum_required(VERSION 3.6)
project(ray_tracing)

find_package(OpenGL REQUIRED)

add_subdirectory(glfw)

include_directories(glfw/include)
include_directories(glm)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

set(SOURCE_FILES main.cpp ImageBitmap.h Scene.h Scene.cpp)
add_executable(ray_tracing ${SOURCE_FILES})

target_link_libraries(ray_tracing glfw ${GLFW_LIBRARIES} ${OPENGL_LIBRARY})
