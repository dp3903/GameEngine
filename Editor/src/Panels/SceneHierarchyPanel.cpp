#include "SceneHierarchyPanel.h"

#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include "Engine/Scene/Components.h"

namespace Engine {

	SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const std::shared_ptr<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		Entity sceneRoot = { m_Context->m_SceneRoot, m_Context.get() };
		Entity child = { sceneRoot.GetComponent<RelationshipComponent>().FirstChild, m_Context.get() };
		while (child)
		{
			Entity next = { child.GetComponent<RelationshipComponent>().NextSibling, m_Context.get() };
			DrawEntityNode(child);
			child = next;
		}
		
		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectionContext = {};
		
		// Right-click on blank space
		if (ImGui::BeginPopupContextWindow(0, 1 | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Create Empty Entity"))
				m_Context->CreateNewChildEntity(sceneRoot);

			ImGui::EndPopup();
		}

		ImGui::End();

		ImGui::Begin("Properties");
		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}
		
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Create Child Entity"))
				m_Context->CreateNewChildEntity(entity);
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened)
		{
			Entity child = { entity.GetComponent<RelationshipComponent>().FirstChild, m_Context.get() };
			while (child)
			{
				// If DrawEntityNode(child) deletes the child, 'child' becomes invalid, and trying to get 'child.GetComponent' in the next line would CRASH.
				Entity next = { child.GetComponent<RelationshipComponent>().NextSibling, m_Context.get() };

				DrawEntityNode(child);

				child = next;
			}

			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, bool removable = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar(
			);
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (removable && ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), tag.c_str());
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}


		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			DisplayAddComponentEntry<CameraComponent>("Camera");
			DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
			DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
			DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D");
			DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
			DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
			DisplayAddComponentEntry<TextComponent>("Text Component");
			DisplayAddComponentEntry<ScriptComponent>("Script");

			ImGui::EndPopup();
		}
		
		ImGui::PopItemWidth();
		
		DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
			{
				DrawVec3Control("Translation", component.Translation);
				glm::vec3 rotation = glm::degrees(component.Rotation);
				DrawVec3Control("Rotation", rotation);
				component.Rotation = glm::radians(rotation);
				DrawVec3Control("Scale", component.Scale, 1.0f);
			},
			false);

		DrawComponent<RelationshipComponent>("Hierarchy relationship", entity, [&](RelationshipComponent& component)
			{
				Entity parent = Entity(component.Parent, m_Context.get());
				ImGui::Text("Parent: ");
				ImGui::SameLine();
				//ImGui::Text(parent.GetComponent<TagComponent>().Tag.c_str());

				// 1. Create a buffer for the search text
				static char searchBuffer[128] = "";

				// 2. Setup the Preview (what shows when closed)
				const char* preview_value = parent.GetName().c_str();
				Entity newParent = parent;

				// 3. Begin the Combo
				if (ImGui::BeginCombo("##EntitySearchCombo", preview_value))
				{
					// Display the Search Box (InputText)
					if (ImGui::IsWindowAppearing())
						ImGui::SetKeyboardFocusHere(); // Quality of Life: Auto-focus typing
					ImGui::InputText("##Search", searchBuffer, sizeof(searchBuffer));

					// 5. Filter and Display Items
					// Note: For huge lists, use ImGuiListClipper here
					auto view = m_Context->GetAllEntitiesWith<TagComponent>();
					for (auto& e : view)
					{
						Entity candidate = { e, m_Context.get() };
						// Don't show ourselves
						if (candidate == entity)
							continue;

						// Don't show our own children (Circular Dependency)
						if (m_Context->IsDescendant(entity, candidate))
							continue;

						const char* item_text = candidate.GetName().c_str();

						// Simple substring search (case-sensitive here, but you can make it case-insensitive)
						if (searchBuffer[0] != '\0' && strstr(item_text, searchBuffer) == nullptr)
							continue; // Skip items that don't match

						const bool is_selected = (parent == candidate);
						if (ImGui::Selectable(item_text, is_selected))
						{
							// set newparent for selected
							newParent = candidate;

							// Clear search buffer after selection? Optional.
							searchBuffer[0] = '\0'; 
						}

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				if (newParent != parent)
				{
					m_Context->UpdateParent(entity, newParent, true);
				}
			},
			false);

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& component)
			{
				// 1. Color Control
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

				// 2. Texture Control Group
				//    We group this in a tree node or just a separator to make it look distinct
				ImGui::Spacing();
				ImGui::Text("Texture");

				// Calculate the thumbnail size
				float thumbnailSize = 128.0f;

				// Get the texture ID. If the component has no texture, you likely want 
				// to use a "White" or "Checkerboard" default texture from your engine.
				// If you don't have a default texture getter, you can use a colored button as fallback.
				uint32_t textureID = component.Texture ? component.Texture->GetRendererID() : 0; // Replace 0 with your white texture ID if possible

				// Prepare the Drag & Drop area
				// If we have a texture, show the ImageButton with the texture
				if (component.Texture)
				{
					// Image Button (Acts as the thumbnail)
					// We use (void*) cast for the ID because ImGui expects a pointer for IDs
					if (ImGui::ImageButton("##texture", (ImTextureID)(uint64_t)textureID, {thumbnailSize, thumbnailSize}, {0, 1}, {1, 0}))
					{
						// Optional: Click to open file dialog?
					}
				}
				else
				{
					// Placeholder Button when no texture is selected
					if (ImGui::Button("Drag Texture\nHere", { thumbnailSize, thumbnailSize }))
					{
						// Optional: Open generic load dialog
					}
				}

				// --- Drag and Drop Logic (Applies to the item directly above) ---
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM_TEXTURE"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path texturePath = Project::GetAssetFileSystemPath(path);
						component.Texture = Texture2D::Create(texturePath.string());
					}
					ImGui::EndDragDropTarget();
				}

				// --- Texture Properties & Remove Button (Right side of thumbnail) ---
				// If a texture is loaded, show its name and a remove button
				if (component.Texture)
				{
					ImGui::SameLine();

					// Use a group to stack the text and the remove button vertically next to the image
					ImGui::BeginGroup();

					// Display Filename (optional, requires storing path in Texture2D or Component)
					// ImGui::Text(component.Texture->GetPath().c_str()); 
					ImGui::Text("Texture Loaded"); // Placeholder text
					ImGui::Text("Texture Width: %d", component.Texture->GetWidth());
					ImGui::Text("Texture Height: %d", component.Texture->GetHeight());

					ImGui::Checkbox("Use Sub-Texture", &component.IsSubTexture);
					

					// The "Remove" Button
					// We use a red color for the button to indicate a destructive action
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
					bool removeTexture = false;
					if (ImGui::Button("Remove Texture"))
					{
						// cannot set texture to nullptr directly as it is used in subtexture if present. instead use deferred removal
						removeTexture = true;
					}
					ImGui::PopStyleColor(2);

					ImGui::EndGroup();

					// SubTexture group
					if (component.IsSubTexture)
					{
						
						ImVec2 uv0 = ImVec2((float)(component.XSpriteIndex + 0) * component.SpriteWidth / (float)component.Texture->GetWidth(), (float)(component.YSpriteIndex + 1) * component.SpriteHeight / (float)component.Texture->GetHeight());
						ImVec2 uv1 = ImVec2((float)(component.XSpriteIndex + 1) * component.SpriteWidth / (float)component.Texture->GetWidth(), (float)(component.YSpriteIndex + 0) * component.SpriteHeight / (float)component.Texture->GetHeight());
						ImGui::Image(component.Texture->GetRendererID(), ImVec2(thumbnailSize / 2, thumbnailSize / 2), uv0, uv1);
						ImGui::SameLine();
						
						ImGui::BeginGroup();
						
						ImGui::DragInt("Sprite Width", (int*)&component.SpriteWidth, 1, 1, component.Texture->GetWidth());
						ImGui::DragInt("Sprite Height", (int*)&component.SpriteHeight, 1, 1, component.Texture->GetHeight());

						ImGui::DragInt("Sprite Index X", (int*)&component.XSpriteIndex, 1, 0, component.Texture->GetWidth() / component.SpriteWidth - 1);
						ImGui::DragInt("Sprite Index Y", (int*)&component.YSpriteIndex, 1, 0, component.Texture->GetHeight() / component.SpriteHeight - 1);
						
						ImGui::EndGroup();
					}

					if (removeTexture)
					{
						component.IsSubTexture = false;
						component.Texture = nullptr;
					}
				}

				ImGui::Spacing();

				// 3. Tiling Factor
				ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
			});

		DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& component)
			{
				auto& camera = component.Camera;

				ImGui::Checkbox("Primary", &component.Primary);

				const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
				const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
				if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
						if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
						{
							currentProjectionTypeString = projectionTypeStrings[i];
							camera.SetProjectionType((SceneCamera::ProjectionType)i);
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					float verticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
					if (ImGui::DragFloat("Vertical FOV", &verticalFov))
						camera.SetPerspectiveVerticalFOV(glm::radians(verticalFov));

					float perspectiveNear = camera.GetPerspectiveNearClip();
					if (ImGui::DragFloat("Near", &perspectiveNear))
						camera.SetPerspectiveNearClip(perspectiveNear);

					float perspectiveFar = camera.GetPerspectiveFarClip();
					if (ImGui::DragFloat("Far", &perspectiveFar))
						camera.SetPerspectiveFarClip(perspectiveFar);
				}

				if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
				{
					float orthoSize = camera.GetOrthographicSize();
					if (ImGui::DragFloat("Size", &orthoSize))
						camera.SetOrthographicSize(orthoSize);

					float orthoNear = camera.GetOrthographicNearClip();
					if (ImGui::DragFloat("Near", &orthoNear))
						camera.SetOrthographicNearClip(orthoNear);

					float orthoFar = camera.GetOrthographicFarClip();
					if (ImGui::DragFloat("Far", &orthoFar))
						camera.SetOrthographicFarClip(orthoFar);

					ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
				}

			});

		DrawComponent<CircleRendererComponent>("Circle Renderer", entity, [](CircleRendererComponent& component)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
				ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
				ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
			});

		DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", entity, [](Rigidbody2DComponent& component)
			{
				const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
				const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];
				if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
						if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
						{
							currentBodyTypeString = bodyTypeStrings[i];
							component.Type = (Rigidbody2DComponent::BodyType)i;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
			});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& component)
			{
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
				ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
				ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);  
			});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& component)
			{
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
				ImGui::DragFloat("Radius", &component.Radius);
				ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
			});

		DrawComponent<TextComponent>("Text Renderer", entity, [](TextComponent& component)
			{
				ImGui::InputTextMultiline("Text String", &component.TextString);
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

				std::string fontname = "Default";
				if (!component.FontAsset->GetFilePath().empty())
					fontname = component.FontAsset->GetFilePath().filename().string();
				ImGui::Button(fontname.c_str(), ImVec2(-1.0f, 0.0f));

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM_FONT"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path fontPath = Project::GetAssetFileSystemPath(path);
						component.FontAsset = Font::Create(fontPath);
						// TODO: Trigger a reload here!
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::DragFloat("Scale", &component.Scale, 0.025f);
				ImGui::DragFloat2("Allign", glm::value_ptr(component.Allign), 0.025f);
				ImGui::DragFloat("Kerning", &component.Kerning, 0.025f);
				ImGui::DragFloat("Line Spacing", &component.LineSpacing, 0.025f);
			});

		DrawComponent<ScriptComponent>("Script", entity, [](ScriptComponent& component)
			{
				// 1. Script File Name (Drop Target)
				std::string filename = "None";
				if (!component.ScriptPath.empty())
					filename = std::filesystem::path(component.ScriptPath).filename().string();

				ImGui::Button(filename.c_str(), ImVec2(-1.0f, 0.0f));

				// Drag and Drop Logic
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM_SCRIPT"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path scriptPath(path);
						component.ScriptPath = scriptPath.string();
						// TODO: Trigger a reload here!
					}
					ImGui::EndDragDropTarget();
				}

				if (!component.ScriptPath.empty())
				{
					// The "Remove" Button
					// We use a red color for the button to indicate a destructive action
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
					if (ImGui::Button("Remove Script"))
					{
						component.ScriptPath = std::string();
					}
					ImGui::PopStyleColor(2);
				}
			});
	}
}