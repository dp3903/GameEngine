#pragma once
#include "VertexArray.h"
#include "glm/glm.hpp"
#include "Engine/Renderer/Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "EditorCamera.h"
#include <Engine/Scene/Components.h>
#include "Font.h"

namespace Engine
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// RenderCommand /////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class RenderCommand
	{
	public:
		static void Init();

		static void SetClearColor(const glm::vec4& color);
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		static void Clear();

		static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);
		static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount);
		static void SetLineWidth(float width);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Renderer //////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Renderer
	{
	public:
		static void Init();
		static void OnWindowResize(uint32_t width, uint32_t height);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Renderer2D ///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void FlushQuads();
		static void FlushCircles();
		static void FlushLines();
		static void FlushText();

		// Primitives
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), const glm::vec2& uv0 = glm::vec2(0.0), const glm::vec2& uv1 = glm::vec2(1.0), int entityID = -1);

		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), const glm::vec2& uv0 = glm::vec2(0.0), const glm::vec2& uv1 = glm::vec2(1.0));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), const glm::vec2& uv0 = glm::vec2(0.0), const glm::vec2& uv1 = glm::vec2(1.0));

		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), const glm::vec2& uv0 = glm::vec2(0.0), const glm::vec2& uv1 = glm::vec2(1.0));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), const glm::vec2& uv0 = glm::vec2(0.0), const glm::vec2& uv1 = glm::vec2(1.0));
		
		static void DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID);

		static void DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		static void DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color, int entityID = -1);

		static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID = -1);
		static void DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

		static float GetLineWidth();
		static void SetLineWidth(float width);

		struct TextParams
		{
			glm::vec4 Color{ 1.0f };
			float Kerning = 0.0f;
			float LineSpacing = 0.0f;
			float Scale = 1.0f;
			glm::vec2 Allign{ 0.0f, 0.0f };
		};
		static void DrawString(const std::string& string, std::shared_ptr<Font> font, const glm::mat4& transform, const TextParams& textParams, int entityID = -1);
		static void DrawString(const std::string& string, const glm::mat4& transform, const TextComponent& component, int entityID = -1);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};
		static void ResetStats();
		static Statistics GetStats();
	
	private:
		static void StartQuadsBatch();
		static void StartCirclesBatch();
		static void StartLinesBatch();
		static void StartTextBatch();
		static void NextQuadsBatch();
		static void NextCirclesBatch();
		static void NextLinesBatch();
		static void NextTextBatch();
	};
}

