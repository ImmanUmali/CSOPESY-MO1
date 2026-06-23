#pragma once

#include <vector>
#include <string>

#include "CPUCore.h"
#include "FCFSScheduler.h"
#include "RoundRobinScheduler.h"

class Scheduler {
private:
    std::string m_schedulerType;

public:
    Scheduler(const std::string& schedulerType);

    void run(
        std::vector<CPUCore>& cpuCores,
        FCFSScheduler& fcfsScheduler,
        RoundRobinScheduler& rrScheduler);
};