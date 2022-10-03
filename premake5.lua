workspace "hxtool"
    configurations { "Debug", "Release"}
    targetdir "bin/%{cfg.buildcfg}"
    objdir "obj/%{cfg.buildcfg}"
    startproject "hxtool"
    debugdir "."

    filter "system:not windows"
        location "build"

    filter "system:windows"
        location "."
        platforms { "x64" }

    filter { "system:windows", "language:not C#" }
        defines {
            "_WIN32",
            "_WIN32_WINNT=0x0601",
            "_CRT_SECURE_NO_WARNINGS",
        }

    filter { "platforms:x64", "language:not C#" }
        targetsuffix "64"

    filter { "system:not windows", "language:C++" }
        buildoptions { "-std=c++11" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        symbols "On"

    include "external/glfw"
    include "external/glad"
    include "external/glm"
    include "external/imgui"
    include "hxtool"