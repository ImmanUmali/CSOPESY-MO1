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

        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);
            // STEP A: Process waiting tasks and decrement sleep durations
            for (auto it = m_waitingProcesses.begin(); it != m_waitingProcesses.end(); ) {
                (*it)->decrementSleep();

                if ((*it)->getRemainingSleep() == 0) {
                    (*it)->setState(ProcessState::READY);

                    // Re-queue the process back into its active scheduling algorithm
                    if (m_schedulerType == "fcfs") {
                        m_fcfsScheduler.addProcess(*it);
                    }
                    else if (m_schedulerType == "rr") {
                        m_rrScheduler.addProcess(*it);
                    }
                    it = m_waitingProcesses.erase(it); // Remove from waiting list
                }
                else {
                    ++it;

                }
            }
        }

        // MILESTONE 5: AUTOMATED BACKGROUND GENERATION
        if (m_generationEnabled.load()) {
            // Check if generation interval parameter is hit matching frequency ticks
            if (m_cpuCycles.load() % m_batchProcessFreq == 0) {
                int pid = ++m_generatedPidCounter;
                std::string processName = "p" + std::to_string(pid);
                
                auto batchProc = std::make_shared<Process>(pid, processName, m_minIns, m_maxIns);
                
                {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    m_allTrackedProcesses.push_back(batchProc);
                }

                if (m_schedulerType == "fcfs") {
                    m_fcfsScheduler.addProcess(batchProc);
                } else if (m_schedulerType == "rr") {
                    m_rrScheduler.addProcess(batchProc);
                }
            }
        }

        for (auto& cpu : m_cpuCores) {
            if (m_schedulerType == "fcfs") {
                if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                    cpu.assignProcess(m_fcfsScheduler.getNextProcess());
                }

                cpu.executeCycle();
                auto process = cpu.getCurrentProcess();

                // STEP B: Handle a process that went to sleep during its cycle execution
                if (process && process->getState() == ProcessState::WAITING) {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    m_waitingProcesses.push_back(process);
                    cpu.assignProcess(nullptr); // Evict from CPU core
                }

                if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                    cpu.assignProcess(m_fcfsScheduler.getNextProcess());
                }
            }
            else if (m_schedulerType == "rr") {
                if (cpu.isIdle() && m_rrScheduler.hasProcess()) {
                    cpu.assignProcess(m_rrScheduler.getNextProcess());
                    cpu.resetCyclesExecuted();
                }

                cpu.executeCycle();
                auto process = cpu.getCurrentProcess();

                // STEP C: Handle a process that went to sleep under Round Robin
                if (process && process->getState() == ProcessState::WAITING) {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    m_waitingProcesses.push_back(process);
                    cpu.assignProcess(nullptr); // Evict from CPU core
                    cpu.resetCyclesExecuted();
                    process = nullptr;
                }

                // Handle Quantum Expiry Preemption
                if (process && cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                    if (!process->isFinished()) {
                        m_rrScheduler.addProcess(process);
                    }
                    cpu.assignProcess(nullptr);
                    cpu.resetCyclesExecuted();
                }

                if (cpu.isIdle() && m_rrScheduler.hasProcess()) {
                    cpu.assignProcess(m_rrScheduler.getNextProcess());
                    cpu.resetCyclesExecuted();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
    }
}