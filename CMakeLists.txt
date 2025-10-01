cmake_minimum_required(VERSION 4.0)

project(pathtracer)

add_executable(
    pathtracer
    "src/main.cpp"
    "src/camera.h"
    "src/image.h"
    "src/material.h"
    "src/math.h"
    "src/primitives.h"
    "src/renderer.h"
    "src/scene.h"
)
set_property(TARGET pathtracer PROPERTY CXX_STANDARD 23)

# vs debug working directory
set_property(TARGET pathtracer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
