#include <stdio.h>
#include <Engine.h>

class ExampleLayer : public Engine::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{
	}

	void OnUpdate() override
	{
		APP_LOG_INFO("ExampleLayer::Update");
	}

	void OnEvent(Engine::Event& event) override
	{
		APP_LOG_TRACE("{0}", event);
	}

};

class Game : public Engine::Application
{
public:
	Game()
	{
		PushLayer(new ExampleLayer());
		PushOverlay(new Engine::ImGuiLayer());
	}
	~Game()
	{

	}
};

Engine::Application* Engine::CreateApplication()
{
	return new Game();
}