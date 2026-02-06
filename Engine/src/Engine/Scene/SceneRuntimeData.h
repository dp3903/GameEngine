#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include "glm/glm.hpp"
#include "sol/sol.hpp"

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
		inline static std::unordered_map<std::string, sol::protected_function> RuntimeFunctionStore;
		
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

		static bool HasFunction(const std::string& key)
		{
			return RuntimeFunctionStore.find(key) != RuntimeFunctionStore.end();
		}

		static void RegisterFunction(const std::string& key, sol::protected_function func)
		{
			RuntimeFunctionStore[key] = func;
		}

		static sol::object CallFunction(const std::string& name, sol::variadic_args args)
		{
			if (RuntimeFunctionStore.find(name) == RuntimeFunctionStore.end())
			{
				ENGINE_LOG_ERROR("Function '{}' not found in registry!", name);
				return sol::lua_nil;
			}

			// Call the stored function with the arguments passed in
			auto result = RuntimeFunctionStore[name](args);

			if (!result.valid())
			{
				sol::error err = result;
				ENGINE_LOG_ERROR("Error executing global function '{}': {}", name, err.what());
				return sol::lua_nil;
			}

			// Return the result back to Lua (if any)
			return result;
		}

		static void ClearFunctionRegistry()
		{
			RuntimeFunctionStore.clear();
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