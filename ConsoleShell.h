#pragma once


#include "ISystemContext.h"
#include "ICommand.h"
#include <unordered_map>
#include <vector>
#include <string>

class ConsoleShell : public ISystemContext {
private:
    bool m_initialized;
    bool m_exitFlag;
    TerminalView m_currentView;
    std::string m_attachedProcessName;

    // Registry database mapping text tokens to command behaviors
    std::unordered_map<std::string, CommandPtr> m_commandRegistry;

	// Tokenize input string into command and arguments
    std::vector<std::string> tokenizeInput(const std::string& rawInput);
    void registerCommand(CommandPtr command);
    void setupCommands();

public:
    ConsoleShell();
    ~ConsoleShell() override = default;

    // ISystemContext Interface Implementations
    bool isInitialized() const override { return m_initialized; }
    void setInitialized(bool value) override { m_initialized = value; }
    bool shouldExit() const override { return m_exitFlag; }
    void flagExit() override { m_exitFlag = true; }
    TerminalView getCurrentView() const override { return m_currentView; }
    void changeView(TerminalView newView) override { m_currentView = newView; }
    void setAttachedProcess(const std::string& processName) override { m_attachedProcessName = processName; }
    std::string getAttachedProcess() const override { return m_attachedProcessName; }

    
    void run();
};
