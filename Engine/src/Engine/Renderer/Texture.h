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
	/// Texture /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;
		virtual const std::string& GetPath() const = 0;
		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Texture2D ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Texture2D : public Texture
	{
	public:
		static std::shared_ptr<Texture2D> Create(const TextureSpecification& specification);
		static std::shared_ptr<Texture2D> Create(const std::string& path);
		~Texture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }
		virtual const std::string& GetPath() const override { return m_Path; }
		virtual const TextureSpecification& GetSpecification() const { return m_Specification; }

		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;

		bool operator==(const Texture& other) const
		{
			return m_RendererID == other.GetRendererID();
		}
	private:
		Texture2D(const std::string& path);
		Texture2D(const TextureSpecification& specification);

		TextureSpecification m_Specification;

		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

}