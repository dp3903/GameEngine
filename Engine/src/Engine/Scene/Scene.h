#pragma once

#include "Engine/Utils/UUID.h"
#include "Engine/Renderer/EditorCamera.h"
#include "entt.hpp"
#include "sol/sol.hpp"

class b2World;

namespace Engine {

	// Forward declaration. do not import actual Entity class as it will cause circular dependency loop.
	class Entity;
	class PhysicsContactListener;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateNewChildEntity(Entity parentEntity);
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		static std::shared_ptr<Scene> Copy(std::shared_ptr<Scene> other);

		bool IsDescendant(Entity potentialAncestor, Entity potentialDescendant);
		void UpdateParent(Entity& child, Entity& newParent, bool keepWorldTransform=false);
		
		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnSimulationStart();
		void OnSimulationStop();
		
		void OnUpdateRuntime(float ts);
		void OnUpdateSimulation(float ts, EditorCamera& camera);
		void OnUpdateEditor(float ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		
		void DuplicateEntity(Entity entity);

		const entt::entity& GetSceneRoot() { return m_SceneRoot; }
		Entity GetPrimaryCameraEntity();

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		std::string m_SceneName = "Untitled Scene";
		glm::vec2 m_Acc = { 0.0f, -9.8f };

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		Entity DuplicateEntityRecursive(Entity entity);
		void UpdateGlobalTransforms();
		void UpdateTransformRecursive(entt::entity entity, const glm::mat4& parentTransform);
		void SyncPhysicsToTransform(Entity entity);

		void OnPhysics2DStart();
		void OnPhysics2DStop();
		void OnUpdatePhysics2D(float ts);

		void OnScriptingStart();
		void OnScriptingStop();
		void RunScripts(float ts);

		void RenderScene();

	private:
		entt::registry m_Registry;
		entt::entity m_SceneRoot;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		
		b2World* m_PhysicsWorld = nullptr;
		PhysicsContactListener* m_ContactListener = nullptr;
		sol::state* m_Lua = nullptr;

	friend class Entity;
	friend class SceneHierarchyPanel;
	friend class SceneSerializer;
	friend void BindLuaTypesAndFunctions(sol::state* m_Lua, Scene* scene);
	};

}