#pragma once

// For use by external applications only

#include "Engine/Application.h"
#include "Engine/Logger.h"
#include "Engine/Layers/Layer.h"
#include "Engine/Layers/ImGuiLayer.h"

#include "Engine/Scene/Components.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"

#include "Engine/Window/Input.h"
#include "Engine/Window/KeyCodes.h"
#include "Engine/Window/MouseButtonCodes.h"

// ---------Renderer------------------

#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/OrthographicCamera.h"

// -----------------------------------

// -------Entry point(add this include only in the file that is the entrypoint to the game.)--------
//#include "Engine/Entrypoint.h"
// --------------------------