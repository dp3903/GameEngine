#include <stdio.h>
#include <Engine.h>
#include "Engine/Entrypoint.h"

#include "EditorLayer.h"

namespace Engine
{
	class Editor : public Application
	{
	public:
		Editor(const ApplicationSpecification& spec)
			: Application(spec)
		{

			PushLayer(new EditorLayer());
		
		}
		~Editor()
		{

		}
	};

	Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Editor Application";
		spec.CommandLineArgs = args;

		return new Editor(spec);
	}
}