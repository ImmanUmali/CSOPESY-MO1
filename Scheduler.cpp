#include "Scheduler.h"
#include "Process.h"
#include <chrono>

Scheduler::Scheduler(const std::string& type, int numCpu, unsigned int quantum, unsigned int delayPerExec, std::shared_ptr<MemoryManager> memManager)
    : m_schedulerType(type),
    m_rrScheduler(quantum),
    m_delayPerExec(delayPerExec),
    m_cpuCycles(0),
    m_running(false),
    m_memoryManager(memManager) // Hook up the memory manager
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
    std::lock_guard<std::mutex> memLock(m_memoryMutex);

    // 1. Try allocating memory first
    if (m_memoryManager && m_memoryManager->allocate(process->getName())) {
        m_allTrackedProcesses.push_back(process);

        if (m_schedulerType == "fcfs") {
            m_fcfsScheduler.addProcess(process);
        }
        else if (m_schedulerType == "rr") {
            m_rrScheduler.addProcess(process);
        }
    }
    else {
        // Log or handle "Out of Memory" state for this process
        process->setState(ProcessState::WAITING); // Assuming WAITING exists for OOM
    }
}

void Scheduler::threadLoop() {
    while (m_running) {
        m_cpuCycles++;

        // Automated Process Generation
        // Inside Scheduler::threadLoop() under Automated Process Generation:
        if (m_generationEnabled.load()) {
            if (m_cpuCycles.load() % m_batchProcessFreq == 0) {
                int pid = ++m_generatedPidCounter;
                std::string processName = "p" + std::to_string(pid);

                auto batchProc = std::make_shared<Process>(pid, processName, m_minIns, m_maxIns);

                {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    std::lock_guard<std::mutex> memLock(m_memoryMutex);

                    // Verify memory budget before making it runnable
                    if (m_memoryManager && m_memoryManager->allocate(batchProc->getName())) {
                        batchProc->setState(ProcessState::READY);
                        m_allTrackedProcesses.push_back(batchProc);

                        if (m_schedulerType == "fcfs") {
                            m_fcfsScheduler.addProcess(batchProc);
                        }
                        else if (m_schedulerType == "rr") {
                            m_rrScheduler.addProcess(batchProc);
                        }
                    }
                    else {
                        // System is out of memory; drop or store in a separate backed-up queue
                    }
                }
            }
        }

        // Variable tracking if work was actually managed this cycle
        // Variable tracking if work was actually managed this cycle
        bool activeWorkDone = false;

        // Scope lock for core pipeline operations
        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);
            std::lock_guard<std::mutex> memLock(m_memoryMutex); // Protect shared memory structures

            for (auto& cpu : m_cpuCores) {
                if (m_schedulerType == "fcfs") {
                    // 1. If core is idle, pull a process
                    if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                        auto proc = m_fcfsScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                    }

                    // 2. Execute work if a process is assigned
                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle();
                    }

                    // 3. HOOK: Check if the process finished during this cycle
                    auto process = cpu.getCurrentProcess();
                    if (process && process->isFinished()) {
                        if (m_memoryManager) {
                            m_memoryManager->deallocate(process->getName());
                        }
                        process->setState(ProcessState::FINISHED); // Explicitly update state
                        cpu.assignProcess(nullptr);                // Free up the core
                    }

                    // 4. Backfill core immediately if it just became idle
                    if (cpu.isIdle() && m_fcfsScheduler.hasProcess()) {
                        auto proc = m_fcfsScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                    }
                }
                else if (m_schedulerType == "rr") {
                    // 1. If core is idle, pull a process
                    if (cpu.isIdle() && m_rrScheduler.hasProcess()) {
                        auto proc = m_rrScheduler.getNextProcess();
                        if (proc) {
                            proc->setState(ProcessState::RUNNING);
                            cpu.assignProcess(proc);
                        }
                        cpu.resetCyclesExecuted();
                    }

                    // 2. Execute work if a process is assigned
                    if (!cpu.isIdle()) {
                        activeWorkDone = true;
                        cpu.executeCycle();
                    }

                    // 3. HOOK: Check if it finished OR if its time quantum expired
                    auto process = cpu.getCurrentProcess();
                    if (process) {
                        if (process->isFinished()) {
                            // Process completed -> Free memory completely
                            if (m_memoryManager) {
                                m_memoryManager->deallocate(process->getName());
                            }
                            process->setState(ProcessState::FINISHED);
                            cpu.assignProcess(nullptr);
                            cpu.resetCyclesExecuted();
                        }
                        else if (cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                            // Quantum expired but process isn't done -> Retain memory, re-queue to ready queue
                            process->setState(ProcessState::READY);
                            m_rrScheduler.addProcess(process);
                            cpu.assignProcess(nullptr);
                            cpu.resetCyclesExecuted();
                        }
                    }

                    // 4. Backfill core immediately if it just became idle
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