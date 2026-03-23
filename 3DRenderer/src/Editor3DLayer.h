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

		std::vector<Renderer3D::Sphere> m_Spheres;
	};
}