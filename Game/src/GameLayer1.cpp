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
			Engine::Renderer2D::DrawQuad({ x, y }, { 0.5f, 0.5f }, color);
		}
	}
	Engine::Renderer2D::EndScene();
}

void GameLayer1::OnImGuiRender()
{
	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));

	auto stats = Engine::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::End();
}

void GameLayer1::OnEvent(Engine::Event& e)
{
	m_CameraController.OnEvent(e);
}