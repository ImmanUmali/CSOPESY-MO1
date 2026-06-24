#include "FCFS.h"

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_readyQueue.push(process);
}

std::shared_ptr<Process> FCFSScheduler::getNextProcess() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_readyQueue.empty())
        return nullptr;

    auto process = m_readyQueue.front();
    m_readyQueue.pop();
    return process;
}

bool FCFSScheduler::hasProcess() const {
    return !m_readyQueue.empty();
}