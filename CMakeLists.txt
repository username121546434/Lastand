cmake_minimum_required(VERSION 3.20)

project(Lastand LANGUAGES CXX)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set debug-specific compiler definitions
target_compile_definitions(Lastand-Core PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(Lastand-Client PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(Lastand-Server PRIVATE $<$<CONFIG:Debug>:DEBUG>)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ext/enet)

# Add subdirectories for core, client, and server
add_subdirectory(Lastand-Core)
add_subdirectory(Lastand-Client)
add_subdirectory(Lastand-Server)
