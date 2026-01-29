project "ImGui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
    staticruntime "on"

	targetdir ("bin/" .. output_dir .. "/%{prj.name}")
	objdir ("bin-int/" .. output_dir .. "/%{prj.name}")

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp",
		"imgui_impl_glfw.h",
		"imgui_impl_glfw.cpp",
		"imgui_impl_opengl3.h",
		"imgui_impl_opengl3.cpp",
		"imgui_impl_opengl3_loader.h",
		"imgui_stdlib.h",
		"imgui_stdlib.cpp"
	}

	includedirs
	{
        "%{wks.location}/Engine/vendors/GLFW/include"
	}

	filter "system:windows"
		systemversion "latest"

	filter "system:linux"
		pic "On"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"