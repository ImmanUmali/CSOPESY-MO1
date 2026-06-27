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
        bool activeWorkDone = false;
        
        m_cpuCycles++; 

        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);

            // Automated Process Generation
            if (m_generationEnabled && (m_cpuCycles % m_batchProcessFreq == 0)) {
                int nextPid = ++m_generatedPidCounter;
                std::string procName = "p" + std::to_string(nextPid);
                
                auto newProc = std::make_shared<Process>(nextPid, procName, m_minIns, m_maxIns);
                
                m_allTrackedProcesses.push_back(newProc);

                if (m_schedulerType == "fcfs") {
                    m_fcfsScheduler.addProcess(newProc);
                } else if (m_schedulerType == "rr") {
                    m_rrScheduler.addProcess(newProc);
                }
            }

            // Sleep handler
            for (auto it = m_waitingProcesses.begin(); it != m_waitingProcesses.end(); ) {
                auto proc = *it;
                proc->decrementSleepTicks(); 

                if (proc->getState() == ProcessState::READY) {
                    if (m_schedulerType == "fcfs") m_fcfsScheduler.addProcess(proc);
                    else if (m_schedulerType == "rr") m_rrScheduler.addProcess(proc);
                    
                    it = m_waitingProcesses.erase(it);
                } else {
                    ++it;
                }
            }

            // Process cycle execution
            for (auto& cpu : m_cpuCores) {
                if (!cpu.isIdle()) {
                    auto process = cpu.getCurrentProcess();
                    
                    cpu.executeCycle();
                    activeWorkDone = true;

                    if (process && process->getState() == ProcessState::WAITING) {
                        m_waitingProcesses.push_back(process);
                        cpu.assignProcess(nullptr); // Free core immediately to hit 0% utilization if empty
                        cpu.resetCyclesExecuted();
                        continue;
                    }

                    // Handle natural finish
                    if (process && process->isFinished()) {
                        cpu.assignProcess(nullptr);
                        cpu.resetCyclesExecuted();
                        continue;
                    }

                    // Handle Round Robin Quantum preemption
                    if (m_schedulerType == "rr" && process && 
                        cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                        if (!process->isFinished()) {
                            process->setState(ProcessState::READY);
                            m_rrScheduler.addProcess(process);
                        }
                        cpu.assignProcess(nullptr);
                        cpu.resetCyclesExecuted();
                    }
                }

                // Dispatch next ready process to idle cores
                if (cpu.isIdle()) {
                    std::shared_ptr<Process> nextProc = nullptr;
                    if (m_schedulerType == "fcfs" && m_fcfsScheduler.hasProcess()) {
                        nextProc = m_fcfsScheduler.getNextProcess();
                    } else if (m_schedulerType == "rr" && m_rrScheduler.hasProcess()) {
                        nextProc = m_rrScheduler.getNextProcess();
                    }

                    if (nextProc) {
                        nextProc->setState(ProcessState::RUNNING);
                        cpu.assignProcess(nextProc);
                        cpu.resetCyclesExecuted();
                        activeWorkDone = true;
                    }
                }
            }
        }

        // 6. THROTTLE CONTROL SLICE
        if (m_delayPerExec == 0) {
            if (!activeWorkDone) {
                std::this_thread::yield();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
        }
    }
}