#pragma once

#include <memory>

class Process;

class CPUCore {
private:
    int m_id;
    unsigned int m_cyclesExecuted;
    std::shared_ptr<Process> m_currentProcess;

public:
    CPUCore(int id);

    bool isIdle() const;

    void assignProcess(std::shared_ptr<Process> process);

    void executeCycle();

    std::shared_ptr<Process> getCurrentProcess() const;

    void resetCyclesExecuted();

    unsigned int getCyclesExecuted() const;

    int getId() const;
};