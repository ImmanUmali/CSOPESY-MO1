# MO1 - Process Multiplexer and CLI
This project is an implementation of a Process Multiplexer and CLI in accordance to the CSOPESY MCO1 Project for Term 3 AY 2025-2026. 

### S09 Group 8:
- UMALI, Immanuel
- LAZARO, Heisel Janine
- TRIA, Chynna Mae
  
### To run the program:
- Step 0: Locate the `config.txt` file to modify the range of instruction length per process if needed.
- Step 1: Locate the `MO1.cpp` file (where the main function is located) and run using `Local Windows Debugger`.
- Step 2: Type `initialize` to start the processor configuration of the application.
- Step 3: Use commands as you see fit.

#### List of Commands available (Case Sensitive):
- **Main Console Commands**
1. `initialize` - initialize the processor configuration of the application
2. `exit` - exit the main console (terminates the console)
3. `screen -s` - adds one process
4. `screen -ls` - lists CPU utilization of all the processes
5. `screen -r` - reads one process
6. `scheduler-start` - continuously generates a batch of dummy processes for the CPU scheduler
7. `scheduler-stop` - stops generating dummy processes
8. `report-util` - generates CPU utilization report
- **Process Console Commands**
1. `process-smi` - prints a simple information about the process
2. `exit`- exit the process console
