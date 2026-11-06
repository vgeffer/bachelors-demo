cmake_minimum_required(VERSION 3.30.0)
project(BachelorsDemo C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(Lib/Engine)

add_executable(demo entry.cpp)
target_include_directories(demo PRIVATE Lib/Engine)
target_link_libraries(demo PgrEngine::Engine)

# Copy Engine's dll to the exe (Windows only)
if (WIN32)
    add_custom_command(TARGET demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:PgrEngine::Engine>
            $<TARGET_FILE_DIR:demo>
    )
endif()

# Copy assets to the executable
add_custom_command(TARGET demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:demo>/Assets
)