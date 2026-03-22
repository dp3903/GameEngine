#include <stdio.h>
#include <Engine.h>
#include "Engine/Entrypoint.h"

#include "Editor3DLayer.h"

namespace Engine
{
	class Editor3D : public Application
	{
	public:
		Editor3D(const ApplicationSpecification& spec)
			: Application(spec)
		{

			PushLayer(new Editor3DLayer());
		
		}
		~Editor3D()
		{

		}
	};

	Engine::Application* Engine::CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Editor3D Application";
		spec.CommandLineArgs = args;

		return new Editor3D(spec);
	}
}