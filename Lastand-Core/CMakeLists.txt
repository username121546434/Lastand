file(GLOB_RECURSE CORE_SOURCES "src/*.cpp" "src/*.h")

add_library(Lastand-Core STATIC ${CORE_SOURCES})

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
endif()

target_compile_definitions(Lastand-Core PRIVATE $<$<CONFIG:Debug>:DEBUG>)

# Include directories
target_include_directories(Lastand-Core PUBLIC src)
