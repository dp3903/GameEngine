#pragma once

#include "egpch.h"

namespace Engine {

	class UniformBuffer
	{
	public:
		static std::shared_ptr<UniformBuffer> Create(uint32_t size, uint32_t binding);
		~UniformBuffer();
		void SetData(const void* data, uint32_t size, uint32_t offset = 0);
	
	private:
		UniformBuffer(uint32_t size, uint32_t binding);

	private:
		uint32_t m_RendererID = 0;
	};

}