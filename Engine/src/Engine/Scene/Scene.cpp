#include "egpch.h"
#include "Scene.h"

#include "ScriptGlue.h"

#include "Engine/Utils/Math.h"
#include "Engine/Utils/AudioEngine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Project/Project.h"

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>


namespace Engine {

	template<typename Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto view = src.view<Component>();
		for (auto e : view)
		{
			UUID uuid = src.get<IDComponent>(e).ID;
			ASSERT(enttMap.find(uuid) != enttMap.end(), "Entity from source not found in the map.");
			entt::entity dstEnttID = enttMap.at(uuid);

			auto& component = src.get<Component>(e);
			dst.emplace_or_replace<Component>(dstEnttID, component);
		}
	}

	// relationship component requires a special copy function as the entt::entity handles may be different in src and dst registries
	template<>
	static void CopyComponent<RelationshipComponent>(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto view = src.view<RelationshipComponent>();
		for (auto e : view)
		{
			UUID uuid = src.get<IDComponent>(e).ID;
			ASSERT(enttMap.find(uuid) != enttMap.end(), "Entity from source not found in the map.");
			entt::entity dstEnttID = enttMap.at(uuid);

			auto& srcRelation = src.get<RelationshipComponent>(e);
			auto& dstRelation = dst.get<RelationshipComponent>(dstEnttID);
			
			auto GetMappedEntity = [&](entt::entity originalHandle) -> entt::entity
				{
					// Check for null BEFORE accessing the registry
					if (originalHandle == entt::null)
						return entt::null;

					// ensure the handle is actually valid in source
					if (!src.valid(originalHandle))
						return entt::null;

					UUID originalUUID = src.get<IDComponent>(originalHandle).ID;

					// If the linked entity wasn't copied (e.g. it's outside the selection), return null
					if (enttMap.find(originalUUID) == enttMap.end())
						return entt::null;

					return enttMap.at(originalUUID);
				};

			// Assign to the correct member variables
			dstRelation.Parent = GetMappedEntity(srcRelation.Parent);
			dstRelation.FirstChild = GetMappedEntity(srcRelation.FirstChild);
			dstRelation.NextSibling = GetMappedEntity(srcRelation.NextSibling);
			dstRelation.PrevSibling = GetMappedEntity(srcRelation.PrevSibling);
		}
	}

	template<typename Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		if (src.HasComponent<Component>())
		{
			// 1. Copy by value to a safe local variable first!
			Component copy = src.GetComponent<Component>();

			// 2. Safely emplace the copy
			dst.AddOrReplaceComponent<Component>(copy);
		}
	}

	// Helper to transform child data into parent's local space
	static void AttachColliders(Entity rootEntity, Entity currentEntity, Scene* scene)
	{
		currentEntity.AttachFixturesToRigidbodyParent();
		
		// 6. Recursion
		if (currentEntity.HasComponent<RelationshipComponent>())
		{
			auto& rel = currentEntity.GetComponent<RelationshipComponent>();
			entt::entity childHandle = rel.FirstChild;

			while (childHandle != entt::null)
			{
				Entity child = { childHandle, scene };
				if (!child.HasComponent<Rigidbody2DComponent>())
				{
					AttachColliders(rootEntity, child, scene);
				}
				childHandle = child.GetComponent<RelationshipComponent>().NextSibling;
			}
		}
	}

	static void CreateRigidbody(Entity entity, b2World* physicsWorld, Scene* scene)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		glm::vec3 scale;
		glm::vec3 rotation;
		glm::vec3 translation;
		if (!Math::DecomposeTransform(transform.GlobalTransform, translation, rotation, scale))
		{
			ASSERT(false, "Could not decompose transform.");
			return;
		}
		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = Rigidbody2DTypeToBox2DBody(rb2d.Type);
		bodyDef.position.Set(translation.x, translation.y);
		bodyDef.angle = rotation.z;
		bodyDef.userData.pointer = (uintptr_t)(uint32_t)entity;

		b2Body* body = physicsWorld->CreateBody(&bodyDef);
		body->SetFixedRotation(rb2d.FixedRotation);
		body->SetEnabled(entity.isEnabled());
		rb2d.RuntimeBody = body;

		AttachColliders(entity, entity, scene);
	}


	static void DestroyUnlinkedFixtureRecursive(Entity& entity, Scene* scene)
	{
		// if entity is a rigidbody, children are gauranteed to be linked to it
		if (entity.HasComponent<Rigidbody2DComponent>())
			return;

		entity.DetachFixturesFromRigidbodyParent();

		// recursion
		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& rel = entity.GetComponent<RelationshipComponent>();
			entt::entity childHandle = rel.FirstChild;

			while (childHandle != entt::null)
			{
				Entity child = { childHandle, scene };
				DestroyUnlinkedFixtureRecursive(child, scene);	
				childHandle = child.GetComponent<RelationshipComponent>().NextSibling;
			}
		}
	}

	// To be ued only for debugging
	namespace Debug
	{
		static void PrintHierarchy(Entity entity, Scene* scene, int n=0)
		{
			std::cout << std::left;
			std::cout << std::setw(20) << entity.GetName() << ' ' << std::setw(5) << (uint32_t)entity << ' ' << std::setw(20) << entity.GetUUID();
			auto& rc = entity.GetComponent<RelationshipComponent>();
			Entity child = { rc.FirstChild, scene };
			while (child)
			{
				std::cout << std::endl << std::setw(n*4) << ' ' << '|' << std::string(3, '_');
				PrintHierarchy(child, scene, n+1);
				child = Entity{ child.GetComponent<RelationshipComponent>().NextSibling, scene };
			}
			std::cout << std::endl;
			std::cout << std::right;
		}

		static void PrintHierarchyAsTable(Scene* scene)
		{
			std::cout << "\t\t|" << std::setw(10) << "EnTT ID " << '|' << std::setw(20) << "UUID " << '|' << std::setw(20) << "Name " << '|' << std::setw(5) << "P " << '|' << std::setw(5) << "FC " << '|' << std::setw(5) << "PS " << '|' << std::setw(5) << "NS " << '|' << std::endl;
			auto view = scene->GetAllEntitiesWith<IDComponent,TagComponent,RelationshipComponent>();
			for (auto& e : view)
			{
				auto& ic = view.get<IDComponent>(e);
				auto& tc = view.get<TagComponent>(e);
				auto& rc = view.get<RelationshipComponent>(e);

				std::cout << "\t\t|" << std::setw(10) << (uint32_t)e << '|' << std::setw(20) << ic.ID << '|' << std::setw(20) << tc.Tag << '|' << std::setw(5) << (rc.Parent == entt::null ? -1 : (uint32_t)rc.Parent) << '|' << std::setw(5) << (rc.FirstChild == entt::null ? -1 : (uint32_t)rc.FirstChild) << '|' << std::setw(5) << (rc.PrevSibling == entt::null ? -1 : (uint32_t)rc.PrevSibling) << '|' << std::setw(5) << (rc.NextSibling == entt::null ? -1 : (uint32_t)rc.NextSibling) << '|' << std::endl;
			}
		}
	}
		
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Scene //////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Scene::Scene()
	{
		m_SceneRoot = CreateEntity("::SCENE_ROOT::");
	}

	Scene::~Scene()
	{
		if (m_Lua)
			OnScriptingStop();
		if (m_PhysicsWorld)
			OnPhysics2DStop();
	}

	std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
	{
		std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;
		newScene->m_Acc = other->m_Acc;
		newScene->m_SceneName = other->m_SceneName;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			if (e != other->m_SceneRoot)
			{
				const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
				Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
				enttMap[uuid] = (entt::entity)newEntity;
			}
			else
			{
				enttMap[uuid] = newScene->m_SceneRoot;
			}
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<ScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<Rigidbody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<TextComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<RelationshipComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<DisabledComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<AudioSourceComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		ASSERT(entity.BelongsToScene(this), "This Scene cannot duplicate entity of other scene.")
		if (entity == m_SceneRoot)
		{
			ASSERT(false, "Trying duplicate root is not valid.")
				return Entity{};
		}

		std::unordered_map<entt::entity, entt::entity> duplicationMap;
		CreateDuplicationMap(entity, duplicationMap);

		// A helper lambda to safely translate an original handle to a clone handle
		auto getMapped = [&](entt::entity originalHandle) -> entt::entity {
			if (duplicationMap.find(originalHandle) != duplicationMap.end())
				return duplicationMap[originalHandle];
			return entt::null;
		};

		for (auto& [originalHandle, cloneHandle] : duplicationMap)
		{
			auto& originalRel = m_Registry.get<RelationshipComponent>(originalHandle);
			auto& cloneRel = m_Registry.get<RelationshipComponent>(cloneHandle);

			// Remap internal tree pointers
			cloneRel.Parent = getMapped(originalRel.Parent);
			cloneRel.FirstChild = getMapped(originalRel.FirstChild);
			cloneRel.NextSibling = getMapped(originalRel.NextSibling);
			cloneRel.PrevSibling = getMapped(originalRel.PrevSibling);

			Entity originalEntity{ originalHandle, this };
			Entity cloneEntity{ cloneHandle, this };
		}

		Entity newEntity = Entity{ duplicationMap[entity], this };
		auto& newRelation = newEntity.GetComponent<RelationshipComponent>();
		auto& oldRelation = entity.GetComponent<RelationshipComponent>();
		Entity nextsibling = Entity{ oldRelation.NextSibling, this };
		oldRelation.NextSibling = newEntity;
		newRelation.PrevSibling = entity;
		newRelation.Parent = oldRelation.Parent;
		newRelation.NextSibling = nextsibling;
		if (nextsibling)
			nextsibling.GetComponent<RelationshipComponent>().PrevSibling = newEntity;

		// The tree is now 100% built and stitched. It is completely safe to run Lua!
		if (m_Lua)
		{
			for (auto& [originalHandle, cloneHandle] : duplicationMap)
			{
				Entity cloneEntity{ cloneHandle, this };
				if (cloneEntity.HasComponent<ScriptComponent>())
				{
					cloneEntity.OnScriptStart(); // Fire OnCreate safely!
				}
			}
		}

		return newEntity;
	}

	void Scene::CreateDuplicationMap(Entity& entity, std::unordered_map<entt::entity, entt::entity>& map)
	{
		if (entity == m_SceneRoot)
		{
			ASSERT(false, "Trying duplicate root is not valid.")
			return;
		}

		std::string name = entity.GetName();
		ENGINE_LOG_INFO("Duplicating entity {}", name);
		Entity newEntity = CreateEntity(name);

		CopyComponentIfExists<TransformComponent>(newEntity, entity);
		CopyComponentIfExists<DisabledComponent>(newEntity, entity);
		CopyComponentIfExists<CameraComponent>(newEntity, entity);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
		CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
		CopyComponentIfExists<Rigidbody2DComponent>(newEntity, entity);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity, entity);
		CopyComponentIfExists<TextComponent>(newEntity, entity);
		CopyComponentIfExists<ScriptComponent>(newEntity, entity);
		CopyComponentIfExists<AudioSourceComponent>(newEntity, entity);

		auto& srcRelation = entity.GetComponent<RelationshipComponent>();

		Entity child = Entity(srcRelation.FirstChild, this);
		while (child)
		{
			CreateDuplicationMap(child, map);
			child = Entity(child.GetComponent<RelationshipComponent>().NextSibling, this);
		}
		
		map[entity] = newEntity;
	}


	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateNewChildEntity(Entity parentEntity)
	{
		ASSERT(parentEntity.BelongsToScene(this), "This Scene cannot create a child entity of a parent of other other scene.")

		auto& childEntity = CreateEntity("Empty entity");
		auto& childRelation = childEntity.GetComponent<RelationshipComponent>();
		auto& parentRelation = parentEntity.GetComponent<RelationshipComponent>();

		childRelation.Parent = parentEntity;
		if (parentRelation.FirstChild != entt::null)
		{
			Entity oldFirstChild = { parentRelation.FirstChild, this };
			auto& oldChildRelation = oldFirstChild.GetComponent<RelationshipComponent>();
			oldChildRelation.PrevSibling = childEntity;
		}
		childRelation.NextSibling = parentRelation.FirstChild;
		parentRelation.FirstChild = childEntity;

		UpdateTransformRecursive(childEntity, parentEntity.GetComponent<TransformComponent>().GlobalTransform);

		return childEntity;
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string & name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		auto& relation = entity.AddComponent<RelationshipComponent>();
		relation.Parent = m_SceneRoot;
		entity.AddComponent<TransformComponent>();

		return entity;
	}
	
	void Scene::DestroyEntity(Entity entity)
	{
		ASSERT(entity.BelongsToScene(this), "This Scene cannot destroy entity of other scene.")

		if (entity == m_SceneRoot)
			return;

		auto& entityRelation = entity.GetComponent<RelationshipComponent>();
		
		if (entityRelation.FirstChild != entt::null)
		{
			Entity child = Entity(entityRelation.FirstChild, this);
			while (child)
			{
				// If delete the child, 'child' becomes invalid, and trying to get 'child.GetComponent' in the next line would CRASH.
				Entity next = { child.GetComponent<RelationshipComponent>().NextSibling, this };

				DestroyEntity(child);

				child = next;
			}
		}

		if (entityRelation.PrevSibling != entt::null)
		{
			Entity prev = Entity(entityRelation.PrevSibling, this);
			prev.GetComponent<RelationshipComponent>().NextSibling = entityRelation.NextSibling;
		}

		if (entityRelation.NextSibling != entt::null)
		{
			Entity next = Entity(entityRelation.NextSibling, this);
			next.GetComponent<RelationshipComponent>().PrevSibling = entityRelation.PrevSibling;
		}
		
		Entity parent = Entity(entityRelation.Parent, this);
		if (parent.GetComponent<RelationshipComponent>().FirstChild == entity)
		{
			parent.GetComponent<RelationshipComponent>().FirstChild = entityRelation.NextSibling;
		}

		m_Registry.destroy(entity);
	}

	bool Scene::IsDescendant(Entity potentialAncestor, Entity potentialDescendant)
	{
		ASSERT(potentialAncestor.BelongsToScene(this), "This Scene cannot verify ancestry of other scene.")
		ASSERT(potentialDescendant.BelongsToScene(this), "This Scene cannot verify descentry of other scene.")

		// Walk up the tree from 'potentialDescendant'
		// If we hit 'potentialAncestor', then it IS a descendant (bad!)
		if (!potentialDescendant.HasComponent<RelationshipComponent>()) return false;

		auto& rel = potentialDescendant.GetComponent<RelationshipComponent>();
		Entity parent = { rel.Parent, this }; // or m_Context

		while (parent)
		{
			if (parent == potentialAncestor)
				return true; // Found a loop!

			if (!parent.HasComponent<RelationshipComponent>()) break;
			parent = { parent.GetComponent<RelationshipComponent>().Parent, this };
		}
		return false;
	}

	void Scene::UpdateParent(Entity& child, Entity& newParent, bool keepWorldTransform)
	{
		ASSERT(child.BelongsToScene(this), "This Scene cannot modify child of other scene.")
		ASSERT(newParent.BelongsToScene(this), "This child cannot update parent to other scene.")

		auto& childRelation = child.GetComponent<RelationshipComponent>();
		auto& newParentRelation = newParent.GetComponent<RelationshipComponent>();
		Entity oldParent = { childRelation.Parent, this };
		auto& oldParentRelation = oldParent.GetComponent<RelationshipComponent>();

		// Update relationship components
		{
			Entity oldPrevSibling = { childRelation.PrevSibling, this };
			Entity oldNextSibling = { childRelation.NextSibling, this };
			if (oldNextSibling)
				oldNextSibling.GetComponent<RelationshipComponent>().PrevSibling = oldPrevSibling;
			if (oldPrevSibling)
			{
				// child was not the first child of previous parent
				oldPrevSibling.GetComponent<RelationshipComponent>().NextSibling = oldNextSibling;
			}
			else
			{
				// child was the first child of previous parent
				oldParentRelation.FirstChild = childRelation.NextSibling;
			}

			Entity newParentFirstChild = { newParentRelation.FirstChild, this };
			if (newParentFirstChild)
				newParentFirstChild.GetComponent<RelationshipComponent>().PrevSibling = child;
			newParentRelation.FirstChild = child;
			childRelation.NextSibling = newParentFirstChild;
			childRelation.PrevSibling = entt::null;
			childRelation.Parent = newParent;
		}

		// update local transform if world transform is to be kept same
		if (keepWorldTransform)
		{
			// We calculate what Local Position is needed to maintain the current World Position
			// NewLocal = Inverse(NewParentWorld) * OldWorld
			auto& childTransform = child.GetComponent<TransformComponent>();
			auto& newParentTransform = newParent.GetComponent<TransformComponent>();

			glm::mat4 parentInverse = glm::inverse(newParentTransform.GlobalTransform);
			glm::mat4 newLocal = parentInverse * childTransform.GlobalTransform;

			Math::DecomposeTransform(newLocal, childTransform.Translation, childTransform.Rotation, childTransform.Scale);
		}

		// Update Global transforms recursively for all children
		UpdateTransformRecursive(child, newParent.GetComponent<TransformComponent>().GlobalTransform);

		// if physics world is not nullptr, it means we are already in simulation/play mode and this was likely called from a script.
		// so we need to update the runtime fixtures and bodies
		if (m_PhysicsWorld)
		{
			// update fixtures only if child does not have rigidbody, otherwise everything already is relative to this child or it's children
			if (!child.HasComponent<Rigidbody2DComponent>())
			{
				// first we destroy fixtures from parent bodies
				DestroyUnlinkedFixtureRecursive(child, this);

				// find closes parent with rigidbody
				Entity e = child;
				while (e && !e.HasComponent<Rigidbody2DComponent>())
					e = { e.GetComponent<RelationshipComponent>().Parent, this };
			
				if (e)
				{
					AttachColliders(e, child, this);
				}
			}
		}

		ENGINE_LOG_INFO("Parent of '{0}' changed from '{1}' to '{2}'", child.GetName(), oldParent.GetName(), newParent.GetName());
	}

	void Scene::UpdateGlobalTransforms()
	{
		// Start directly at the top. 
		// The Root's transform is always Identity, so passing Identity is correct.
		UpdateTransformRecursive(m_SceneRoot, glm::mat4(1.0f));
	}

	void Scene::UpdateTransformRecursive(entt::entity entity, const glm::mat4& parentTransform)
	{
		// Optimization: dont propogate down disabled hierarchy
		if (m_Registry.has<DisabledComponent>(entity))
			return;
		// Calculate Global = Parent * Local
		// (Optimization: If entity == m_SceneRoot, we know it's Identity, but the math holds up anyway)
		auto& tc = m_Registry.get<TransformComponent>(entity);
		tc.GlobalTransform = parentTransform * tc.GetTransform();

		// Iterate the Linked List of Children
		auto& rel = m_Registry.get<RelationshipComponent>(entity);
		entt::entity child = rel.FirstChild;

		while (child != entt::null)
		{
			// Recursion
			UpdateTransformRecursive(child, tc.GlobalTransform);

			// Next Sibling
			child = m_Registry.get<RelationshipComponent>(child).NextSibling;
		}
	}

	void Scene::SyncPhysicsToTransform(Entity entity)
	{
		ASSERT(entity.BelongsToScene(this), "This Scene cannot sync physics of entity of other scene.")
		// Safety: Only works if physics is running
		if (!m_PhysicsWorld) return;

		// Handle Rigidbodies (Update Position/Rotation)
		// If the entity has a body, we can just teleport it to the new position.
		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			if (rb2d.RuntimeBody)
			{
				b2Body* body = (b2Body*)rb2d.RuntimeBody;

				glm::vec3 scale;
				glm::vec3 rotation;
				glm::vec3 translation;
				if (!Math::DecomposeTransform(entity.GetComponent<TransformComponent>().GlobalTransform, translation, rotation, scale))
				{
					ASSERT(false, "Could not decompose transform.");
					return;
				}
				body->SetTransform(b2Vec2(translation.x, translation.y), rotation.z);
				body->SetAwake(true);

				AttachColliders(entity, entity, this);
			}
			return;
		}

		entity.AttachFixturesToRigidbodyParent();
		
	}

	void Scene::OnRuntimeStart()
	{
		UpdateGlobalTransforms();
		OnPhysics2DStart();
		
		// Load all sounds into memory
		auto view = m_Registry.view<AudioSourceComponent>();
		for (auto entityID : view)
		{
			auto& audio = view.get<AudioSourceComponent>(entityID);

			// Initialize the sound object and store the pointer in the component
			audio.SoundHandle = AudioEngine::LoadSound(Project::GetAssetFileSystemPath(audio.FilePath).string(), audio.Loop);

			// Play immediately if flagged
			if (audio.PlayOnAwake)
			{
				AudioEngine::StartSound(audio.SoundHandle, audio.Volume, audio.Pitch);
				audio.IsPlaying = true;
			}
		}

		OnScriptingStart();
		UpdateGlobalTransforms();
	}

	void Scene::OnRuntimeStop()
	{
		OnScriptingStop();
		OnPhysics2DStop();
		// Clean up memory when the scene ends to prevent memory leaks!
		auto view = m_Registry.view<AudioSourceComponent>();
		for (auto entityID : view)
		{
			auto& audio = view.get<AudioSourceComponent>(entityID);
			if (AudioEngine::IsSoundPlaying(audio.SoundHandle))
				AudioEngine::StopSound(audio.SoundHandle);
			AudioEngine::UnloadSound(audio.SoundHandle);
			audio.SoundHandle = nullptr;
			audio.IsPlaying = false;
		}
	}

	void Scene::OnSimulationStart()
	{
		UpdateGlobalTransforms();
		OnPhysics2DStart();
	}

	void Scene::OnSimulationStop()
	{
		OnPhysics2DStop();
	}

	void Scene::OnUpdateRuntime(float ts)
	{
		// Update scripts
		RunScripts(ts);

		// Update global transforms
		UpdateGlobalTransforms();

		// Physics
		OnUpdatePhysics2D(ts);

		// Update global transforms
		UpdateGlobalTransforms();

		auto audioView = m_Registry.view<AudioSourceComponent>();
		for (auto entityID : audioView)
		{
			auto& audio = audioView.get<AudioSourceComponent>(entityID);

			// Only check sounds that the engine *thinks* are playing
			if (audio.IsPlaying)
			{
				if (!AudioEngine::IsSoundPlaying(audio.SoundHandle))
				{
					// The sound finished! Reset the flag.
					audio.IsPlaying = false;

				}
			}
		}

		// Render 2D
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GlobalTransform;
					break;
				}
			}
		}

		if (mainCamera)
		{
			// Render
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			RenderScene();

			Renderer2D::EndScene();
		}
	}

	void Scene::OnUpdateSimulation(float ts, EditorCamera& camera)
	{
		// Physics
		OnUpdatePhysics2D(ts);
		
		// Update global transforms
		UpdateGlobalTransforms();
		
		// Render
		Renderer2D::BeginScene(camera);

		RenderScene();

		Renderer2D::EndScene();
	}

	void Scene::OnUpdateEditor(float ts, EditorCamera& camera)
	{
		// Update global transforms
		UpdateGlobalTransforms();

		// Render
		Renderer2D::BeginScene(camera);

		RenderScene();
		
		Renderer2D::EndScene();
	}

	void Scene::OnPhysics2DStart()
	{
		m_PhysicsWorld = new b2World({ m_Acc.x, m_Acc.y });
		m_ContactListener = new PhysicsContactListener(this);
		m_PhysicsWorld->SetContactListener(m_ContactListener);

		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			CreateRigidbody(entity, m_PhysicsWorld, this);
		}
	}

	void Scene::OnPhysics2DStop()
	{
		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;

		delete m_ContactListener;
		m_ContactListener = nullptr;
	}

	void Scene::OnUpdatePhysics2D(float ts)
	{
		
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 2;
		m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);
		// Fire pending callbacks
		m_ContactListener->FireCallbacks();

		// Retrieve transform from Box2D
		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			
			if (rb2d.Type == Rigidbody2DComponent::BodyType::Static)
				continue;	// skip calculations for static body for optimization

			auto& transform = entity.GetComponent<TransformComponent>();
			auto& parentTransform = Entity(entity.GetComponent<RelationshipComponent>().Parent, this).GetComponent<TransformComponent>();

			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			const auto& position = body->GetPosition();

			glm::mat4 inverseParent = glm::inverse(parentTransform.GlobalTransform);

			// Apply Inverse to the Box2D World Position. This subtracts the parent's position, rotation, and scale mathematically
			glm::vec4 localPos = inverseParent * glm::vec4(glm::vec3{position.x, position.y, transform.Translation.z}, 1.0f);

			transform.Translation.x = localPos.x;
			transform.Translation.y = localPos.y;

			// We don't care about Position/Scale here, just Rotation.
			glm::vec3 pPos, pRot, pScale;
			Math::DecomposeTransform(parentTransform.GlobalTransform, pPos, pRot, pScale);

			// New Local = Child World - Parent World
			transform.Rotation.z = body->GetAngle() - pRot.z;
		}
		
	}

	void Scene::OnScriptingStart()
	{
		m_Lua = new sol::state();

		// Load standard libraries
		m_Lua->open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::table,   // For table manipulation
			sol::lib::math,    // Needed for math.sin, math.cos, math.pi
			sol::lib::string,  // Needed for string manipulation
			sol::lib::os       // Optional: Needed if you want os.time() for random seeds
		);

		// Just add the root scripts folder
		std::string scriptPath = Project::GetAssetDirectory().string() + "/?.lua;";
		(*m_Lua)["package"]["path"] = scriptPath + (*m_Lua)["package"]["path"].get_or<std::string>("");

		// bind lua types and functions
		BindLuaTypesAndFunctions(m_Lua, this);

		// Iterate all entities with scripts
		auto view = m_Registry.view<ScriptComponent, TagComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			entity.OnScriptStart();
		}
	}

	void Scene::OnScriptingStop()
	{
		auto view = m_Registry.view<ScriptComponent>();
		for (auto e : view)
		{
			auto& sc = view.get<ScriptComponent>(e);

			// Check if instance exists
			if (sc.Instance.valid())
			{
				// Call OnDestroy(self)
				if (sc.Instance["OnDestroy"].valid())
				{
					// We must pass 'sc.Instance' as the first argument to simulate ':' call
					sc.Instance["OnDestroy"](sc.Instance);
				}
			}

			// Assigning a default (empty) table disconnects it from the Lua State.
			sc.Instance = sol::nil;
		}

		// IMPORTANT : DO NOT REMOVE
		// clear runtime function registry
		RuntimeData::ClearFunctionRegistry();
		m_ScriptCache.clear();
		delete m_Lua;
		m_Lua = nullptr;
	}

	void Scene::RunScripts(float ts)
	{
		// Update Scripts
		auto view = m_Registry.view<ScriptComponent>();
		for (auto e : view)
		{
			auto& sc = view.get<ScriptComponent>(e);

			// Check if instance exists
			if (!sc.Instance.valid())
				continue;

			// Use protected_function to catch errors
			if (sc.Instance["OnUpdate"].valid())
			{
				sol::protected_function onUpdate = sc.Instance["OnUpdate"];

				// Call the function and capture the result
				sol::protected_function_result result = onUpdate(sc.Instance, ts);

				// Check if the script crashed
				if (!result.valid())
				{
					sol::error err = result;
					// LOG_ERROR("Lua Error: {0}", err.what()); // Use your engine logger
					std::cout << "Lua Error: " << err.what() << std::endl;
				}
			}

		}
	}

	void Scene::RenderScene()
	{

		// Draw sprites
		{
			auto view = m_Registry.view<TransformComponent,SpriteRendererComponent>(entt::exclude<DisabledComponent>);
			for (auto entity : view)
			{
				auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawSprite(transform.GlobalTransform, sprite, (int)entity);
			}
		}

		// Draw circles
		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>(entt::exclude<DisabledComponent>);
			for (auto entity : view)
			{
				auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

				Renderer2D::DrawCircle(transform.GlobalTransform, circle.Color, circle.Thickness, circle.Fade, (int)entity);
			}
		}

		// Draw text
		{
			auto view = m_Registry.view<TransformComponent, TextComponent>(entt::exclude<DisabledComponent>);
			for (auto entity : view)
			{
				auto [transform, text] = view.get<TransformComponent, TextComponent>(entity);

				Renderer2D::DrawString(text.TextString, transform.GlobalTransform, text, (int)entity);
			}
		}

	}


	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{ entity, this };
		}
		return {};
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}
	
	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
		Entity parent = { entity.GetComponent<RelationshipComponent>().Parent, this };
		if (parent)
			component.GlobalTransform = parent.GetComponent<TransformComponent>().GlobalTransform * component.GetTransform();
		else // it's scene root
			component.GlobalTransform = glm::mat4(1);
		
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
		component.RuntimeBody = nullptr;
		if(m_PhysicsWorld)
			CreateRigidbody(entity, m_PhysicsWorld, this);
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
		if (m_PhysicsWorld)
		{
			component.ClosestRigidbodyParent = entity.ClosestRigidbodyParent();
			component.RuntimeFixture = nullptr;
			SyncPhysicsToTransform(entity);
		}
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
		if (m_PhysicsWorld)
		{
			component.ClosestRigidbodyParent = entity.ClosestRigidbodyParent();
			component.RuntimeFixture = nullptr;
			SyncPhysicsToTransform(entity);
		}
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<DisabledComponent>(Entity entity, DisabledComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<AudioSourceComponent>(Entity entity, AudioSourceComponent& component)
	{
		if (m_Lua)
		{
			component.SoundHandle = AudioEngine::LoadSound(Project::GetAssetFileSystemPath(component.FilePath).string(), component.Loop);

			// Play immediately if flagged
			if (component.PlayOnAwake)
			{
				AudioEngine::StartSound(component.SoundHandle, component.Volume, component.Pitch);
				component.IsPlaying = true;
			}
		}
	}
}