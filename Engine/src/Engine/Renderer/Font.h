#pragma once

#include <filesystem>

#include "MSDFData.h"
#include "Engine/Renderer/Texture.h"


namespace Engine {

	class Font
	{
	public:
		Font(const std::filesystem::path& font);
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		std::shared_ptr<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

		static std::shared_ptr<Font> GetDefault();

	private:
		MSDFData* m_Data;
		std::shared_ptr<Texture2D> m_AtlasTexture;
	};

}