#pragma once

#include "ISystemContext.h"
#include "ICommand.h"
#include "ConfigStructure.h"
#include "Process.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

class Scheduler;

class ConsoleShell : public ISystemContext {
private:
    bool m_initialized;
    bool m_exitFlag;
    TerminalView m_currentView;
    SystemConfig m_systemConfig;

    std::string m_attachedProcessName;
    int m_pidCounter;

    std::unordered_map<std::string, CommandPtr> m_commandRegistry;

    // std::vector<std::unique_ptr<Process>> m_processList;
    std::vector<std::shared_ptr<Process>> m_processList;

    std::shared_ptr<Scheduler> m_scheduler;

    std::vector<std::string> tokenizeInput(const std::string& rawInput);
    void registerCommand(CommandPtr command);
    void setupCommands();

public:
    ConsoleShell();
    ~ConsoleShell() override = default;

    void printMainMenu() const;

    bool isInitialized() const override { return m_initialized; }
    void setInitialized(bool value) override { m_initialized = value; }
    bool shouldExit() const override { return m_exitFlag; }
    void flagExit() override { m_exitFlag = true; }
    TerminalView getCurrentView() const override { return m_currentView; }
    void changeView(TerminalView newView) override { m_currentView = newView; }

    void setConfig(const SystemConfig& config) override;
    SystemConfig getConfig() const override;

    void setAttachedProcess(const std::string& processName) override;
    std::string getAttachedProcess() const override;

    int generateNextPid() { return ++m_pidCounter; }
    // void addProcess(std::unique_ptr<Process> proc) { m_processList.push_back(std::move(proc)); }
    void addProcess(std::shared_ptr<Process> proc) { m_processList.push_back(proc); }

    Process* findProcess(const std::string& name);

    void run();

    std::shared_ptr<Scheduler> getScheduler() override { return m_scheduler; }
    void setScheduler(std::shared_ptr<Scheduler> scheduler) override { m_scheduler = scheduler; }
};
