#ifndef ISYSTEMCONTEXT_H
#define ISYSTEMCONTEXT_H

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
};

#endif // ISYSTEMCONTEXT_H