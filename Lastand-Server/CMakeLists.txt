file(GLOB_RECURSE SERVER_SOURCES "src/*.cpp" "src/*.h")

add_executable(Lastand-Server ${SERVER_SOURCES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

# Include directories
target_include_directories(Lastand-Server PRIVATE 
    src 
    ../Lastand-Core/src
    ../ext/enet/include
)

# Link libraries
target_link_libraries(Lastand-Server PRIVATE Lastand-Core enet)

if(MSVC)
    target_link_libraries(Lastand-Server PRIVATE ws2_32 winmm)
endif()


# Link enet
target_link_directories(Lastand-Server PRIVATE ../ext/ENet/lib)

