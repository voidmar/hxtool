project "*"
    sysincludedirs
    {
        "imgui",
    }

group "external"
project "imgui"
    language "C++"
    kind "StaticLib"
    files
    {
        "imgui/imgui.cpp",
        "imgui/imgui_draw.cpp",
        "imgui/imgui_demo.cpp",
        "imgui/*.h"
    }

    filter "system:windows"
        location "."
