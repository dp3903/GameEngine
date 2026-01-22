#pragma once

#include "Engine/Renderer/EditorCamera.h"
#include "entt.hpp"

namespace Engine {

	// Forward declaration. do not import actual Entity class as it will cause circular dependency loop.
	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnUpdateRuntime(float ts);
		void OnUpdateEditor(float ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		
		Entity GetPrimaryCameraEntity();

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	friend class Entity;
	friend class SceneHierarchyPanel;
	friend class SceneSerializer;
	};

}