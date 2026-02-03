#include "egpch.h"
#include "ProjectSerializer.h"

#include <fstream>
#include <iomanip> // Required for std::setw (Pretty Printing)
#include <nlohmann_json/json.hpp>

using json = nlohmann::ordered_json;

namespace Engine
{
	ProjectSerializer::ProjectSerializer(std::shared_ptr<Project> project)
		: m_Project(project)
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		APP_LOG_INFO("Serializing project info to file: {0} START", std::filesystem::absolute(filepath).string());

		const auto& config = m_Project->GetConfig();

		json projectData;
		projectData["Project"] = {
			{"Name",				config.Name},
			{"StartScene",			config.StartScene},
			{"AssetDirectory",		config.AssetDirectory}
		};

		// Write to file
		std::ofstream fout(filepath);
		// std::setw(4) makes it "Pretty Print" with 4 spaces of indentation
		fout << std::setw(4) << projectData;

		APP_LOG_INFO("Serializing project info FINISH");

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		APP_LOG_INFO("Deserializing project info from file: {0} START", std::filesystem::absolute(filepath).string());

		std::ifstream stream(filepath);
		if (!stream.is_open()) return false;

		json projectData;
		try {
			stream >> projectData;
		}
		catch (const json::parse_error& e) {
			APP_LOG_ERROR("JSON Parse Error: {0}", e.what());
			return false;
		}

		if (!projectData.contains("Project"))
		{
			APP_LOG_ERROR("Failed to load project from file '{0}'.\n Does not contain project object.", filepath.string());
			return false;
		}

		auto project = projectData["Project"];

		auto& config = m_Project->GetConfig();

		try
		{
			config.Name = project["Name"];
			config.StartScene = std::string(project["StartScene"]);
			config.AssetDirectory = std::string(project["AssetDirectory"]);
		}
		catch (const json::exception& e) {
			APP_LOG_ERROR("JSON Error: {0}", e.what());
			return false;
		}

		return true;
	}
}