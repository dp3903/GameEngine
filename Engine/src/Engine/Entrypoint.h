#pragma once

#ifdef ENGINE_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	Engine::Logger::init();

	auto app = Engine::CreateApplication({ argc, argv });
	app->run();
	delete app;
}

#endif // ENGINE_PLATFORM_WINDOWS
