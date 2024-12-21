file(GLOB_RECURSE CLIENT_SOURCES "src/*.cpp" "src/*.h")

add_executable(Lastand-Client ${CLIENT_SOURCES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

# Include directories
target_include_directories(Lastand-Client PRIVATE 
    src 
    ../Lastand-Core/src
    ../ext/SDL3/include
    ../ext/enet/include
)

# Link directories
target_link_directories(Lastand-Client PRIVATE 
    ../ext/SDL3/lib
)

# Link libraries
target_link_libraries(Lastand-Client PRIVATE 
    Lastand-Core 
    SDL3
    enet
)

# Handle copying DLLs
add_custom_command(TARGET Lastand-Client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/ext/SDL3/bin $<TARGET_FILE_DIR:Lastand-Client>
)
