#include <stdio.h>
#include <Engine.h>
#include "Engine/Entrypoint.h"

#include "EditorLayer.h"

namespace Engine
{
	class Editor : public Application
	{
	public:
		Editor()
		{
			//PushLayer(new ExampleLayer());
			PushLayer(new EditorLayer());
		}
		~Editor()
		{

		}
	};

	Engine::Application* Engine::CreateApplication()
	{
		return new Editor();
	}
}