#include "egpch.h"
#include "Entity.h"

#include "Engine/Project/Project.h"
#include "Engine/Utils/Math.h"

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

namespace Engine {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	Entity Entity::ClosestRigidbodyParent()
	{
		Entity rbp = { m_EntityHandle, m_Scene };
		int safetyDepth = 0;
		while (rbp && !rbp.HasComponent<Rigidbody2DComponent>())
		{
			entt::entity currentHandle = rbp;
			rbp = Entity{ rbp.GetComponent<RelationshipComponent>().Parent, m_Scene };
			entt::entity parentHandle = rbp;

			if (++safetyDepth > 100) { // 100 is plenty deep
				spdlog::error("CYCLIC GRAPH DETECTED! Entity ID: {} is looping with Parent ID: {}",
					(uint32_t)currentHandle, (uint32_t)parentHandle);
				break;
			}
		}
		return rbp;
	}

    void Entity::setEnabled(bool enabled)
    {
        // Prevent redundant updates
        if (isEnabled() == enabled)
            return;

        auto& rc = GetComponent<RelationshipComponent>();
        Entity parent = { rc.Parent, m_Scene };
        bool status = enabled && parent.isEnabled();
        if (HasComponent<Rigidbody2DComponent>())
        {
            auto& rb2d = GetComponent<Rigidbody2DComponent>();
            if (rb2d.RuntimeBody)
            {
                b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
                body->SetEnabled(status);
            }
        }
		if (status)
		{
            RemoveComponent<DisabledComponent>();
			AttachFixturesToRigidbodyParent();
		}
		else if (isEnabled())
		{
            AddComponent<DisabledComponent>();
			DetachFixturesFromRigidbodyParent();
		}


        Entity child = { rc.FirstChild, m_Scene };
        while (child)
        {
            child.setEnabled(status);
            child = { child.GetComponent<RelationshipComponent>().NextSibling, m_Scene };
        }
    }

    void Entity::AttachFixturesToRigidbodyParent()
    {
		if (!isEnabled())
			return;

		Entity rootEntity = ClosestRigidbodyParent();
		if (!rootEntity || !rootEntity.HasComponent<Rigidbody2DComponent>())
			return;

		b2Body* body = (b2Body*)rootEntity.GetComponent<Rigidbody2DComponent>().RuntimeBody;
		if (!body)
			return;

		// Calculate Relative Transform
		glm::mat4 parentTransform = rootEntity.GetComponent<TransformComponent>().GlobalTransform;
		glm::mat4 childTransform = GetComponent<TransformComponent>().GlobalTransform;

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

		// Attach Box Collider
		if (HasComponent<BoxCollider2DComponent>())
		{
			auto& bc2d = GetComponent<BoxCollider2DComponent>();
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
			fixtureDef.filter.categoryBits = bc2d.Category;
			fixtureDef.filter.maskBits = bc2d.Mask;
			fixtureDef.userData.pointer = (uintptr_t)(uint32_t)m_EntityHandle;

			if (bc2d.RuntimeFixture && bc2d.ClosestRigidbodyParent != entt::null)
			{
				Entity oldRigidbodyParent = { bc2d.ClosestRigidbodyParent, m_Scene };
				b2Body* oldBody = (b2Body*)oldRigidbodyParent.GetComponent<Rigidbody2DComponent>().RuntimeBody;
				oldBody->DestroyFixture((b2Fixture*)bc2d.RuntimeFixture);
				bc2d.RuntimeFixture = nullptr;
			}

			bc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
			bc2d.ClosestRigidbodyParent = rootEntity;
		}

		// Attach Circle Collider
		if (HasComponent<CircleCollider2DComponent>())
		{
			auto& cc2d = GetComponent<CircleCollider2DComponent>();
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
			fixtureDef.filter.categoryBits = cc2d.Category;
			fixtureDef.filter.maskBits = cc2d.Mask;
			fixtureDef.userData.pointer = (uintptr_t)(uint32_t)m_EntityHandle;

			if (cc2d.RuntimeFixture && cc2d.ClosestRigidbodyParent != entt::null)
			{
				Entity oldRigidbodyParent = { cc2d.ClosestRigidbodyParent, m_Scene };
				b2Body* oldBody = (b2Body*)oldRigidbodyParent.GetComponent<Rigidbody2DComponent>().RuntimeBody;
				oldBody->DestroyFixture((b2Fixture*)cc2d.RuntimeFixture);
				cc2d.RuntimeFixture = nullptr;
			}

			cc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
			cc2d.ClosestRigidbodyParent = rootEntity;
		}

    }
    
