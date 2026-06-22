#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "Instruction.h"

enum class ProcessState { READY, RUNNING, WAITING, FINISHED };

class Process {
private:
    int m_pid;
    std::string m_name;
    ProcessState m_state;
    size_t m_commandCounter;
    size_t m_linesOfCode;
    std::vector<Instruction> m_instructions;
    std::vector<std::string> m_logs;
    std::string m_timestamp;

public:
    Process(int pid, const std::string& name, uint32_t minIns, uint32_t maxIns);

    int getPid() const { return m_pid; }
    std::string getName() const { return m_name; }
    ProcessState getState() const { return m_state; }
    size_t getCommandCounter() const { return m_commandCounter; }
    size_t getLinesOfCode() const { return m_linesOfCode; }
    const std::vector<std::string>& getLogs() const { return m_logs; }
    std::string getTimestamp() const { return m_timestamp; }

    void addLog(const std::string& message);
    void executeNextLine(); // To be driven by the scheduler in Milestone 4
    bool isFinished() const { return m_commandCounter >= m_linesOfCode; }
};