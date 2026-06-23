#include "Scheduler.h"

Scheduler::Scheduler(const std::string& schedulerType)
    : m_schedulerType(schedulerType)
{
}

void Scheduler::run(std::vector<CPUCore>& cpuCores, FCFSScheduler& fcfsScheduler, RoundRobinScheduler& rrScheduler) {
    for (auto& cpu : cpuCores) {
        // =====================
        // FCFS
        // =====================
        if (m_schedulerType == "fcfs") {
            if (cpu.isIdle() && fcfsScheduler.hasProcess())
            {
                cpu.assignProcess(
                    fcfsScheduler.getNextProcess()
                );
            }

            cpu.executeCycle();
        }

        // =====================
        // ROUND ROBIN
        // =====================
        else if (m_schedulerType == "rr") {
            if (cpu.isIdle() && rrScheduler.hasProcess()) {
                cpu.assignProcess(rrScheduler.getNextProcess());
                cpu.resetCyclesExecuted();
            }

            cpu.executeCycle();
            auto process = cpu.getCurrentProcess();

            if (!process)
                continue;

            if (cpu.getCyclesExecuted() >= rrScheduler.getQuantum()) {
                rrScheduler.addProcess(process);
                cpu.assignProcess(nullptr);
                cpu.resetCyclesExecuted();
            }
        }
    }
}