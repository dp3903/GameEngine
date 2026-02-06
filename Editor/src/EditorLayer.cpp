#include "EditorLayer.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Scene/SceneRuntimeData.h"
#include "Engine/Utils/FileDialogs.h"
#include "Engine/Utils/Math.h"

#include "ImGuizmo.h"
#include <Engine/Renderer/Font.h>
// Temporary includes
#include "sol/sol.hpp"
#include "lua.hpp" // If this fails, your include paths are wrong


#include "box2d/include/box2d/b2_world.h"
#include "box2d/include/box2d/b2_body.h"
#include "box2d/include/box2d/b2_fixture.h"
#include "box2d/include/box2d/b2_polygon_shape.h"
#include "box2d/include/box2d/b2_circle_shape.h"

namespace Engine
{

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{

	}

	void EditorLayer::OnAttach()
	{
		m_IconPlay = Texture2D::Create("Resources/Icons/PlayButton.png");
		m_IconStop = Texture2D::Create("Resources/Icons/StopButton.png");
		m_IconSimulate = Texture2D::Create("Resources/Icons/SimulateButton.png");

		m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>();

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		m_Framebuffer = Framebuffer::Create(fbSpec);

		m_EditorScene = std::make_shared<Scene>();

		auto commandLineArgs = Application::Get().GetSpecification().CommandLineArgs;
		if (commandLineArgs.Count > 1)
		{
			auto projectFilePath = commandLineArgs[1];
			OpenProject(projectFilePath);
		}
		else
		{
			//OpenProject();
			// TODO remove temporary while not debugging
			OpenProject("Projects/GameProject/GameProject.gmproj");
		}

		m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>();

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_SceneHierarchyPanel->SetContext(m_ActiveScene);

		APP_LOG_INFO("Editor Attached");
	}

	void EditorLayer::OnDetach()
	{
		APP_LOG_INFO("Editor Detached");
	}

