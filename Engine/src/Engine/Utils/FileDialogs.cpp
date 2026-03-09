#include "egpch.h"
#include "FileDialogs.h"
//#include <shlobj.h> // Required for SHBrowseForFolder
#include <shobjidl.h> // Required for IFileOpenDialog
#include <filesystem>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <thread> // Ensure this is included at the top
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
        std::string resultPath = "";

        // 1. Wrap the entire dialog process in a lambda function
        auto dialogThreadFunc = [&resultPath]()
            {
                // 2. Initialize COM safely as STA on this NEW, untouched thread
                HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
                if (FAILED(hr))
                {
                    ENGINE_LOG_ERROR("Failed to initialize COM on dialog thread.");
                    return;
                }

                IFileOpenDialog* pFileOpen;
                hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

                if (SUCCEEDED(hr))
                {
                    DWORD dwOptions;
                    if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
                    {
                        pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
                    }

                    std::filesystem::path initialPath = std::filesystem::absolute("Projects");
                    if (!std::filesystem::exists(initialPath))
                        std::filesystem::create_directories(initialPath);

                    IShellItem* pDefaultFolderItem = NULL;
                    std::wstring widePath = initialPath.wstring();

                    hr = SHCreateItemFromParsingName(widePath.c_str(), NULL, IID_PPV_ARGS(&pDefaultFolderItem));
                    if (SUCCEEDED(hr))
                    {
                        pFileOpen->SetFolder(pDefaultFolderItem);
                        pDefaultFolderItem->Release();
                    }

                    // 3. Passing the HWND across threads is perfectly legal and keeps the dialog modal
                    //HWND hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
                    hr = pFileOpen->Show(NULL);

                    if (SUCCEEDED(hr))
                    {
                        IShellItem* pItem;
                        if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
                        {
                            PWSTR pszFilePath;
                            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                            {
                                std::wstring ws(pszFilePath);
                                resultPath = std::string(ws.begin(), ws.end());
                                CoTaskMemFree(pszFilePath);
                            }
                            pItem->Release();
                        }
                    }
                    pFileOpen->Release();
                }

                // 4. Clean up the thread's COM instance safely
                CoUninitialize();
            };

        // 5. Spawn the thread, run the lambda, and halt the main thread until the user picks a folder
        std::thread dialogThread(dialogThreadFunc);
        dialogThread.join();

        return resultPath;
    }

}