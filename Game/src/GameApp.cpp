#include <stdio.h>
#include <Engine.h>
#include "Engine/Entrypoint.h"

#include "GameLayer1.h"


class Game : public Engine::Application
{
public:
	Game(const Engine::ApplicationSpecification& specification)
		: Engine::Application(specification)
	{
		PushLayer(new GameLayer1());
	}
	~Game()
	{

	}
};

Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
{
	ApplicationSpecification spec;
	spec.Name = "Game Application";
	spec.WorkingDirectory = "../Editor";
	spec.CommandLineArgs = args;

	return new Game(spec);
}