#pragma once

#include <string>

class FileUtils
{
public:
    FileUtils() = delete;

    [[nodiscard]] static std::string readFile(const std::string& path);
};