#include <stdio.h>
#include <Engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"

class ExampleLayer : public Engine::Layer
{
public:
	ExampleLayer()
		: Layer("Example"), m_CameraController(1280.0f / 720.0f, true)
	{
		m_VertexArray = Engine::VertexArray::Create();
		m_TextureVA = Engine::VertexArray::Create();

		float vertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};

		std::shared_ptr<Engine::VertexBuffer> vertexBuffer = Engine::VertexBuffer::Create(vertices, sizeof(vertices));
		Engine::BufferLayout layout = {
			{ Engine::ShaderDataType::Float3, "a_Position" },
			{ Engine::ShaderDataType::Float2, "a_TexCoord" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
		std::shared_ptr<Engine::IndexBuffer> indexBuffer = Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		std::string vertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec3 a_TexCoord;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);	
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			
			uniform vec3 u_Color;

			void main()
			{
				color = vec4(0.2, 0.3, 0.8, 1.0);
				color = vec4(u_Color, 1.0);
			}
		)";

		m_Shader = Engine::Shader::Create("FlatcolorShader", vertexSrc, fragmentSrc);

		// ---------------------------------------------------------------------------------------------------------

		

		m_TextureVA->AddVertexBuffer(vertexBuffer);
		m_TextureVA->SetIndexBuffer(indexBuffer);

		auto textureShader = m_ShaderLibrary.Load("assets/shaders/TextureShader.glsl");
		textureShader->Bind();
		textureShader->UploadUniformInt("u_Texture", 0);


		m_Texture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");
		m_GroundTexture = Engine::Texture2D::Create("assets/textures/ground.png");
	}

	void OnAttach() override
	{
		APP_LOG_INFO("ExampleLayer::Attach");
	}

	void OnDetach() override
	{
		APP_LOG_INFO("ExampleLayer::Detach");
	}

	void OnUpdate(float ts) override
	{
		// Update
		m_CameraController.OnUpdate(ts);

		// Render
		Engine::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Engine::RenderCommand::Clear();

		Engine::Renderer::BeginScene(m_CameraController.GetCamera());
		{
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
			m_Shader->Bind();
			m_Shader->UploadUniformFloat3("u_Color", m_SquareColor);

			for (int y = 0; y < 20; y++)
			{
				for (int x = 0; x < 20; x++)
				{
					glm::vec3 pos(x * 0.11f, y * 0.11f, 0.0f);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
					Engine::Renderer::Submit(m_Shader, m_VertexArray, transform);
				}
			}

			auto textureShader = m_ShaderLibrary.Get("TextureShader");
			m_Texture->Bind();
			Engine::Renderer::Submit(textureShader, m_TextureVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));
			m_GroundTexture->Bind();
			Engine::Renderer::Submit(textureShader, m_TextureVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		}
		Engine::Renderer::EndScene();
	}

	void OnEvent(Engine::Event& event) override
	{
		//APP_LOG_TRACE("{0}", event);

		m_CameraController.OnEvent(event);
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Settings");
		ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));
		ImGui::End();
	}

private:
	std::shared_ptr<Engine::Shader> m_Shader;
	std::shared_ptr<Engine::VertexArray> m_VertexArray;

	Engine::ShaderLibrary m_ShaderLibrary;
	std::shared_ptr<Engine::VertexArray> m_TextureVA;
	std::shared_ptr<Engine::Texture2D> m_Texture;
	std::shared_ptr<Engine::Texture2D> m_GroundTexture;

	Engine::OrthographicCameraController m_CameraController;

	glm::vec3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};

class Game : public Engine::Application
{
public:
	Game()
	{
		PushLayer(new ExampleLayer());
	}
	~Game()
	{

	}
};

Engine::Application* Engine::CreateApplication()
{
	return new Game();
}