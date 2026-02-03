#pragma once

#include <string>

namespace Engine {

	class FileDialogs
	{
	public:
		// These return empty strings if cancelled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter, const char* extension = "");

		static std::string SelectDirectory();
	};

}