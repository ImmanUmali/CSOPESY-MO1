#pragma once
#include "ICommand.h"
#include "ISystemContext.h"
#include "ConsoleShell.h"
#include <iostream>
#include <fstream>
#include <iomanip>

class ConsoleShell;

void ClearTerminal() {
#if defined(_WIN32) || defined(_WIN64)
    std::system("cls");
#else
    std::system("clear");
#endif
}

// Helper structure to print log
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
    out << "CSOPESY Emulator Log\n";
    out << "---------------------------------------------------------\n";
    out << "CPU utilization: " << std::fixed << std::setprecision(0) << utilization << "%\n";
    out << "Cores used: " << coresUsed << "\n";
    out << "Cores available: " << (totalCores - coresUsed) << "\n";
    out << "---------------------------------------------------------\n\n";

    // Display process status if running
    out << "Running processes:\n";
    for (const auto& proc : trackingList) {
        if (!proc->isFinished()) {
            std::string coreDisplay = "Core: N/A"; // If a process is READY but not assigned to a core yet
            for (const auto& core : cores) {
                if (!core.isIdle() && core.getCurrentProcess() && core.getCurrentProcess()->getPid() == proc->getPid()) {
                    coreDisplay = "Core # " + std::to_string(core.getId());
                    break;
                }
            }

            out << std::left << std::setw(12) << proc->getName()
                << " (" << proc->getTimestamp() << ")    "
                << std::left << std::setw(12) << coreDisplay
                << proc->getCommandCounter() << " / " << proc->getLinesOfCode() << "\n";
        }
    }

    // Display process status if finished
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
        if (args.empty()) {
            std::cout << "Usage: screen -s <process_name> | screen -r <process_name> | screen -ls\n" << std::endl;
            return;
        }

        std::string flag = args[0];
        ConsoleShell& shell = static_cast<ConsoleShell&>(context);

        // If input is screen -ls
        if (flag == "-ls") {
            GenerateReportStream(std::cout, shell);
            return;
        }

        // 3. Guard for -s and -r which require a process name 
        if (args.size() < 2) {
            std::cout << "Usage: screen " << flag << " <process_name>\n" << std::endl;
            return;
        }

        std::string processName = args[1];

       // If input is screen -s
        if (flag == "-s") {
            if (shell.findProcess(processName) != nullptr) {
                std::cout << "Error: Process with name '" << processName << "' already exists.\n" << std::endl;
                return;
            }

            SystemConfig cfg = shell.getConfig();
            int newPid = shell.generateNextPid();
            
            auto newProc = std::make_shared<Process>(newPid, processName, cfg.minIns, cfg.maxIns);

            shell.addProcess(newProc); 
            
            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);

            ClearTerminal();

            auto sched = shell.getScheduler();
            if (sched) {
                sched->addProcess(newProc);
            }

            std::cout << "Attached to new process screen: " << processName << std::endl;
        }
        
        // If input is screen -r
        else if (flag == "-r") {
            Process* existingProc = shell.findProcess(processName);
            
            if (existingProc == nullptr) {
                std::cout << "Error: Process '" << processName << "' not found.\n" << std::endl;
                return;
            }

            if (existingProc->isFinished()) {
                std::cout << "Error: Process '" << processName << "' has already finished execution. Cannot re-attach.\n" << std::endl;
                return;
            }

            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);

            ClearTerminal();

            std::cout << "Re-attached to process screen: " << processName << std::endl;
        }
        else {
            std::cout << "Invalid flag. Use -s to start, -r to re-attach, or -ls to list dashboard.\n" << std::endl;
        }
    }
};

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

        std::cout << "Process name: " << proc->getName() << "\n";
        std::cout << "ID: " << proc->getPid() << "\n";

        std::cout << "Current Line: " << proc->getCommandCounter() << "\n";
        std::cout << "Total Lines: " << proc->getLinesOfCode() << "\n";

        std::cout << "Logs:\n";
        for (const auto& log : proc->getLogs()) {
            std::cout << log << "\n";
        }

        std::cout << "\n";

        if (proc->isFinished()) {
            std::cout << "Finished!\n\n";
        }
    }
};


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

        std::cout << "Report generated at csopesy-log.txt!\n" << std::endl;
    }
};