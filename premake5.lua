workspace "GameEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release"
	}

output_dir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Engine/vendors/glfw/include"
IncludeDir["GLAD"] = "Engine/vendors/glad/include"
IncludeDir["GLM"] = "Engine/vendors/glm"
IncludeDir["ImGui"] = "Engine/vendors/imgui"
IncludeDir["stb_image"] = "Engine/vendors/stb_image"
IncludeDir["entt"] = "Engine/vendors/entt/include"
IncludeDir["nlohmann_json"] = "Engine/vendors/nlohmann_json"
IncludeDir["ImGuizmo"] = "Engine/vendors/ImGuizmo"

include "Engine/vendors/glfw"
include "Engine/vendors/glad"
include "Engine/vendors/imgui"

project "Engine"
	location "Engine"
	kind "StaticLib"
	cppdialect "C++17"
	staticruntime "on"
	language "C++"

	targetdir ("bin/" .. output_dir .. "/%{prj.name}")
	objdir ("bin-int/" .. output_dir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendors/stb_image/**.h",
		"%{prj.name}/vendors/stb_image/**.cpp",
		"%{prj.name}/vendors/glm/glm/**.hpp",
		"%{prj.name}/vendors/glm/glm/**.inl",
		"%{prj.name}/vendors/nlohmann_json/**.hpp",
		"%{prj.name}/vendors/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendors/ImGuizmo/ImGuizmo.cpp"
	}

	includedirs
	{
		"%{prj.name}/vendors",
		"%{prj.name}/src",
        "%{IncludeDir.GLFW}",
		"%{IncludeDir.GLAD}",
		"%{IncludeDir.GLM}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

    pchheader "egpch.h"
    pchsource "Engine/src/egpch.cpp"

	filter "files:**/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"ENGINE_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}


	filter "configurations:Debug"
		defines {"ENGINE_DEBUG", "ENGINE_ENABLE_ASSERTS"}
		symbols "on"
		
	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		optimize "on"
		

project "Editor"
	location "Editor"
	kind "ConsoleApp"
	cppdialect "C++17"
	staticruntime "on"
	language "C++"

	targetdir ("bin/" .. output_dir .. "/%{prj.name}")
	objdir ("bin-int/" .. output_dir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Engine/vendors",
		"Engine/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.GLAD}",
		"%{IncludeDir.GLM}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"Engine"
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		symbols "on"
		
	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		optimize "on"


project "Game"
	location "Game"
	kind "ConsoleApp"
	cppdialect "C++17"
	staticruntime "on"
	language "C++"

	targetdir ("bin/" .. output_dir .. "/%{prj.name}")
	objdir ("bin-int/" .. output_dir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Engine/vendors",
		"Engine/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.GLAD}",
		"%{IncludeDir.GLM}",
		"%{IncludeDir.entt}"
	}

	links
	{
		"Engine"
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		symbols "on"
		
	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		optimize "on"