#include "egpch.h"
#include "Scene.h"

#include "ScriptGlue.h"

#include "Engine/Utils/Math.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Project/Project.h"

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>


namespace Engine {

	static b2BodyType Rigidbody2DTypeToBox2DBody(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
		case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}

		ASSERT(false, "Unknown body type");
		return b2_staticBody;
	}

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
			dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
	}

	// Helper to transform child data into parent's local space
	static void AttachColliders(b2Body* body, Entity rootEntity, Entity currentEntity, Scene* scene)
	{
		// Calculate Relative Transform
		glm::mat4 parentTransform = rootEntity.GetComponent<TransformComponent>().GlobalTransform;
		glm::mat4 childTransform = currentEntity.GetComponent<TransformComponent>().GlobalTransform;

		// Extract the Parent's pure Position and Rotation
		glm::vec3 pPos, pRot, pScale;
		Math::DecomposeTransform(parentTransform, pPos, pRot, pScale);

		// Rebuild the Parent Transform WITHOUT scale
		// Box2D bodies act as the anchor, and they only understand position and rotation.
		glm::mat4 unscaledParentTransform = glm::translate(glm::mat4(1.0f), pPos) *
			glm::rotate(glm::mat4(1.0f), pRot.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), pRot.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), pRot.z, glm::vec3(0.0f, 0.0f, 1.0f));

		// Calculate relative matrix against the UNSCALED parent body
		glm::mat4 relativeMatrix = glm::inverse(unscaledParentTransform) * childTransform;

		// Decompose to get the correct absolute scale and offset
		glm::vec3 relativePos, relativeRot, relativeScale;
		Math::DecomposeTransform(relativeMatrix, relativePos, relativeRot, relativeScale);

		// 4. Attach Box Collider
		if (currentEntity.HasComponent<BoxCollider2DComponent>())
		{
			auto& bc2d = currentEntity.GetComponent<BoxCollider2DComponent>();
			b2PolygonShape boxShape;

			// Size matches the component settings * the entity's scale
			float width = bc2d.Size.x * relativeScale.x;
			float height = bc2d.Size.y * relativeScale.y;

			// SetAsBox takes "Half Width/Height", "Center Position", "Angle"
			boxShape.SetAsBox(
				width / 2.0f,
				height / 2.0f,
				b2Vec2(
					relativePos.x + bc2d.Offset.x * relativeScale.x,
					relativePos.y + bc2d.Offset.y * relativeScale.y
				),
				relativeRot.z);

			// Define Fixture
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &boxShape;
			fixtureDef.density = bc2d.Density;
			fixtureDef.friction = bc2d.Friction;
			fixtureDef.restitution = bc2d.Restitution;
			fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
			fixtureDef.userData.pointer = (uintptr_t)(uint32_t)currentEntity;

			bc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
			bc2d.ClosestRigidbodyParent = rootEntity;
		}

		// 5. Attach Circle Collider
		if (currentEntity.HasComponent<CircleCollider2DComponent>())
		{
			auto& cc2d = currentEntity.GetComponent<CircleCollider2DComponent>();
			b2CircleShape circleShape;

			float maxScale = std::max(relativeScale.x, relativeScale.y);

			// For circles, we just set Position (m_p)
			circleShape.m_p.Set(
				relativePos.x + cc2d.Offset.x * relativeScale.x,
				relativePos.y + cc2d.Offset.y * relativeScale.y
			);
			circleShape.m_radius = cc2d.Radius * maxScale;

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &circleShape;
			fixtureDef.density = cc2d.Density;
			fixtureDef.friction = cc2d.Friction;
			fixtureDef.restitution = cc2d.Restitution;
			fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
			fixtureDef.userData.pointer = (uintptr_t)(uint32_t)currentEntity;

			cc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
			cc2d.ClosestRigidbodyParent = rootEntity;
		}

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
					AttachColliders(body, rootEntity, child, scene);
				}
				childHandle = child.GetComponent<RelationshipComponent>().NextSibling;
			}
		}
	}

	static void DestroyUnlinkedFixtureRecursive(Entity& entity, Scene* scene)
	{
		// if entity is a rigidbody, children are gauranteed to be linked to it
		if (entity.HasComponent<Rigidbody2DComponent>())
			return;

		// destroy box collider
		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
			if (bc2d.ClosestRigidbodyParent != entt::null)
			{
				Entity oldRigidbodyParent = { bc2d.ClosestRigidbodyParent, scene };
				b2Body* body = (b2Body*)oldRigidbodyParent.GetComponent<Rigidbody2DComponent>().RuntimeBody;
				body->DestroyFixture((b2Fixture*)bc2d.RuntimeFixture);
				bc2d.RuntimeFixture = nullptr;
			}
		}

		// destroy circle collider
		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
			if (cc2d.ClosestRigidbodyParent != entt::null)
			{
				Entity oldRigidbodyParent = { cc2d.ClosestRigidbodyParent, scene };
				b2Body* body = (b2Body*)oldRigidbodyParent.GetComponent<Rigidbody2DComponent>().RuntimeBody;
				body->DestroyFixture((b2Fixture*)cc2d.RuntimeFixture);
				cc2d.RuntimeFixture = nullptr;
			}
		}

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

		return newScene;
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		if (entity == m_SceneRoot)
		{
			ASSERT(false, "Trying duplicate root is not valid.")
			return;
		}

		Entity newEntity = DuplicateEntityRecursive(entity);

		auto& srcRelation = entity.GetComponent<RelationshipComponent>();
		auto& dstRelation = newEntity.GetComponent<RelationshipComponent>();

		if (srcRelation.NextSibling != entt::null)
		{
			Entity nextOld = Entity(srcRelation.NextSibling, this);
			nextOld.GetComponent<RelationshipComponent>().PrevSibling = newEntity;
		}
		dstRelation.Parent = srcRelation.Parent;
		dstRelation.NextSibling = srcRelation.NextSibling;
		dstRelation.PrevSibling = entity;
		srcRelation.NextSibling = newEntity;
	}

	Entity Scene::DuplicateEntityRecursive(Entity entity)
	{
		if (entity == m_SceneRoot)
		{
			ASSERT(false, "Trying duplicate root is not valid.")
			return Entity();
		}

		std::string name = entity.GetName();
		ENGINE_LOG_INFO("Duplicating entity {}", name);
		Entity newEntity = CreateEntity(name);

		CopyComponentIfExists<TransformComponent>(newEntity, entity);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
		CopyComponentIfExists<CameraComponent>(newEntity, entity);
		CopyComponentIfExists<ScriptComponent>(newEntity, entity);
		CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
		CopyComponentIfExists<Rigidbody2DComponent>(newEntity, entity);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity, entity);
		CopyComponentIfExists<TextComponent>(newEntity, entity);

		auto& srcRelation = entity.GetComponent<RelationshipComponent>();

		if (srcRelation.FirstChild != entt::null)
		{
			Entity prevNew;
			Entity crntOld = Entity(srcRelation.FirstChild, this);
			while (crntOld)
			{
				Entity crntNew = DuplicateEntityRecursive(crntOld);
				auto& crntNewRelation = crntNew.GetComponent<RelationshipComponent>();
				crntNewRelation.Parent = newEntity;
				crntNewRelation.PrevSibling = prevNew;
				if (prevNew)
					prevNew.GetComponent<RelationshipComponent>().NextSibling = crntNew;
				else
					newEntity.GetComponent<RelationshipComponent>().FirstChild = crntNew;
				prevNew = crntNew;
				crntOld = Entity(crntOld.GetComponent<RelationshipComponent>().NextSibling, this);
			}
		}

		return newEntity;
	}


	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateNewChildEntity(Entity parentEntity)
	{
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
		entity.AddComponent<TransformComponent>();
		auto& relation = entity.AddComponent<RelationshipComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		relation.Parent = m_SceneRoot;

		return entity;
	}
	
	void Scene::DestroyEntity(Entity entity)
	{
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
					AttachColliders((b2Body*)e.GetComponent<Rigidbody2DComponent>().RuntimeBody, e, child, this);
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
				
				b2Fixture* fixture = body->GetFixtureList();

				while (fixture)
				{
					b2Fixture* nextFixture = fixture->GetNext();

					body->DestroyFixture(fixture);

					fixture = nextFixture;
				}

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

				AttachColliders(body, entity, entity, this);
			}
			return;
		}

		// 4. Attach Box Collider
		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

			// Calculate Relative Transform
			Entity parentEntity = Entity{ bc2d.ClosestRigidbodyParent, this };
			if (!parentEntity)
			{
				ENGINE_LOG_ERROR("No parent rigid body found for '{0}.'", entity.GetName());
				return;
			}
			glm::mat4 parentTransform = parentEntity.GetComponent<TransformComponent>().GlobalTransform;
			glm::mat4 childTransform = entity.GetComponent<TransformComponent>().GlobalTransform;

			// Extract the Parent's pure Position and Rotation
			glm::vec3 pPos, pRot, pScale;
			Math::DecomposeTransform(parentTransform, pPos, pRot, pScale);

			// Rebuild the Parent Transform WITHOUT scale
			// Box2D bodies act as the anchor, and they only understand position and rotation.
			glm::mat4 unscaledParentTransform = glm::translate(glm::mat4(1.0f), pPos) *
				glm::rotate(glm::mat4(1.0f), pRot.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
				glm::rotate(glm::mat4(1.0f), pRot.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
				glm::rotate(glm::mat4(1.0f), pRot.z, glm::vec3(0.0f, 0.0f, 1.0f));

			// Calculate relative matrix against the UNSCALED parent body
			glm::mat4 relativeMatrix = glm::inverse(unscaledParentTransform) * childTransform;

			// Decompose to get relative position and scale
			glm::vec3 relativePos, relativeRot, relativeScale;
			Math::DecomposeTransform(relativeMatrix, relativePos, relativeRot, relativeScale);

			b2PolygonShape boxShape;

			// Size matches the component settings * the entity's scale
			float width = bc2d.Size.x * relativeScale.x;
			float height = bc2d.Size.y * relativeScale.y;

			// SetAsBox takes "Half Width/Height", "Center Position", "Angle"
			boxShape.SetAsBox(
				width / 2.0f,
				height / 2.0f,
				b2Vec2(
					relativePos.x + bc2d.Offset.x * relativeScale.x,
					relativePos.y + bc2d.Offset.y * relativeScale.y),
				relativeRot.z);

			// Define Fixture
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &boxShape;
			fixtureDef.density = bc2d.Density;
			fixtureDef.friction = bc2d.Friction;
			fixtureDef.restitution = bc2d.Restitution;
			fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
			fixtureDef.userData.pointer = (uintptr_t)(uint32_t)entity;

			b2Body* body = (b2Body*)(parentEntity.GetComponent<Rigidbody2DComponent>().RuntimeBody);
			if(bc2d.RuntimeFixture)
				body->DestroyFixture((b2Fixture*)bc2d.RuntimeFixture);
			bc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}

		// 5. Attach Circle Collider
		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

			// Calculate Relative Transform
			Entity parentEntity = Entity{ cc2d.ClosestRigidbodyParent, this };
			if (!parentEntity)
			{
				ENGINE_LOG_ERROR("No parent rigid body found for '{0}.'", entity.GetName());
				return;
			}
			glm::mat4 parentTransform = parentEntity.GetComponent<TransformComponent>().GlobalTransform;
			glm::mat4 childTransform = entity.GetComponent<TransformComponent>().GlobalTransform;

			// Extract the Parent's pure Position and Rotation
			glm::vec3 pPos, pRot, pScale;
			Math::DecomposeTransform(parentTransform, pPos, pRot, pScale);

			// Rebuild the Parent Transform WITHOUT scale
			// Box2D bodies act as the anchor, and they only understand position and rotation.
			glm::mat4 unscaledParentTransform = glm::translate(glm::mat4(1.0f), pPos) *
				glm::rotate(glm::mat4(1.0f), pRot.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
				glm::rotate(glm::mat4(1.0f), pRot.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
				glm::rotate(glm::mat4(1.0f), pRot.z, glm::vec3(0.0f, 0.0f, 1.0f));

			// Calculate relative matrix against the UNSCALED parent body
			glm::mat4 relativeMatrix = glm::inverse(unscaledParentTransform) * childTransform;

			// Decompose to get relative position and scale
			glm::vec3 relativePos, relativeRot, relativeScale;
			Math::DecomposeTransform(relativeMatrix, relativePos, relativeRot, relativeScale);

			b2CircleShape circleShape;

			float maxScale = std::max(relativeScale.x, relativeScale.y);

			// For circles, we just set Position (m_p)
			circleShape.m_p.Set(
				relativePos.x + cc2d.Offset.x * relativeScale.x,
				relativePos.y + cc2d.Offset.y * relativeScale.y
			);
			circleShape.m_radius = cc2d.Radius * maxScale;

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &circleShape;
			fixtureDef.density = cc2d.Density;
			fixtureDef.friction = cc2d.Friction;
			fixtureDef.restitution = cc2d.Restitution;
			fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
			fixtureDef.userData.pointer = (uintptr_t)(uint32_t)entity;

			b2Body* body = (b2Body*)(parentEntity.GetComponent<Rigidbody2DComponent>().RuntimeBody);
			if(cc2d.RuntimeFixture)
				body->DestroyFixture((b2Fixture*)cc2d.RuntimeFixture);
			cc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}
		
	}

	void Scene::OnRuntimeStart()
	{
		UpdateGlobalTransforms();
		OnPhysics2DStart();
		OnScriptingStart();
		UpdateGlobalTransforms();
	}

	void Scene::OnRuntimeStop()
	{
		OnScriptingStop();
		OnPhysics2DStop();
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
			auto& transform = entity.GetComponent<TransformComponent>();
			glm::vec3 scale;
			glm::vec3 rotation;
			glm::vec3 translation;
			if (!Math::DecomposeTransform(transform.GlobalTransform, translation, rotation, scale))
			{
				ASSERT(false, "Could not decompose transform.");
				continue;
			}
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = Rigidbody2DTypeToBox2DBody(rb2d.Type);
			bodyDef.position.Set(translation.x, translation.y);
			bodyDef.angle = rotation.z;
			bodyDef.userData.pointer = (uintptr_t)e;

			b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);
			rb2d.RuntimeBody = body;

			AttachColliders(body, entity, entity, this);
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

		// Create a cache to store file's returned class
		std::unordered_map<std::filesystem::path, sol::table> script_cache;

		// Iterate all entities with scripts
		auto view = m_Registry.view<ScriptComponent, TagComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto [sc, tag] = view.get<ScriptComponent, TagComponent>(e);

			if (sc.ScriptPath.empty())
			{
				// Just skip this entity, it has no script attached.
				ENGINE_LOG_WARN("Entity {0} has script entity but no script path.", tag.Tag);
				continue;
			}

			std::filesystem::path scriptPath = Project::GetAssetFileSystemPath(sc.ScriptPath);

			// Prevents crashing if you deleted the file but didn't update the entity
			if (!std::filesystem::exists(scriptPath))
			{
				ENGINE_LOG_ERROR("Script file not found: {0} for Entity: {1}", scriptPath.string(), tag.Tag);
				continue;
			}

			// add script class to script cache if it does not exists, get from the cache otherwise
			sol::table scriptClass;
			if (script_cache.find(scriptPath) == script_cache.end())
			{

				// Load the file (Get the "Class" table)
				// 1. LOAD the file (This checks for Syntax Errors)
// Unlike safe_script_file, this does NOT run the script yet.
				sol::load_result loadResult = m_Lua->load_file(scriptPath.string());

				// 2. CHECK if loading succeeded
				if (!loadResult.valid())
				{
					sol::error err = loadResult;
					ENGINE_LOG_ERROR("Syntax Error in script '{0}'", scriptPath.string());
					ENGINE_LOG_ERROR("Details: {0}", err.what());
					continue; // Skip this entity safely
				}

				// 3. EXECUTE the script (This runs the code to return the table)
				// Now we convert the loaded script into a protected function and run it.
				sol::protected_function scriptFunc = loadResult;
				sol::protected_function_result result = scriptFunc();

				if (!result.valid())
				{
					// Handle Syntax Errors (e.g., "unexpected symbol near 'if'")
					sol::error err = result;
					ENGINE_LOG_ERROR("Failed to compile script '{0}'", scriptPath.string());
					ENGINE_LOG_ERROR("Error: {0}", err.what());
					continue; // Skip this entity
				}

				// Create the Instance
				// We take the "Class" table returned by the script and deep copy it
				// into our component's Instance slot.
				scriptClass = result;
				script_cache[scriptPath] = scriptClass;
			}
			else
				scriptClass = script_cache.at(scriptPath);

			sc.Instance = m_Lua->create_table();

			// Setup metatable for inheritance (so Instance behaves like Class)
			// This is Lua magic to make the instance allow variable overrides
			sc.Instance[sol::metatable_key] = m_Lua->create_table_with("__index", scriptClass);

			// Inject the Entity into the instance
			// This is how the script knows which entity it belongs to!
			sc.Instance["Entity"] = entity;

			// Call OnCreate
			sol::protected_function onCreate = sc.Instance["OnCreate"];
			sol::protected_function_result result = onCreate(sc.Instance);

			if (!result.valid())
			{
				sol::error err = result;
				std::string errorMsg = err.what();

				// 1. Log to Terminal (Standard)
				ENGINE_LOG_ERROR("Script Runtime Error: {0}", errorMsg);
			}
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
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawSprite(transform.GlobalTransform, sprite, (int)entity);
			}
		}

		// Draw circles
		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

				Renderer2D::DrawCircle(transform.GlobalTransform, circle.Color, circle.Thickness, circle.Fade, (int)entity);
			}
		}

		// Draw text
		{
			auto view = m_Registry.view<TransformComponent, TextComponent>();
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
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}
}