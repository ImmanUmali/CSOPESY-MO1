#pragma once
#include "ConfigStructure.h"

class ConfigLoader {
public:
    static bool loadAndValidate(const std::string& filename, SystemConfig& outConfig);
};