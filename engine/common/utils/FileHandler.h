#pragma once
#include <fstream>
#include <filesystem>
#include "exceptions/RevidRuntimeException.h"

namespace RevidUtils
{
	static std::vector<char> ReadFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			String msg = "Failed to open file: " + filename;
			throw Revid::RevidRuntimeException(msg.c_str());
		}

		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
}
