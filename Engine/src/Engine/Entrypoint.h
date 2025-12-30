#pragma once

#ifdef ENGINE_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
	Engine::Logger::init();
	ENGINE_LOG_FATAL("Fatal log.");
	ENGINE_LOG_ERROR("Error log.");
	ENGINE_LOG_WARN("Warn log.");
	ENGINE_LOG_INFO("Info Log.");
	ENGINE_LOG_TRACE("Trace log.");

	auto app = Engine::CreateApplication();
	app->run();
	delete app;
}

#endif // ENGINE_PLATFORM_WINDOWS
