project(ImGui)

add_library(ImGui STATIC)

target_include_directories(ImGui PUBLIC imgui/ imgui/backends/ ../SDL3/include/)
target_sources(ImGui PRIVATE 
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_sdlrenderer3.cpp
    imgui/misc/cpp/imgui_stdlib.cpp
)

target_link_libraries(ImGui PUBLIC SDL3::SDL3)

