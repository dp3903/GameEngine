#pragma once

#include "egpch.h"
#include "Engine/Core.h"
#include "Engine/Events/Event.h"
#include <GLFW/glfw3.h>

namespace Engine {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "Dhruv's Engine",
			uint32_t width = 1280,
			uint32_t height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		Window(const WindowProps& props);
		~Window();

		using EventCallbackFn = std::function<void(Event&)>;

		void OnUpdate();

		// Window attributes
		void SetVSync(bool enabled);
		bool IsVSync() const;

		static Window* Create(const WindowProps& props = WindowProps());

		inline uint32_t GetWidth() const { return m_Data.Width; }
		inline uint32_t GetHeight() const { return m_Data.Height; }
		inline void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
		inline virtual void* GetNativeWindow() const { return m_Window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}