#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include "Core.h"

namespace Engine {

	class ENGINE_API Logger
	{
	private:
		static std::shared_ptr<spdlog::logger> CoreLogger;
		static std::shared_ptr<spdlog::logger> ClientLogger;

	public:
		static void init();

		inline static std::shared_ptr<spdlog::logger>& GetEngineLogger() { return CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetAppLogger() { return ClientLogger; }
	};


}

#define ENGINE_LOG_FATAL(...) ::Engine::Logger::GetEngineLogger()->critical(__VA_ARGS__)
#define ENGINE_LOG_ERROR(...) ::Engine::Logger::GetEngineLogger()->error(__VA_ARGS__)
#define ENGINE_LOG_WARN(...) ::Engine::Logger::GetEngineLogger()->warn(__VA_ARGS__)
#define ENGINE_LOG_INFO(...) ::Engine::Logger::GetEngineLogger()->info(__VA_ARGS__)
#define ENGINE_LOG_TRACE(...) ::Engine::Logger::GetEngineLogger()->trace(__VA_ARGS__)

#define APP_LOG_FATAL(...) ::Engine::Logger::GetAppLogger()->critical(__VA_ARGS__)
#define APP_LOG_ERROR(...) ::Engine::Logger::GetAppLogger()->error(__VA_ARGS__)
#define APP_LOG_WARN(...) ::Engine::Logger::GetAppLogger()->warn(__VA_ARGS__)
#define APP_LOG_INFO(...) ::Engine::Logger::GetAppLogger()->info(__VA_ARGS__)
#define APP_LOG_TRACE(...) ::Engine::Logger::GetAppLogger()->trace(__VA_ARGS__)