#include <stdio.h>
#include <Engine.h>
#include "Engine/Entrypoint.h"

#include "ExampleLayer.h"
#include "GameLayer1.h"


class Game : public Engine::Application
{
public:
	Game()
	{
		//PushLayer(new ExampleLayer());
		PushLayer(new GameLayer1());
	}
	~Game()
	{

	}
};

Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
{
	return new Game();
}