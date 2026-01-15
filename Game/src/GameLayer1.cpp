#include "GameLayer1.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GameLayer1::GameLayer1()
	: Layer("GameLayer1"), m_CameraController(1280.0f / 720.0f, true), m_SquareColor({ 0.2f, 0.3f, 0.8f, 1.0f })
{
}

void GameLayer1::OnAttach()
{
	m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");

	// Particle Init here
	m_Particle.ColorBegin = { 3   / 255.0f, 252 / 255.0f, 252 / 255.0f, 1.0f };
	m_Particle.ColorEnd =   { 236 / 255.0f, 3   / 255.0f, 252 / 255.0f, 1.0f };
	m_Particle.SizeBegin = 0.2f, m_Particle.SizeVariation = 0.1f, m_Particle.SizeEnd = 0.0f;
	m_Particle.LifeTime = 1.0f;
	m_Particle.Velocity = { 0.0f, 0.0f };
	m_Particle.VelocityVariation = { 3.0f, 1.0f };
	m_Particle.Position = { 0.0f, 0.0f };

	APP_LOG_INFO("Game Layer 1 Attached");
}

void GameLayer1::OnDetach()
{
	APP_LOG_INFO("Game Layer 1 Detached");
}

void GameLayer1::OnUpdate(float ts)
{

	static float rotation = 0.0f;
	rotation += ts * 50.0f;

	// Update
	m_CameraController.OnUpdate(ts);

	// Render
	Engine::Renderer2D::ResetStats();

	Engine::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Engine::RenderCommand::Clear();

	Engine::Renderer2D::BeginScene(m_CameraController.GetCamera());
	Engine::Renderer2D::DrawQuad({  0.0f,  0.0f, -0.1f }, { 20.0f, 20.0f }, m_CheckerboardTexture, 10);
	Engine::Renderer2D::DrawQuad({ -1.0f,  0.0f }, { 0.8f,  0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
	Engine::Renderer2D::DrawRotatedQuad({  0.5f, -0.5f }, { 0.5f,  0.75f }, rotation, m_SquareColor);

	for (float y = -5.0f; y < 5.0f; y += 0.5f)
	{
		for (float x = -5.0f; x < 5.0f; x += 0.5f)
		{
			glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f };
			Engine::Renderer2D::DrawQuad({ x, y }, { m_BGSquareSize, m_BGSquareSize }, color);
		}
	}

	if (Engine::Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		auto [x, y] = Engine::Input::GetMousePosition();
		auto width = Engine::Application::Get().GetWindow().GetWidth();
		auto height = Engine::Application::Get().GetWindow().GetHeight();

		auto bounds = m_CameraController.GetBounds();
		auto pos = m_CameraController.GetCamera().GetPosition();
		x = (x / width) * bounds.GetWidth() - bounds.GetWidth() * 0.5f;
		y = bounds.GetHeight() * 0.5f - (y / height) * bounds.GetHeight();
		m_Particle.Position = { x + pos.x, y + pos.y };
		for (int i = 0; i < 5; i++)
			m_ParticleSystem.Emit(m_Particle);
	}

	m_ParticleSystem.OnUpdate(ts);
	m_ParticleSystem.OnRender();
	
	Engine::Renderer2D::EndScene();

}

void GameLayer1::OnImGuiRender()
{
	
	ImGui::Begin("Settings");

	auto stats = Engine::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
	ImGui::DragFloat("BG Square size", &m_BGSquareSize, 0.001f, 0.0f, 1.0f);

	ImGui::End();

	
}

void GameLayer1::OnEvent(Engine::Event& e)
{
	m_CameraController.OnEvent(e);
}