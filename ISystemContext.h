#pragma once

#include "ConfigStructure.h"
#include <string>

enum class TerminalView {
    MAIN_MENU,
    SCREEN_MULTIPLEXER
};

class ISystemContext {
public:
    virtual ~ISystemContext() = default;

    // Initialization Tracking
    virtual bool isInitialized() const = 0;
    virtual void setInitialized(bool value) = 0;

    // Lifecycle Flags
    virtual bool shouldExit() const = 0;
    virtual void flagExit() = 0;

    // View Management
    virtual TerminalView getCurrentView() const = 0;
    virtual void changeView(TerminalView newView) = 0;
    virtual void setAttachedProcess(const std::string& processName) = 0;
    virtual std::string getAttachedProcess() const = 0;

    virtual void setConfig(const SystemConfig& config) = 0;
    virtual SystemConfig getConfig() const = 0;
    virtual std::shared_ptr<Scheduler> getScheduler() = 0;
    virtual void setScheduler(std::shared_ptr<Scheduler> scheduler) = 0;
};
