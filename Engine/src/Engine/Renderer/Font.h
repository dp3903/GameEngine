#pragma once

#include <filesystem>

#include "MSDFData.h"
#include "Engine/Renderer/Texture.h"


namespace Engine {

	class Font
	{
	public:
		static std::shared_ptr<Font> Create(const std::filesystem::path& filepath = "assets/fonts/opensans/OpenSans-Regular.ttf");
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		std::shared_ptr<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		std::filesystem::path GetFilePath() { return m_Filepath; }
	private:
		Font(const std::filesystem::path& filepath);

		MSDFData* m_Data;
		std::shared_ptr<Texture2D> m_AtlasTexture;
		std::filesystem::path m_Filepath;

		inline static std::unordered_map<std::string, std::shared_ptr<Font>> FontCache;
	};

}