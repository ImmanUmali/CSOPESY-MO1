#pragma once

#include <string>
#include <vector>
#include <memory>

class ISystemContext;

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual void execute(ISystemContext& context, const std::vector<std::string>& args) = 0;
    virtual std::string getName() const = 0;
    virtual bool isBypassingInitialization() const = 0;
};

using CommandPtr = std::unique_ptr<ICommand>;
