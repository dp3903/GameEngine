#include <stdio.h>
#include <Engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class ExampleLayer : public Engine::Layer
{
public:
	ExampleLayer()
		: Layer("Example"), m_Camera(-1.6, 1.6, -0.9, 0.9), m_CameraPosition(0.0f, 0.0f, 0.0f)
	{
		m_VertexArray.reset(new Engine::VertexArray());

		float vertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		std::shared_ptr<Engine::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(new Engine::VertexBuffer(vertices, sizeof(vertices)));
		Engine::BufferLayout layout = {
			{ Engine::ShaderDataType::Float3, "a_Position" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
		std::shared_ptr<Engine::IndexBuffer> indexBuffer;
		indexBuffer.reset(new Engine::IndexBuffer(indices, sizeof(indices) / sizeof(uint32_t)));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		std::string vertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;

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

		m_Shader.reset(new Engine::Shader(vertexSrc, fragmentSrc));
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
		//APP_LOG_INFO("ExampleLayer::Update");

		if (Engine::Input::IsKeyPressed(KEY_LEFT))
			m_CameraPosition.x -= m_CameraMoveSpeed * ts;
		else if (Engine::Input::IsKeyPressed(KEY_RIGHT))
			m_CameraPosition.x += m_CameraMoveSpeed * ts;

		if (Engine::Input::IsKeyPressed(KEY_UP))
			m_CameraPosition.y += m_CameraMoveSpeed * ts;
		else if (Engine::Input::IsKeyPressed(KEY_DOWN))
			m_CameraPosition.y -= m_CameraMoveSpeed * ts;

		if (Engine::Input::IsKeyPressed(KEY_A))
			m_CameraRotation += m_CameraRotationSpeed * ts;
		if (Engine::Input::IsKeyPressed(KEY_D))
			m_CameraRotation -= m_CameraRotationSpeed * ts;

		Engine::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Engine::RenderCommand::Clear();

		m_Camera.SetPosition(m_CameraPosition);
		m_Camera.SetRotation(m_CameraRotation);

		Engine::Renderer::BeginScene(m_Camera);
		{
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
			m_Shader->Bind();
			m_Shader->UploadUniformFloat3("u_Color", {0.3f, 0.2f, 0.8f});

			for (int y = 0; y < 20; y++)
			{
				for (int x = 0; x < 20; x++)
				{
					glm::vec3 pos(x * 0.11f, y * 0.11f, 0.0f);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
					Engine::Renderer::Submit(m_Shader, m_VertexArray, transform);
				}
			}

		}
		Engine::Renderer::EndScene();
	}

	void OnEvent(Engine::Event& event) override
	{
		APP_LOG_TRACE("{0}", event);
	}

private:
	std::shared_ptr<Engine::Shader> m_Shader;
	std::shared_ptr<Engine::VertexArray> m_VertexArray;
	Engine::OrthographicCamera m_Camera;

	glm::vec3 m_CameraPosition;
	float m_CameraMoveSpeed = 5.0f;

	float m_CameraRotation = 0.0f;
	float m_CameraRotationSpeed = 180.0f;
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