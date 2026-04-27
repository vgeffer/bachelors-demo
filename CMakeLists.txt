cmake_minimum_required(VERSION 3.30.0)
project(raytracer C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fix as OpenVDB is STOOPID
set(OPENVDB_CMAKE_PATH "/System/Volumes/Data/opt/homebrew/Cellar/openvdb/13.0.0_1/lib/cmake/OpenVDB")
list(APPEND CMAKE_MODULE_PATH "${OPENVDB_CMAKE_PATH}")

find_package(glm CONFIG REQUIRED)
find_package(glfw3 3.4 REQUIRED)
find_package(OpenVDB REQUIRED openvdb)

#include(${OPENVDB_CMAKE}/FindBlosc.cmake)
#include(${OPENVDB_CMAKE}/FindOpenVDB.cmake)

add_executable(raytracer
    "camera.cpp"
    "camera.hpp"
    "raytracer.cpp"
    "raytracer.hpp"
    "lib/glad/glad.c"
    "lib/glad/glad.h"
    "lib/glad/khrplatform.h"
    "lib/glpix.hpp"
    "lib/glpix.cpp"
)

target_link_libraries(raytracer PRIVATE glm::glm glfw OpenVDB::openvdb)


