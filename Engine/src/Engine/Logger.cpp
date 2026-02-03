#include "egpch.h"
#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Engine
{

	std::shared_ptr<spdlog::logger> Logger::CoreLogger;
	std::shared_ptr<spdlog::logger> Logger::ClientLogger;

	void Logger::Init()
	{
		spdlog::set_pattern("%^[%D %T] [%l] [%n] [%@]%$ \"%v\"");
		CoreLogger = spdlog::stdout_color_mt("ENGINE");
		CoreLogger->set_level(spdlog::level::level_enum::trace);
		ClientLogger = spdlog::stdout_color_mt("APP");
		ClientLogger->set_level(spdlog::level::level_enum::trace);
	}

	std::shared_ptr<spdlog::logger>& Logger::GetEngineLogger()
	{
		// Safety check: If accessing before init, initialize immediately.
		if (!CoreLogger) {
			Init();
		}
		return CoreLogger;
	}

	std::shared_ptr<spdlog::logger>& Logger::GetAppLogger()
	{
		// Safety check
		if (!ClientLogger) {
			Init();
		}
		return ClientLogger;
	}
}