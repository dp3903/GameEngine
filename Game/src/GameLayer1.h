#pragma once

#include <Engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "ParticleSystem.h"

class GameLayer1 : public Engine::Layer
{
public:
	GameLayer1();
	virtual ~GameLayer1() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(float ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Engine::Event& e) override;
private:
	Engine::SceneCamera m_Camera;
	glm::vec3 m_CameraPosition = {0.0, 0.0, 0.0};

	// Temp
	std::shared_ptr<Engine::VertexArray> m_SquareVA;
	std::shared_ptr<Engine::Shader> m_FlatColorShader;
	std::shared_ptr<Engine::Texture2D> m_CheckerboardTexture;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
	float m_BGSquareSize = 0.5f;

	ParticleProps m_Particle;
	ParticleSystem m_ParticleSystem;
};