#include "util/FileUtils.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string FileUtils::readFile(const std::string& path)
{
	std::ifstream file(path);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path);
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}