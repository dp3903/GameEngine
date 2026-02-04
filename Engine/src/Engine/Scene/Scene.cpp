#include "egpch.h"
#include "Scene.h"

#include "Components.h"
#include "Entity.h"

#include "SceneRuntimeData.h"

#include "Engine/Utils/Random.h"
#include "Engine/Utils/Math.h"
#include "Engine/Window/Input.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Project/Project.h"

#include <glm/glm.hpp>

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

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
		// 1. Get Global Transforms for BOTH
		// We need to compare Child vs Root to find the relative difference
		glm::vec3 rootPos, rootRot, rootScale;
		Math::DecomposeTransform(rootEntity.GetComponent<TransformComponent>().GlobalTransform, rootPos, rootRot, rootScale);

		glm::vec3 childPos, childRot, childScale;
		Math::DecomposeTransform(currentEntity.GetComponent<TransformComponent>().GlobalTransform, childPos, childRot, childScale);

		// 2. Prepare Box2D Parameters
		// Box2D Shape Dimensions should match the VISUAL Global Scale
		// (We use abs because physics shapes can't be negative)
		float finalScaleX = std::abs(childScale.x);
		float finalScaleY = std::abs(childScale.y);

		// Box2D Fixture Angle is RELATIVE to the Body
		float relativeRotation = childRot.z - rootRot.z;

		// 3. Calculate Relative Position (The Tricky Part)
		// We need the Child's position in the Root's LOCAL space.
		// Step A: Get distance vector in World Space
		glm::vec2 worldOffset = { childPos.x - rootPos.x, childPos.y - rootPos.y };

		// Step B: Rotate this vector "backwards" by the Root's rotation 
		// to align it with the Root's local axes.
		float c = cos(-rootRot.z);
		float s = sin(-rootRot.z);
		glm::vec2 localOffset = {
			worldOffset.x * c - worldOffset.y * s,
			worldOffset.x * s + worldOffset.y * c
		};

		// 4. Attach Box Collider
		if (currentEntity.HasComponent<BoxCollider2DComponent>())
		{
			auto& bc2d = currentEntity.GetComponent<BoxCollider2DComponent>();
			b2PolygonShape boxShape;

			// Calculate the center of the box relative to the Body
			// This includes the Entity's offset from parent + The Collider's offset from Entity
			// Note: Collider Offset must be rotated by the RELATIVE rotation
			float offX = bc2d.Offset.x * finalScaleX;
			float offY = bc2d.Offset.y * finalScaleY;

			float relC = cos(relativeRotation);
			float relS = sin(relativeRotation);

			// Rotate collider offset and add to entity offset
			float finalOffsetX = localOffset.x + (offX * relC - offY * relS);
			float finalOffsetY = localOffset.y + (offX * relS + offY * relC);

			boxShape.SetAsBox(
				bc2d.Size.x * finalScaleX,
				bc2d.Size.y * finalScaleY,
				b2Vec2(finalOffsetX, finalOffsetY),
				relativeRotation
			);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &boxShape;
			fixtureDef.density = bc2d.Density;
			fixtureDef.friction = bc2d.Friction;
			fixtureDef.restitution = bc2d.Restitution;
			fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;

			bc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}

		// 5. Attach Circle Collider
		if (currentEntity.HasComponent<CircleCollider2DComponent>())
		{
			auto& cc2d = currentEntity.GetComponent<CircleCollider2DComponent>();
			b2CircleShape circleShape;

			float maxScale = std::max(finalScaleX, finalScaleY);

			// Same offset logic as box
			float offX = cc2d.Offset.x * finalScaleX; // Circle offset is usually pre-scaling, but let's stick to standard
			float offY = cc2d.Offset.y * finalScaleY; // (Wait, circle scale should be uniform usually)

			// For circles, we just set Position (m_p)
			// We still need to rotate the offset if the circle is not centered on the entity
			// (Though rotating a circle around its center does nothing, rotating it around the entity pivot does)
			float relC = cos(relativeRotation);
			float relS = sin(relativeRotation);

			circleShape.m_p.Set(
				localOffset.x + (offX * relC - offY * relS),
				localOffset.y + (offX * relS + offY * relC)
			);
			circleShape.m_radius = cc2d.Radius * maxScale;

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &circleShape;
			fixtureDef.density = cc2d.Density;
			fixtureDef.friction = cc2d.Friction;
			fixtureDef.restitution = cc2d.Restitution;
			fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

			cc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
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

	void Scene::OnRuntimeStart()
	{
		OnPhysics2DStart();
		OnScriptingStart();
	}

	void Scene::OnRuntimeStop()
	{
		OnScriptingStop();
		OnPhysics2DStop();
	}

	void Scene::OnSimulationStart()
	{
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
		m_PhysicsWorld = new b2World({ 0.0f, -9.8f });

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
				return;
			}
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = Rigidbody2DTypeToBox2DBody(rb2d.Type);
			bodyDef.position.Set(translation.x, translation.y);
			bodyDef.angle = rotation.z;

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
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2Body* body = (b2Body*)rb2d.RuntimeBody;
			const auto& position = body->GetPosition();
			transform.Translation.x = position.x;
			transform.Translation.y = position.y;
			transform.Rotation.z = body->GetAngle();
		}
		
	}

	void Scene::BindLuaTypes()
	{
		// Bind Key Codes
		// 1. Create the Table
		auto keys = m_Lua->create_named_table("Key");

		// 2. Define a macro to save typing (and sanity)
#define BIND_KEY(name) keys[#name] = KeyCode::name

// 3. Bind all keys individually (No limit!)
		BIND_KEY(Space);
		BIND_KEY(Apostrophe);
		BIND_KEY(Comma);
		BIND_KEY(Minus);
		BIND_KEY(Period);
		BIND_KEY(Slash);

		BIND_KEY(D0);
		BIND_KEY(D1);
		BIND_KEY(D2);
		BIND_KEY(D3);
		BIND_KEY(D4);
		BIND_KEY(D5);
		BIND_KEY(D6);
		BIND_KEY(D7);
		BIND_KEY(D8);
		BIND_KEY(D9);

		BIND_KEY(Semicolon);
		BIND_KEY(Equal);

		BIND_KEY(A);
		BIND_KEY(B);
		BIND_KEY(C);
		BIND_KEY(D);
		BIND_KEY(E);
		BIND_KEY(F);
		BIND_KEY(G);
		BIND_KEY(H);
		BIND_KEY(I);
		BIND_KEY(J);
		BIND_KEY(K);
		BIND_KEY(L);
		BIND_KEY(M);
		BIND_KEY(N);
		BIND_KEY(O);
		BIND_KEY(P);
		BIND_KEY(Q);
		BIND_KEY(R);
		BIND_KEY(S);
		BIND_KEY(T);
		BIND_KEY(U);
		BIND_KEY(V);
		BIND_KEY(W);
		BIND_KEY(X);
		BIND_KEY(Y);
		BIND_KEY(Z);

		BIND_KEY(LeftBracket);
		BIND_KEY(Backslash);
		BIND_KEY(RightBracket);
		BIND_KEY(GraveAccent);

		BIND_KEY(World1);
		BIND_KEY(World2);

		/* Function keys */
		BIND_KEY(Escape);
		BIND_KEY(Enter);
		BIND_KEY(Tab);
		BIND_KEY(Backspace);
		BIND_KEY(Insert);
		BIND_KEY(Delete);
		BIND_KEY(Right);
		BIND_KEY(Left);
		BIND_KEY(Down);
		BIND_KEY(Up);
		BIND_KEY(PageUp);
		BIND_KEY(PageDown);
		BIND_KEY(Home);
		BIND_KEY(End);
		BIND_KEY(CapsLock);
		BIND_KEY(ScrollLock);
		BIND_KEY(NumLock);
		BIND_KEY(PrintScreen);
		BIND_KEY(Pause);

		BIND_KEY(F1);
		BIND_KEY(F2);
		BIND_KEY(F3);
		BIND_KEY(F4);
		BIND_KEY(F5);
		BIND_KEY(F6);
		BIND_KEY(F7);
		BIND_KEY(F8);
		BIND_KEY(F9);
		BIND_KEY(F10);
		BIND_KEY(F11);
		BIND_KEY(F12);
		BIND_KEY(F13);
		BIND_KEY(F14);
		BIND_KEY(F15);
		BIND_KEY(F16);
		BIND_KEY(F17);
		BIND_KEY(F18);
		BIND_KEY(F19);
		BIND_KEY(F20);
		BIND_KEY(F21);
		BIND_KEY(F22);
		BIND_KEY(F23);
		BIND_KEY(F24);
		BIND_KEY(F25);

		/* Keypad */
		BIND_KEY(KP0);
		BIND_KEY(KP1);
		BIND_KEY(KP2);
		BIND_KEY(KP3);
		BIND_KEY(KP4);
		BIND_KEY(KP5);
		BIND_KEY(KP6);
		BIND_KEY(KP7);
		BIND_KEY(KP8);
		BIND_KEY(KP9);
		BIND_KEY(KPDecimal);
		BIND_KEY(KPDivide);
		BIND_KEY(KPMultiply);
		BIND_KEY(KPSubtract);
		BIND_KEY(KPAdd);
		BIND_KEY(KPEnter);
		BIND_KEY(KPEqual);

		BIND_KEY(LeftShift);
		BIND_KEY(LeftControl);
		BIND_KEY(LeftAlt);
		BIND_KEY(LeftSuper);
		BIND_KEY(RightShift);
		BIND_KEY(RightControl);
		BIND_KEY(RightAlt);
		BIND_KEY(RightSuper);
		BIND_KEY(Menu);

		// 4. Undefine the macro so it doesn't leak
#undef BIND_KEY

// --- REPEAT FOR MOUSE ---

		auto mouse = m_Lua->create_named_table("Mouse");
#define BIND_MOUSE(name) mouse[#name] = MouseCode::name

		BIND_MOUSE(Button0);
		BIND_MOUSE(Button1);
		BIND_MOUSE(Button2);
		BIND_MOUSE(Button3);
		BIND_MOUSE(Button4);
		BIND_MOUSE(Button5);
		BIND_MOUSE(Button6);
		BIND_MOUSE(Button7);
		BIND_MOUSE(ButtonLast);
		BIND_MOUSE(ButtonLeft);
		BIND_MOUSE(ButtonRight);
		BIND_MOUSE(ButtonMiddle);

#undef BIND_MOUSE
		
		// Bind types
		m_Lua->new_usertype<glm::vec2>("Vec2",
			sol::constructors<glm::vec2(), glm::vec2(float, float), glm::vec2(float)>(),
			"x", &glm::vec2::x,
			"y", &glm::vec2::y
		);
		m_Lua->new_usertype<glm::vec3>("Vec3",
			sol::constructors<glm::vec3(), glm::vec3(float, float, float), glm::vec3(float)>(),
			"x", &glm::vec3::x,
			"y", &glm::vec3::y,
			"z", &glm::vec3::z,
			"r", &glm::vec3::r,
			"g", &glm::vec3::g,
			"b", &glm::vec3::b
		);
		m_Lua->new_usertype<glm::vec4>("Vec4",
			sol::constructors<glm::vec4(), glm::vec4(float, float, float, float), glm::vec4(float)>(),
			"x", &glm::vec4::x,
			"y", &glm::vec4::y,
			"z", &glm::vec4::z,
			"w", &glm::vec4::w,
			"r", &glm::vec4::r,
			"g", &glm::vec4::g,
			"b", &glm::vec4::b,
			"a", &glm::vec4::a
		);
		m_Lua->new_usertype<TransformComponent>("Transform",
			"Translation", &TransformComponent::Translation,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale
		);
		m_Lua->new_usertype<SpriteRendererComponent>("SpriteRenderer",
			"Color", &SpriteRendererComponent::Color
		);
		m_Lua->new_usertype<Entity>("Entity",
			"GetUUID", &Entity::GetUUID,
			"GetName", &Entity::GetName,
			"Transform", sol::property([this](Entity& entity) -> TransformComponent& {
				if (!entity)
				{
					// This throws a soft Lua error: "runtime error: Entity is destroyed"
					// The C++ engine continues running, only the script stops.
					throw std::runtime_error("Attempted to access Transform on a destroyed entity!");
				}

				// Return REFERENCE (std::ref is automatic here because of return type &)
				return entity.GetComponent<TransformComponent>();
			}),
			"SpriteRenderer", sol::property([](Entity& entity) -> SpriteRendererComponent* {
				if (!entity)
				{
					// This throws a soft Lua error: "runtime error: Entity is destroyed"
					// The C++ engine continues running, only the script stops.
					throw std::runtime_error("Attempted to access Transform on a destroyed entity!");
				}

				if (entity.HasComponent<SpriteRendererComponent>())
				{
					return &entity.GetComponent<SpriteRendererComponent>();
				}

				// If missing, return nullptr. Sol2 converts this to Lua 'nil'
				return nullptr;
			})
		);

		// Bind functions
		m_Lua->set_function("GetEntityByTag", [&](std::string tag) -> sol::object {

			auto& view = m_Registry.view<TagComponent>();
			for (auto& e : view)
			{
				if (view.get<TagComponent>(e).Tag == tag)
					return sol::make_object(*m_Lua, Entity{ e, this });
			}
			return sol::make_object(*m_Lua, sol::nil); // Returns Lua 'nil'
		});
		// Group into tables for cleaner Lua code (eg: Input.IsKeyPressed)
		auto randomTable = m_Lua->create_named_table("Random");
		randomTable.set_function("Float", &Random::Float);
		auto inputTable = m_Lua->create_named_table("Input");
		inputTable.set_function("IsKeyPressed", &Input::IsKeyPressed);
		inputTable.set_function("GetMouseX", &Input::GetMouseX);
		inputTable.set_function("GetMouseY", &Input::GetMouseY);
		inputTable.set_function("GetMousePosition", &Input::GetMousePosition);

		// Global Data
		m_Lua->set_function("GetGlobal", [&](std::string key) -> sol::object {
			// Helper to unwrap variant to Lua
			if (RuntimeData::HasData(key)) {
				return std::visit([&](auto&& val) -> sol::object {
					return sol::make_object(*m_Lua, val);
				}, RuntimeData::GetData(key));
			}
			return sol::nil;
		});
		// We register multiple C++ functions to the SAME Lua name "Set"
		m_Lua->set_function("SetGlobal", sol::overload(
			&RuntimeData::SetData<int>,
			&RuntimeData::SetData<float>,
			&RuntimeData::SetData<bool>,
			&RuntimeData::SetData<std::string>,
			&RuntimeData::SetData<glm::vec2>,
			&RuntimeData::SetData<glm::vec3>,
			&RuntimeData::SetData<glm::vec4>
		));

		// Scene change request handler
		m_Lua->set_function("RequestSceneChange", &RuntimeData::RequestSceneChange);
	}

	void Scene::OnScriptingStart()
	{
		m_Lua = new sol::state();

		// Load standard libraries
		m_Lua->open_libraries(sol::lib::base, sol::lib::math);

		// bind lua types and functions
		BindLuaTypes();

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

			// Load the file (Get the "Class" table)
			// Note: In a real engine, you would cache this result so you don't 
			// read the disk 1000 times for 1000 enemies.

			sol::protected_function_result result = m_Lua->safe_script_file(scriptPath.string());

			if (!result.valid())
			{
				// Handle Syntax Errors (e.g., "unexpected symbol near 'if'")
				sol::error err = result;
				ENGINE_LOG_ERROR("Failed to compile script '{0}'", scriptPath.string());
				ENGINE_LOG_ERROR("Error: {0}", err.what());
				continue; // Skip this entity
			}

			if (result.valid())
			{
				// Create the Instance
				// We take the "Class" table returned by the script and deep copy it
				// into our component's Instance slot.
				sol::table scriptClass = result;
				sc.Instance = m_Lua->create_table();

				// Setup metatable for inheritance (so Instance behaves like Class)
				// This is Lua magic to make the instance allow variable overrides
				sc.Instance[sol::metatable_key] = m_Lua->create_table_with("__index", scriptClass);

				// Inject the Entity into the instance
				// This is how the script knows which entity it belongs to!
				sc.Instance["Entity"] = entity;

				// Call OnCreate
				if (sc.Instance["OnCreate"].valid())
					sc.Instance["OnCreate"](sc.Instance);
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
			if (!sc.Instance.valid())
				continue;

			// Call OnDestroy(self)
			if (sc.Instance["OnDestroy"].valid())
			{
				// We must pass 'sc.Instance' as the first argument to simulate ':' call
				sc.Instance["OnDestroy"](sc.Instance);
			}

			// Assigning a default (empty) table disconnects it from the Lua State.
			sc.Instance = sol::table();
		}

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