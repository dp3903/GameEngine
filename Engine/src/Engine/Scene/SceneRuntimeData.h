#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include "glm/glm.hpp"

namespace Engine
{
	struct SceneChangeRequest
	{
		bool Requested = false;
		std::string scenePath = "";
	};

	// Define the "Super Type" once
	using RuntimeValue = std::variant<
		int,
		float,
		bool,
		std::string,
		glm::vec2,
		glm::vec3,
		glm::vec4
	>;

	class RuntimeData
	{
	private:
		inline static std::unordered_map<std::string, RuntimeValue> RuntimeDataStore;
		
		inline static SceneChangeRequest changeRequest;
	
	public:
		static bool HasData(const std::string& key)
		{
			return RuntimeDataStore.find(key) != RuntimeDataStore.end();
		}
		
		template<typename T>
		static void SetData(const std::string& key, T value)
		{
			RuntimeDataStore[key] = value;
		}
		
		static RuntimeValue GetData(const std::string& key)
		{
			return RuntimeDataStore[key];
		}

		static void RequestSceneChange(const std::string& path)
		{
			ENGINE_LOG_WARN("Scene change requested to \'{}\'.", path);
			changeRequest.Requested = true;
			changeRequest.scenePath = path;
		}
		
		static SceneChangeRequest RequestStatus()
		{
			return changeRequest;
		}

		static void ResetChangeRequest()
		{
			changeRequest.Requested = false;
			changeRequest.scenePath = "";
		}
	};
}