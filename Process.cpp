#include "Process.h"
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

// TODO: Address comments once Process.cpp has been finalized

// Local random generation state wrappers
static std::random_device rd;
static std::mt19937 gen(rd());


std::string getCurrentTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;

    // Formats matching standard system timestamps: YYYY-MM-DD HH:MM:SS
    // Note: If your compiler complains about localtime being unsafe, 
    // you can use localtime_s on Windows/MSVC or localtime_r on Linux.
    struct tm buf;
#if defined(_MSC_VER)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif

    ss << std::put_time(&buf, "%m/%d/%Y %I:%M:%S %p");
    return ss.str();
}

// Helper function to recursively build instructions up to the max nesting depth
Instruction generateRandomInstruction(int currentDepth) {
    std::uniform_int_distribution<int> opDist(0, 5); // Now covers 0 to 5 (including FOR)
    OpCode randomOp = static_cast<OpCode>(opDist(gen));

    // Force a non-FOR instruction if we have hit the maximum nested limit of 3
    if (randomOp == OpCode::FOR && currentDepth >= 3) {
        std::uniform_int_distribution<int> linearOpDist(0, 4); // Fallback to non-FOR
        randomOp = static_cast<OpCode>(linearOpDist(gen));
    }

    Instruction ins;
    ins.op = randomOp;

    if (ins.op == OpCode::FOR) {
        std::uniform_int_distribution<uint32_t> loopDist(2, 10); // Loop iterations counter
        ins.repeatCount = loopDist(gen);

        // Randomize how many lines of instructions exist inside this specific FOR block
        std::uniform_int_distribution<size_t> childLinesDist(1, 5);
        size_t linesInBlock = childLinesDist(gen);

        for (size_t i = 0; i < linesInBlock; ++i) {
            // Generate a nested child instruction, tracking the increased depth
            ins.childInstructions.push_back(generateRandomInstruction(currentDepth + 1));
        }
    }
    else {
        ins.repeatCount = 0;
    }

    return ins;
}

Process::Process(int pid, const std::string& name, uint32_t minIns, uint32_t maxIns)
    : m_pid(pid), m_name(name), m_state(ProcessState::READY), m_commandCounter(0) {

    m_timestamp = getCurrentTimestampString();

    std::uniform_int_distribution<size_t> dist(minIns, maxIns);
    m_linesOfCode = dist(gen);

    // Build the master instruction sequence
    for (size_t i = 0; i < m_linesOfCode; ++i) {
        m_instructions.push_back(generateRandomInstruction(1)); // Start at depth level 1
    }

    // Default configuration log requirement matching project parameters
    m_logs.push_back("Hello world from " + m_name + "!"); // 
}

void Process::setState(ProcessState state) {
    m_state = state;
}

void Process::addLog(const std::string& message) {
    m_logs.push_back(message);
}

void Process::executeNextLine() {
    if (isFinished())
        return;

    m_state = ProcessState::RUNNING;

    addLog(
        "Executed instruction #" +
        std::to_string(m_commandCounter + 1)
    );

    m_commandCounter++;

    if (m_commandCounter >= m_linesOfCode)
    {
        m_state = ProcessState::FINISHED;

        addLog("Process completed.");
    }
}