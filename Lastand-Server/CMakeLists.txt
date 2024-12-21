file(GLOB_RECURSE SERVER_SOURCES "src/*.cpp" "src/*.h")

add_executable(Lastand-Server ${SERVER_SOURCES})

# Include directories
target_include_directories(Lastand-Server PRIVATE 
    src 
    ../Lastand-Core/src
    ../ext/enet/include
)

# Link libraries
target_link_libraries(Lastand-Server PRIVATE Lastand-Core enet)

# Link GameNetworkingSockets
target_link_directories(Lastand-Server PRIVATE ../ext/ENet/lib)

