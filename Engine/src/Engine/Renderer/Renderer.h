#pragma once
#include "VertexArray.h"
#include "glm/glm.hpp"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/OrthographicCamera.h"

namespace Engine
{
	class RenderCommand
	{
	public:
		static void Init();

		static void SetClearColor(const glm::vec4& color);
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		static void Clear();

		static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray);
	};

	class Renderer
	{
	public:
		static void Init();
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);
		static void EndScene();

		static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform);

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};
}

