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
IncludeDir["GLFW"] =	 		"%{wks.location}/Engine/vendors/glfw/include"
IncludeDir["GLAD"] = 	 		"%{wks.location}/Engine/vendors/glad/include"
IncludeDir["GLM"] =		 		"%{wks.location}/Engine/vendors/glm"
IncludeDir["Box2D"] =	 		"%{wks.location}/Engine/vendors/box2d/include"
IncludeDir["ImGui"] =	 		"%{wks.location}/Engine/vendors/imgui"
IncludeDir["stb_image"] = 		"%{wks.location}/Engine/vendors/stb_image"
IncludeDir["entt"] =			"%{wks.location}/Engine/vendors/entt/include"
IncludeDir["nlohmann_json"] = 	"%{wks.location}/Engine/vendors/nlohmann_json"
IncludeDir["ImGuizmo"] =		"%{wks.location}/Engine/vendors/ImGuizmo"
IncludeDir["msdfgen"] = 		"%{wks.location}/Engine/vendors/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = 	"%{wks.location}/Engine/vendors/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["lua"] = 			"%{wks.location}/Engine/vendors/lua/include"
IncludeDir["sol"] = 			"%{wks.location}/Engine/vendors/sol2/include"

include "Engine/vendors/glfw"
include "Engine/vendors/glad"
include "Engine/vendors/imgui"
include "Engine/vendors/box2D"
include "Engine/vendors/msdf-atlas-gen"
include "Engine/vendors/lua"

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
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.sol}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"Box2D",
		"ImGui",
		"msdf-atlas-gen",
		"Lua",
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
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.lua}",
		"%{IncludeDir.sol}"
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