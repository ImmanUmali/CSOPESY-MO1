#include "Scheduler.h"
#include "Process.h"
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
    std::lock_guard<std::mutex> lock(m_schedulerMutex);
    
    // Track globally for reporting metrics
    m_allTrackedProcesses.push_back(process);

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

        // Automated Process Generation
        if (m_generationEnabled.load()) {
            if (m_cpuCycles.load() % m_batchProcessFreq == 0) {
                int pid = ++m_generatedPidCounter;
                std::string processName = "p" + std::to_string(pid);

                auto batchProc = std::make_shared<Process>(pid, processName, m_minIns, m_maxIns);

                // Keep the state explicit
                batchProc->setState(ProcessState::READY);

                {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    m_allTrackedProcesses.push_back(batchProc);

                    if (m_schedulerType == "fcfs") {
                        m_fcfsScheduler.addProcess(batchProc);
                    }
                    else if (m_schedulerType == "rr") {
                        m_rrScheduler.addProcess(batchProc);
                    }
                }
            }
        }

        // Variable tracking if work was actually managed this cycle
        bool activeWorkDone = false;

        // Scope lock for core pipeline operations
        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);

            for (auto& cpu : m_cpuCores) {
                if (m_schedulerType == "fcfs") {
                    if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                        auto proc = m_fcfsScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                    }

                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle();
                    }

                    if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                        auto proc = m_fcfsScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                    }
                }
                else if (m_schedulerType == "rr") {
                    if (cpu.isIdle() && m_rrScheduler.hasProcess()) {
                        auto proc = m_rrScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                        cpu.resetCyclesExecuted();
                    }

                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle();
                    }

                    auto process = cpu.getCurrentProcess();

                    // Handle Quantum Expiry Preemption Safely
                    if (process && cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                        if (!process->isFinished()) {
                            // Flip state back to READY before re-queuing
                            process->setState(ProcessState::READY);
                            m_rrScheduler.addProcess(process);
                        }
                        cpu.assignProcess(nullptr);
                        cpu.resetCyclesExecuted();
                    }

                    if (cpu.isIdle() && m_rrScheduler.hasProcess()) {
                        auto proc = m_rrScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                        cpu.resetCyclesExecuted();
                    }
                }
            }
        }

        // If the system has 0 delay and nothing is runnable, yield CPU slice 
        // to let dashboard rendering commands print cleanly without thread choking
        if (m_delayPerExec == 0) {
            if (!activeWorkDone) {
                std::this_thread::yield();
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
        }
    }
}