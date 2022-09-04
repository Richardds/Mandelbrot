cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

project(Mandelbrot VERSION 0.0.1
        DESCRIPTION "OpenGL Mandelbrot demo"
        LANGUAGES CXX)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(Mandelbrot Mandelbrot.cpp)
target_link_libraries(Mandelbrot OpenGL::GL GLEW::GLEW glfw)
