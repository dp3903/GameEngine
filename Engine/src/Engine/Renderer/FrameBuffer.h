#pragma once

#include "egpch.h"
#include "Engine/Core.h"

namespace Engine {

	struct FramebufferSpecification
	{
		uint32_t Width, Height;
		// FramebufferFormat Format = 
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		~Framebuffer();
		static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
		
		void Invalidate();

		void Bind();
		void Unbind();

		void Resize(uint32_t width, uint32_t height);

		uint32_t GetColorAttachmentRendererID() const { return m_ColorAttachment; }
		const FramebufferSpecification& GetSpecification() const { return m_Specification; }

	private:
		Framebuffer(const FramebufferSpecification& spec);
		uint32_t m_RendererID = 0;
		uint32_t m_ColorAttachment = 0, m_DepthAttachment = 0;
		FramebufferSpecification m_Specification;
	};


}