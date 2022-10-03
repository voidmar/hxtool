project "hxtool"
    location "."
    language "C++"
    kind "WindowedApp"

    files
    {
        "**.cpp",
        "**.h",
    }

    links
    {
        "glad",
        "GLFW",
        "imgui",
        "OpenGL32",
    }