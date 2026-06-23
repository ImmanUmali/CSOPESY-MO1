#include "FCFSScheduler.h"

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    m_readyQueue.push(process);
}

std::shared_ptr<Process> FCFSScheduler::getNextProcess() {
    if (m_readyQueue.empty())
        return nullptr;

    auto process = m_readyQueue.front();
    m_readyQueue.pop();

    return process;
}

bool FCFSScheduler::hasProcess() const {
    return !m_readyQueue.empty();
}