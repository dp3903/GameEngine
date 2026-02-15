#pragma once
#include "Entity.h"
#include "Components.h"
#include "SceneRuntimeData.h"
#include "Engine/Window/Input.h"
#include "Engine/Window/KeyCodes.h"
#include "Engine/Window/MouseCodes.h"
#include "Engine/Utils/Random.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"
#include "box2d/b2_contact.h"


namespace Engine
{
	static void BindLuaTypesAndFunctions(sol::state* m_Lua, Scene* scene)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind Key Codes to 'Key' table
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		auto keys = m_Lua->create_named_table("Key");

#define BIND_KEY(name) keys[#name] = KeyCode::name

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

		// Undefine the macro so it doesn't leak
#undef BIND_KEY

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind Mouse codes to 'Mouse' table
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind types
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
		m_Lua->new_usertype<glm::mat4>("Mat4",
			sol::constructors<glm::mat4(float), glm::mat4()>(),

			// Operator Overloads
			sol::meta_function::multiplication, sol::overload(
				// Mat4 * Mat4
				[](const glm::mat4& a, const glm::mat4& b) { return a * b; },
				// Mat4 * Vec4 (Transforming a vector)
				[](const glm::mat4& m, const glm::vec4& v) { return m * v; },
				// Mat4 * float (Scaling all elements)
				[](const glm::mat4& m, float f) { return m * f; }
			),
			sol::meta_function::addition, [](const glm::mat4& a, const glm::mat4& b) { return a + b; },
			sol::meta_function::subtraction, [](const glm::mat4& a, const glm::mat4& b) { return a - b; },

			// Transformation Helper Functions
			// Usage: local newMat = currentMat:Translate(vec3)
			"Translate", [](const glm::mat4& m, const glm::vec3& v) {
				return glm::translate(m, v);
			},

			"Rotate", [](const glm::mat4& m, float angleRad, const glm::vec3& axis) {
				return glm::rotate(m, angleRad, axis);
			},

			"Scale", [](const glm::mat4& m, const glm::vec3& v) {
				return glm::scale(m, v);
			},

			"Inverse", [](const glm::mat4& m) {
				return glm::inverse(m);
			},

			// Helper to Create Identity easily: Mat4.Identity()
			"Identity", []() { return glm::mat4(1.0f); }
		);
		m_Lua->new_usertype<TransformComponent>("Transform",
			"Translation", &TransformComponent::Translation,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale
		);
		m_Lua->new_usertype<SpriteRendererComponent>("SpriteRenderer",
			"Color", &SpriteRendererComponent::Color,
			"TilingFactor", &SpriteRendererComponent::TilingFactor,
			"IsSubTexture", &SpriteRendererComponent::IsSubTexture,
			"SpriteHeight", &SpriteRendererComponent::SpriteHeight,
			"SpriteWidth", &SpriteRendererComponent::SpriteWidth,
			"XSpriteIndex", &SpriteRendererComponent::XSpriteIndex,
			"YSpriteIndex", &SpriteRendererComponent::YSpriteIndex
		);
		m_Lua->new_usertype<Camera>("Camera",
			// Constructors (Optional: usually you don't instantiate raw Cameras in Lua)
			sol::constructors<Camera(), Camera(const glm::mat4&)>(),

			// Methods
			"GetProjection", &Camera::GetProjection
		);
		m_Lua->new_enum("ProjectionType",
			"Perspective", SceneCamera::ProjectionType::Perspective,
			"Orthographic", SceneCamera::ProjectionType::Orthographic
		);
		m_Lua->new_usertype<SceneCamera>("SceneCamera",
			// Constructors
			sol::constructors<SceneCamera()>(),

			// Base Classes (Optional: Include if you have bound the 'Camera' class and want polymorphism)
			 sol::base_classes, sol::bases<Camera>(),

			// Methods
			"SetOrthographic", &SceneCamera::SetOrthographic,
			"SetPerspective", &SceneCamera::SetPerspective,
			"SetViewportSize", &SceneCamera::SetViewportSize,

			// Properties (Getters & Setters)
			// This converts GetPerspectiveVerticalFOV() / SetPerspectiveVerticalFOV() into a variable property
			"PerspectiveVerticalFOV", sol::property(&SceneCamera::GetPerspectiveVerticalFOV, &SceneCamera::SetPerspectiveVerticalFOV),
			"PerspectiveNearClip", sol::property(&SceneCamera::GetPerspectiveNearClip, &SceneCamera::SetPerspectiveNearClip),
			"PerspectiveFarClip", sol::property(&SceneCamera::GetPerspectiveFarClip, &SceneCamera::SetPerspectiveFarClip),

			"OrthographicSize", sol::property(&SceneCamera::GetOrthographicSize, &SceneCamera::SetOrthographicSize),
			"OrthographicNearClip", sol::property(&SceneCamera::GetOrthographicNearClip, &SceneCamera::SetOrthographicNearClip),
			"OrthographicFarClip", sol::property(&SceneCamera::GetOrthographicFarClip, &SceneCamera::SetOrthographicFarClip),

			"ProjectionType", sol::property(&SceneCamera::GetProjectionType, &SceneCamera::SetProjectionType),
			"AspectRatio", sol::property(&SceneCamera::GetAspectRatio)
		);
		m_Lua->new_usertype<Entity>("Entity",
			"GetUUID", &Entity::GetUUID,
			"GetName", &Entity::GetName,
			"Transform", sol::property([scene](Entity& entity) -> TransformComponent& {
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
			}),
			"SetScale", [scene](Entity entity, glm::vec3 scale) {
				entity.GetComponent<TransformComponent>().Scale = scale;
				scene->UpdateGlobalTransforms();
				scene->SyncPhysicsToTransform(entity);
			},
			"SetPosition", [scene](Entity entity, glm::vec3 pos) {
				entity.GetComponent<TransformComponent>().Translation = pos;
				scene->UpdateGlobalTransforms();
				scene->SyncPhysicsToTransform(entity);
			},
			"SetRotation", [scene](Entity entity, glm::vec3 rot) {
				entity.GetComponent<TransformComponent>().Rotation = rot;
				scene->UpdateGlobalTransforms();
				scene->SyncPhysicsToTransform(entity);
			}
		);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind global functions
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		m_Lua->set_function("GetEntityByTag", [scene, m_Lua](std::string tag) -> sol::object {

			auto& view = scene->m_Registry.view<TagComponent>();
			for (auto& e : view)
			{
				if (view.get<TagComponent>(e).Tag == tag)
					return sol::make_object(*m_Lua, Entity{ e, scene });
			}
			return sol::make_object(*m_Lua, sol::nil); // Returns Lua 'nil'
			});

