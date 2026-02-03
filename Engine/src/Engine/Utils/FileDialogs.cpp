#include "egpch.h"
#include "FileDialogs.h"
//#include <shlobj.h> // Required for SHBrowseForFolder
#include <shobjidl.h> // Required for IFileOpenDialog
#include <filesystem>
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
        std::string resultPath;
        IFileOpenDialog* pFileOpen;

        // 1. Initialize COM Library (Required for these modern dialogs)
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr)) return "";

        // 2. Create the FileOpenDialog object
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // 3. Configure the dialog to pick FOLDERS, not files
            DWORD dwOptions;
            if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
            {
                pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }

            // 4. Set the Default Folder (Modern Way)
            // We need a specialized "Shell Item" (IShellItem) to set the folder.
            std::filesystem::path initialPath = std::filesystem::absolute("Projects");
            if (!std::filesystem::exists(initialPath))
                std::filesystem::create_directories(initialPath);

            IShellItem* pDefaultFolderItem = NULL;
            // Convert std::string path to Wide String (LPCWSTR) for Windows API
            std::wstring widePath = initialPath.wstring();

            hr = SHCreateItemFromParsingName(widePath.c_str(), NULL, IID_PPV_ARGS(&pDefaultFolderItem));
            if (SUCCEEDED(hr))
            {
                pFileOpen->SetFolder(pDefaultFolderItem);
                pDefaultFolderItem->Release();
            }

            // 5. Show the Dialog
            // Pass the GLFW window handle so the dialog blocks input to the engine
            HWND hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
            hr = pFileOpen->Show(hwndOwner);

            // 6. Get the Result
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    // Get the path string from the item
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // 7. Success! Convert back to std::string
                    if (SUCCEEDED(hr))
                    {
                        // Quick-and-dirty conversion from Wide char to Std String
                        std::wstring ws(pszFilePath);
                        resultPath = std::string(ws.begin(), ws.end());

                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }

        // 8. Uninitialize COM
        CoUninitialize();

        return resultPath;
	}

}