#pragma once

#include <string>
#include <memory>
#include "Engine/Core.h"
#include "glad/glad.h"

namespace Engine {

	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F
	};

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Texture2D ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Texture2D
	{
	public:

		static std::shared_ptr<Texture2D> Create(const TextureSpecification& specification);
		static std::shared_ptr<Texture2D> Create(const std::string& path);
		~Texture2D();

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetRendererID() const { return m_RendererID; }
		const std::string& GetPath() const { return m_Path; }
		const TextureSpecification& GetSpecification() const { return m_Specification; }

		void SetData(void* data, uint32_t size);

		void Bind(uint32_t slot = 0) const;

		bool operator==(const Texture2D& other) const
		{
			return m_RendererID == other.GetRendererID();
		}
	private:
		inline static std::unordered_map<std::string, std::shared_ptr<Texture2D>> TextureRegistry;

		Texture2D(const std::string& path);
		Texture2D(const TextureSpecification& specification);

		TextureSpecification m_Specification;

		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;

	};

}