#pragma once

#include <filesystem>

#include "Engine/Renderer/Texture.h"

namespace Engine {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;

		std::shared_ptr<Texture2D> m_DirectoryIcon;
		std::shared_ptr<Texture2D> m_FileIcon;
	};

}