    void Entity::DetachFixturesFromRigidbodyParent()
    {
        // destroy box collider
        if (HasComponent<BoxCollider2DComponent>())
        {
            auto& bc2d = GetComponent<BoxCollider2DComponent>();
            if (bc2d.ClosestRigidbodyParent != entt::null)
            {
                Entity oldRigidbodyParent = { bc2d.ClosestRigidbodyParent, m_Scene };
                b2Body* body = (b2Body*)oldRigidbodyParent.GetComponent<Rigidbody2DComponent>().RuntimeBody;
                body->DestroyFixture((b2Fixture*)bc2d.RuntimeFixture);
                bc2d.RuntimeFixture = nullptr;
            }
        }

        // destroy circle collider
        if (HasComponent<CircleCollider2DComponent>())
        {
            auto& cc2d = GetComponent<CircleCollider2DComponent>();
            if (cc2d.ClosestRigidbodyParent != entt::null)
            {
                Entity oldRigidbodyParent = { cc2d.ClosestRigidbodyParent, m_Scene };
                b2Body* body = (b2Body*)oldRigidbodyParent.GetComponent<Rigidbody2DComponent>().RuntimeBody;
                body->DestroyFixture((b2Fixture*)cc2d.RuntimeFixture);
                cc2d.RuntimeFixture = nullptr;
            }
        }
    }

	void Entity::OnScriptStart()
	{
		if (HasComponent<ScriptComponent>() && m_Scene->m_Lua)
		{
			auto& sc = GetComponent<ScriptComponent>();
			auto& tag = GetComponent<TagComponent>();

			if (sc.ScriptPath.empty())
			{
				// Just skip this entity, it has no script attached.
				ENGINE_LOG_WARN("Entity {0} has script entity but no script path.", tag.Tag);
				return;
			}

			std::filesystem::path scriptPath = Project::GetAssetFileSystemPath(sc.ScriptPath);

			// Prevents crashing if you deleted the file but didn't update the entity
			if (!std::filesystem::exists(scriptPath))
			{
				ENGINE_LOG_ERROR("Script file not found: {0} for Entity: {1}", scriptPath.string(), tag.Tag);
				return;
			}

			// add script class to script cache if it does not exists, get from the cache otherwise
			sol::table scriptClass;
			if (m_Scene->m_ScriptCache.find(scriptPath) == m_Scene->m_ScriptCache.end())
			{

				// Load the file (Get the "Class" table)
				// 1. LOAD the file (This checks for Syntax Errors)
	// Unlike safe_script_file, this does NOT run the script yet.
				sol::load_result loadResult = m_Scene->m_Lua->load_file(scriptPath.string());

				// 2. CHECK if loading succeeded
				if (!loadResult.valid())
				{
					sol::error err = loadResult;
					ENGINE_LOG_ERROR("Syntax Error in script '{0}'", scriptPath.string());
					ENGINE_LOG_ERROR("Details: {0}", err.what());
					return; // Skip this entity safely
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
					return; // Skip this entity
				}

				// Create the Instance
				// We take the "Class" table returned by the script and deep copy it
				// into our component's Instance slot.
				scriptClass = result;
				m_Scene->m_ScriptCache[scriptPath] = scriptClass;
			}
			else
				scriptClass = m_Scene->m_ScriptCache.at(scriptPath);

			sc.Instance = m_Scene->m_Lua->create_table();

			// Setup metatable for inheritance (so Instance behaves like Class)
			// This is Lua magic to make the instance allow variable overrides
			sc.Instance[sol::metatable_key] = m_Scene->m_Lua->create_table_with("__index", scriptClass);

			// Inject the Entity into the instance
			// This is how the script knows which entity it belongs to!
			sc.Instance["Entity"] = *this;

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
}