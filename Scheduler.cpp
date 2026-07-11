#include "Scheduler.h"
#include "Process.h"
#include <chrono>

Scheduler::Scheduler(const std::string& type, int numCpu, unsigned int quantum, unsigned int delayPerExec, std::shared_ptr<MemoryManager> memManager)
    : m_schedulerType(type),
    m_rrScheduler(quantum),
    m_delayPerExec(delayPerExec),
    m_cpuCycles(0),
    m_running(false),
    m_memoryManager(memManager)
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

    m_allTrackedProcesses.push_back(process); // ALWAYS track it, even if memory fails

    // 1. Try allocating memory first
    if (m_memoryManager && m_memoryManager->allocate(process->getName())) {
        process->setState(ProcessState::READY); // Explicitly mark ready

        if (m_schedulerType == "fcfs") {
            m_fcfsScheduler.addProcess(process);
        }
        else if (m_schedulerType == "rr") {
            m_rrScheduler.addProcess(process);
        }
    }
    else {
        // Not enough memory; put in waiting state to be retried later
        process->setState(ProcessState::WAITING);
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

                {
                    std::lock_guard<std::mutex> lock(m_schedulerMutex);
                    std::lock_guard<std::mutex> memLock(m_memoryMutex);

                    m_allTrackedProcesses.push_back(batchProc); // ALWAYS track it

                    if (m_memoryManager && m_memoryManager->allocate(batchProc->getName())) {
                        batchProc->setState(ProcessState::READY);

                        if (m_schedulerType == "fcfs") {
                            m_fcfsScheduler.addProcess(batchProc);
                        }
                        else if (m_schedulerType == "rr") {
                            m_rrScheduler.addProcess(batchProc);
                        }
                    }
                    else {
                        batchProc->setState(ProcessState::WAITING); // Queue for later
                    }
                }
            }
        }

        bool activeWorkDone = false;

        // Scope lock for core pipeline operations
        {
            std::lock_guard<std::mutex> lock(m_schedulerMutex);
            std::lock_guard<std::mutex> memLock(m_memoryMutex);

            // NEW: Scan for waiting processes and try to allocate them now that memory might be free
            for (auto& p : m_allTrackedProcesses) {
                if (p->getState() == ProcessState::WAITING) {
                    if (m_memoryManager && m_memoryManager->allocate(p->getName())) {
                        p->setState(ProcessState::READY);
                        if (m_schedulerType == "fcfs") m_fcfsScheduler.addProcess(p);
                        else if (m_schedulerType == "rr") m_rrScheduler.addProcess(p);
                    }
                }
            }

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

                    // 2. Fetch process pointer BEFORE execution to ensure we don't lose it
                    auto process = cpu.getCurrentProcess();

                    // 3. Execute and Check
                    if (process) {
                        activeWorkDone = true;
                        cpu.executeCycle();

                        if (process->isFinished()) {
                            if (m_memoryManager) {
                                m_memoryManager->deallocate(process->getName());
                            }
                            process->setState(ProcessState::FINISHED);
                            cpu.assignProcess(nullptr);
                        }
                    }

                    // 4. Backfill core immediately
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

                    // 2. Fetch process pointer BEFORE execution
                    auto process = cpu.getCurrentProcess();

                    // 3. Execute and Check
                    if (process) {
                        activeWorkDone = true;
                        cpu.executeCycle();

                        if (process->isFinished()) {
                            if (m_memoryManager) {
                                m_memoryManager->deallocate(process->getName());
                            }
                            process->setState(ProcessState::FINISHED);
                            cpu.assignProcess(nullptr);
                            cpu.resetCyclesExecuted();
                        }
                        else if (cpu.getCyclesExecuted() >= m_rrScheduler.getQuantum()) {
                            process->setState(ProcessState::READY);
                            m_rrScheduler.addProcess(process);

                            static uint32_t realQuantumCycle = 0;
                            realQuantumCycle += m_rrScheduler.getQuantum(); 

                            if (m_memoryManager) {
                                // Pass our clean counter instead of m_cpuCycles
                                m_memoryManager->generateSnapshot(realQuantumCycle);
                            }

                            cpu.assignProcess(nullptr);
                            cpu.resetCyclesExecuted();
                        }
                    }

                    // 4. Backfill core immediately
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