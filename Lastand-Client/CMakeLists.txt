file(GLOB_RECURSE CLIENT_SOURCES "src/*.cpp" "src/*.h")

add_executable(Lastand-Client ${CLIENT_SOURCES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

add_subdirectory(../ext/SDL3 EXLUDE_FROM_ALL)

# Include directories
target_include_directories(Lastand-Client PRIVATE 
    src 
    ../Lastand-Core/src
    ../ext/SDL3/include
    ../ext/enet/include
)

# Link libraries
target_link_libraries(Lastand-Client PRIVATE 
    Lastand-Core 
    SDL3::SDL3
    enet
)

if(MSVC)
    target_link_libraries(Lastand-Server PRIVATE ws2_32 winmm)
endif()

# Handle copying SDL3 DLLs
if(WIN32)
    add_custom_command(
        TARGET Lastand-Client POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:Lastand-Client>
        VERBATIM
    )
endif()
