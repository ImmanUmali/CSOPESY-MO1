#pragma once

#include <queue>
#include <memory>

class Process;

class FCFSScheduler {
private:
    std::queue<std::shared_ptr<Process>> m_readyQueue;

public:
    void addProcess(std::shared_ptr<Process> process);

    std::shared_ptr<Process> getNextProcess();

    bool hasProcess() const;
};