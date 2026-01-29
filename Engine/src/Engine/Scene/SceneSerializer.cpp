#include "egpch.h"
#include "SceneSerializer.h"
#include "Components.h"
#include "Entity.h"

#include <fstream>
#include <iomanip> // Required for std::setw (Pretty Printing)
#include <nlohmann_json/json.hpp>

using json = nlohmann::ordered_json;

namespace Engine {

    static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponent::BodyType bodyType)
    {
        switch (bodyType)
        {
        case Rigidbody2DComponent::BodyType::Static:    return "Static";
        case Rigidbody2DComponent::BodyType::Dynamic:   return "Dynamic";
        case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
        }

        ASSERT(false, "Unknown body type");
        return {};
    }

    static Rigidbody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
    {
        if (bodyTypeString == "Static")    return Rigidbody2DComponent::BodyType::Static;
        if (bodyTypeString == "Dynamic")   return Rigidbody2DComponent::BodyType::Dynamic;
        if (bodyTypeString == "Kinematic") return Rigidbody2DComponent::BodyType::Kinematic;

        ASSERT(false, "Unknown body type");
        return Rigidbody2DComponent::BodyType::Static;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // SceneSerializer //////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SceneSerializer::SceneSerializer(const std::shared_ptr<Scene>& scene)
        : m_Scene(scene)
	{

	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
        APP_LOG_INFO("Serializing scene to file: {0} START", std::filesystem::absolute(filepath).string());

        json sceneData;
        sceneData["Scene"] = "Untitled Scene";
        sceneData["Entities"] = json::array(); // Create an empty array

        m_Scene->m_Registry.each([&](auto entityID)
            {
             
                Entity entity = { entityID, m_Scene.get() };
                if (!entity) return;

                APP_LOG_INFO("Serializing entity: {0}", entity.GetComponent<TagComponent>().Tag);

                json entityJson;

                // Serialize Basic Info
                entityJson["Entity"] = (uint64_t)entity.GetUUID(); // Assumes you have UUIDs

                // Serialize Tag Component
                if (entity.HasComponent<TagComponent>())
                {
                    entityJson["TagComponent"] = {
                        { "Tag", entity.GetComponent<TagComponent>().Tag }
                    };
                }

                // Serialize Transform
                if (entity.HasComponent<TransformComponent>())
                {
                    auto& tc = entity.GetComponent<TransformComponent>();

                    entityJson["TransformComponent"] = {
                        { "Translation", { tc.Translation.x, tc.Translation.y, tc.Translation.z } },
                        { "Rotation",    { tc.Rotation.x, tc.Rotation.y, tc.Rotation.z } },
                        { "Scale",       { tc.Scale.x, tc.Scale.y, tc.Scale.z } }
                    };
                }

                // Serialize SpriteRenderer
                if (entity.HasComponent<SpriteRendererComponent>())
                {
                    auto& sc = entity.GetComponent<SpriteRendererComponent>();

                    entityJson["SpriteRendererComponent"] = {
                        { "Color", { sc.Color.r, sc.Color.g, sc.Color.b, sc.Color.a } },
                        { "TexturePath", sc.Texture ? sc.Texture->GetPath() : "##None##" },
                        { "TilingFactor", sc.TilingFactor }
                    };
                }

                // Serialize Camera
                if (entity.HasComponent<CameraComponent>())
                {
                    auto& cc = entity.GetComponent<CameraComponent>();
                    auto& camera = cc.Camera;

                    entityJson["CameraComponent"] = {
                        {
                            "Camera", {
                                { "ProjectionType"  , (int)camera.GetProjectionType() },
                                { "PerspectiveFOV"  , camera.GetPerspectiveVerticalFOV() },
                                { "PerspectiveNear" , camera.GetPerspectiveNearClip() },
                                { "PerspectiveFar"  , camera.GetPerspectiveFarClip() },
                                { "OrthographicSize", camera.GetOrthographicSize() },
                                { "OrthographicNear", camera.GetOrthographicNearClip() },
                                { "OrthographicFar" , camera.GetOrthographicFarClip() }
                            }
                        },
                        {
                            "Primary", cc.Primary
                        },
                        {
                            "FixedAspectRatio", cc.FixedAspectRatio
                        }
                    };
                }

                // Serialize Circle
                if (entity.HasComponent<CircleRendererComponent>())
                {
                    auto& cc = entity.GetComponent<CircleRendererComponent>();

                    entityJson["CircleRendererComponent"] = {
                        { "Color",       { cc.Color.r, cc.Color.g, cc.Color.b, cc.Color.a } },
                        { "Thickness",   cc.Thickness },
                        { "Fade",        cc.Fade }
                    };
                }

                // Serialize RigidBody2D
                if (entity.HasComponent<Rigidbody2DComponent>())
                {
                    auto& rb2c = entity.GetComponent<Rigidbody2DComponent>();

                    entityJson["Rigidbody2DComponent"] = {
                        { "BodyType",       RigidBody2DBodyTypeToString(rb2c.Type) },
                        { "FixedRotation",  rb2c.FixedRotation }
                    };
                }

                // Serialize BoxCollider2D
                if (entity.HasComponent<BoxCollider2DComponent>())
                {
                    auto& bc2c = entity.GetComponent<BoxCollider2DComponent>();

                    entityJson["BoxCollider2DComponent"] = {
                        { "Offset",                 { bc2c.Offset.x, bc2c.Offset.y } },
                        { "Size",                   { bc2c.Size.x, bc2c.Size.y } },
                        { "Density",                bc2c.Density },
                        { "Friction",               bc2c.Friction },
                        { "Restitution",            bc2c.Restitution },
                        { "RestitutionThreshold",   bc2c.RestitutionThreshold },
                    };
                }

                // Serialize CircleCollider2D
                if (entity.HasComponent<CircleCollider2DComponent>())
                {
                    auto& cc2c = entity.GetComponent<CircleCollider2DComponent>();

                    entityJson["CircleCollider2DComponent"] = {
                        { "Offset",                 { cc2c.Offset.x, cc2c.Offset.y } },
                        { "Radius",                 cc2c.Radius },
                        { "Density",                cc2c.Density },
                        { "Friction",               cc2c.Friction },
                        { "Restitution",            cc2c.Restitution },
                        { "RestitutionThreshold",   cc2c.RestitutionThreshold },
                    };
                }

                // Serialize Text
                if (entity.HasComponent<TextComponent>())
                {
                    auto& tc = entity.GetComponent<TextComponent>();

                    entityJson["TextComponent"] = {
                        { "TextString",             tc.TextString },
                        { "Color",                  { tc.Color.r, tc.Color.g, tc.Color.b, tc.Color.a } },
                        { "Kerning",                tc.Kerning},
                        { "LineSpacing",            tc.LineSpacing}
                    };
                }

                // Add to main list
                sceneData["Entities"].push_back(entityJson);
            });

        // Write to file
        std::ofstream fout(filepath);
        // std::setw(4) makes it "Pretty Print" with 4 spaces of indentation
        fout << std::setw(4) << sceneData;

        APP_LOG_INFO("Serializing scene FINISH");
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		ASSERT(false, "SerializeRuntime not implemented");
	}
	
	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
        APP_LOG_INFO("Deserializing scene from file: {0} START", std::filesystem::absolute(filepath).string());

        std::ifstream stream(filepath);
        if (!stream.is_open()) return false;

        json sceneData;
        try {
            stream >> sceneData;
        }
        catch (const json::parse_error& e) {
            APP_LOG_ERROR("JSON Parse Error: {0}", e.what());
            return false;
        }

        auto entities = sceneData["Entities"];
        if (entities.is_array())
        {
            // Helper lambda to load vec2 from array
            auto loadVec2 = [](json& j) { return glm::vec2(j[0], j[1]); };

            // Helper lambda to load vec3 from array
            auto loadVec3 = [](json& j) { return glm::vec3(j[0], j[1], j[2]); };

            // Helper lambda to load vec4 from array
            auto loadVec4 = [](json& j) { return glm::vec4(j[0], j[1], j[2], j[3]); };

            for (auto& entityJson : entities)
            {
                // Create Entity
                uint64_t uuid = entityJson["Entity"];
                std::string name;
                if (entityJson.contains("TagComponent"))
                    name = entityJson["TagComponent"]["Tag"];

                Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

                APP_LOG_INFO("Deserializing entity: {0} name: {1}.", uuid, name);

                // Load Transform
                if (entityJson.contains("TransformComponent"))
                {
                    auto& tc = deserializedEntity.GetComponent<TransformComponent>();
                    auto& tJson = entityJson["TransformComponent"];

                    tc.Translation = loadVec3(tJson["Translation"]);
                    tc.Rotation = loadVec3(tJson["Rotation"]);
                    tc.Scale = loadVec3(tJson["Scale"]);
                }

                // Load Camera
                if (entityJson.contains("CameraComponent"))
                {
                    auto& cc = deserializedEntity.AddComponent<CameraComponent>();
                    auto& cJson = entityJson["CameraComponent"];

                    cc.Primary = cJson["Primary"];
                    cc.FixedAspectRatio = cJson["FixedAspectRatio"];

                    auto& cameraJson = cJson["Camera"];
                    cc.Camera.SetOrthographic(cameraJson["OrthographicSize"], cameraJson["OrthographicNear"], cameraJson["OrthographicFar"]);
                    cc.Camera.SetPerspective(cameraJson["PerspectiveFOV"], cameraJson["PerspectiveNear"], cameraJson["PerspectiveFar"]);
                    cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraJson["ProjectionType"]);
                }

                // Load SpriteRenderer
                if (entityJson.contains("SpriteRendererComponent"))
                {
                    auto& sc = deserializedEntity.AddComponent<SpriteRendererComponent>();
                    auto& sJson = entityJson["SpriteRendererComponent"];

                    sc.Color = loadVec4(sJson["Color"]);
                    if(sJson["TexturePath"] != "##None##")
                        sc.Texture = Texture2D::Create(sJson["TexturePath"]);
                    sc.TilingFactor = sJson["TilingFactor"];
                }

                // Load Circle
                if (entityJson.contains("CircleRendererComponent"))
                {
                    auto& cc = deserializedEntity.AddComponent<CircleRendererComponent>();
                    auto& cJson = entityJson["CircleRendererComponent"];

                    cc.Color = loadVec4(cJson["Color"]);
                    cc.Thickness = cJson["Thickness"];
                    cc.Fade = cJson["Fade"];
                }

                // Load RigidBody2D
                if (entityJson.contains("Rigidbody2DComponent"))
                {
                    auto& rb2c = deserializedEntity.AddComponent<Rigidbody2DComponent>();
                    auto& rb2Json = entityJson["Rigidbody2DComponent"];

                    rb2c.Type = RigidBody2DBodyTypeFromString(rb2Json["BodyType"]);
                    rb2c.FixedRotation = rb2Json["FixedRotation"];
                }

                // Load BoxCollider2D
                if (entityJson.contains("BoxCollider2DComponent"))
                {
                    auto& bc2c = deserializedEntity.AddComponent<BoxCollider2DComponent>();
                    auto& bc2Json = entityJson["BoxCollider2DComponent"];

                    bc2c.Offset = loadVec2(bc2Json["Offset"]);
                    bc2c.Size = loadVec2(bc2Json["Size"]);
                    bc2c.Density = bc2Json["Density"];
                    bc2c.Friction = bc2Json["Friction"];
                    bc2c.Restitution = bc2Json["Restitution"];
                    bc2c.RestitutionThreshold = bc2Json["RestitutionThreshold"];
                }

                // Load CircleCollider2D
                if (entityJson.contains("CircleCollider2DComponent"))
                {
                    auto& cc2c = deserializedEntity.AddComponent<CircleCollider2DComponent>();
                    auto& cc2Json = entityJson["CircleCollider2DComponent"];

                    cc2c.Offset = loadVec2(cc2Json["Offset"]);
                    cc2c.Radius = cc2Json["Radius"];
                    cc2c.Density = cc2Json["Density"];
                    cc2c.Friction = cc2Json["Friction"];
                    cc2c.Restitution = cc2Json["Restitution"];
                    cc2c.RestitutionThreshold = cc2Json["RestitutionThreshold"];
                }

                // Load Text
                if (entityJson.contains("TextComponent"))
                {
                    auto& tc = deserializedEntity.AddComponent<TextComponent>();
                    auto& tJson = entityJson["TextComponent"];

                    tc.TextString = tJson["TextString"];
                    tc.Color = loadVec4(tJson["Color"]);
                    tc.Kerning = tJson["Kerning"];
                    tc.LineSpacing = tJson["LineSpacing"];
                }
            }
        }

        APP_LOG_INFO("Deserializing scene FINISH");

        return true;
	}
	
	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		ASSERT(false, "DeserializeRuntime not implemented");
		return false;
	}
}