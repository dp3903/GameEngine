#include <stdio.h>
#include <Engine.h>

class Game : public Engine::Application
{
public:
	Game()
	{

	}
	~Game()
	{

	}
};

Engine::Application* Engine::CreateApplication()
{
	return new Game();
}