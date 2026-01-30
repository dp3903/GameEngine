#pragma once

#include "Engine/Utils/UUID.h"
#include "Engine/Renderer/EditorCamera.h"
#include "entt.hpp"


class b2World;

namespace Engine {

	// Forward declaration. do not import actual Entity class as it will cause circular dependency loop.
	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);
		
		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();
		
		void OnUpdateRuntime(float ts);
		void OnUpdateSimulation(float ts, EditorCamera& camera);
		void OnUpdateEditor(float ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		
		void DuplicateEntity(Entity entity);

		Entity GetPrimaryCameraEntity();

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		void OnPhysics2DStart();
		void OnPhysics2DStop();
		void OnUpdatePhysics2D(float ts);

		void OnScriptingStart();
		void OnScriptingStop();
		void RunScripts(float ts);

		void RenderScene();

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		
		b2World* m_PhysicsWorld = nullptr;

	friend class Entity;
	friend class SceneHierarchyPanel;
	friend class SceneSerializer;
	};

}