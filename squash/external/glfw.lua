project "*"
    sysincludedirs
    {
        "glfw/include",
    }

    filter "system:macosx"
        links
        {
            "Cocoa.framework",
            "IOKit.framework",
            "CoreFoundation.framework",
            "CoreVideo.framework",
        }

    filter "system:linux"
        links
        {
            "GL",
            "X11",
            "Xinerama",
            "Xrandr",
            "Xcursor",
            "dl",
            "pthread",
        }

group "external"
project "GLFW"
    language "C"
    kind "StaticLib"
    
    includedirs
    {
        "glfw/include",
        "glfw/src"
    }
    
    files
    {
        "glfw/src/context.c",
        "glfw/src/init.c",
        "glfw/src/input.c",
        "glfw/src/monitor.c",
        "glfw/src/vulkan.c",
        "glfw/src/window.c",
        "glfw/src/osmesa_context.c",
    }

    filter "system:windows"
        location "."
        defines
        {
            "_GLFW_WIN32"
        }

        files
        {
            "glfw/src/win32_init.c",
            "glfw/src/win32_joystick.c",
            "glfw/src/win32_monitor.c",
            "glfw/src/win32_time.c",
            "glfw/src/win32_thread.c",
            "glfw/src/win32_window.c",
            "glfw/src/wgl_context.c",
            "glfw/src/egl_context.c",
        }

    filter "system:macosx"
        defines
        {
            "_GLFW_COCOA",
            "_GLFW_USE_MENUBAR",
        }

        files
        {
            "glfw/src/cocoa_init.m",
            "glfw/src/cocoa_joystick.m",
            "glfw/src/cocoa_monitor.m",
            "glfw/src/cocoa_window.m",
            "glfw/src/cocoa_time.c",
            "glfw/src/posix_tls.c",
            "glfw/src/nsgl_context.m",
        }

    filter "system:linux"
        defines
        {
            "_GLFW_X11"
        }

        files {
            "glfw/src/x11_init.c",
            "glfw/src/x11_monitor.c",
            "glfw/src/x11_window.c",
            "glfw/src/xkb_unicode.c",
            "glfw/src/linux_joystick.c",
            "glfw/src/posix_time.c",
            "glfw/src/posix_tls.c",
            "glfw/src/glx_context.c",
            "glfw/src/egl_context.c",
            "glfw/src/egl_context.c",
        }

