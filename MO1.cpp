#include "ConsoleShell.h"

int main() {
    // Instantiate our core terminal shell engine context
    ConsoleShell emulatorShell;

    // Hand over control to the REPL execution sequence
    emulatorShell.run();

    return 0;
}