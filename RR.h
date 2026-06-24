#pragma once

#include <queue>
#include <memory>
#include <mutex>

class Process;

class RoundRobinScheduler {
private:
    std::queue<std::shared_ptr<Process>> m_readyQueue;
    unsigned int m_quantum;
	std::mutex m_mutex;

public:
    RoundRobinScheduler(unsigned int quantum);

    void addProcess(std::shared_ptr<Process> process);

    std::shared_ptr<Process> getNextProcess();

    bool hasProcess() const;

    unsigned int getQuantum() const;
};