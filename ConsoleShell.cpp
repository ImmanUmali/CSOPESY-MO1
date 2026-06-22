#include "ConsoleShell.h"
#include "CoreCommands.h"
#include <iostream>
#include <sstream>

ConsoleShell::ConsoleShell()
    : m_initialized(false),
    m_exitFlag(false),
    m_currentView(TerminalView::MAIN_MENU),
    m_attachedProcessName("") {
    setupCommands();
}

void ConsoleShell::registerCommand(CommandPtr command) {
    if (command) {
        m_commandRegistry[command->getName()] = std::move(command);
    }
}

void ConsoleShell::setupCommands() {
    // Register the two core functional pieces for Milestone 1
    registerCommand(std::make_unique<ExitCommand>());
    registerCommand(std::make_unique<InitializeCommand>());
}

std::vector<std::string> ConsoleShell::tokenizeInput(const std::string& rawInput) {
    std::vector<std::string> tokens;
    std::stringstream ss(rawInput);
    std::string token;

    // Split input line by blank spaces
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void ConsoleShell::setConfig(const SystemConfig& config) {
    m_systemConfig = config;
}

SystemConfig ConsoleShell::getConfig() const {
    return m_systemConfig;
}

void ConsoleShell::setAttachedProcess(const std::string& processName) {
    m_attachedProcessName = processName;
}

std::string ConsoleShell::getAttachedProcess() const {
    return m_attachedProcessName;
}

void ConsoleShell::run() {
    std::cout << "  /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$  /$$$$$$$$  /$$$$$$  /$$     /$$\n";
    std::cout << " /$$__  $$ /$$__  $$ /$$__  $$| $$__  $$| $$_____/ /$$__  $$|  $$   /$$/\n";
    std::cout << "| $$  \\__/| $$  \\__/| $$  \\ $$| $$  \\ $$| $$      | $$  \\__/ \\  $$ /$$/ \n";
    std::cout << "| $$      |  $$$$$$ | $$  | $$| $$$$$$$/| $$$$$   |  $$$$$$   \\  $$$$/  \n";
    std::cout << "| $$       \\____  $$| $$  | $$| $$____/ | $$__/    \\____  $$   \\  $$/   \n";
    std::cout << "| $$    $$ /$$  \\ $$| $$  | $$| $$      | $$       /$$  \\ $$    | $$    \n";
    std::cout << "|  $$$$$$/|  $$$$$$/|  $$$$$$/| $$      | $$$$$$$$|  $$$$$$/    | $$    \n";
    std::cout << " \\______/  \\______/  \______/ |__/      |________/ \\______/     |__/    \n";
                                                                        
                                                                        
                                                                        
    std::cout << "Welcome to CSOPESY Emulator!\n";
    std::cout << "Developers:\n     Lazaro, Heisel Janine C. \n     Tria, Chynna Mae Z. \n     Umali, Immanuel Z. \n";
    std::cout << "Last updated: 06-21-2026\n";
    std::cout << "___________________________________________\n\n";


    std::string rawInputLine;

   
    while (!shouldExit()) {
        std::cout << "root:\\> ";

        if (!std::getline(std::cin, rawInputLine)) {
            break; 
        }

        std::vector<std::string> tokens = tokenizeInput(rawInputLine);

		// user did not enter any text, just hit enter
        if (tokens.empty()) {
            continue;
        }

        std::string commandToken = tokens[0];

        // Extract tokenized command
        std::vector<std::string> commandArgs(tokens.begin() + 1, tokens.end());

        // find command
        auto it = m_commandRegistry.find(commandToken);
        if (it != m_commandRegistry.end()) {
            ICommand* commandToExecute = it->second.get();

            
            if (!isInitialized() && !commandToExecute->isBypassingInitialization()) {
                std::cout << "Error: You must run the \"initialize\" command before performing this action.\n" << std::endl;
            }
            else {
                commandToExecute->execute(*this, commandArgs);
            }
        }
        else {
            // Unrecognized
            if (!isInitialized()) {
                std::cout << "Error: Command not recognized. System requires \"initialize\" first.\n" << std::endl;
            }
            else {
                std::cout << "Error: Command \"" << commandToken << "\" not found.\n" << std::endl;
            }
        }
    }
}