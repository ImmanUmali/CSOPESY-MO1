#pragma once

#include <queue>
#include <memory>

class Process;

class RoundRobinScheduler {
private:
    std::queue<std::shared_ptr<Process>> m_readyQueue;
    unsigned int m_quantum;

public:
    RoundRobinScheduler(unsigned int quantum);

    void addProcess(std::shared_ptr<Process> process);

    std::shared_ptr<Process> getNextProcess();

    bool hasProcess() const;

    unsigned int getQuantum() const;
};