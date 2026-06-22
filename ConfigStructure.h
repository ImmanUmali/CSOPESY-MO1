#pragma once

#include <string>
#include <cstdint>

struct SystemConfig {
    uint32_t numCpu = 1;
    std::string scheduler = "fcfs";
    uint32_t quantumCycles = 1;
    uint32_t batchProcessFreq = 1;
    uint32_t minIns = 1;
    uint32_t maxIns = 1;
    uint32_t delayPerExec = 0;
};