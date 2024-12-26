file(GLOB_RECURSE CORE_SOURCES "src/*.cpp" "src/*.h")
target_compile_options(Lastand-Client PRIVATE $<$<CXX_COMPILER_ID:GNU>:-static-libstdc++ -static-libgcc>)

add_library(Lastand-Core STATIC ${CORE_SOURCES})

# Include directories
target_include_directories(Lastand-Core PUBLIC src)
