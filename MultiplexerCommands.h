#pragma once
#include "ICommand.h"
#include "ISystemContext.h"
#include "ConsoleShell.h"
#include <iostream>

class ScreenCommand : public ICommand {
public:
    std::string getName() const override { return "screen"; }
    bool isBypassingInitialization() const override { return false; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        if (args.size() < 2) {
            std::cout << "Usage: screen -s <process_name>  OR  screen -r <process_name>\n" << std::endl;
            return;
        }

        std::string flag = args[0];
        std::string processName = args[1];

        ConsoleShell& shell = static_cast<ConsoleShell&>(context);

        if (flag == "-s") {
            // Check if process name already exists to prevent duplicate processes
            if (shell.findProcess(processName) != nullptr) {
                std::cout << "Error: Process with name '" << processName << "' already exists.\n" << std::endl;
                return;
            }

            // Create a brand new process using the min/max bounds parsed in Milestone 2
            SystemConfig cfg = shell.getConfig();
            int newPid = shell.generateNextPid();
            auto newProc = std::make_unique<Process>(newPid, processName, cfg.minIns, cfg.maxIns);

            shell.addProcess(std::move(newProc));
            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);

            std::cout << "Attached to new process screen: " << processName << std::endl;
        }
        else if (flag == "-r") {
            // Attempt to re-attach to an existing process
            if (shell.findProcess(processName) == nullptr) {
                std::cout << "Error: Process '" << processName << "' not found.\n" << std::endl;
                return;
            }

            shell.setAttachedProcess(processName);
            shell.changeView(TerminalView::SCREEN_MULTIPLEXER);
            std::cout << "Re-attached to process screen: " << processName << std::endl;
        }
        else {
            std::cout << "Invalid flag. Use -s to start or -r to re-attach.\n" << std::endl;
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

        std::cout << "-------------------------------------------\n";
        std::cout << " Process Name : " << proc->getName() << "\n";
        std::cout << " ID           : " << proc->getPid() << "\n";
        std::cout << " Created At   : " << proc->getTimestamp() << "\n";

        if (proc->isFinished()) {
            std::cout << " Status       : Finished!\n";
        }
        else {
            std::cout << " Current Line : " << proc->getCommandCounter() << "\n";
            std::cout << " Total Lines  : " << proc->getLinesOfCode() << "\n";
        }
        std::cout << "-------------------------------------------\n";
        std::cout << " Logs:\n";
        for (const auto& log : proc->getLogs()) {
            std::cout << "  > " << log << "\n";
        }
        std::cout << "-------------------------------------------\n" << std::endl;
    }
};