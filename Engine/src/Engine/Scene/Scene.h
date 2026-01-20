#pragma once

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

		void OnUpdate(float ts);
		void OnViewportResize(uint32_t width, uint32_t height);
	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	friend class Entity;
	friend class SceneHierarchyPanel;
	};

}