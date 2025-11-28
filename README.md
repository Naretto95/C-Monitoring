# C-Monitoring

This project is a simulation of a fault-tolerant distributed computing system written in C, designed to demonstrate monitoring and recovery in a networked environment. It divides a computational task (calculating the sum of integers from 0 to 1000) among multiple "calculator" processes running as TCP clients, while a central "monitor" server tracks their progress, handles failures, and reports system status.

## Getting Started

To compile and run the project on a Linux system, simply run the following commands:
```
make
./bin/executable
```

The program will prompt for the number of sub-tasks/processes. Press Enter to launch calculator clients interactively. The "evil monkey" will periodically kill calculator threads to simulate failures.

If you wish to run the project on a Windows system, you will need to modify the Makefile and re-import the necessary network packages in the code.

## Features

- Distributed computation simulation with TCP-based client-server architecture
- Fault tolerance through task reassignment when calculators fail
- Real-time monitoring of computational progress, network communication, and system performance
- Evil monkey failure simulator to test resilience
- Periodic status reporting (partial sums, processing times, task statuses)
- Customizable number of processes and failure simulation frequency

## Architecture

- **Launcher**: Initializes monitor, evil monkey, and calculator threads
- **Monitor**: TCP server managing sub-tasks and client connections
- **Calculator**: TCP clients performing incremental sum calculations
- **Process Manager**: Assigns tasks to incoming calculators
- **Report System**: Provides console-based system overviews
- **Evil Monkey**: Simulates random failures by killing calculator threads

## Built With

- C programming language
- POSIX threads (pthread)
- TCP sockets for network communication

