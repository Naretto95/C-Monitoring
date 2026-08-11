# C-Monitoring

A simulation of a fault-tolerant distributed computing system, written in C
with POSIX threads and TCP sockets. A **monitor** splits the sum of
integers `0..1000` into sub-tasks and hands them out over TCP to
**calculator** clients; an **evil monkey** thread periodically kills a
calculator's connection to simulate a crash, and the monitor detects the
silence and reassigns the unfinished part of that task to the next
calculator that connects.

Calculators run as threads within the same process as the monitor and talk
to it over `127.0.0.1` — it's a simulation of a distributed system's
failure/recovery behavior, not an actual multi-machine deployment.

## Getting Started

Built and tested on Linux (uses POSIX threads and BSD sockets).

```bash
make
./bin/executable
```

You'll be prompted for the number of tasks to split the sum into. After
that, press **Enter** to launch a calculator; each Enter press starts one
more. The evil monkey starts attacking in the background immediately and
will kill a random calculator's reporting connection every 20 seconds.
Once every task has been completed by some calculator, the monitor prints
a final report and the program exits (or press Ctrl+D on an empty line to
stop launching calculators early).

To build on Windows you'd need to replace the BSD sockets calls with
Winsock and adjust the Makefile accordingly — not done here.

## How it works

1. The monitor asks for a task count `N`, opens a listening TCP socket,
   and divides `[0, 1000]` into `N` contiguous ranges, one per task slot.
2. Each time you press Enter, the launcher spawns a calculator thread that
   connects to the monitor and receives the range of a free task slot.
3. The calculator sums its range at one integer per second and reports
   its running total back every 2 seconds.
4. If a task slot stops receiving reports (its calculator was killed, or
   never connects), the monitor puts it back up for reassignment — the
   next calculator to connect picks it up **from where the last one left
   off**, not from scratch.
5. Once every task slot reports "done", the monitor prints the final sum,
   per-task partial sums, timings and statuses, and the program exits.

## Features

- TCP client/server task distribution with automatic reassignment on
  calculator failure
- Mutex-protected shared state (global sum, task table, calculator
  bookkeeping) — safe under compiler optimization, not just by luck
- Evil monkey failure injector with a configurable kill frequency
- Periodic console status reports (partial sums, elapsed time, task state)
- Bounded, validated user input; no hardcoded process/task limits beyond a
  configurable sanity cap

## Project layout

```
src/
  common.h / common.c        shared constants (port, limits) and an error helper
  structures.h                shared structs and the monitor <-> calculator wire messages
  monitor.h / monitor.c       TCP server: task table, process manager & report threads
  thread_functions.h / .c     monitor-side worker threads (assignment, reporting, evil monkey)
  calculator.h / calculator.c TCP client: computes and reports a task's partial sum
  launcher.c                  entry point: starts the monitor, evil monkey, and calculators
```

## Known simplifications

- Calculators are simulated as threads of one process rather than separate
  machines/processes; this keeps the demo self-contained but means the
  "network" is loopback TCP, not a real distributed transport.
- Monitor/calculator messages are fixed-size structs sent with a single
  `send()`/`recv()` each, without length-prefixed framing. That's safe
  here because messages are small and spaced ~2s apart over loopback, but
  it isn't a general-purpose wire protocol.

## Built With

- C (C11)
- POSIX threads (pthread)
- BSD/POSIX sockets (TCP)
