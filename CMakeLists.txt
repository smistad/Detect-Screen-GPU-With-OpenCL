cmake_minimum_required(VERSION 2.8)
project(DetectScreen)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

add_executable(detect main.cpp)

target_link_libraries(detect ${OPENGL_LIBRARIES} ${OPENCL_LIBRARIES})
