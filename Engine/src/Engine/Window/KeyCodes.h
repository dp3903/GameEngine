#pragma once

namespace Engine
{
	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */

		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */

		Semicolon = 59, /* ; */
		Equal = 61, /* = */

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		LeftBracket = 91,  /* [ */
		Backslash = 92,  /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,  /* ` */

		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */

		/* Function keys */
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,

		/* Keypad */
		KP0 = 320,
		KP1 = 321,
		KP2 = 322,
		KP3 = 323,
		KP4 = 324,
		KP5 = 325,
		KP6 = 326,
		KP7 = 327,
		KP8 = 328,
		KP9 = 329,
		KPDecimal = 330,
		KPDivide = 331,
		KPMultiply = 332,
		KPSubtract = 333,
		KPAdd = 334,
		KPEnter = 335,
		KPEqual = 336,

		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348
	} Key;

	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}
}

// From glfw3.h
#define KEY_SPACE           ::Engine::Key::Space
#define KEY_APOSTROPHE      ::Engine::Key::Apostrophe    /* ' */
#define KEY_COMMA           ::Engine::Key::Comma         /* , */
#define KEY_MINUS           ::Engine::Key::Minus         /* - */
#define KEY_PERIOD          ::Engine::Key::Period        /* . */
#define KEY_SLASH           ::Engine::Key::Slash         /* / */
#define KEY_0               ::Engine::Key::D0
#define KEY_1               ::Engine::Key::D1
#define KEY_2               ::Engine::Key::D2
#define KEY_3               ::Engine::Key::D3
#define KEY_4               ::Engine::Key::D4
#define KEY_5               ::Engine::Key::D5
#define KEY_6               ::Engine::Key::D6
#define KEY_7               ::Engine::Key::D7
#define KEY_8               ::Engine::Key::D8
#define KEY_9               ::Engine::Key::D9
#define KEY_SEMICOLON       ::Engine::Key::Semicolon     /* ; */
#define KEY_EQUAL           ::Engine::Key::Equal         /* = */
#define KEY_A               ::Engine::Key::A
#define KEY_B               ::Engine::Key::B
#define KEY_C               ::Engine::Key::C
#define KEY_D               ::Engine::Key::D
#define KEY_E               ::Engine::Key::E
#define KEY_F               ::Engine::Key::F
#define KEY_G               ::Engine::Key::G
#define KEY_H               ::Engine::Key::H
#define KEY_I               ::Engine::Key::I
#define KEY_J               ::Engine::Key::J
#define KEY_K               ::Engine::Key::K
#define KEY_L               ::Engine::Key::L
#define KEY_M               ::Engine::Key::M
#define KEY_N               ::Engine::Key::N
#define KEY_O               ::Engine::Key::O
#define KEY_P               ::Engine::Key::P
#define KEY_Q               ::Engine::Key::Q
#define KEY_R               ::Engine::Key::R
#define KEY_S               ::Engine::Key::S
#define KEY_T               ::Engine::Key::T
#define KEY_U               ::Engine::Key::U
#define KEY_V               ::Engine::Key::V
#define KEY_W               ::Engine::Key::W
#define KEY_X               ::Engine::Key::X
#define KEY_Y               ::Engine::Key::Y
#define KEY_Z               ::Engine::Key::Z
#define KEY_LEFT_BRACKET    ::Engine::Key::LeftBracket   /* [ */
#define KEY_BACKSLASH       ::Engine::Key::Backslash     /* \ */
#define KEY_RIGHT_BRACKET   ::Engine::Key::RightBracket  /* ] */
#define KEY_GRAVE_ACCENT    ::Engine::Key::GraveAccent   /* ` */
#define KEY_WORLD_1         ::Engine::Key::World1        /* non-US #1 */
#define KEY_WORLD_2         ::Engine::Key::World2        /* non-US #2 */

