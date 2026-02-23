#include "egpch.h"
#include "Entity.h"

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
		while (rbp && !rbp.HasComponent<Rigidbody2DComponent>())
			rbp = Entity{ rbp.GetComponent<RelationshipComponent>().Parent, m_Scene };
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
}