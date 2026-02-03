#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "egpch.h"
#include <spdlog/spdlog.h>
#include "spdlog/fmt/ostr.h"
#include "Core.h"

namespace Engine {

	class Logger
	{
	private:
		static std::shared_ptr<spdlog::logger> CoreLogger;
		static std::shared_ptr<spdlog::logger> ClientLogger;

	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetEngineLogger();
		static std::shared_ptr<spdlog::logger>& GetAppLogger();
	};


}

// Engine Logging
#define ENGINE_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(::Engine::Logger::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::Engine::Logger::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_LOG_WARN(...)  SPDLOG_LOGGER_WARN(::Engine::Logger::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_LOG_INFO(...)  SPDLOG_LOGGER_INFO(::Engine::Logger::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::Engine::Logger::GetEngineLogger(), __VA_ARGS__)

// App Logging
#define APP_LOG_FATAL(...)    SPDLOG_LOGGER_CRITICAL(::Engine::Logger::GetAppLogger(), __VA_ARGS__)
#define APP_LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(::Engine::Logger::GetAppLogger(), __VA_ARGS__)
#define APP_LOG_WARN(...)     SPDLOG_LOGGER_WARN(::Engine::Logger::GetAppLogger(), __VA_ARGS__)
#define APP_LOG_INFO(...)     SPDLOG_LOGGER_INFO(::Engine::Logger::GetAppLogger(), __VA_ARGS__)
#define APP_LOG_TRACE(...)    SPDLOG_LOGGER_TRACE(::Engine::Logger::GetAppLogger(), __VA_ARGS__)