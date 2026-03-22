#pragma once

#include <Engine.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>
#include <Engine/Renderer/EditorCamera.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"

namespace Engine
{
	class Editor3DLayer : public Layer
	{
	public:
		Editor3DLayer();
		virtual ~Editor3DLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(float ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		// UI Panels
		void UI_Stats();
		void UI_Viewport();

	private:
		std::shared_ptr<Framebuffer> m_Framebuffer;

		EditorCamera m_EditorCamera;

		bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];
		glm::vec2 m_DockspaceLocation = { 0.0f,0.0f };

		glm::vec3 m_LightPosition = { 5.0f,5.0f,-5.0f };
		
		glm::vec3 Sphere1Pos = { 0,0,0 };
		float Sphere1Rad = 1;
		glm::vec4 Sphere1Col = { 1,1,1,1 };
		glm::vec3 Sphere2Pos = { -2,-2,2 };
		float Sphere2Rad = 1;
		glm::vec4 Sphere2Col = { 0.9, 0.4, 0.8, 1 };
	};
}