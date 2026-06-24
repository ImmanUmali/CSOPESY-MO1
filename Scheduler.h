#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include "CPUCore.h"
#include "FCFSScheduler.h"
#include "RoundRobinScheduler.h"

class Scheduler {
private:
    std::string m_schedulerType;
    std::vector<CPUCore> m_cpuCores;         // CPU Core Array owned here
    FCFSScheduler m_fcfsScheduler;           // Ready Queue owned here
    RoundRobinScheduler m_rrScheduler;       // Ready Queue owned here
    unsigned int m_delayPerExec;

    // Threading & Counters
    std::atomic<unsigned int> m_cpuCycles;   // Master CPU cycle counter
    std::atomic<bool> m_running;
    std::thread m_schedulerThread;           // The Scheduler Thread

    void threadLoop();                       // Background execution loop

public:
    Scheduler(const std::string& type, int numCpu, unsigned int quantum, unsigned int delayPerExec);
    ~Scheduler();

    void start();                            // Spins up the background thread
    void stop();                             // Safely stops the thread
    void addProcess(std::shared_ptr<Process> process);

    unsigned int getCpuCycles() const { return m_cpuCycles.load(); }
    const std::vector<CPUCore>& getCores() const { return m_cpuCores; }
};