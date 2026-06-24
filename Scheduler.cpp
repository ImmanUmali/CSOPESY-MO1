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

        // --- MILESTONE 5: AUTOMATED BACKGROUND GENERATION ---
        if (m_generationEnabled && (m_cpuCycles % m_batchProcessFreq == 0)) {
            int nextPid = ++m_generatedPidCounter;
            
            // Generate padded clean formatting string e.g. p01, p02...
            std::string procName = std::string("p") + (nextPid < 10 ? "0" : "") + std::to_string(nextPid);
            
            auto batchProc = std::make_shared<Process>(nextPid, procName, m_minIns, m_maxIns);
            
            // Use internal call bypassing lock duplication
            m_allTrackedProcesses.push_back(batchProc);
            if (m_schedulerType == "fcfs") {
                m_fcfsScheduler.addProcess(batchProc);
            } else if (m_schedulerType == "rr") {
                m_rrScheduler.addProcess(batchProc);
            }
        }

        // --- CORE CPU EXECUTION PIPELINE (Unmodified from your team's code) ---
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
                    if (!process->isFinished()) {
                        m_rrScheduler.addProcess(process);
                    }
                    cpu.assignProcess(nullptr);
                    cpu.resetCyclesExecuted();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_delayPerExec));
    }
}