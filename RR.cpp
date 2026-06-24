#include "RR.h"

RoundRobinScheduler::RoundRobinScheduler(unsigned int quantum): m_quantum(quantum) {
}

void RoundRobinScheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_readyQueue.push(process);
}

std::shared_ptr<Process> RoundRobinScheduler::getNextProcess() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_readyQueue.empty())
        return nullptr;

    auto process = m_readyQueue.front();
    m_readyQueue.pop();

    return process;
}

bool RoundRobinScheduler::hasProcess() const {
    return !m_readyQueue.empty();
}

unsigned int RoundRobinScheduler::getQuantum() const {
    return m_quantum;
}