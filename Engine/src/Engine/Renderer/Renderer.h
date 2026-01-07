#pragma once
#include "VertexArray.h"
#include "glad/glad.h"
#include "glm/glm.hpp"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/OrthographicCamera.h"

namespace Engine
{
	class RenderCommand
	{
	public:
		inline static void SetClearColor(const glm::vec4& color)
		{

			glClearColor(color.r, color.g, color.b, color.a);
		}

		inline static void Clear()
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		inline static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
		{
			glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
		}
	};

	class Renderer
	{
	public:
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

