cmake_minimum_required(VERSION 3.30.0)
project(BachelorsDemo C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(Lib/Engine)

add_executable(demo entry.cpp)
target_include_directories(demo PRIVATE Lib/Engine)
target_link_libraries(demo PgrEngine::Engine)
