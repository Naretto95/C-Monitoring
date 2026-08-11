/*!
\file monitor.h
\brief Public entry point for the monitor thread.
*/

#ifndef MONITOR_H
#define MONITOR_H

/*!
Runs the monitor: prompts for the number of tasks, opens the TCP
listening socket, spawns the process manager and report threads, then
blocks until the whole computation is complete.
\param arg a monitor_init_param*, see structures.h
*/
void *monitor(void *arg);

#endif /* MONITOR_H */