/* Function keys */
#define KEY_ESCAPE          ::Engine::Key::Escape
#define KEY_ENTER           ::Engine::Key::Enter
#define KEY_TAB             ::Engine::Key::Tab
#define KEY_BACKSPACE       ::Engine::Key::Backspace
#define KEY_INSERT          ::Engine::Key::Insert
#define KEY_DELETE          ::Engine::Key::Delete
#define KEY_RIGHT           ::Engine::Key::Right
#define KEY_LEFT            ::Engine::Key::Left
#define KEY_DOWN            ::Engine::Key::Down
#define KEY_UP              ::Engine::Key::Up
#define KEY_PAGE_UP         ::Engine::Key::PageUp
#define KEY_PAGE_DOWN       ::Engine::Key::PageDown
#define KEY_HOME            ::Engine::Key::Home
#define KEY_END             ::Engine::Key::End
#define KEY_CAPS_LOCK       ::Engine::Key::CapsLock
#define KEY_SCROLL_LOCK     ::Engine::Key::ScrollLock
#define KEY_NUM_LOCK        ::Engine::Key::NumLock
#define KEY_PRINT_SCREEN    ::Engine::Key::PrintScreen
#define KEY_PAUSE           ::Engine::Key::Pause
#define KEY_F1              ::Engine::Key::F1
#define KEY_F2              ::Engine::Key::F2
#define KEY_F3              ::Engine::Key::F3
#define KEY_F4              ::Engine::Key::F4
#define KEY_F5              ::Engine::Key::F5
#define KEY_F6              ::Engine::Key::F6
#define KEY_F7              ::Engine::Key::F7
#define KEY_F8              ::Engine::Key::F8
#define KEY_F9              ::Engine::Key::F9
#define KEY_F10             ::Engine::Key::F10
#define KEY_F11             ::Engine::Key::F11
#define KEY_F12             ::Engine::Key::F12
#define KEY_F13             ::Engine::Key::F13
#define KEY_F14             ::Engine::Key::F14
#define KEY_F15             ::Engine::Key::F15
#define KEY_F16             ::Engine::Key::F16
#define KEY_F17             ::Engine::Key::F17
#define KEY_F18             ::Engine::Key::F18
#define KEY_F19             ::Engine::Key::F19
#define KEY_F20             ::Engine::Key::F20
#define KEY_F21             ::Engine::Key::F21
#define KEY_F22             ::Engine::Key::F22
#define KEY_F23             ::Engine::Key::F23
#define KEY_F24             ::Engine::Key::F24
#define KEY_F25             ::Engine::Key::F25

/* Keypad */
#define KEY_KP_0            ::Engine::Key::KP0
#define KEY_KP_1            ::Engine::Key::KP1
#define KEY_KP_2            ::Engine::Key::KP2
#define KEY_KP_3            ::Engine::Key::KP3
#define KEY_KP_4            ::Engine::Key::KP4
#define KEY_KP_5            ::Engine::Key::KP5
#define KEY_KP_6            ::Engine::Key::KP6
#define KEY_KP_7            ::Engine::Key::KP7
#define KEY_KP_8            ::Engine::Key::KP8
#define KEY_KP_9            ::Engine::Key::KP9
#define KEY_KP_DECIMAL      ::Engine::Key::KPDecimal
#define KEY_KP_DIVIDE       ::Engine::Key::KPDivide
#define KEY_KP_MULTIPLY     ::Engine::Key::KPMultiply
#define KEY_KP_SUBTRACT     ::Engine::Key::KPSubtract
#define KEY_KP_ADD          ::Engine::Key::KPAdd
#define KEY_KP_ENTER        ::Engine::Key::KPEnter
#define KEY_KP_EQUAL        ::Engine::Key::KPEqual

#define KEY_LEFT_SHIFT      ::Engine::Key::LeftShift
#define KEY_LEFT_CONTROL    ::Engine::Key::LeftControl
#define KEY_LEFT_ALT        ::Engine::Key::LeftAlt
#define KEY_LEFT_SUPER      ::Engine::Key::LeftSuper
#define KEY_RIGHT_SHIFT     ::Engine::Key::RightShift
#define KEY_RIGHT_CONTROL   ::Engine::Key::RightControl
#define KEY_RIGHT_ALT       ::Engine::Key::RightAlt
#define KEY_RIGHT_SUPER     ::Engine::Key::RightSuper
#define KEY_MENU            ::Engine::Key::Menu