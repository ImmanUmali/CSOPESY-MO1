#ifndef ICOMMAND_H
#define ICOMMAND_H

#include <string>
#include <vector>
#include <memory>

// Forward declaration of the system context to prevent circular dependencies
class ISystemContext;

class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Executes the specific logic of the command.
     * @param context Interface to the underlying scheduler, state tracking, and view switcher.
     * @param args A vector of string arguments passed after the command token.
     */
    virtual void execute(ISystemContext& context, const std::vector<std::string>& args) = 0;

    /**
     * @brief Returns the primary token string (e.g., "initialize", "exit", "screen").
     */
    virtual std::string getName() const = 0;

    /**
     * @brief True if this command is allowed to run before 'initialize' is called.
     */
    virtual bool isBypassingInitialization() const = 0;
};

using CommandPtr = std::unique_ptr<ICommand>;

#endif // ICOMMAND_H