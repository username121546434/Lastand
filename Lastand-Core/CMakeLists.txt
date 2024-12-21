file(GLOB_RECURSE CORE_SOURCES "src/*.cpp" "src/*.h")

add_library(Lastand-Core STATIC ${CORE_SOURCES})

# Include directories
target_include_directories(Lastand-Core PUBLIC src)
