#include "Editor3DLayer.h"
#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ImGuizmo.h"

namespace Engine
{

	Editor3DLayer::Editor3DLayer()
		: Layer("EditorLayer")
	{
	}

	void Editor3DLayer::OnAttach()
	{
		FramebufferSpecification HDRfbSpec;
		// Slot 0: Scene Color (RGBA16F)
		// Slot 1: Bloom Extraction (RGBA16F)
		// Slot 2: Entity ID (RED_INTEGER)
		// Slot 3: Depth
		HDRfbSpec.Attachments = {
			FramebufferTextureFormat::RGBA16F,
			FramebufferTextureFormat::RGBA16F,
			FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::Depth
		};
		HDRfbSpec.Width = 1280;
		HDRfbSpec.Height = 720;
		m_HDRFramebuffer = Framebuffer::Create(HDRfbSpec);

		FramebufferSpecification finalfbSpec;
		finalfbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		finalfbSpec.Width = 1280;
		finalfbSpec.Height = 720;
		m_FinalFramebuffer = Framebuffer::Create(finalfbSpec);

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

		m_Spheres.push_back({ { 0,0,0 }, 1 });
		m_Spheres.push_back({ { -2,-2,-2 }, 1 });

		m_Cuboids.push_back({ {0,0,-5} });

		APP_LOG_INFO("Editor3D Attached");
	}

	void Editor3DLayer::OnDetach()
	{
		APP_LOG_INFO("Editor3D Detached");
	}

	void Editor3DLayer::OnUpdate(float ts)
	{

		// Resize
		if (Engine::FramebufferSpecification spec = m_FinalFramebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_FinalFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_HDRFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			Renderer3D::OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		}

		// Update Editor Camera
		m_EditorCamera.OnUpdate(ts);

		// ==========================================
		// PASS 1: RENDER THE 3D SCENE (HDR)
		// ==========================================
		{
			// Bind frame buffer before any renderer calls
			m_HDRFramebuffer->Bind();

			RenderCommand::SetClearColor({ 0.01f, 0.01f, 0.01f, 1 });
			RenderCommand::Clear();

			// Clear our entity ID attachment to -1
			m_HDRFramebuffer->ClearAttachment(1, -1);


			// Draw
			{
				Renderer3D::BeginScene(m_EditorCamera);

				for (auto& sphere : m_Spheres)
					Renderer3D::DrawSphere(sphere);
				for (auto& cuboid : m_Cuboids)
					Renderer3D::DrawCuboid(cuboid);

				Renderer3D::EndScene();
			}

			m_HDRFramebuffer->Unbind();
		}

		// Post-process
		if(m_PostProcessing)
			Renderer3D::PostProcess(m_HDRFramebuffer, m_FinalFramebuffer);
		
	}

	void Editor3DLayer::OnImGuiRender()
	{
		// Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
			ImGui::PopStyleVar();

			if (opt_fullscreen)
				ImGui::PopStyleVar(2);

			// DockSpace
			ImGuiIO& io = ImGui::GetIO();
			ImGuiStyle& style = ImGui::GetStyle();
			float minWinSizeX = style.WindowMinSize.x;
			style.WindowMinSize.x = 370.0f;
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
				ImVec2 dockSpacePosScreen = ImGui::GetWindowPos();
				m_DockspaceLocation = { dockSpacePosScreen.x,dockSpacePosScreen.y };
			}
			style.WindowMinSize.x = minWinSizeX;

			UI_Stats();

			UI_Viewport();

		ImGui::End();
		
	}

	void Editor3DLayer::OnEvent(Event& e)
	{
		m_EditorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(std::bind(&Editor3DLayer::OnKeyPressed, this, std::placeholders::_1));
		dispatcher.Dispatch<MouseButtonPressedEvent>(std::bind(&Editor3DLayer::OnMouseButtonPressed, this, std::placeholders::_1));
	}

	bool Editor3DLayer::OnKeyPressed(KeyPressedEvent& e)
	{

		return false;
	}

	bool Editor3DLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		
		return false;
	}

	void Editor3DLayer::UI_Stats()
	{
		ImGui::Begin("Stats");

		ImGui::DragFloat3("Light Position", glm::value_ptr(Renderer3D::m_LightPosition), 0.1f);
		ImGui::Checkbox("Enable Bloom", &m_PostProcessing);
		ImGui::SliderInt("Bounce Factor", (int*)&Renderer3D::m_BounceFactor, 1, 10);
		ImGui::SliderInt("Sampling Rate", (int*)&Renderer3D::m_SamplingRate, 3, 30);

		for (uint32_t i = 0 ; i < m_Spheres.size() ; i++)
		{
			ImGui::PushID(i);
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
			
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)&m_Spheres[i], treeNodeFlags, "Sphere: %d", i);
			ImGui::PopStyleVar(
			);

			if (open)
			{
				ImGui::Text("Sphere: %d", i);
				ImGui::DragFloat3("Position", glm::value_ptr(m_Spheres[i].Position), 0.05f);
				ImGui::DragFloat("Radius", &m_Spheres[i].Radius, 0.05f);
				ImGui::ColorEdit3("Albedo", glm::value_ptr(m_Spheres[i].Albedo));
				ImGui::SliderFloat("Roughness", &m_Spheres[i].Roughness, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &m_Spheres[i].Metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("Opacity", &m_Spheres[i].Opacity, 0.0f, 1.0f);
				ImGui::SliderFloat("IOR", &m_Spheres[i].IOR, 1.0f, 3.0f);

				ImGui::TreePop();
			}

			ImGui::PopID();		
		}

		for (uint32_t i = 0; i < m_Cuboids.size(); i++)
		{
			ImGui::PushID(i);
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;

			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)&m_Cuboids[i], treeNodeFlags, "Cuboid: %d", i);
			ImGui::PopStyleVar(
			);

			if (open)
			{
				ImGui::Text("Cuboid: %d", i);
				ImGui::DragFloat3("Position", glm::value_ptr(m_Cuboids[i].Position), 0.05f);
				ImGui::DragFloat3("Rotation", glm::value_ptr(m_Cuboids[i].Rotation), 0.05f);
				ImGui::DragFloat3("Scale", glm::value_ptr(m_Cuboids[i].Scale), 0.05f);
				ImGui::ColorEdit3("Albedo", glm::value_ptr(m_Cuboids[i].Albedo));
				ImGui::SliderFloat("Roughness", &m_Cuboids[i].Roughness, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &m_Cuboids[i].Metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("Opacity", &m_Cuboids[i].Opacity, 0.0f, 1.0f);
				ImGui::SliderFloat("IOR", &m_Cuboids[i].IOR, 1.0f, 3.0f);

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::End();
	}

	void Editor3DLayer::UI_Viewport()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
		
		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		if (m_PostProcessing)
		{
			uint32_t textureID = m_FinalFramebuffer->GetColorAttachmentRendererID();
			ImGui::Image((void*)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		}
		else
		{
			uint32_t textureID = m_HDRFramebuffer->GetColorAttachmentRendererID();
			ImGui::Image((void*)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}