#pragma once

#include "egpch.h"
#include "Engine/Core.h"
#include "MouseCodes.h"
#include "KeyCodes.h"

namespace Engine {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};

}