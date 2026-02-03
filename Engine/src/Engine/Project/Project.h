#pragma once

#include <string>
#include <filesystem>

#include "Engine/Core.h"

namespace Engine {

	struct ProjectConfig
	{
		std::string Name = "Untitled";

		std::filesystem::path StartScene;

		std::filesystem::path AssetDirectory;
	};

	class Project
	{
	public:
		static const std::filesystem::path& GetProjectDirectory()
		{
			ASSERT(s_ActiveProject, "No Active Project set.");
			return s_ActiveProject->m_ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			ASSERT(s_ActiveProject, "No Active Project set.");
			return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
		}

		// TODO(Yan): move to asset manager when we have one
		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			ASSERT(s_ActiveProject, "No Active Project set.");
			return GetAssetDirectory() / path;
		}

		ProjectConfig& GetConfig() { return m_Config; }

		static std::shared_ptr<Project> GetActive() { return s_ActiveProject; }

		static std::shared_ptr<Project> New();
		static std::shared_ptr<Project> Load(const std::filesystem::path& path);
		static bool SaveActive(const std::filesystem::path& path);
	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		inline static std::shared_ptr<Project> s_ActiveProject;
	};

}