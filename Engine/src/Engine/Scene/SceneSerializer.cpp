#include "egpch.h"
#include "SceneSerializer.h"
#include "Components.h"

#include <fstream>
#include <iomanip> // Required for std::setw (Pretty Printing)
#include <nlohmann_json/json.hpp>

using json = nlohmann::ordered_json;

namespace Engine {
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
                entityJson["Entity"] = 12345678; //(uint64_t)entity.GetUUID(); // Assumes you have UUIDs

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
                        { "Color", { sc.Color.r, sc.Color.g, sc.Color.b, sc.Color.a } }
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

                Entity deserializedEntity = m_Scene->CreateEntity(name);
                // deserializedEntity.SetUUID(uuid); // If you have this method

                APP_LOG_INFO("Deserializing entity: {0}", name);

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