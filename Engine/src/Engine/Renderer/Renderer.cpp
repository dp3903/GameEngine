#include "egpch.h"
#include "Renderer.h"
#include "Shader.h"
#include "glad/glad.h"
#include <glm/gtc/matrix_transform.hpp>
#include "UniformBuffer.h"

#include "Engine/Utils/Math.h"

namespace Engine {

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Render-command ////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void RenderCommand::Init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
	}

	void RenderCommand::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void RenderCommand::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void RenderCommand::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount==0 ? vertexArray->GetIndexBuffer()->GetCount() : indexCount;
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void RenderCommand::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void RenderCommand::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Renderer //////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Renderer::Init()
	{
		RenderCommand::Init();
		Renderer2D::Init();
		Renderer3D::Init();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Renderer-2D ///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;

		// Editor-only
		int EntityID;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;

		// Editor-only
		int EntityID;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

		// Editor-only
		int EntityID;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		// TODO: bg color for outline/bg

		// Editor-only
		int EntityID;
	};


	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 10000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		std::shared_ptr<Texture2D> WhiteTexture;

		std::shared_ptr<VertexArray> QuadVertexArray;
		std::shared_ptr<VertexBuffer> QuadVertexBuffer;
		std::shared_ptr<Shader> QuadShader;

		std::shared_ptr<VertexArray> CircleVertexArray;
		std::shared_ptr<VertexBuffer> CircleVertexBuffer;
		std::shared_ptr<Shader> CircleShader;

		std::shared_ptr<VertexArray> LineVertexArray;
		std::shared_ptr<VertexBuffer> LineVertexBuffer;
		std::shared_ptr<Shader> LineShader;

		std::shared_ptr<VertexArray> TextVertexArray;
		std::shared_ptr<VertexBuffer> TextVertexBuffer;
		std::shared_ptr<Shader> TextShader;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;
		float LineWidth = 2.0f;

		uint32_t TextIndexCount = 0;
		TextVertex* TextVertexBufferBase = nullptr;
		TextVertex* TextVertexBufferPtr = nullptr;



		std::array<std::shared_ptr<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		std::array<std::shared_ptr<Texture2D>, MaxTextureSlots> FontAtlasTextures;
		uint32_t FontAtlasTextureIndex = 0;

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		std::shared_ptr<UniformBuffer> CameraUniformBuffer;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		// Quads
		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float,  "a_TexIndex" },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});

		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}
		std::shared_ptr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		// Circles
		s_Data.CircleVertexArray = VertexArray::Create();

		s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          },
			{ ShaderDataType::Int,    "a_EntityID"      }
			});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
		s_Data.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
		s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

		// Lines
		s_Data.LineVertexArray = VertexArray::Create();

		s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Int,    "a_EntityID" }
			});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

		// Text
		s_Data.TextVertexArray = VertexArray::Create();

		s_Data.TextVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(TextVertex));
		s_Data.TextVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float,  "a_TexIndex"     },
			{ ShaderDataType::Int,    "a_EntityID"     }
			});
		s_Data.TextVertexArray->AddVertexBuffer(s_Data.TextVertexBuffer);
		s_Data.TextVertexArray->SetIndexBuffer(quadIB);
		s_Data.TextVertexBufferBase = new TextVertex[s_Data.MaxVertices];

		s_Data.WhiteTexture = Texture2D::Create(TextureSpecification());
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));


		s_Data.QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");
		s_Data.CircleShader = Shader::Create("assets/shaders/Renderer2D_Circle.glsl");
		s_Data.LineShader = Shader::Create("assets/shaders/Renderer2D_Line.glsl");
		s_Data.TextShader = Shader::Create("assets/shaders/Renderer2D_Text.glsl");

		// Set all texture slots to 0
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f,  0.0f,  1.0f };
		s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f,  0.0f,  1.0f };
		s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f,  0.0f,  1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f,  0.0f,  1.0f };

		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);
	}

	void Renderer2D::Shutdown()
	{

	}
	
	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartQuadsBatch();
		StartCirclesBatch();
		StartLinesBatch();
		StartTextBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartQuadsBatch();
		StartCirclesBatch();
		StartLinesBatch();
		StartTextBatch();
	}

	void Renderer2D::EndScene()
	{
		FlushQuads();
		FlushCircles();
		FlushLines();
		FlushText();
	}

	void Renderer2D::StartQuadsBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::StartCirclesBatch()
	{
		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;
	}

	void Renderer2D::StartLinesBatch()
	{
		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
	}

	void Renderer2D::StartTextBatch()
	{
		s_Data.TextIndexCount = 0;
		s_Data.TextVertexBufferPtr = s_Data.TextVertexBufferBase;

		s_Data.FontAtlasTextureIndex = 0;
	}

	void Renderer2D::FlushQuads()
	{
		if (s_Data.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
				s_Data.TextureSlots[i]->Bind(i);

			s_Data.QuadShader->Bind();
			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::FlushCircles()
	{
		if (s_Data.CircleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

			s_Data.CircleShader->Bind();
			RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::FlushLines()
	{
		if (s_Data.LineVertexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

			s_Data.LineShader->Bind();
			RenderCommand::SetLineWidth(s_Data.LineWidth);
			RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::FlushText()
	{
		if (s_Data.TextIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TextVertexBufferPtr - (uint8_t*)s_Data.TextVertexBufferBase);
			s_Data.TextVertexBuffer->SetData(s_Data.TextVertexBufferBase, dataSize);

			for(uint32_t i = 0 ; i < s_Data.FontAtlasTextureIndex ; i++)
				s_Data.FontAtlasTextures[i]->Bind(i);

			s_Data.TextShader->Bind();
			RenderCommand::DrawIndexed(s_Data.TextVertexArray, s_Data.TextIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::NextQuadsBatch()
	{
		FlushQuads();
		StartQuadsBatch();
	}

	void Renderer2D::NextCirclesBatch()
	{
		FlushCircles();
		StartCirclesBatch();
	}

	void Renderer2D::NextLinesBatch()
	{
		FlushLines();
		StartLinesBatch();
	}

	void Renderer2D::NextTextBatch()
	{
		FlushText();
		StartTextBatch();
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextQuadsBatch();

		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, const glm::vec2& uv0, const glm::vec2& uv1, int entityID)
	{
		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextQuadsBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextQuadsBatch();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}
		constexpr size_t quadVertexCount = 4;
		glm::vec2 textureCoords[quadVertexCount] = {
			uv0,
			{ uv1.x, uv0.y },
			uv1,
			{ uv0.x, uv1.y }
		};

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, const glm::vec2& uv0, const glm::vec2& uv1)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor, uv0, uv1);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, const glm::vec2& uv0, const glm::vec2& uv1)
	{

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor, uv0, uv1);
	}


	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		
		DrawQuad(transform, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, const glm::vec2& uv0, const glm::vec2& uv1)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor, uv0, uv1);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, const glm::vec2& uv0, const glm::vec2& uv1)
	{
		
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		
		DrawQuad(transform, texture, tilingFactor, tintColor, uv0, uv1);
	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
	{
		if (src.Texture)
		{
			glm::vec2 uv0 = { 0.0f, 0.0f };
			glm::vec2 uv1 = { 1.0f, 1.0f };
			if (src.IsSubTexture)
			{
				uv0 = { (float)(src.XSpriteIndex + 0) / src.SpriteWidth, (float)(src.YSpriteIndex + 0) / src.SpriteHeight };
				uv1 = { (float)(src.XSpriteIndex + 1) / src.SpriteWidth, (float)(src.YSpriteIndex + 1) / src.SpriteHeight };
			}
			if (src.FlipX)
				std::swap(uv0.x, uv1.x);
			if (src.FlipY)
				std::swap(uv0.y, uv1.y);
			DrawQuad(transform, src.Texture, src.TilingFactor, src.Color, uv0, uv1, entityID);
		}
		else
			DrawQuad(transform, src.Color, entityID);
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
	{

		 if (s_Data.CircleIndexCount >= Renderer2DData::MaxIndices)
		 	NextCirclesBatch();

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
			s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		if (s_Data.LineVertexCount >= Renderer2DData::MaxIndices)
			NextLinesBatch();

		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color, entityID);
		DrawLine(p1, p2, color, entityID);
		DrawLine(p2, p3, color, entityID);
		DrawLine(p3, p0, color, entityID);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		glm::vec3 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);
	}

	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::DrawString(const std::string& string, const glm::mat4& transform, const TextComponent& component, int entityID)
	{
		DrawString(string, component.FontAsset, transform, { component.Color, component.Kerning, component.LineSpacing, component.Scale, component.Allign }, entityID);
	}

	void Renderer2D::DrawString(const std::string& string, std::shared_ptr<Font> font, const glm::mat4& transform, const TextParams& textParams, int entityID)
	{
		const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();
		std::shared_ptr<Texture2D> fontAtlas = font->GetAtlasTexture();

		if (s_Data.TextIndexCount >= Renderer2DData::MaxIndices)
			NextTextBatch();

		float textureIndex = -1.0f;
		for (uint32_t i = 0; i < s_Data.FontAtlasTextureIndex; i++)
		{
			if (*s_Data.FontAtlasTextures[i].get() == *fontAtlas.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == -1.0f)
		{
			if (s_Data.FontAtlasTextureIndex >= Renderer2DData::MaxTextureSlots)
				NextTextBatch();

			textureIndex = (float)s_Data.FontAtlasTextureIndex;
			s_Data.FontAtlasTextures[s_Data.FontAtlasTextureIndex] = fontAtlas;
			s_Data.FontAtlasTextureIndex++;
		}

		double XStart = textParams.Allign.x;
		double YStart = textParams.Allign.y;
		double x = XStart;
		double y = YStart;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY) * textParams.Scale;
		
		const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

		for (size_t i = 0; i < string.size(); i++)
		{
			char character = string[i];
			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = XStart;
				y -= fsScale * metrics.lineHeight + (textParams.LineSpacing * textParams.Scale);
				continue;
			}

			if (character == ' ')
			{
				float advance = spaceGlyphAdvance;
				if (i < string.size() - 1)
				{
					char nextCharacter = string[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}

				x += fsScale * advance + (textParams.Kerning * textParams.Scale);
				continue;
			}

			if (character == '\t')
			{
				x += 4.0f * (fsScale * spaceGlyphAdvance + (textParams.Kerning * textParams.Scale));
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph('?');
			if (!glyph)
				return;


			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			glm::vec2 texCoordMin((float)al, (float)ab);
			glm::vec2 texCoordMax((float)ar, (float)at);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			glm::vec2 quadMin((float)pl, (float)pb);
			glm::vec2 quadMax((float)pr, (float)pt);

			quadMin *= fsScale, quadMax *= fsScale;
			quadMin += glm::vec2(x, y);
			quadMax += glm::vec2(x, y);

			float texelWidth = 1.0f / fontAtlas->GetWidth();
			float texelHeight = 1.0f / fontAtlas->GetHeight();
			texCoordMin *= glm::vec2(texelWidth, texelHeight);
			texCoordMax *= glm::vec2(texelWidth, texelHeight);

			// render here
			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin, 0.001f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = texCoordMin;
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin.x, quadMax.y, 0.001f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = { texCoordMin.x, texCoordMax.y };
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax, 0.001f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = texCoordMax;
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax.x, quadMin.y, 0.001f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = { texCoordMax.x, texCoordMin.y };
			s_Data.TextVertexBufferPtr->TexIndex = textureIndex;
			s_Data.TextVertexBufferPtr->EntityID = entityID;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextIndexCount += 6;
			s_Data.Stats.QuadCount++;

			if (i < string.size() - 1)
			{
				double advance = glyph->getAdvance();
				char nextCharacter = string[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				float kerningOffset = 0.0f;
				x += fsScale * advance + (textParams.Kerning * textParams.Scale);
			}
		}
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Renderer-3D ///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	struct Renderer3DData
	{
		static const uint32_t MaxSphereCount = 10;

		// We only need geometry for the Screen Quad now!
		std::shared_ptr<VertexArray> QuadVertexArray;
		std::shared_ptr<VertexBuffer> QuadVertexBuffer;
		std::shared_ptr<IndexBuffer> QuadIndexBuffer;

		std::shared_ptr<Shader> SphereShader;

		// CPU Batching Data (This stays exactly the same!)
		Renderer3D::Sphere* SphereBufferBase = nullptr;
		Renderer3D::Sphere* SphereBufferPtr = nullptr;
		uint32_t SphereCount = 0;

		struct CameraData
		{
			// Raytracers need inverse matrices to calculate per-pixel ray directions
			glm::mat4 InverseProjection;
			glm::mat4 InverseView;
		};
		CameraData CameraBuffer;

		// We now have TWO Uniform Buffers
		std::shared_ptr<UniformBuffer> CameraUniformBuffer;
		std::shared_ptr<UniformBuffer> SphereUniformBuffer;
		
		// For post-processing
		std::shared_ptr<Framebuffer> PingPongFBO[2];
		std::shared_ptr<Shader> BlurShader;
		std::shared_ptr<Shader> CompositeShader;
	};

	static Renderer3DData s_3DData;

	// This struct mirrors the GLSL std140 layout exactly
	struct SphereUBOData
	{
		Renderer3D::Sphere Spheres[Renderer3DData::MaxSphereCount];
		uint32_t SphereCount;
		float padding[3]; // std140 requires the end of the struct to align to 16 bytes
	};


	void Renderer3D::Init()
	{
		// --- Create the Full Screen Quad ---
		float quadVertices[] = {
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f
		};
		uint32_t quadIndices[] = { 0, 1, 2, 2, 3, 0 };

		s_3DData.QuadVertexArray = VertexArray::Create();

		s_3DData.QuadVertexBuffer = VertexBuffer::Create(quadVertices, sizeof(quadVertices));
		s_3DData.QuadVertexBuffer->SetLayout({ { ShaderDataType::Float3, "a_Position" } });
		s_3DData.QuadVertexArray->AddVertexBuffer(s_3DData.QuadVertexBuffer);

		s_3DData.QuadIndexBuffer = IndexBuffer::Create(quadIndices, 6);
		s_3DData.QuadVertexArray->SetIndexBuffer(s_3DData.QuadIndexBuffer);

		// Keep the CPU buffer for batching
		s_3DData.SphereBufferBase = new Sphere[s_3DData.MaxSphereCount];

		// Create Camera UBO at binding point 0
		s_3DData.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);

		// Create Sphere UBO at binding point 1
		s_3DData.SphereUniformBuffer = UniformBuffer::Create(sizeof(SphereUBOData), 1);

		s_3DData.SphereShader = Shader::Create("assets/shaders/Renderer3D_Sphere.glsl");

		// Post-processing assets
		FramebufferSpecification pingpongSpec;
		pingpongSpec.Attachments = { FramebufferTextureFormat::RGBA16F };
		pingpongSpec.Width = 640;
		pingpongSpec.Height = 360;
		s_3DData.PingPongFBO[0] = Framebuffer::Create(pingpongSpec);
		s_3DData.PingPongFBO[1] = Framebuffer::Create(pingpongSpec);
		s_3DData.BlurShader = Shader::Create("assets/shaders/Blur.glsl");
		s_3DData.CompositeShader = Shader::Create("assets/shaders/Composite.glsl");
	}

	void Renderer3D::Shutdown()
	{

	}

	void Renderer3D::BeginScene(const EditorCamera& camera)
	{
		// Pass the matrices so the shader can un-project the screen coordinates
		s_3DData.CameraBuffer.InverseProjection = glm::inverse(camera.GetProjection());
		s_3DData.CameraBuffer.InverseView = glm::inverse(camera.GetViewMatrix());

		s_3DData.CameraUniformBuffer->SetData(&s_3DData.CameraBuffer, sizeof(Renderer3DData::CameraData));

		s_3DData.SphereBufferPtr = s_3DData.SphereBufferBase;
		s_3DData.SphereCount = 0;
	}

	void Renderer3D::EndScene()
	{
		if (s_3DData.SphereCount)
		{
			s_3DData.SphereShader->Bind();

			// 1. Upload standard uniforms (Lighting is usually fine as standard uniforms unless you have many lights)
			s_3DData.SphereShader->SetFloat3("u_LightPos", m_LightPosition);
			s_3DData.SphereShader->SetInt("u_BounceCount", m_BounceFactor);
			s_3DData.SphereShader->SetInt("u_SampleCount", m_SamplingRate);

			// 2. The Clean UBO Upload!
			SphereUBOData uboData;

			// Copy the batched spheres into our UBO struct
			memcpy(uboData.Spheres, s_3DData.SphereBufferBase, sizeof(Sphere) * s_3DData.SphereCount);
			uboData.SphereCount = s_3DData.SphereCount;

			// Send the entire chunk of memory to the GPU in one single API call
			s_3DData.SphereUniformBuffer->SetData(&uboData, sizeof(SphereUBOData));

			// 3. Draw the Quad
			RenderCommand::DrawIndexed(s_3DData.QuadVertexArray, 6);
		}
	}

	void Renderer3D::DrawSphere(const Sphere& sphere)
	{
		if (s_3DData.SphereCount >= Renderer3DData::MaxSphereCount)
			return;

		*(s_3DData.SphereBufferPtr) = sphere;
		s_3DData.SphereBufferPtr++;
		s_3DData.SphereCount++;
	}

	void Renderer3D::PostProcess(const std::shared_ptr<Framebuffer>& HDRframeBuffer, const std::shared_ptr<Framebuffer>& targetFrameBuffer)
	{
		// ==========================================
		// PASS 2: EXTRACT & BLUR (Ping-Pong)
		// ==========================================
		bool horizontal = true, first_iteration = true;
		int amount = 10; // Blur it 10 times

		s_3DData.BlurShader->Bind();
		for (unsigned int i = 0; i < amount; i++)
		{
			s_3DData.PingPongFBO[horizontal]->Bind();
			s_3DData.BlurShader->SetInt("u_Horizontal", horizontal);

			// On the first pass, we blur the main scene. After that, we blur the blurs!
			uint32_t textureToBlur = first_iteration ? HDRframeBuffer->GetColorAttachmentRendererID(1) : s_3DData.PingPongFBO[!horizontal]->GetColorAttachmentRendererID(0);

			// Bind the texture and draw a flat 2D Screen Quad
			{
				// 1. Bind the framebuffer image we want to blur/composite to Slot 0
				glBindTextureUnit(0, textureToBlur);

				// 2. Bind the exact same geometry used by your Ray Tracer
				s_3DData.QuadVertexArray->Bind();

				// 3. Draw it!
				RenderCommand::DrawIndexed(s_3DData.QuadVertexArray, 6);
			}

			s_3DData.PingPongFBO[horizontal]->Unbind();
			horizontal = !horizontal;
			if (first_iteration) first_iteration = false;
		}

		// ==========================================
		// PASS 3: COMPOSITE (Add Bloom to Original)
		// ==========================================
		// We bind the target Framebuffer one last time to draw the glow over top of the scene
		targetFrameBuffer->Bind();

		// Clear the color and depth buffers so the 2D Quad can safely draw!
		RenderCommand::Clear();

		s_3DData.CompositeShader->Bind();
		s_3DData.CompositeShader->SetInt("u_SceneTexture", 0);
		s_3DData.CompositeShader->SetInt("u_BlurTexture", 1);
		s_3DData.CompositeShader->SetFloat("u_Exposure", m_Exposure);

		// Bind the final blurred texture
		uint32_t sceneTexture = HDRframeBuffer->GetColorAttachmentRendererID(0);
		uint32_t blurredTexture = s_3DData.PingPongFBO[!horizontal]->GetColorAttachmentRendererID(0);
		{
			// 1. Bind the framebuffer image we want to blur/composite to Slot 0
			glBindTextureUnit(0, sceneTexture);
			// Bind blurred texture to slot 1
			glBindTextureUnit(1, blurredTexture);

			// 2. Bind the exact same geometry used by your Ray Tracer
			s_3DData.QuadVertexArray->Bind();

			// 3. Draw it!
			RenderCommand::DrawIndexed(s_3DData.QuadVertexArray, 6);
		}

		targetFrameBuffer->Unbind();
	}

	void Renderer3D::OnViewportResize(uint32_t width, uint32_t height)
	{
		// Keep the bloom buffers at half resolution for massive performance gains!
		uint32_t bloomWidth = width / 2;
		uint32_t bloomHeight = height / 2;

		// Safety check to prevent OpenGL crash on 0x0 size when minimizing the window
		if (bloomWidth > 0 && bloomHeight > 0)
		{
			s_3DData.PingPongFBO[0]->Resize(bloomWidth, bloomHeight);
			s_3DData.PingPongFBO[1]->Resize(bloomWidth, bloomHeight);
		}
	}
}