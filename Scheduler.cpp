#include "Scheduler.h"
#include <chrono>

Scheduler::Scheduler(const std::string& type, int numCpu, unsigned int quantum, unsigned int delayPerExec)
    : m_schedulerType(type),
    m_rrScheduler(quantum),
    m_delayPerExec(delayPerExec),
    m_cpuCycles(0),
    m_running(false)
{
    for (int i = 0; i < numCpu; ++i) {
        m_cpuCores.emplace_back(i);
    }
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (!m_running) {
        m_running = true;
        m_schedulerThread = std::thread(&Scheduler::threadLoop, this);
    }
}

void Scheduler::stop() {
    if (m_running) {
        m_running = false;
        if (m_schedulerThread.joinable()) {
            m_schedulerThread.join();
        }
    }
}

void Scheduler::addProcess(std::shared_ptr<Process> process) {
    if (m_schedulerType == "fcfs") {
        m_fcfsScheduler.addProcess(process);
    }
    else if (m_schedulerType == "rr") {
        m_rrScheduler.addProcess(process);
    }
}

void Scheduler::threadLoop() {
    while (m_running) {
        m_cpuCycles++;

        for (auto& cpu : m_cpuCores) {
            if (m_schedulerType == "fcfs") {
                if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                    cpu.assignProcess(m_fcfsScheduler.getNextProcess());
                }
                cpu.executeCycle();
            }
            else if (m_schedulerType == "rr") {
                if (cpu.isIdle() && m_rrScheduler.hasProcess()) {
                    cpu.assignProcess(m_rrScheduler.getNextProcess());
                    cpu.resetCyclesExecuted();
                }

                cpu.executeCycle();
                auto process = cpu.getCurrentProcess();

                if (!process)
                    continue;

                if (cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                    m_rrScheduler.addProcess(process);
                    cpu.assignProcess(nullptr);
                    cpu.resetCyclesExecuted();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
    }
}