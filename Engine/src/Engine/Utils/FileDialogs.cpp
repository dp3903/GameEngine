#include "egpch.h"
#include "FileDialogs.h"
#include <shlobj.h> // Required for SHBrowseForFolder
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Engine/Application.h"

namespace Engine {

	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	std::string FileDialogs::SaveFile(const char* filter, const char* extension)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (strlen(extension) != 0)
			ofn.lpstrDefExt = extension;

		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	std::string FileDialogs::SelectDirectory()
	{
		char path[MAX_PATH];
		WCHAR wpath[MAX_PATH]; // Used for converting result

		// 1. Setup the Browse Info Struct (Similar to OPENFILENAME)
		BROWSEINFOA bi = { 0 };
		bi.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		bi.lpszTitle = "Select Project Directory";
		// BIF_RETURNONLYFSDIRS ensures the user can only select file system directories
		// BIF_NEWDIALOGSTYLE gives it a resizable window (better UI)
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

		// 2. Show the Dialog
		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

		if (pidl != 0)
		{
			// 3. Convert the result (PIDL) to a string path
			if (SHGetPathFromIDListA(pidl, path))
			{
				// Free the PIDL memory allocated by Windows
				IMalloc* imalloc = 0;
				if (SUCCEEDED(SHGetMalloc(&imalloc)))
				{
					imalloc->Free(pidl);
					imalloc->Release();
				}
				return std::string(path);
			}
		}

		return std::string();
	}

}