		// Global Data
		m_Lua->set_function("GetGlobal", [m_Lua](std::string key) -> sol::object {
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
		m_Lua->set_function("RegisterFunction", &RuntimeData::RegisterFunction);
		m_Lua->set_function("HasFunction", &RuntimeData::HasFunction);
		m_Lua->set_function("CallFunction", &RuntimeData::CallFunction);
		// Scene change request handler
		m_Lua->set_function("RequestSceneChange", &RuntimeData::RequestSceneChange);
		// get primary scene camera
		m_Lua->set_function("GetPrimaryCamera", [scene, m_Lua]() -> sol::object { // Return sol::object to allow returning nil

			// Safety Check: Does a primary camera exist?
			Entity primaryCam = scene->GetPrimaryCameraEntity();
			if (!primaryCam)
			{
				return sol::make_object(*m_Lua, sol::nil);
			}

			// 3. Return by REFERENCE?
			// Be careful here. If you return 'SceneCamera', you return a COPY.
			// Modifying the copy in Lua won't update the actual game engine camera.
			// Usually, you want to return a reference: -> SceneCamera&
			return sol::make_object(*m_Lua, std::ref(primaryCam.GetComponent<CameraComponent>().Camera));
		});
		// get viewport dimensions
		m_Lua->set_function("GetViewportWidth", [scene, m_Lua]() -> float {
			return scene->m_ViewportWidth;
		});
		m_Lua->set_function("GetViewportHeight", [scene, m_Lua]() -> float {
			return scene->m_ViewportHeight;
		});

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind random function to 'Random'table
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		auto randomTable = m_Lua->create_named_table("Random");
		randomTable.set_function("Float", &Random::Float);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind Input functions to 'Input' table
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		auto inputTable = m_Lua->create_named_table("Input");
		inputTable.set_function("IsKeyPressed", &Input::IsKeyPressed);
		inputTable.set_function("IsMouseButtonPressed", &Input::IsMouseButtonPressed);
		inputTable.set_function("GetMouseX", &Input::GetMouseX);
		inputTable.set_function("GetMouseY", &Input::GetMouseY);
		inputTable.set_function("GetMousePosition", &Input::GetMousePosition);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Bind Box2D functions to 'Physics' table
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		auto physicsTable = m_Lua->create_named_table("Physics");
		auto ExecuteOnPhysicsBody = [](Entity& entity, auto&& func) -> RuntimeValue
			{
				if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
					return false;

				auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
				b2Body* body = (b2Body*)rb2d.RuntimeBody;

				if (body)
				{
					// Execute the provided lambda, passing the body
					return func(body);
				}

				return false;
			};

		physicsTable["ApplyLinearImpulse"] = [ExecuteOnPhysicsBody](Entity& entity, glm::vec2 impulse, glm::vec2 point, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->ApplyLinearImpulse(b2Vec2(impulse.x, impulse.y), b2Vec2(point.x, point.y), wake);
				return true;
				});
			};

