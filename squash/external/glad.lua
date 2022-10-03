project "*"
    sysincludedirs
    {
        "glad/include",
    }

group "external"
project "glad"
    language "C"
    kind "StaticLib"
    files
    {
        "glad/src/glad.c",
        "glad/include/**.h"
    }

    filter "system:windows"
        location "."
