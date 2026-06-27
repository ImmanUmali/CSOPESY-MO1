#pragma once
#include "ICommand.h"
#include "ISystemContext.h"
#include "ConsoleShell.h"
#include <iostream>
#include <fstream>
#include <iomanip>

class ConsoleShell;

// Milestone 6

// Helper structure to print exactly the dashboard to any output stream
inline void GenerateReportStream(std::ostream& out, ConsoleShell& shell) {
    auto sched = shell.getScheduler();
    if (!sched) return;

    auto cores = sched->getCores();
    auto trackingList = sched->getAllTrackedProcesses();

    size_t totalCores = cores.size();
    size_t coresUsed = 0;
    for (const auto& core : cores) {
        if (!core.isIdle()) coresUsed++;
    }

    double utilization = (totalCores > 0) ? ((double)coresUsed / totalCores) * 100.0 : 0.0;

    out << "---------------------------------------------------------\n";
    out << "CSOPESY Emulator Dashboard Context State Log\n";
    out << "---------------------------------------------------------\n";
    out << "CPU utilization: " << std::fixed << std::setprecision(0) << utilization << "%\n";
    out << "Cores used: " << coresUsed << "\n";
    out << "Cores available: " << (totalCores - coresUsed) << "\n";
    out << "---------------------------------------------------------\n\n";

    out << "Running processes:\n";
    for (const auto& proc : trackingList) {
        if (!proc->isFinished()) {
            // Display: process_name (timestamp) Core: ID Current_Line / Total_Lines
            out << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getTimestamp() << ")    "
                << "Progress: " << proc->getCommandCounter() << " / " << proc->getLinesOfCode() << "\n";
        }
    }

    out << "\nFinished processes:\n";
    for (const auto& proc : trackingList) {
        if (proc->isFinished()) {
            out << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getTimestamp() << ")    "
                << "Finished    " << proc->getLinesOfCode() << " / " << proc->getLinesOfCode() << "\n";
        }
    }
    out << "---------------------------------------------------------\n";
}

class ScreenCommand : public ICommand {
public:
    std::string getName() const override { return "screen"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        // 1. Guard against no flags passed
        if (args.empty()) {
            std::cout << "Usage: screen -s <name> | screen -r <name> | screen -ls\n" << std::endl;
            return;
        }

        std::string flag = args[0];
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);

        // 2. Handle 'screen -ls' (Only requires 1 argument)
        if (flag == "-ls") {
            GenerateReportStream(std::cout, shell);
            return;
        }

        // 3. Guard for -s and -r which require a process name (2 arguments)
        if (args.size() < 2) {
            std::cout << "Usage: screen " << flag << " <process_name>\n" << std::endl;
            return;
        }

        std::string processName = args[1];

        // 4. Handle 'screen -s' (Create/Spawn and attach)
        if (flag == "-s") {
            if (shell.findProcess(processName) != nullptr) {
                std::cout << "Error: Process with name '" << processName << "' already exists.\n" << std::endl;
                return;
            }

            SystemConfig cfg = shell.getConfig();
            int newPid = shell.generateNextPid();
            
            // 1. Create the process as a shared_ptr so both ConsoleShell and Scheduler can track it
            auto newProc = std::make_shared<Process>(newPid, processName, cfg.minIns, cfg.maxIns);

            // 2. Register it to your ConsoleShell's internal history tracker
            // (Note: If your addProcess takes unique_ptr, see the adjustment step below)
            shell.addProcess(newProc); 
            
            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);

            // 3. SEND IT TO THE SCHEDULER LAYER SO CORES CAN ACTUALLY EXECUTE IT!
            auto sched = shell.getScheduler();
            if (sched) {
                sched->addProcess(newProc);
            }

            std::cout << "Attached to new process screen: " << processName << std::endl;
        }
        
        // 5. Handle 'screen -r' (Re-attach to existing)
        else if (flag == "-r") {
            if (shell.findProcess(processName) == nullptr) {
                std::cout << "Error: Process '" << processName << "' not found.\n" << std::endl;
                return;
            }

            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);
            std::cout << "Re-attached to process screen: " << processName << std::endl;
        }
        else {
            std::cout << "Invalid flag. Use -s to start, -r to re-attach, or -ls to list dashboard.\n" << std::endl;
        }
    }
};

// TODO: Fix implmentation of process-smi later in accordance to the specs
class ProcessSmiCommand : public ICommand {
public:
    std::string getName() const override { return "process-smi"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        if (context.getCurrentView() != TerminalView::SCREEN_MULTIPLEXER) {
            std::cout << "Error: 'process-smi' can only be executed inside an attached process screen.\n" << std::endl;
            return;
        }

        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        Process* proc = shell.findProcess(shell.getAttachedProcess());

        if (!proc) {
            std::cout << "Error: Attached process references a null context state.\n" << std::endl;
            return;
        }

        // --- Specs Compliant Output Format with Line Counters ---
        std::cout << "Process name: " << proc->getName() << "\n";
        std::cout << "ID: " << proc->getPid() << "\n";

        // Added instruction counters right below the ID:
        std::cout << "Current Line: " << proc->getCommandCounter() << "\n";
        std::cout << "Total Lines: " << proc->getLinesOfCode() << "\n";

        std::cout << "Logs:\n";
        for (const auto& log : proc->getLogs()) {
            std::cout << log << "\n";
        }

        // Clean line break before optional state messages
        std::cout << "\n";

        if (proc->isFinished()) {
            std::cout << "Finished!\n\n";
        }
    }
};
// Milestone 5

class SchedulerStartCommand : public ICommand {
public:
    std::string getName() const override { return "scheduler-start"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        auto sched = shell.getScheduler();
        
        if (!sched) {
            std::cout << "Error: Engine Scheduler context layer uninstantiated.\n" << std::endl;
            return;
        }

        SystemConfig cfg = shell.getConfig();
        // Seed parameters dynamically from Milestone 2 Configuration Loader structure
        sched->setGenerationParameters(cfg.batchProcessFreq, cfg.minIns, cfg.maxIns, shell.generateNextPid() - 1);
        sched->startGeneration();
        
        std::cout << "Automated batch job scheduler engine successfully STARTED.\n" << std::endl;
    }
};

class SchedulerStopCommand : public ICommand {
public:
    std::string getName() const override { return "scheduler-stop"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        auto sched = shell.getScheduler();
        
        if (sched) {
            sched->stopGeneration();
            std::cout << "Automated batch job scheduler engine successfully STOPPED.\n" << std::endl;
        }
    }
};

class ScreenLsCommand : public ICommand {
public:
    std::string getName() const override { return "screen"; } 
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        
        // Intercepting 'screen -ls' variants directly
        if (!args.empty() && args[0] == "-ls") {
            GenerateReportStream(std::cout, shell);
            return;
        }
        
        std::cout << "Usage: screen -ls  (To display system performance dashboards)\n" << std::endl;
    }
};

class ReportUtilCommand : public ICommand {
public:
    std::string getName() const override { return "report-util"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);
        
        std::ofstream logFile("csopesy-log.txt", std::ios::trunc);
        if (!logFile.is_open()) {
            std::cout << "Error: Unresolved IO access exceptions generating 'csopesy-log.txt'.\n" << std::endl;
            return;
        }

        GenerateReportStream(logFile, shell);
        logFile.close();

        std::cout << "Report successfully snapshot/generated at csopesy-log.txt!\n" << std::endl;
    }
};