		physicsTable["ApplyLinearImpulseToCenter"] = [ExecuteOnPhysicsBody](Entity& entity, glm::vec2 force, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->ApplyLinearImpulseToCenter(b2Vec2(force.x, force.y), wake);
				return true;
				});
			};

		physicsTable["ApplyAngularImpulse"] = [ExecuteOnPhysicsBody](Entity& entity, float impulse, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->ApplyAngularImpulse(impulse, wake);
				return true;
				});
			};

		physicsTable["ApplyForce"] = [ExecuteOnPhysicsBody](Entity& entity, glm::vec2 force, glm::vec2 point, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->ApplyForce(b2Vec2(force.x, force.y), b2Vec2(point.x, point.y), wake);
				return true;
				});
			};

		physicsTable["ApplyForceToCenter"] = [ExecuteOnPhysicsBody](Entity& entity, glm::vec2 force, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->ApplyForceToCenter(b2Vec2(force.x, force.y), wake);
				return true;
				});
			};

		physicsTable["ApplyTorque"] = [ExecuteOnPhysicsBody](Entity& entity, float torque, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->ApplyTorque(torque, wake);
				return true;
				});
			};

		physicsTable["SetAngularDamping"] = [ExecuteOnPhysicsBody](Entity& entity, float damp) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->SetAngularDamping(damp);
				return true;
				});
			};

		physicsTable["SetAngularVelocity"] = [ExecuteOnPhysicsBody](Entity& entity, float vel) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->SetAngularVelocity(vel);
				return true;
				});
			};

		physicsTable["SetLinearDamping"] = [ExecuteOnPhysicsBody](Entity& entity, float damp) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->SetLinearDamping(damp);
				return true;
				});
			};

		physicsTable["SetLinearVelocity"] = [ExecuteOnPhysicsBody](Entity& entity, glm::vec2 vel) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->SetLinearVelocity(b2Vec2(vel.x, vel.y));
				return true;
				});
			};

		physicsTable["SetAwake"] = [ExecuteOnPhysicsBody](Entity& entity, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->SetAwake(wake);
				return true;
				});
			};

		physicsTable["SetEnabled"] = [ExecuteOnPhysicsBody](Entity& entity, bool wake) {
			ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				body->SetEnabled(wake);
				return true;
				});
			};

		physicsTable["GetAngularDamping"] = [ExecuteOnPhysicsBody](Entity& entity) {
			return ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				return body->GetAngularDamping();
				});
			};

		physicsTable["GetAngularVelocity"] = [ExecuteOnPhysicsBody](Entity& entity) {
			return ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				return body->GetAngularVelocity();
				});
			};

		physicsTable["GetLinearDamping"] = [ExecuteOnPhysicsBody](Entity& entity) {
			return ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				return body->GetLinearDamping();
				});
			};

		physicsTable["GetLinearVelocity"] = [ExecuteOnPhysicsBody](Entity& entity) {
			return ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				b2Vec2 vel =  body->GetLinearVelocity();
				return glm::vec2(vel.x, vel.y);
				});
			};

		physicsTable["IsAwake"] = [ExecuteOnPhysicsBody](Entity& entity) {
			return ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				return body->IsAwake();
				});
			};

		physicsTable["IsEnabled"] = [ExecuteOnPhysicsBody](Entity& entity) {
			return ExecuteOnPhysicsBody(entity, [&](b2Body* body) {
				return body->IsEnabled();
				});
			};
	}

	class PhysicsContactListener : public b2ContactListener
	{
	public:
		PhysicsContactListener(Scene* scene)
			: m_Scene(scene)
		{
		}

		// Called when two fixtures begin to touch
		virtual void BeginContact(b2Contact* contact) override
		{
			// Get fixtures
			b2Fixture* fixtureA = contact->GetFixtureA();
			b2Fixture* fixtureB = contact->GetFixtureB();

			// Get the Entities from UserData
			uintptr_t userDataA = fixtureA->GetUserData().pointer;
			uintptr_t userDataB = fixtureB->GetUserData().pointer;

			// Convert back to Entity
			Entity entityA = { (entt::entity)userDataA, m_Scene };
			Entity entityB = { (entt::entity)userDataB, m_Scene };

			// --- YOUR GAME LOGIC HERE ---

			// Call for both entities
			callCollision(entityA, entityB, "OnCollisionBegin");
			callCollision(entityB, entityA, "OnCollisionBegin");
		}

		// Called when two fixtures cease to touch
		virtual void EndContact(b2Contact* contact) override
		{
			// Get fixtures
			b2Fixture* fixtureA = contact->GetFixtureA();
			b2Fixture* fixtureB = contact->GetFixtureB();

			// Get the Entities from UserData
			uintptr_t userDataA = fixtureA->GetUserData().pointer;
			uintptr_t userDataB = fixtureB->GetUserData().pointer;

			// Convert back to Entity
			Entity entityA = { (entt::entity)userDataA, m_Scene };
			Entity entityB = { (entt::entity)userDataB, m_Scene };

			// --- YOUR GAME LOGIC HERE ---

			// Call for both entities
			callCollision(entityA, entityB, "OnCollisionEnd");
			callCollision(entityB, entityA, "OnCollisionEnd");
		}

	private:
		Scene* m_Scene;
		
		// Helper function to avoid code duplication
		void callCollision(Entity& self, Entity& other, const std::string& function) {
			if (!self.HasComponent<ScriptComponent>())
				return;

			auto& sc = self.GetComponent<ScriptComponent>();

			// Check if the Instance table is valid
			if (!sc.Instance.valid())
				return;

			// Check if the function exists
			sol::protected_function onCollisionFunc = sc.Instance[function];
			if (onCollisionFunc.valid())
			{
				// Call it safely (catches Lua runtime errors like "attempt to index nil")
				sol::protected_function_result result = onCollisionFunc(sc.Instance, other);

				if (!result.valid())
				{
					sol::error err = result;
					ENGINE_LOG_ERROR("Script Error in {0}: {1}", function, err.what());
				}
			}
		}
	};
}