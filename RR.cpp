#include "RoundRobinScheduler.h"

RoundRobinScheduler::RoundRobinScheduler(unsigned int quantum): m_quantum(quantum) {
}

void RoundRobinScheduler::addProcess(std::shared_ptr<Process> process) {
    m_readyQueue.push(process);
}

std::shared_ptr<Process> RoundRobinScheduler::getNextProcess() {
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