cmake_minimum_required(VERSION 3.6)
project(ray_tracing)

find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)

add_subdirectory(glfw)

include_directories(glfw/include)
include_directories(glm)
include_directories(${OpenCL_INCLUDE_DIRS})

link_directories(${OpenCL_LIBRARY})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

set(SOURCE_FILES
        lib/json.h
        lib/lodepng.h
        lib/lodepng.cpp
        scene.h
        scene.cpp
        ray_tracer.h
        ray_tracer.cpp
        opencl_executor.h
        opencl_executor.cpp
        image_bitmap.h
        main.cpp
        tex_image.h synchronized_queue.h config.h ray_tracer_cl.cpp ray_tracer_cl.h)
add_executable(ray_tracing ${SOURCE_FILES})

target_include_directories(ray_tracing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ray_tracing glfw ${GLFW_LIBRARIES} ${OPENGL_LIBRARY} ${OpenCL_LIBRARY})
