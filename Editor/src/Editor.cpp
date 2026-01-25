#include <stdio.h>
#include <Engine.h>
#include "Engine/Entrypoint.h"

#include "EditorLayer.h"

namespace Engine
{
	class Editor : public Application
	{
	public:
		Editor(ApplicationCommandLineArgs args)
			: Application("Level Editor", args)
		{
			//PushLayer(new ExampleLayer());
			PushLayer(new EditorLayer());
		}
		~Editor()
		{

		}
	};

	Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
	{
		return new Editor(args);
	}
}