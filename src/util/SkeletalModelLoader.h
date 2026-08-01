#pragma once

#include "rendering/SkeletalModel.h"

#include <string>

class SkeletalModelLoader
{
public:
    SkeletalModelLoader() = delete;
    [[nodiscard]] static SkeletalModelData load(const std::string& path);
};