	void EditorLayer::OnUpdate(float ts)
	{

		static float rotation = 0.0f;
		rotation += ts * 50.0f;

		// Resize
		if (Engine::FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		// Bind frame buffer before any renderer calls
		m_Framebuffer->Bind();

		// Render
		Renderer2D::ResetStats();

		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		RenderCommand::Clear();
		
		// Clear our entity ID attachment to -1
		m_Framebuffer->ClearAttachment(1, -1);

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				m_EditorCamera.OnUpdate(ts);

				m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
				break;
			}
			case SceneState::Simulate:
			{
				m_EditorCamera.OnUpdate(ts);

				m_ActiveScene->OnUpdateSimulation(ts, m_EditorCamera);
				break;
			}
			case SceneState::Play:
			{
				m_ActiveScene->OnUpdateRuntime(ts);
				break;
		}
		}

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
			m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
		}

		OnOverlayRender();

		m_Framebuffer->Unbind();

		// handle scene change request
		if (RuntimeData::RequestStatus().Requested)
		{
			m_ActiveScene->OnRuntimeStop();

			std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
			SceneSerializer serializer(newScene);
			serializer.Deserialize(Project::GetAssetFileSystemPath(RuntimeData::RequestStatus().scenePath).string());
			
			m_ActiveScene = newScene;
			m_SceneHierarchyPanel->SetContext(m_ActiveScene);
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

			m_ActiveScene->OnRuntimeStart();

			RuntimeData::ResetChangeRequest();
		}
	}

	void EditorLayer::OnImGuiRender()
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
			}
			style.WindowMinSize.x = minWinSizeX;

			if (ImGui::BeginMenuBar())
			{
				if (m_SceneState == SceneState::Edit)
				{
					if (ImGui::BeginMenu("File"))
					{
						// Disabling fullscreen would allow the window to be moved to the front of other windows, 
						// which we can't undo at the moment without finer window depth/z control.
						//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

						if (ImGui::MenuItem("Open new or existing project...", "Ctrl+O"))
							OpenProject();

						if (ImGui::MenuItem("Save Project", "Ctrl+Alt+S"))
							SaveProject();

						if (ImGui::MenuItem("New Scene", "Ctrl+N"))
							NewScene();
					
						if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
							SaveScene();

						if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
							SaveSceneAs();

						if (ImGui::MenuItem("Exit"))
							Application::Get().Close();

						ImGui::EndMenu();
					}
				}
				ImGui::EndMenuBar();
			}

			UI_Stats();

			UI_Settings();

			m_SceneHierarchyPanel->OnImGuiRender();
			m_ContentBrowserPanel->OnImGuiRender();

			UI_Viewport();

			UI_Toolbar();

		ImGui::End();
		
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (m_SceneState != SceneState::Play)
		{
			m_EditorCamera.OnEvent(e);
		}

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(std::bind(&EditorLayer::OnKeyPressed, this, std::placeholders::_1));
		dispatcher.Dispatch<MouseButtonPressedEvent>(std::bind(&EditorLayer::OnMouseButtonPressed, this, std::placeholders::_1));
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Shortcuts
		if (e.GetRepeatCount() > 0 || m_SceneState == SceneState::Play)
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		bool alt = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);

		switch (e.GetKeyCode())
		{
			case Key::N:
			{
				if (control)
					NewScene();

				break;
			}
			case Key::O:
			{
				if (control)
					OpenProject();

				break;
			}
			case Key::S:
			{
				if (control)
				{
					if (alt)
						SaveProject();
					if (shift)
						SaveSceneAs();
					else
						SaveScene();
				}

				break;
			}

			// Scene Commands
			case Key::D:
			{
				if (control)
					OnDuplicateEntity();

				break;
			}

			// Gizmos
			case Key::Q:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = -1;
				break;
			case Key::W:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				break;
			case Key::E:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				break;
			case Key::R:
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::SCALE;
				break;
		}

		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
			if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
				m_SceneHierarchyPanel->SetSelectedEntity(m_HoveredEntity);
		}
		return false;
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
			if (!camera)
				return;

			Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera, camera.GetComponent<TransformComponent>().GlobalTransform);
		}
		else
		{
			Renderer2D::BeginScene(m_EditorCamera);
		}

		if (m_ShowPhysicsColliders)
		{
			// 1. Draw Box Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<BoxCollider2DComponent>();
				for (auto e : view)
				{
					auto& bc2d = view.get<BoxCollider2DComponent>(e);

					// Only draw if physics is running and fixture exists
					if (bc2d.RuntimeFixture)
					{
						b2Fixture* fixture = (b2Fixture*)bc2d.RuntimeFixture;
						b2Body* body = fixture->GetBody();
						const b2PolygonShape* shape = (const b2PolygonShape*)fixture->GetShape();

						// Box2D Polygon Shapes store 4 vertices in Local Space.
						// We use the Body's transform to convert them to World Space.

						int vertexCount = shape->m_count; // Usually 4 for a box
						std::vector<glm::vec3> worldVertices;

						for (int i = 0; i < vertexCount; i++)
						{
							// "GetWorldPoint" handles Rotation + Position + Offset automatically
							b2Vec2 p = body->GetWorldPoint(shape->m_vertices[i]);
							worldVertices.push_back({ p.x, p.y, 0.0f });
						}

						// Draw lines between vertices (0-1, 1-2, 2-3, 3-0)
						for (int i = 0; i < vertexCount; i++)
						{
							glm::vec3 p1 = worldVertices[i];
							glm::vec3 p2 = worldVertices[(i + 1) % vertexCount];
							Renderer2D::DrawLine(p1, p2, m_PhysicsCollidersColor); // Green Box
						}
					}
				}
			}

			// 2. Draw Circle Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<CircleCollider2DComponent>();
				for (auto e : view)
				{
					auto& cc2d = view.get<CircleCollider2DComponent>(e);

					if (cc2d.RuntimeFixture)
					{
						b2Fixture* fixture = (b2Fixture*)cc2d.RuntimeFixture;
						b2Body* body = fixture->GetBody();
						const b2CircleShape* shape = (const b2CircleShape*)fixture->GetShape();

						// Calculate World Center
						// b2CircleShape::m_p is the local offset relative to the body center
						b2Vec2 worldCenter = body->GetWorldPoint(shape->m_p);

						// Radius is stored directly in the shape (already scaled during creation)
						float radius = shape->m_radius;

						// Draw
						glm::mat4 transform = glm::translate(glm::mat4(1.0f), { worldCenter.x, worldCenter.y, 0.0f })
							* glm::scale(glm::mat4(1.0f), { radius * 2.0f, radius * 2.0f, 1.0f });

						Renderer2D::DrawCircle(transform, m_PhysicsCollidersColor, 0.1f);
					}
				}
			}
		}

		if (Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity())
		{
			const TransformComponent& transform = selectedEntity.GetComponent<TransformComponent>();
			Renderer2D::DrawRect(transform.GlobalTransform, m_SelectedEntityColor);
		}
		
		Renderer2D::EndScene();

	}

	void EditorLayer::NewProject(const std::string& projectDir)
	{
		Project::New(projectDir);
		NewScene();
	}

	void EditorLayer::OpenProject()
	{
		std::string projectDir = FileDialogs::SelectDirectory();

		if (!std::filesystem::exists(projectDir))
			return;

		std::filesystem::path projectFile = "";

		// Iterate through the directory to find a .gmproj file
		for (const auto& entry : std::filesystem::directory_iterator(projectDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".gmproj")
			{
				projectFile = entry.path();
				break; // Found one!
			}
		}

		if (!projectFile.empty())
		{
			// CASE A: Project Exists -> Load it
			OpenProject(projectFile);
		}
		else
		{
			// CASE B: No Project -> Create New
			NewProject(projectDir);
		}
	}

	void EditorLayer::OpenProject(const std::filesystem::path& path)
	{
		if (path.extension().string() != ".gmproj")
		{
			ENGINE_LOG_ERROR("Invalid game project file: {0}", path.string());
			ASSERT(false);
		}
		if (Project::Load(path))
		{
			auto startScenePath = Project::GetAssetFileSystemPath(Project::GetActive()->GetConfig().StartScene);
			OpenScene(startScenePath);
			m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>();
		}
	}

	void EditorLayer::SaveProject()
	{
		 Project::SaveActive();
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = std::make_shared<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_SceneHierarchyPanel->SetContext(m_ActiveScene);

		m_EditorScenePath = std::filesystem::path();
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("Serialized Scene Format (*.ssfmt)\0*.ssfmt\0");
		if (!filepath.empty())
		{
			OpenScene(filepath);
		}
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (path.extension().string() != ".ssfmt")
		{
			APP_LOG_WARN("Could not load {0} - not a scene file", path.filename().string());
			NewScene();
			return;
		}

		std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			m_EditorScene = newScene;
			m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_SceneHierarchyPanel->SetContext(m_EditorScene);

			m_ActiveScene = m_EditorScene;
			m_EditorScenePath = path;
		}

	}

	void EditorLayer::SaveScene()
	{
		if (!m_EditorScenePath.empty())
			SerializeScene(m_ActiveScene, m_EditorScenePath);
		else
			SaveSceneAs();
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Serialized Scene Format (*.ssfmt)\0*.ssfmt\0", "ssfmt");
		if (!filepath.empty())
		{
			SerializeScene(m_ActiveScene, filepath);
			m_EditorScenePath = filepath;
		}
	}

	void EditorLayer::UI_Stats()
	{
		ImGui::Begin("Stats");

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		std::string name = "None";
		if (m_HoveredEntity)
			name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
		ImGui::Text("Hovered Entity: %s", name.c_str());

		ImGui::End();
	}

	void EditorLayer::UI_Settings()
	{
		ImGui::Begin("Settings");

		ImGui::ColorEdit4("Selected entity Color", glm::value_ptr(m_SelectedEntityColor));
		ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);
		if (m_ShowPhysicsColliders)
		{
			ImGui::ColorEdit4("Physics Collider Color", glm::value_ptr(m_PhysicsCollidersColor));
		}
		
		ImGui::End();
	}

	void EditorLayer::UI_Viewport()
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

		uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image((void*)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (m_SceneState == SceneState::Edit)
		{
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM_SCENE"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					std::filesystem::path scenePath = Project::GetAssetFileSystemPath(path);
					OpenScene(scenePath);
				}
				ImGui::EndDragDropTarget();
			}
		}

		// Gizmos
		Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
		if (selectedEntity && m_GizmoType != -1 && m_SceneState != SceneState::Play)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

			// Runtime camera from entity
			//auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
			//const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
			//const glm::mat4& cameraProjection = camera.GetProjection();
			//glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

			// Editor camera
			const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
			glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

			// Entity transform
			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GlobalTransform;

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 0.5f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
				snapValue = 45.0f;

			float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
				nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, rotation, scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);

				glm::vec3 deltaRotation = rotation - tc.Rotation;
				tc.Translation = translation;
				tc.Rotation += deltaRotation;
				tc.Scale = scale;
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorLayer::UI_Toolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float size = ImGui::GetWindowHeight() - 4.0f;
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

		{
			std::shared_ptr<Texture2D> icon = (m_SceneState != SceneState::Play) ? m_IconPlay : m_IconStop;
			if (ImGui::ImageButton("Play/Stop", (ImTextureID)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1)))
			{
				if (m_SceneState != SceneState::Play)
					OnScenePlay();
				else if (m_SceneState == SceneState::Play)
					OnSceneStop();
			}
		}
		ImGui::SameLine();
		{
			std::shared_ptr<Texture2D> icon = (m_SceneState != SceneState::Simulate) ? m_IconSimulate : m_IconStop;
			if (ImGui::ImageButton("Simulate/Stop", (ImTextureID)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1)))
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
					OnSceneSimulate();
				else if (m_SceneState == SceneState::Simulate)
					OnSceneStop();
			}
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::End();
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Simulate)
			OnSceneStop();

		m_SceneState = SceneState::Play;

		m_EditorScene = Scene::Copy(m_ActiveScene);
		m_ActiveScene->OnRuntimeStart();
		
		m_SceneHierarchyPanel->SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneSimulate()
	{
		if (m_SceneState == SceneState::Play)
			OnSceneStop();

		m_SceneState = SceneState::Simulate;

		m_EditorScene = Scene::Copy(m_ActiveScene);
		m_ActiveScene->OnSimulationStart();

		m_SceneHierarchyPanel->SetContext(m_ActiveScene);
	}

	void EditorLayer::OnSceneStop()
	{
		if (m_SceneState == SceneState::Play)
			m_ActiveScene->OnRuntimeStop();
		else if (m_SceneState == SceneState::Simulate)
			m_ActiveScene->OnSimulationStop();

		m_SceneState = SceneState::Edit;

		m_ActiveScene = m_EditorScene;

		m_SceneHierarchyPanel->SetContext(m_ActiveScene);
	}

	void EditorLayer::SerializeScene(std::shared_ptr<Scene> scene, const std::filesystem::path& path)
	{
		SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
	}

	void EditorLayer::OnDuplicateEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		Entity selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
		if (selectedEntity)
			m_EditorScene->DuplicateEntity(selectedEntity);
	}
}