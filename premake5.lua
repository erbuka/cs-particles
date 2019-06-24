workspace "Particles"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    startproject "Particles"

    filter "configurations:Debug"
        symbols "On"
        optimize "Off"
    
    filter "configurations:Release"
        symbols "Off"
        optimize "On"
    
    filter "system:windows"
        systemversion "latest"
    
project "ImGui"
    kind "StaticLib"
    language "C++"
    location "vendor/imgui"

    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }
    
    includedirs { 
        "vendor/imgui", 
        "vendor/glfw/include", 
        "vendor/glad/include" 
    }

    libdirs { "vendor/glfw/lib" }

    links { "glfw3", "Glad" }

    files { "vendor/imgui/**.cpp" }


project "Glad"
    kind "StaticLib"
    language "C++"
    location "vendor/glad"

    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    
    includedirs { "vendor/glad/include" }
    files { "vendor/glad/src/**.c" }

project "Particles"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    location "Particles"

    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    
    includedirs {
        "vendor/glad/include",
        "vendor/glfw/include",
        "vendor/imgui",
        "vendor/spdlog/include",
        "vendor/glm"
    }

    libdirs { "vendor/glfw/lib" }

    links { "Glad", "ImGui", "OpenGL32", "glfw3" }

    files { "Particles/**.cpp", "Particles/**.h" }

    