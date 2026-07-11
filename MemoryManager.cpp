#include "MemoryManager.h"
#include <algorithm>

MemoryManager::MemoryManager(uint32_t maxOverallMem, uint32_t memPerProc)
    : m_maxOverallMem(maxOverallMem), m_memPerProc(memPerProc) {
    // Initially, the system starts with one giant free block spanning all memory
    m_blocks.push_back({ 0, m_maxOverallMem, false, "" });
}

bool MemoryManager::allocate(const std::string& processName) {
    // 1. Search sequentially from the beginning for the first free block that fits
    for (size_t i = 0; i < m_blocks.size(); ++i) {
        if (!m_blocks[i].isAllocated && m_blocks[i].size >= m_memPerProc) {

            // 2. If the block is exactly the right size, just allocate it
            if (m_blocks[i].size == m_memPerProc) {
                m_blocks[i].isAllocated = true;
                m_blocks[i].assignedProcessName = processName;
            }
            // 3. If it's larger, split the block into an allocated part and a free remainder
            else {
                uint32_t originalSize = m_blocks[i].size;
                uint32_t originalStart = m_blocks[i].startAddress;

                // Adjust current block to become the allocated partition
                m_blocks[i].size = m_memPerProc;
                m_blocks[i].isAllocated = true;
                m_blocks[i].assignedProcessName = processName;

                // Insert a new unallocated trailing block representing the leftover gap
                MemoryBlock leftoverBlock;
                leftoverBlock.startAddress = originalStart + m_memPerProc;
                leftoverBlock.size = originalSize - m_memPerProc;
                leftoverBlock.isAllocated = false;
                leftoverBlock.assignedProcessName = "";

                m_blocks.insert(m_blocks.begin() + i + 1, leftoverBlock);
            }
            return true; // Successfully placed in memory!
        }
    }
    return false; // Insufficient continuous space available (Memory Full)
}

void MemoryManager::deallocate(const std::string& processName) {
    // 1. Locate the process block and mark it as free
    for (auto& block : m_blocks) {
        if (block.isAllocated && block.assignedProcessName == processName) {
            block.isAllocated = false;
            block.assignedProcessName = "";
            break;
        }
    }

    // 2. Coalescing step: Merge adjacent unallocated blocks to eliminate fake fragmentation
    for (size_t i = 0; i < m_blocks.size() - 1; ) {
        if (!m_blocks[i].isAllocated && !m_blocks[i + 1].isAllocated) {
            m_blocks[i].size += m_blocks[i + 1].size; // Absorbs trailing space
            m_blocks.erase(m_blocks.begin() + i + 1); // Delete the redundant block entry
            // Do not increment 'i' so we can check if the newly expanded block can merge further
        }
        else {
            ++i;
        }
    }
}

size_t MemoryManager::getNumProcessesInMemory() const {
    size_t count = 0;
    for (const auto& block : m_blocks) {
        if (block.isAllocated) count++;
    }
    return count;
}

uint32_t MemoryManager::calculateExternalFragmentation() const {
    uint32_t totalFreeBytes = 0;
    
    // Sum up ALL unallocated memory space
    for (const auto& block : m_blocks) {
        if (!block.isAllocated) {
            totalFreeBytes += block.size;
        }
    }
    
    // Convert bytes to KB (e.g., 8192 bytes / 1024 = 8 KB)
    return totalFreeBytes / 1024;
}

// Add this to the very bottom of memorymanager.cpp

void MemoryManager::generateSnapshot(uint32_t currentQuantum) const {
    // 1. Generate filename: memory_stamp_<qq>.txt
    std::string filename = "memory_stamp_" + std::to_string(currentQuantum) + ".txt";
    std::ofstream outFile(filename);
    if (!outFile.is_open()) return;

    // 2. Get formatted timestamp: (MM/DD/YYYY HH:MM:SSAM/PM)
    std::time_t now = std::time(nullptr);
    std::tm* ltm = std::localtime(&now);
    char timeBuffer[40];
    
    // Determine AM/PM
    const char* ampm = (ltm->tm_hour >= 12) ? "PM" : "AM";
    int hour12 = ltm->tm_hour % 12;
    if (hour12 == 0) hour12 = 12;

    std::snprintf(timeBuffer, sizeof(timeBuffer), "(%02d/%02d/%04d %02d:%02d:%02d%s)",
                 ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_year + 1900,
                 hour12, ltm->tm_min, ltm->tm_sec, ampm);

    // 3. Gather stats
    size_t activeProcesses = getNumProcessesInMemory();
    uint32_t fragBytes = calculateExternalFragmentation();

    // 4. Print Header
    outFile << "Timestamp: " << timeBuffer << "\n";
    outFile << "Number of processes in memory: " << activeProcesses << "\n";
    outFile << "Total external fragmentation in KB: " << fragBytes << "\n\n";

    // 5. Inverted ASCII Printout (From end address 16384 down to 0)
    outFile << "----end---- = " << m_maxOverallMem << "\n\n";

    // Loop backward to match mockup layout (high addresses on top)
    for (auto it = m_blocks.rbegin(); it != m_blocks.rend(); ++it) {
        uint32_t upperLimit = it->startAddress + it->size;
        uint32_t lowerLimit = it->startAddress;

        outFile << upperLimit << "\n";
        if (it->isAllocated) {
            // Convert process name to uppercase if desired to match "P1", "P9"
            std::string name = it->assignedProcessName;
            for (auto& c : name) c = std::toupper(c);
            outFile << name << "\n";
        } else {
            outFile << "Free Space\n"; // Or leave a blank line depending on preference
        }
        outFile << lowerLimit << "\n\n";
    }

    outFile << "----start----- = 0\n";
    outFile.close();
}