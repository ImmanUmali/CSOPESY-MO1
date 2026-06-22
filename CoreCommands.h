#pragma once


#include "ICommand.h"
#include "ISystemContext.h"
#include <iostream>

class ExitCommand : public ICommand {
public:
    std::string getName() const override { return "exit"; }
    bool isBypassingInitialization() const override { return true; } // 'exit' can run anytime 

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        std::cout << "Terminating CSOPESY Emulator console..." << std::endl;
        context.flagExit();
    }
};

class InitializeCommand : public ICommand {
public:
    std::string getName() const override { return "initialize"; }
    bool isBypassingInitialization() const override { return true; }

    void execute(ISystemContext& context, const std::vector<std::string>& args) override {
        if (context.isInitialized()) {
            std::cout << "Error: Processor configuration has already been initialized." << std::endl;
            return;
        }

        std::cout << "Attempting to read configuration from 'config.txt'..." << std::endl;

        // NOTE: File system verification and full parameter structural loading 
        // will be mapped out comprehensively in Milestone 2.

        context.setInitialized(true);
        std::cout << "System initialized successfully. All operational routines unlocked." << std::endl;
    }
};
